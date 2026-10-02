/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "CaptureSources.h"
#include "LillaSerialFlash.h"
#include "SharedLiveSampler.h"
#include <spi_interrupt.h>

bool LillaSerialFlashFile::Read_audio_samples_background(int file_id, int16_t *destination, int first_sample, int samples_count)
{
    const bool audio_enabled = NVIC_IS_ENABLED(IRQ_SOFTWARE) != 0;
    NVIC_DISABLE_IRQ(IRQ_SOFTWARE);
    AudioStartUsingSPI(); // Keep the reservation alive even when the last Flash voice stops between chunks.
    if (audio_enabled)
    {
        NVIC_ENABLE_IRQ(IRQ_SOFTWARE);
    }

    bool success = true;
    while (samples_count > 0)
    {
        const int chunk = samples_count < 128 ? samples_count : 128;
        if (!Read_audio_samples(file_id, destination, first_sample, chunk))
        {
            success = false;
            break;
        }
        // Each completed transaction restores the audio IRQ before another chunk can acquire SPI.
        destination += chunk;
        first_sample += chunk;
        samples_count -= chunk;
    }

    NVIC_DISABLE_IRQ(IRQ_SOFTWARE);
    AudioStopUsingSPI();
    if (audio_enabled)
    {
        NVIC_ENABLE_IRQ(IRQ_SOFTWARE);
    }
    return success;
}

void LillaSerialFlashFile::fast_open(int id_file)
{
  // se usato per Packet: id_file = id_packet + RAW_FILES

  this->address = FlashFileRegisterParser::address(id_file);
  this->length = FlashFileRegisterParser::length(id_file);
  this->offset = 0;
  this->dirindex = FlashFileRegisterParser::dirindex(id_file);
}

void LillaSerialFlashFile::packet_fast_open(int id_packet)
{
  this->address = FlashFileRegisterParser::address(id_packet + RAW_FILES);
  this->length = FlashFileRegisterParser::length(id_packet + RAW_FILES);
  this->offset = 0;
  this->dirindex = FlashFileRegisterParser::dirindex(id_packet + RAW_FILES);
}

bool LillaSerialFlashFile::Read_audio_samples(int file_id, int16_t *destination, int first_sample, int samples_count)
{
    if (Capture_find(file_id) != nullptr)
    {
        return Capture_read(file_id, destination, first_sample, samples_count);
    }
    if (file_id < 0 || file_id >= FIRST_LIVE_SAMPLING_FILE || first_sample < 0 || samples_count < 0)
    {
        return false;
    }

    if (samples_count == 0)
    {
        return true;
    }

    if (destination == nullptr)
    {
        return false;
    }

    const uint32_t first_byte = static_cast<uint32_t>(first_sample) * 2u;
    const uint32_t total_bytes = static_cast<uint32_t>(samples_count) * 2u;
    uint8_t *destination_bytes = reinterpret_cast<uint8_t *>(destination);
    LillaSerialFlashFile rawfile;

    if (file_id < FIRST_RECORDING_FILE)
    {
        rawfile.fast_open(file_id);
        const uint32_t file_bytes = rawfile.size();

        if (!rawfile || first_byte > file_bytes || total_bytes > file_bytes - first_byte)
        {
            rawfile.close();
            return false;
        }

        rawfile.seek(first_byte);
        const bool complete = rawfile.read(destination_bytes, total_bytes) == total_bytes;
        rawfile.close();
        return complete;
    }

    static_assert(FIRST_LIVE_SAMPLING_FILE - FIRST_RECORDING_FILE == 2 * RECORDINGS); // The validated file ID already bounds the recording index.
    const int recording_id = (file_id - FIRST_RECORDING_FILE) / 2;

    const VFS_Recording &source = Recording[recording_id];

    if (source.first_packet < 0 || source.first_packet >= PACKETS || source.packets <= 0 || source.bytes <= 0)
    {
        return false;
    }

    const uint32_t recording_bytes = static_cast<uint32_t>(source.bytes);

    if (first_byte > recording_bytes || total_bytes > recording_bytes - first_byte)
    {
        return false;
    }

    const uint32_t packet_stride = source.stereo ? 2u : 1u;
    const uint32_t channel_offset = source.stereo ? static_cast<uint32_t>((file_id - FIRST_RECORDING_FILE) % 2) : 0u;
    const uint32_t first_packet = static_cast<uint32_t>(source.first_packet) + channel_offset;
    uint32_t packet_index = first_byte / static_cast<uint32_t>(PACKET_DIM);
    uint32_t packet_offset = first_byte % static_cast<uint32_t>(PACKET_DIM);
    uint32_t remaining_bytes = total_bytes;

    while (remaining_bytes > 0)
    {
        if (packet_index >= static_cast<uint32_t>(source.packets))
        {
            return false;
        }

        const uint32_t packet_id = first_packet + packet_index * packet_stride;

        if (packet_id >= static_cast<uint32_t>(PACKETS))
        {
            return false;
        }

        const uint32_t packet_space = static_cast<uint32_t>(PACKET_DIM) - packet_offset;
        const uint32_t chunk_bytes = remaining_bytes < packet_space ? remaining_bytes : packet_space;

        rawfile.packet_fast_open(static_cast<int>(packet_id));
        const uint32_t file_bytes = rawfile.size();

        if (!rawfile || packet_offset > file_bytes || chunk_bytes > file_bytes - packet_offset)
        {
            rawfile.close();
            return false;
        }

        rawfile.seek(packet_offset);
        const bool complete = rawfile.read(destination_bytes, chunk_bytes) == chunk_bytes;
        rawfile.close();

        if (!complete)
        {
            return false;
        }

        destination_bytes += chunk_bytes;
        remaining_bytes -= chunk_bytes;
        ++packet_index;
        packet_offset = 0;
    }

    return true;
}

void FlashFileRegisterParser::Read_all_file_data(void)
{
    char audio_filename[NAME_FILE_SIZE];
    char packet_filename[NAME_PACKET_SIZE];
  elapsedMicros T;
  int tempo;
  SerialFlashFile rawfile;

  int index = 0;

  Serial.println("*** Lilla_SerialFlash - rilevamento di tutti i file audio presenti ***");
  for (auto i = 0; i < (RAW_FILES + PACKETS); ++i)
  {
    T = 0;
    if (i < FIRST_RECORDING_FILE && !FileNameRegistry::Assigned(i))
    {
        address_array[i] = 0;
        length_array[i] = 0;
        dirindex_array[i] = 0;
        continue;
    }
    if (i < RAW_FILES) // n.raw, n.rec (.liv sono su PSRAM)
    {
      rawfile = SerialFlash.open(Get_file_name(i, audio_filename));
    }
    else //  P(acket)n.raw
    {
      rawfile = SerialFlash.open(Get_packet_name(i - RAW_FILES, packet_filename));
    }

    // T e' il tempo impiegato per da SerialFlash per accedere al file ed e' in gran parte dovuto al parsing
    // del registro file nella Flash.
    // Utilizzando FlashFileRegisterParser::fast_open T si annulla perche' l'indirizzo del (primo byte del) file
    // sulla Flash e' annotato in address_array[].
    tempo = T;

    address_array[i] = rawfile.getFlashAddress();
    length_array[i] = rawfile.size();

    if (length_array[i] > 0)
    {
      dirindex_array[i] = index;
      ++index;
    }

    if (false)
    {
      if (i < RAW_FILES)
      {
        Serial.print(Get_file_name(i, audio_filename));
      }
      else
      {
        Serial.print(Get_packet_name(i - RAW_FILES, packet_filename));
      }

      Serial.print(" SerialFlash.open() waste time:");
      Serial.print(tempo);
      Serial.print("us  dimension:");
      Serial.print(length_array[i]);
      Serial.print("  index:");
      Serial.print(dirindex_array[i]);
      Serial.print(" address_array:");
      Serial.println(address_array[i]);
    }

    rawfile.close();
  }
}

uint32_t FlashFileRegisterParser::address_array[RAW_FILES + PACKETS] = {0};
uint32_t FlashFileRegisterParser::length_array[RAW_FILES + PACKETS] = {0};
uint16_t FlashFileRegisterParser::dirindex_array[RAW_FILES + PACKETS] = {0};

uint32_t FlashFileRegisterParser::address(int id_file)
{
  return address_array[id_file];
}

uint32_t FlashFileRegisterParser::length(int id_file)
{
  return length_array[id_file];
}

uint16_t FlashFileRegisterParser::dirindex(int id_file)
{
  return dirindex_array[id_file];
}

/*
** On-chip SerialFlash file allocation data structures:

  uint32_t signature = 0xFA96554C;
  uint16_t maxfiles
  uint16_t stringssize  // div by 4
  uint16_t hashes[maxfiles]
  struct {
    uint32_t file_begin
    uint32_t file_length
    uint16_t string_index  // div4
  } fileinfo[maxfiles]
  char strings[stringssize]

PSRAM memory bytes:

0 1 2 3 4 = signature
          5 6 = number of files (max: 2^16 = 65.536)
              7 8 = size of strings section divided by 4 (max size: 4*2^16 = 256KB)

** signature
A 32 bit signature is stored at the beginning of the flash memory.
If 0xFFFFFFFF is seen, the entire chip should be assumed blank.
If any value other than 0xFA96554C is found, a different data format
is stored.  This could should refuse to access the flash.

** Number of files stored, size of the string section
The next 4 bytes store number of files (first 2 bytes) and size of the strings
section (last 2 bytes) divided by 4, which allow the position of every other item to be found.
The string section size is given the 16 bit integer multiplied by 4;
maximum size for string data is 4x2^16 = 256KB.

** Hash
An array of 16 bit filename hashes allows for quick linear search
for potentially matching filenames.  A hash value of 0xFFFF indicates
no file is allocated for the remainder of the array.

hashes space = 2*(max number of files) = 128KB

** "fileinfo": localizzazione del file
Following the hashes, and array of 10 byte structs give the location
and length of the file's actual data, and the offset of its filename
in the strings section.

location&offset space = 10*(max number of files) = 640KB

Strings are null terminated.  The remainder of the chip is file data.
*/
