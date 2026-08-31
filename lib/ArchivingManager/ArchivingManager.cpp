/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "ArchivingManager.h"
#include "GlobalInfoMaster.h"

uint32_t ArchivingManager::FRAM_Get_patch_address(uint8_t patch_id)
{
    return FRAM_PATCH_ADDRESS + static_cast<uint32_t>(patch_id) * sizeof(FRAM_Patch_struct);
}

uint32_t ArchivingManager::FRAM_Get_instrument_address(uint8_t patch_id, uint8_t instrument_id)
{
    return FRAM_Get_patch_address(patch_id) + offsetof(FRAM_Patch_struct, Instrument) + static_cast<uint32_t>(instrument_id) * sizeof(FRAM_Instrument_struct);
}

uint32_t ArchivingManager::FRAM_Get_filter_address(uint8_t patch_id, uint8_t instrument_id)
{
    return FRAM_Get_instrument_address(patch_id, instrument_id) + offsetof(FRAM_Instrument_struct, Filter);
}

uint32_t ArchivingManager::FRAM_Get_delay_address(uint8_t patch_id)
{
    return FRAM_Get_patch_address(patch_id) + offsetof(FRAM_Patch_struct, Delay);
}

uint32_t ArchivingManager::FRAM_Get_sound_address(uint16_t sound_id)
{
    return FRAM_SOUND_ADDRESS + static_cast<uint32_t>(sound_id) * sizeof(FRAM_Sound_struct);
}

uint32_t ArchivingManager::FRAM_Get_recording_address(uint8_t recording_id)
{
    return FRAM_RECORDING_ADDRESS + static_cast<uint32_t>(recording_id) * sizeof(FRAM_Recording_struct);
}

void ArchivingManager::Save_CC_lowpass_filter(const int CC_lowpass_filter)
{
    Eeprom_writeAnything(LOCATION_CC_SETTINGS + 8, CC_lowpass_filter);
}

void ArchivingManager::Read_CC_lowpass_filter(uint8_t &CC_lowpass_filter)
{
    Eeprom_readAnything(LOCATION_CC_SETTINGS + 8, CC_lowpass_filter); // Eeprom_readAnything(LOCATION_CC_SETTINGS + 8, CC_lowpass_filter);
}

void ArchivingManager::Save_optimization(const uint8_t optimization)
{
    Eeprom_writeAnything(LOCATION_OPTIMIZATION, optimization);
}

void ArchivingManager::Read_optimization(uint8_t &optimization)
{
    Eeprom_readAnything(LOCATION_OPTIMIZATION, optimization);
}

void ArchivingManager::Save_CC_Sound_gain(const uint8_t instrument_id, const uint8_t CC_Sg_instrument)
{
    Eeprom_writeAnything(LOCATION_CC_SETTINGS + instrument_id, CC_Sg_instrument);
}

void ArchivingManager::Read_CC_Sound_gain(const uint8_t instrument_id, uint8_t &CC_Sg_instrument)
{
    Eeprom_readAnything(LOCATION_CC_SETTINGS + instrument_id, CC_Sg_instrument);
}

void ArchivingManager::Copy_patch_Delay_data_from_Eeprom_to_Ram(Delay_data_struct &delay_data)
{
    Eeprom_readAnything(LOCATION_DELAY, delay_data);
}

void ArchivingManager::Save_Delay_to_Eeprom(const Delay_data_struct &Delay_data)
{
    Eeprom_writeAnything(LOCATION_DELAY, Delay_data);
}

void ArchivingManager::Read_first_octave(int8_t &first_octave)
{
    Eeprom_readAnything(LOCATION_FIRST_OCTAVE, first_octave);
}

void ArchivingManager::Save_first_octave(const int8_t first_octave)
{
    Eeprom_writeAnything(LOCATION_FIRST_OCTAVE, first_octave);
}

void ArchivingManager::Save_Sound(const int sound_id)
{
    Eeprom_writeAnything(Get_location_of_Sound(sound_id), Sound[sound_id]);
}

bool ArchivingManager::Validate_Sound_AB_file_raw(uint32_t sound_id)
{
    if (sound_id >= SOUNDS_MAX)
    {
        PRINT_ERROR(F("ERROR: invalid sound_id - "));
        return false;
    }

    int sample_count = 0;

    if (Sound[sound_id].file < FIRST_RECORDING_FILE)
    {
        sample_count = Info.Raw_file_samples(Sound[sound_id].file);
    }

    const bool invalid = sample_count <= 0 || Sound[sound_id].A >= static_cast<uint32_t>(sample_count) || Sound[sound_id].B >= static_cast<uint32_t>(sample_count) || Sound[sound_id].B < Sound[sound_id].A;

    if (!invalid)
    {
        return true;
    }

    const int fallback_samples = Info.Raw_file_samples(0);

    // La mancanza di 0.raw è un errore di sistema non recuperabile automaticamente.
    if (fallback_samples <= 0)
    {
        PRINT_ERROR(F("ERROR: required 0.raw missing or empty - "));
        return false;
    }

    Sound[sound_id].file = 0;
    Sound[sound_id].A = 0;
    Sound[sound_id].B = static_cast<uint32_t>(fallback_samples - 1);

    Save_Sound(sound_id);
    return true;
}

void ArchivingManager::Read_Sound(const int sound_id)
{
    Eeprom_readAnything(Get_location_of_Sound(sound_id), Sound[sound_id]);
    Validate_Sound_AB_file_raw(sound_id);
}

void ArchivingManager::Save_Patch(const int patch_id)
{
    EEPROM_Patch.used = Patch[patch_id].used;
    EEPROM_Patch.instruments = Patch[patch_id].instruments;

    for (auto instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
    {
        EEPROM_Patch.Instrument[instrument_id].used = Patch[patch_id].Instrument[instrument_id].used;
        EEPROM_Patch.Instrument[instrument_id].sound_id = Get_sound_id(patch_id, instrument_id);
        EEPROM_Patch.Instrument[instrument_id].root_key = Patch[patch_id].Instrument[instrument_id].root_key;

        EEPROM_Patch.Instrument[instrument_id].from_note = Patch[patch_id].Instrument[instrument_id].from_note;
        EEPROM_Patch.Instrument[instrument_id].to_note = Patch[patch_id].Instrument[instrument_id].to_note;
        EEPROM_Patch.Instrument[instrument_id].info = (Patch[patch_id].Instrument[instrument_id].precedence == true ? 0b1 : 0b0) + (Patch[patch_id].Instrument[instrument_id].lock == true ? 0b10 : 0b00); // bit0: precedence, bit1: lock

        EEPROM_Patch.Instrument[instrument_id].Filter.data = Patch[patch_id].Instrument[instrument_id].Filter.use + (Patch[patch_id].Instrument[instrument_id].Filter.modulation << 1) + (Patch[patch_id].Instrument[instrument_id].Filter.type << 4); // bit0: use  bit1,2,3: modulation  bit4,5: type
        EEPROM_Patch.Instrument[instrument_id].Filter.pivot = Patch[patch_id].Instrument[instrument_id].Filter.pivot;                                                                                                                                  // 0 --> 100 filter frequency/note frequency
        EEPROM_Patch.Instrument[instrument_id].Filter.resonance = Patch[patch_id].Instrument[instrument_id].Filter.resonance;                                                                                                                          // 0 --> 40
        EEPROM_Patch.Instrument[instrument_id].Filter.index = Patch[patch_id].Instrument[instrument_id].Filter.index;                                                                                                                                  // 1 --> 20 modulation_index
        EEPROM_Patch.Instrument[instrument_id].Filter.frequency_time = Patch[patch_id].Instrument[instrument_id].Filter.frequency_time;                                                                                                                // 0 --> 20
    }

    Eeprom_writeAnything(GET_location_of_Patch(patch_id), EEPROM_Patch);
}

void ArchivingManager::Read_Patch(const int patch_id)
{
    Eeprom_readAnything(GET_location_of_Patch(patch_id), EEPROM_Patch);

    Patch[patch_id].used = EEPROM_Patch.used;
    Patch[patch_id].instruments = EEPROM_Patch.instruments;

    for (auto instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
    {
        Patch[patch_id].Instrument[instrument_id].used = EEPROM_Patch.Instrument[instrument_id].used;
        Patch[patch_id].Instrument[instrument_id].sound_id = EEPROM_Patch.Instrument[instrument_id].sound_id;

        Patch[patch_id].Instrument[instrument_id].root_key = EEPROM_Patch.Instrument[instrument_id].root_key;
        Patch[patch_id].Instrument[instrument_id].from_note = EEPROM_Patch.Instrument[instrument_id].from_note;
        Patch[patch_id].Instrument[instrument_id].to_note = EEPROM_Patch.Instrument[instrument_id].to_note;
        Patch[patch_id].Instrument[instrument_id].precedence = bitRead(EEPROM_Patch.Instrument[instrument_id].info, 0);
        Patch[patch_id].Instrument[instrument_id].lock = bitRead(EEPROM_Patch.Instrument[instrument_id].info, 1);

        Patch[patch_id].Instrument[instrument_id].Filter.use = bitRead(EEPROM_Patch.Instrument[instrument_id].Filter.data, 0);                                                                                                                                                  // yes/no
        Patch[patch_id].Instrument[instrument_id].Filter.type = bitRead(EEPROM_Patch.Instrument[instrument_id].Filter.data, 4) + 2 * bitRead(EEPROM_Patch.Instrument[instrument_id].Filter.data, 5);                                                                            // bit4,5:filter_type
        Patch[patch_id].Instrument[instrument_id].Filter.pivot = EEPROM_Patch.Instrument[instrument_id].Filter.pivot;                                                                                                                                                           // 0 --> 100 filter frequency/note frequency
        Patch[patch_id].Instrument[instrument_id].Filter.resonance = EEPROM_Patch.Instrument[instrument_id].Filter.resonance;                                                                                                                                                   // 0 --> 40
        Patch[patch_id].Instrument[instrument_id].Filter.modulation = bitRead(EEPROM_Patch.Instrument[instrument_id].Filter.data, 1) + 2 * bitRead(EEPROM_Patch.Instrument[instrument_id].Filter.data, 2) + 4 * bitRead(EEPROM_Patch.Instrument[instrument_id].Filter.data, 3); // bit1,2,3:modulation
        Patch[patch_id].Instrument[instrument_id].Filter.index = EEPROM_Patch.Instrument[instrument_id].Filter.index;                                                                                                                                                           // 1 --> 20 modulation_index
        Patch[patch_id].Instrument[instrument_id].Filter.frequency_time = EEPROM_Patch.Instrument[instrument_id].Filter.frequency_time;                                                                                                                                         // 0 --> 20
    }
}

void ArchivingManager::Save_DS_Recording(const int recording)
{
    Serial.println(F("*** Save_DS_Recording(int recording) ***"));

    if (recording >= 0 && recording < RECORDINGS)
    {
        EEPROM_Recording[recording].first_packet = Recording[recording].first_packet;
        EEPROM_Recording[recording].packets = Recording[recording].packets;
        bitWrite(EEPROM_Recording[recording].info, 1, (Recording[recording].stereo ? 1 : 0));
        bitWrite(EEPROM_Recording[recording].info, 0, (Recording[recording].consistent ? 1 : 0));
        Save_DS_Recording(recording, EEPROM_Recording[recording]);
    }
    else
    {
        Serial.println(F("***** WARNING! --> Save_DS_Recording: recording out of range"));
    }
}

void ArchivingManager::Save_DS_Recording(const int &recording, const EEPROM_VFS_Recording &EEPROM_Rec_recording)
{
    Serial.print("ArchivingManager - GET_location_of_DS_Recording(recording): ");
    Serial.println(GET_location_of_DS_Recording(recording));

    Eeprom_writeAnything(GET_location_of_DS_Recording(recording), EEPROM_Rec_recording);
}

void ArchivingManager::Read_DS_Recording(const int &recording, EEPROM_VFS_Recording &EEPROM_Rec_recording)
{
    Eeprom_readAnything(GET_location_of_DS_Recording(recording), EEPROM_Rec_recording);
}

void ArchivingManager::Save_setup_file(File &file)
{
    auto value = 0;
    auto location = 0;
    String value_txt;

    while (file.available())
    {
        // leggi il byte x
        value_txt = file.readStringUntil('\n'); // restituisce String - es: x_txt = "230" ossia i char "2" "3" "0" "\n"
        value = value_txt.toInt();              // conversione da String a uint8_t es: x = 230

        // trascrivi x
        EEPROM.write(location, value);
        delayMicroseconds(100); // important not to block the process
        ++location;
    }
}

void ArchivingManager::Copy_setup_from_Eeprom_to_SD(File &file)
{
    auto value = 0;

    for (auto location = 0; location < EEPROM_BYTES; ++location)
    {
        // leggi il byte value
        value = EEPROM.read(location); // restituisce uint8_t es: x = 230
        delayMicroseconds(100);        // important not to block the process

        // trascrivi value
        file.println(String(value)); // es: String(x) = "230" ossia si salvano i tre digit con a capo 2 3 0 \n
    }
}

void ArchivingManager::Reset_EEPROM(void)
{
    for (auto location = 0; location < EEPROM_BYTES; ++location)
    {
        EEPROM.write(location, 0);
    }
}

void ArchivingManager::Print_EEPROM_content(void)
{
    auto location = 0;

    for (location = 0; location < LOCATION_RECORDING; ++location)
    {
        if (location % 90 == 0)
        {
            Serial.println();
            Serial.print("Patch ");
            Serial.println(location / 90);
        }

        Serial.print(F("location "));
        Serial.print(location);
        Serial.print(" value: ");
        Serial.println(EEPROM.read(location));
        delay(1);
    }

    Serial.println();

    for (location = LOCATION_RECORDING; location < LOCATION_SOUND; ++location)
    {
        if ((location - LOCATION_RECORDING) % SIZE_OF_EEPROM_RECORDING == 0)
        {
            Serial.println();
        }

        Serial.print(F("Recording - location "));
        Serial.print(location);
        Serial.print(" value: ");
        Serial.println(EEPROM.read(location));
    }

    Serial.println();

    for (location = LOCATION_SOUND; location < LOCATION_OPTIMIZATION; ++location)
    {
        if ((location - LOCATION_SOUND) % SIZE_OF_SOUND == 0)
        {
            Serial.println();
            Serial.print("Sound ");
            Serial.println((location - LOCATION_SOUND) / SIZE_OF_SOUND);
        }

        Serial.print(F("location "));
        Serial.print(location);
        Serial.print(" value: ");
        Serial.println(EEPROM.read(location));
    }

    Serial.println();

    Serial.print(F("SETUP_Optimization - location "));
    Serial.print(LOCATION_OPTIMIZATION);
    Serial.print(" value: ");
    Serial.println(EEPROM.read(LOCATION_OPTIMIZATION));
    Serial.println();

    Serial.print(F("First Octave - location "));
    Serial.print(LOCATION_FIRST_OCTAVE);
    Serial.print(" value: ");
    Serial.println(EEPROM.read(LOCATION_FIRST_OCTAVE));
    Serial.println();

    for (location = LOCATION_DELAY; location < LOCATION_CC_SETTINGS; ++location)
    {
        Serial.print(F("Delay - location "));
        Serial.print(location);
        Serial.print(" value: ");
        Serial.println(EEPROM.read(location));
    }

    Serial.println();

    for (location = LOCATION_CC_SETTINGS; location < EEPROM_BYTES; ++location)
    {
        Serial.print(F("CC Settings - location "));
        Serial.print(location);
        Serial.print(" value: ");
        Serial.println(EEPROM.read(location));
    }

    Serial.println();
}

int ArchivingManager::GET_location_of_DS_Recording(const int &recording)
{
    return (LOCATION_RECORDING + (recording * SIZE_OF_EEPROM_RECORDING));
}

uint16_t ArchivingManager::GET_location_of_Patch(const uint8_t &patch_id)
{
    return LOCATION_PATCH + (patch_id * SIZE_OF_EEPROM_PATCH);
}

uint16_t ArchivingManager::Get_location_of_Sound(const uint8_t &sound_id)
{
    // Serial.print("GET_location_of_sound: ");
    // Serial.println(LOCATION_SOUND + (sound_id * SIZE_OF_SOUND));
    return (LOCATION_SOUND + (sound_id * SIZE_OF_SOUND));
}

template <class T>
int ArchivingManager::Eeprom_writeAnything(size_t address, const T &source)
{
    if (address >= 0)
    {
        const byte *p = (const byte *)(const void *)&source;
        size_t i = 0;
        for (i = 0; i < sizeof(source); ++i)
        {
            EEPROM.write(address++, *p++);
        }

        return i;
    }

    else
    {
        Serial.println(F("Eeprom_writeAnything error!!"));
        return -1;
    }
}

template <class T>
int ArchivingManager::Eeprom_readAnything(size_t address, T &destination)
{
    if (address >= 0)
    {
        byte *p = (byte *)(void *)&destination;
        size_t i = 0;
        for (i = 0; i < sizeof(destination); ++i)
        {
            *p++ = EEPROM.read(address++);
        }
        return i;
    }

    else
    {
        Serial.println(F("Eeprom_readAnything error!!"));
        return -1;
    }
}

void ArchivingManager::Eeprom_write_uint8_t(const size_t address, const uint8_t &value)
{
    if (address >= 0)
    {
        EEPROM.write(address, value);
    }
}

void ArchivingManager::Eeprom_read_uint8_t(const size_t address, uint8_t &destination)
{
    if (address >= 0)
    {
        destination = EEPROM.read(address);
    }
}

void ArchivingManager::Eeprom_write_int8_t(const size_t address, const int8_t &value)
{
    if (address >= 0)
    {
        EEPROM.write(address, value);
    }
}

void ArchivingManager::Eeprom_read_int8_t(const size_t address, int8_t &destination)
{
    if (address >= 0)
    {
        destination = EEPROM.read(address);
    }
}

// File name patch_id - delay
String ArchivingManager::Filename_patch_Delay(const int patch_id)
{
    String filename = String(patch_id);
    return String(filename + ".delay");
}

void ArchivingManager::Copy_Delay_data_from_RAM_to_SD(File &file) // private
{
    const byte *data = (const byte *)(const void *)&Delay_data;

    for (auto i = 0; i < DELAY_DATA_DIM; ++i)
    {
        file.println(*(data + i));
    }
}

void ArchivingManager::Copy_Delay_data_from_Eeprom_to_SD(File &file) // private
{
    auto value = 0;

    for (auto location = LOCATION_DELAY; location < LOCATION_DELAY + DELAY_DATA_DIM; ++location)
    {
        value = EEPROM.read(location);
        delayMicroseconds(100); // important not to block the process
        file.println(String(value));
    }

    if (false)
    {
        Serial.println(F("ArchivingManager::Copy_Delay_data_from_Eeprom_to_SD(File &file) - Read EEPROM data:"));
        Serial.println(file.name());
        Print_Delay_data_reading_from_Eeprom();
    }
}

void ArchivingManager::Copy_Delay_data_from_SD_to_Eeprom(File &file) // private
{
    auto value = 0;
    auto location = LOCATION_DELAY;
    String value_txt;

    while (file.available())
    {
        value_txt = file.readStringUntil('\n'); // restituisce String - es: value_txt = "230" ossia i char "2" "3" "0" "\n"
        value = value_txt.toInt();              // toInt() conversione da String a long es: value = 230
        EEPROM.write(location, value);
        delayMicroseconds(100); // important not to block the process
        location++;
    }

    if (true)
    {
        Serial.println(F("ArchivingManager::Copy_Delay_data_from_SD_to_Eeprom(File &file)- Read EEPROM data:"));
        Print_Delay_data_reading_from_Eeprom();
    }
}

FLASHMEM
void ArchivingManager::Print_Delay_data_reading_from_Eeprom()
{
    auto data_LSB = 0;
    auto data_MSB = 0;
    auto result_int = 0;
    auto result_uint = 0;
    size_t location = 0;

    Serial.println();
    Serial.println(F("ArchivingManager::Print_Delay_data_reading_from_Eeprom()"));

    Serial.print("uint16_t samples: ");
    data_LSB = EEPROM.read(LOCATION_DELAY + location++);
    data_MSB = EEPROM.read(LOCATION_DELAY + location++);
    result_uint = data_MSB << 8 | data_LSB;
    Serial.println(result_uint);

    Serial.print("int16_t samples_LR: ");
    data_LSB = EEPROM.read(LOCATION_DELAY + location++);
    data_MSB = EEPROM.read(LOCATION_DELAY + location++);
    result_int = data_MSB << 8 | data_LSB;
    Serial.println(result_int);

    Serial.print("instrument_route: ");
    Serial.println(EEPROM.read(LOCATION_DELAY + location++));

    Serial.print("modulation: ");
    Serial.println(EEPROM.read(LOCATION_DELAY + location++));

    Serial.print("depth: ");
    Serial.println(EEPROM.read(LOCATION_DELAY + location++));

    Serial.print("frequency: ");
    Serial.println(EEPROM.read(LOCATION_DELAY + location++));

    Serial.print("uint16_t phase_LR: ");
    data_LSB = EEPROM.read(LOCATION_DELAY + location++);
    data_MSB = EEPROM.read(LOCATION_DELAY + location++);
    result_uint = data_MSB << 8 | data_LSB;
    Serial.println(result_uint);

    Serial.print("uint16_t loop_gain: ");
    data_LSB = EEPROM.read(LOCATION_DELAY + location++);
    data_MSB = EEPROM.read(LOCATION_DELAY + location++);
    result_uint = data_MSB << 8 | data_LSB;
    Serial.println(result_uint);
    Serial.println();
}

bool ArchivingManager::Delete_patch_Delay_data_in_SD(const int patch_id)
{
    String filename = Filename_patch_Delay(patch_id);
    String full_path = String("/LILLADELAY/" + filename);

    if (SD.begin(BUILTIN_SDCARD))
    {
        const char *full_path_ = &full_path[0];
        if (SD.exists(full_path_))
        {
            SD.remove(full_path_);

            Serial.print(F("ArchivingManager::Delete_patch_Delay_data_in_SD - existing "));
            Serial.print(full_path);
            Serial.println(" has been deleted.");
            return true;
        }
        else
        {
            Serial.print(F("ArchivingManager::Delete_patch_Delay_data_in_SD - Delay not existing."));
            return true;
        }
    }
    else
    {
        Serial.println(F("ArchivingManager::Delete_patch_Delay_data_in_SD - SD not present!"));
        return false;
    }
}

void ArchivingManager::Copy_patch_Delay_data_from_RAM_to_SD(const int patch_id) // public
{
    String filename = Filename_patch_Delay(patch_id);
    String full_path = String("/LILLADELAY/" + filename);
    auto *full_path_ptr = full_path.c_str();

    if (SD.begin(BUILTIN_SDCARD))
    {
        if (!SD.exists("/LILLADELAY"))
        {
            SD.mkdir("/LILLADELAY");
            Serial.println(F("/LILLADELAY directory created"));
        }

        if (SD.exists(full_path_ptr))
        {
            SD.remove(full_path_ptr);

            Serial.print(F("ArchivingManager::Copy_patch_Delay_data_from_RAM_to_SD - existing "));
            Serial.print(full_path_ptr);
            Serial.println(" has been deleted.");
        }

        File file = SD.open(full_path_ptr, FILE_WRITE); // creazione del file vuoto
        if (file)
        {
            Copy_Delay_data_from_RAM_to_SD(file);
            file.close();

            Serial.print(F("ArchivingManager::Copy_patch_Delay_data_from_RAM_to_SD - Delay_data saved in "));
            Serial.print(full_path_ptr);
            Serial.println(" in SD.");
        }
    }
    else
    {
        Serial.println(F("ArchivingManager::Copy_patch_Delay_data_from_RAM_to_SD - SD not present!"));
    }
}

bool ArchivingManager::Copy_patch_Delay_data_from_SD_to_RAM(const int patch_id) // public
{
    String filename = Filename_patch_Delay(patch_id);
    String full_path = String("/LILLADELAY/" + filename);
    auto *full_path_ptr = full_path.c_str();

    if (SD.begin(BUILTIN_SDCARD))
    {
        if (SD.exists(full_path_ptr))
        {
            Serial.print(F("ArchivingManager::Copy_patch_Delay_data_from_SD_to_RAM - Delay file "));
            Serial.print(full_path);
            Serial.println(F(" found; now starts Delay import."));

            File file = SD.open(full_path_ptr);
            if (file)
            {
                Copy_Delay_data_from_SD_to_RAM(file);
                file.close();
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            Serial.println(F("ArchivingManager::Copy_patch_Delay_data_from_SD_to_RAM - Delay not found on SD!"));
            return false;
        }
    }

    else
    {
        Serial.println(F("ArchivingManager::Copy_patch_Delay_data_from_SD_to_RAM - SD not present!"));
        return false;
    }
}

void ArchivingManager::Copy_Delay_data_from_SD_to_RAM(File &file)
{
    uint8_t value;
    String value_txt;
    uint8_t value_1;
    uint8_t value_2;

    /*
    struct Delay_data_struct // DELAY_DATA_DIM byte
    {
        uint16_t samples;
        int16_t samples_LR;
        uint8_t instrument_route;
        uint8_t modulation_source;
        uint8_t modulation_depth;
        uint8_t modulation_frequency;
        uint16_t modulation_phase_LR;
        uint16_t loop_gain;
    };
    */

    value_txt = file.readStringUntil('\n');
    value_1 = value_txt.toInt();
    value_txt = file.readStringUntil('\n');
    value_2 = value_txt.toInt();
    Delay_data.samples = value_2 << 8 | value_1;

    value_txt = file.readStringUntil('\n');
    value_1 = value_txt.toInt();
    value_txt = file.readStringUntil('\n');
    value_2 = value_txt.toInt();
    Delay_data.samples_LR = static_cast<int16_t>(value_2 << 8 | value_1);

    value_txt = file.readStringUntil('\n'); // restituisce String - es: x_txt = "230" ossia i char "2" "3" "0" "\n"
    value = value_txt.toInt();              // toInt() conversione da String a long es: x = 230
    Delay_data.instrument_route = value;

    value_txt = file.readStringUntil('\n');
    value = value_txt.toInt();
    Delay_data.modulation_source = value;

    value_txt = file.readStringUntil('\n');
    value = value_txt.toInt();
    Delay_data.modulation_depth = value;

    value_txt = file.readStringUntil('\n');
    value = value_txt.toInt();
    Delay_data.modulation_frequency = value;

    value_txt = file.readStringUntil('\n');
    value_1 = value_txt.toInt();
    value_txt = file.readStringUntil('\n');
    value_2 = value_txt.toInt();
    Delay_data.modulation_phase_LR = value_2 << 8 | value_1;

    value_txt = file.readStringUntil('\n');
    value_1 = value_txt.toInt();
    value_txt = file.readStringUntil('\n');
    value_2 = value_txt.toInt();
    Delay_data.loop_gain = value_2 << 8 | value_1;

    Serial.println(F("ArchivingManager::Copy_patch_Delay_data_from_SD_to_RAM - Done."));
}

void ArchivingManager::Copy_patch_Delay_data_from_Eeprom_to_SD(const int patch_id) // public
{
    String filename = Filename_patch_Delay(patch_id);
    String full_path = String("/LILLADELAY/" + filename);
    auto *full_path_ptr = full_path.c_str();

    if (SD.begin(BUILTIN_SDCARD))
    {
        if (!SD.exists("/LILLADELAY"))
        {
            SD.mkdir("/LILLADELAY");
            Serial.println(F("/LILLADELAY directory created"));
        }

        if (SD.exists(full_path_ptr))
        {
            SD.remove(full_path_ptr);
            Serial.print(F("ArchivingManager::Copy_Patch_Delay_data_from_Eeprom_to_SD - existing "));
            Serial.print(full_path_ptr);
            Serial.println(" has been deleted.");
        }

        File file = SD.open(full_path_ptr, FILE_WRITE); // creazione del file
        if (file)
        {
            Copy_Delay_data_from_Eeprom_to_SD(file);
            file.close();

            Serial.print(F("ArchivingManager::Copy_Patch_Delay_data_from_Eeprom_to_SD - delay data have been saved in "));
            Serial.print(full_path_ptr);
            Serial.println(" on SD.");
        }
    }
    else
    {
        Serial.println(F("ArchivingManager::Copy_Patch_Delay_data_from_Eeprom_to_SD - SD not present!"));
    }
}

bool ArchivingManager::Copy_patch_Delay_data_from_SD_to_Eeprom(const int patch_id) // public
{
    String filename = Filename_patch_Delay(patch_id);
    String full_path = String("/LILLADELAY/" + filename);
    auto *full_path_ptr = full_path.c_str();

    if (SD.begin(BUILTIN_SDCARD))
    {
        if (SD.exists(full_path_ptr))
        {
            Serial.print(F("ArchivingManager::Copy_patch_Delay_data_from_SD_to_Eeprom - delay data file "));
            Serial.print(full_path);
            Serial.println(F(" found; now starts data import."));

            File file = SD.open(full_path_ptr);
            if (file)
            {
                Copy_Delay_data_from_SD_to_Eeprom(file);
                file.close();
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            Serial.println(F("ArchivingManager::Copy_patch_Delay_data_from_SD_to_Eeprom - delay data not found on SD!"));
            return false;
        }
    }
    else
    {
        Serial.println(F("ArchivingManager::Copy_patch_Delay_data_from_SD_to_Eeprom - SD not present!"));
        return false;
    }
}

void ArchivingManager::Print_patch_Delay_file_reading_from_SD(const int patch_id) // public
{
    String filename = Filename_patch_Delay(patch_id);
    String full_path = String("/LILLADELAY/" + filename);
    auto *full_path_ptr = full_path.c_str();

    if (SD.begin(BUILTIN_SDCARD))
    {
        if (SD.exists(full_path_ptr))
        {
            Serial.print("ArchivingManager::Print_patch_Delay_file_reading_from_SD - Reading SD Delay file ");
            Serial.println(full_path);

            File file = SD.open(full_path_ptr);
            if (file)
            {
                uint8_t x;
                String x_txt;
                int i = 0;

                while (file.available())
                {
                    x_txt = file.readStringUntil('\n'); // restituisce String - es: x_txt = "230" ossia i char "2" "3" "0" "\n"
                    x = x_txt.toInt();                  // conversione da String a uint8_t es: x = 230
                    delayMicroseconds(100);             // important not to block the process
                    Serial.print("byte ");
                    Serial.print(i);
                    Serial.print(" value: ");
                    Serial.println(x);
                    ++i;
                }
                file.close();
            }
        }
    }
    else
    {
        Serial.println(F("ArchivingManager::Print_patch_Delay_file_reading_from_SD - SD not present!"));
    }
}

// Filename patch description
String ArchivingManager::Filename_Patch(const int patch_id)
{
    String filename = String(patch_id);
    return String(filename + ".patch");
}

// Filename sound description
String ArchivingManager::Filename_Sound(const int patch_id, const int instrument_id)
{
    // String filename = String(patch_id + "_" + instrument_id);
    String filename = String(patch_id) + "_" + instrument_id;
    return String(filename + ".sound");
}

bool ArchivingManager::Copy_Patch_from_RAM_to_SD(const int patch_id) // public
{
    String filename = Filename_Patch(patch_id);
    String full_path = String("/LILLAPATCH/" + filename);
    auto *full_path_ptr = full_path.c_str();

    if (SD.begin(BUILTIN_SDCARD))
    {
        if (!SD.exists("/LILLAPATCH"))
        {
            SD.mkdir("/LILLAPATCH");
            Serial.println(F("/LILLAPATCH directory created"));
        }

        else if (SD.exists(full_path_ptr))
        {
            SD.remove(full_path_ptr);

            Serial.print(F("ArchivingManager::Copy_Patch_from_RAM_to_SD - existing "));
            Serial.print(full_path_ptr);
            Serial.println(" has been deleted.");
        }

        File file = SD.open(full_path_ptr, FILE_WRITE);
        if (file)
        {
            Copy_Patch_from_RAM_to_SD(patch_id, file);
            file.close();

            Serial.print(F("ArchivingManager::Copy_Patch_from_RAM_to_SD - Patch saved in "));
            Serial.print(full_path_ptr);
            Serial.println(" in SD.");

            return true;
        }

        else
        {
            return false;
        }
    }

    else
    {
        Serial.println(F("ArchivingManager::Copy_Patch_from_RAM_to_SD - SD not present!"));
        return false;
    }
}

bool ArchivingManager::Save_Patch_from_RAM_to_SD(const int patch_id)
{
    if (patch_id < 0 || patch_id >= PATCHES_MAX)
    {
        PRINT_ERROR(F("ERROR: patch_id out of range"));
        return false;
    }

    char full_path[PATCH_PATH_SIZE];
    const int length = snprintf(full_path, sizeof(full_path), "/LILLAPATCH/%d.bin.patch", patch_id);

    if (length < 0 || static_cast<size_t>(length) >= sizeof(full_path))
    {
        PRINT_ERROR(F("ERROR: Patch path too long"));
        return false;
    }

    if (!Is_SD_inserted())
    {
        PRINT_ERROR(F("ERROR: SD not inserted"));
        return false;
    }

    else
    {
        if (!SD.exists("/LILLAPATCH"))
        {
            SD.mkdir("/LILLAPATCH");
            Serial.println(F("/LILLAPATCH directory created"));
        }

        if (SD.exists(full_path) && !SD.remove(full_path))
        {
            PRINT_ERROR(F("ERROR: impossible to remove previous Patch"));
            return false;
        }

        File file = SD.open(full_path, FILE_WRITE);

        if (file)
        {
            FileHeader header = {FILEHEADER_VERSION, static_cast<uint16_t>(sizeof(Patch_struct))};

            size_t headerWritten = file.write(reinterpret_cast<const uint8_t *>(&header), sizeof(header));
            size_t payloadWritten = file.write(reinterpret_cast<const uint8_t *>(&Patch[patch_id]), sizeof(Patch[patch_id]));

            bool success = (headerWritten == sizeof(header)) && (payloadWritten == sizeof(Patch[patch_id]));

            if (success)
            {
                Serial.print(F("ArchivingManager::Save_Patch_from_RAM_to_SD - Patch saved in "));
                Serial.println(full_path);

                file.close();
                return success;
            }
            else
            {
                PRINT_ERROR(F("ERROR: incomplete file writing on SD"));

                file.close();
                return false;
            }
        }
        else
        {
            PRINT_ERROR(F("ERROR: file not created on SD"));
            return false;
        }
    }

    PRINT_ERROR(F("ERROR: SD not present for saving Patch"));
    return false;
}

bool ArchivingManager::Resume_Patch_from_SD_to_RAM(const int patch_id)
{
    if (patch_id < 0 || patch_id >= PATCHES_MAX)
    {
        PRINT_ERROR(F("ERROR: patch_id out of range"));
        return false;
    }

    char full_path[PATCH_PATH_SIZE];

    const int length = snprintf(full_path, sizeof(full_path), "/LILLAPATCH/%d.bin.patch", patch_id);

    if (length < 0 || static_cast<size_t>(length) >= sizeof(full_path))
    {
        PRINT_ERROR(F("ERROR: Patch path too long"));
        return false;
    }

    if (!Is_SD_inserted())
    {
        PRINT_ERROR(F("ERROR: SD not inserted"));
        return false;
    }

    if (!SD.exists(full_path))
    {
        PRINT_ERROR(F("Patch not present in SD"));
        return false;
    }

    // Copy data
    File file = SD.open(full_path, FILE_READ);

    if (!file)
    {
        PRINT_ERROR(F("ERROR: unable to open Patch file"));
        return false;
    }

    FileHeader header{};
    const size_t headerRead = file.read(reinterpret_cast<uint8_t *>(&header), sizeof(header));

    if (headerRead != sizeof(header))
    {
        PRINT_ERROR(F("ERROR: incomplete Patch header"));
        file.close();
        return false;
    }

    if (header.version != FILEHEADER_VERSION)
    {
        PRINT_ERROR(F("ERROR: unsupported Patch file version"));
        file.close();
        return false;
    }

    if (header.payloadSize != sizeof(Patch_struct))
    {
        PRINT_ERROR(F("ERROR: incompatible Patch payload size"));
        file.close();
        return false;
    }

    const uint64_t expectedFileSize = sizeof(FileHeader) + sizeof(Patch_struct);

    if (file.size() != expectedFileSize)
    {
        PRINT_ERROR(F("ERROR: invalid Patch file size"));
        file.close();
        return false;
    }

    Patch_struct temporaryPatch{};

    const size_t payloadRead = file.read(reinterpret_cast<uint8_t *>(&temporaryPatch), sizeof(temporaryPatch));

    if (payloadRead != sizeof(temporaryPatch))
    {
        PRINT_ERROR(F("ERROR: incomplete Patch payload"));
        file.close();
        return false;
    }

    file.close();

    // Modifica la Patch attiva soltanto dopo una lettura completa.
    Patch[patch_id] = temporaryPatch;

    Serial.print(F("ArchivingManager::Resume_Patch_from_SD_to_RAM - Patch restored from "));
    Serial.println(full_path);

    return true;
}

void ArchivingManager::Copy_Patch_from_RAM_to_SD(const int patch_id, File &file)
{
    const auto *data = (const byte *)(const void *)&Patch[patch_id];

    for (auto i = 0; i < SIZE_OF_PATCH; ++i)
    {
        file.println(*(data + i));
    }
}

bool ArchivingManager::Copy_Sound_from_RAM_to_SD(const int patch_id, const int instrument_id)
{
    String filename = Filename_Sound(patch_id, instrument_id);
    String full_path = String("/LILLASOUND/" + filename);
    auto *full_path_ptr = full_path.c_str();

    if (SD.begin(BUILTIN_SDCARD))
    {
        if (!SD.exists("/LILLASOUND"))
        {
            SD.mkdir("/LILLASOUND");
            Serial.println(F("/LILLASOUND directory created"));
        }

        else if (SD.exists(full_path_ptr))
        {
            SD.remove(full_path_ptr);

            Serial.print(F("ArchivingManager::Copy_Sound_from_RAM_to_SD - existing "));
            Serial.print(full_path);
            Serial.println(" has been deleted.");
        }

        File file = SD.open(full_path_ptr, FILE_WRITE); // creazione del file vuoto
        if (file)
        {
            Copy_Sound_from_RAM_to_SD(patch_id, instrument_id, file);
            file.close();

            Serial.print(F("ArchivingManager::Copy_Sound_from_RAM_to_SD - Sound saved in "));
            Serial.print(full_path_ptr);
            Serial.println(" in SD.");

            return true;
        }

        else
        {
            return false;
        }
    }

    else
    {
        Serial.println(F("ArchivingManager::Copy_Sound_from_RAM_to_SD - SD not present!"));
        return false;
    }
}

void ArchivingManager::Copy_Sound_from_RAM_to_SD(const int patch_id, const int instrument_id, File &file) // private
{
    const auto *data = (const byte *)(const void *)&Sound[Get_sound_id(patch_id, instrument_id)];

    for (auto i = 0; i < SIZE_OF_SOUND; ++i)
    {
        file.println(*(data + i));
    }
}

bool ArchivingManager::Copy_Patch_from_SD_to_RAM(const int patch_id)
{
    String filename = Filename_Patch(patch_id);
    String full_path = String("/LILLAPATCH/" + filename);
    auto *full_path_ptr = full_path.c_str();

    if (SD.begin(BUILTIN_SDCARD))
    {
        if (SD.exists(full_path_ptr))
        {
            Serial.print(F("ArchivingManager::Copy_Patch_from_SD_to_Eeprom - Patch file "));
            Serial.print(full_path);
            Serial.println(F(" found; now starts Patch import."));

            File file = SD.open(full_path_ptr);
            if (file)
            {
                Copy_Patch_from_SD_to_RAM(patch_id, file);
                file.close();
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            Serial.println(F("ArchivingManager::Copy_Patch_from_SD_to_RAM - Patch not found on SD!"));
            return false;
        }
    }

    else
    {
        Serial.println(F("ArchivingManager::Copy_Patch_from_SD_to_RAM - SD not present!"));
        return false;
    }
}

void ArchivingManager::Copy_Patch_from_SD_to_RAM(const int patch_id, File &file) // private
{
    uint8_t value;
    String value_txt;
    uint8_t value_LSB;
    uint8_t value_MSB;

    /*
    struct Patch_struct
    {
        bool used;
        uint8_t instruments;
        Instrument_struct Instrument[8];
    }
    */

    value_txt = file.readStringUntil('\n'); // restituisce String - es: x_txt = "230" ossia i char "2" "3" "0" "\n"
    value = value_txt.toInt();              // toInt() conversione da String a long es: x = 230
    Patch[patch_id].used = (value++ != 0);

    value_txt = file.readStringUntil('\n');
    value = value_txt.toInt();
    Patch[patch_id].instruments = value;

    /*
    struct Instrument_struct
    {
        bool used;
        uint8_t sound_id;
        uint8_t root_key;
        uint8_t from_note;
        uint8_t to_note;
        bool precedence;
        bool lock;
        Instrument_filter_data_struct Filter;
    };
    */

    for (auto instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
    {
        value_txt = file.readStringUntil('\n');
        value = value_txt.toInt();
        Patch[patch_id].Instrument[instrument_id].used = (value != 0);

        value_txt = file.readStringUntil('\n');
        value_LSB = value_txt.toInt();
        value_txt = file.readStringUntil('\n');
        value_MSB = value_txt.toInt();
        Patch[patch_id].Instrument[instrument_id].sound_id = value_MSB << 8 | value_LSB;

        value_txt = file.readStringUntil('\n');
        value = value_txt.toInt();
        Patch[patch_id].Instrument[instrument_id].root_key = value;

        value_txt = file.readStringUntil('\n');
        value = value_txt.toInt();
        Patch[patch_id].Instrument[instrument_id].from_note = value;

        value_txt = file.readStringUntil('\n');
        value = value_txt.toInt();
        Patch[patch_id].Instrument[instrument_id].to_note = value;

        value_txt = file.readStringUntil('\n');
        value = value_txt.toInt();
        Patch[patch_id].Instrument[instrument_id].precedence = (value != 0);

        value_txt = file.readStringUntil('\n');
        value = value_txt.toInt();
        Patch[patch_id].Instrument[instrument_id].lock = (value != 0);

        /*
        struct Instrument_filter_data_struct
        {
        uint8_t use;            // yes/no
        uint8_t type;           // filter type 0 --> 3
        uint8_t pivot;          // 0 --> 100 filter frequency/note frequency
        uint8_t resonance;      // 0 --> 40
        uint8_t modulation;     // waveform 0 -> 3
        uint8_t index;          // 1 --> 20 modulation_index
        uint8_t frequency_time; // 0 --> 20
        };
        */

        value_txt = file.readStringUntil('\n');
        value = value_txt.toInt();
        Patch[patch_id].Instrument[instrument_id].Filter.use = value;

        value_txt = file.readStringUntil('\n');
        value = value_txt.toInt();
        Patch[patch_id].Instrument[instrument_id].Filter.type = value;

        value_txt = file.readStringUntil('\n');
        value = value_txt.toInt();
        Patch[patch_id].Instrument[instrument_id].Filter.pivot = value;

        value_txt = file.readStringUntil('\n');
        value = value_txt.toInt();
        Patch[patch_id].Instrument[instrument_id].Filter.resonance = value;

        value_txt = file.readStringUntil('\n');
        value = value_txt.toInt();
        Patch[patch_id].Instrument[instrument_id].Filter.modulation = value;

        value_txt = file.readStringUntil('\n');
        value = value_txt.toInt();
        Patch[patch_id].Instrument[instrument_id].Filter.index = value;

        value_txt = file.readStringUntil('\n');
        value = value_txt.toInt();
        Patch[patch_id].Instrument[instrument_id].Filter.frequency_time = value;
    }

    if (true)
    {
        Serial.println(F("ArchivingManager::Copy_Patch_from_SD_to_RAM(int patch_id, File &file)- Done."));
    }
}

bool ArchivingManager::Copy_Sound_from_SD_to_RAM(const int patch_id, const int instrument_id, const int sound_id) // public
{
    String filename = Filename_Sound(patch_id, instrument_id);
    String full_path = String("/LILLASOUND/" + filename);
    auto *full_path_ptr = full_path.c_str();

    if (SD.begin(BUILTIN_SDCARD))
    {
        if (SD.exists(full_path_ptr))
        {
            Serial.print(F("ArchivingManager::Copy_Sound_from_SD_to_RAM - Sound file "));
            Serial.print(full_path);
            Serial.println(F(" found; now starts Sound import."));

            File file = SD.open(full_path_ptr);
            if (file)
            {
                Copy_Sound_from_SD_to_RAM(patch_id, instrument_id, sound_id, file);
                file.close();
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            Serial.println(F("ArchivingManager::Copy_Sound_from_SD_to_RAM - Sound not found on SD!"));
            return false;
        }
    }

    else
    {
        Serial.println(F("ArchivingManager::Copy_Sound_from_SD_to_RAM - SD not present!"));
        return false;
    }
}

void ArchivingManager::Copy_Sound_from_SD_to_RAM(const int patch_id, const int instrument_id, const int sound_id, File &file) // private
{
    uint8_t value;
    String value_txt;
    uint8_t value_1;
    uint8_t value_2;
    uint8_t value_3;
    uint8_t value_4;

    /*
    struct Sound_struct // 22 bytes
    {
        bool used;
        uint16_t file;
        uint8_t mode;
        int8_t pitch; // -128 + 127
        uint32_t A;
        uint32_t B;
        uint16_t Noclick;
        int8_t pan;
        uint8_t data; // bit4-3-2-1: midi_channel bit0: Attack ramp ("0" Slow, "1" Fast)
        uint8_t attack;
        uint8_t decay;
        uint8_t sustain;
        uint8_t release;
        uint8_t gain;
    }
    */

    value_txt = file.readStringUntil('\n'); // restituisce String - es: x_txt = "230" ossia i char "2" "3" "0" "\n"
    value = value_txt.toInt();              // toInt() conversione da String a long es: x = 230
    Sound[sound_id].used = (value != 0);

    value_txt = file.readStringUntil('\n');
    value_1 = value_txt.toInt();
    value_txt = file.readStringUntil('\n');
    value_2 = value_txt.toInt();
    Sound[sound_id].file = value_2 << 8 | value_1;

    value_txt = file.readStringUntil('\n');
    value = value_txt.toInt();
    Sound[sound_id].pitch = static_cast<int8_t>(value);

    value_txt = file.readStringUntil('\n');
    value_1 = value_txt.toInt();
    value_txt = file.readStringUntil('\n');
    value_2 = value_txt.toInt();
    value_txt = file.readStringUntil('\n');
    value_3 = value_txt.toInt();
    value_txt = file.readStringUntil('\n');
    value_4 = value_txt.toInt();
    Sound[sound_id].A = value_4 << 24 | value_3 << 16 | value_2 << 8 | value_1;

    value_txt = file.readStringUntil('\n');
    value_1 = value_txt.toInt();
    value_txt = file.readStringUntil('\n');
    value_2 = value_txt.toInt();
    value_txt = file.readStringUntil('\n');
    value_3 = value_txt.toInt();
    value_txt = file.readStringUntil('\n');
    value_4 = value_txt.toInt();
    Sound[sound_id].B = value_4 << 24 | value_3 << 16 | value_2 << 8 | value_1;

    value_txt = file.readStringUntil('\n');
    value = value_txt.toInt();
    Sound[sound_id].Noclick = value;

    value_txt = file.readStringUntil('\n');
    value = value_txt.toInt();
    Sound[sound_id].pan = static_cast<int8_t>(value);

    value_txt = file.readStringUntil('\n');
    value = value_txt.toInt();
    Sound[sound_id].data = value;

    value_txt = file.readStringUntil('\n');
    value = value_txt.toInt();
    Sound[sound_id].attack = value;

    value_txt = file.readStringUntil('\n');
    value = value_txt.toInt();
    Sound[sound_id].decay = value;

    value_txt = file.readStringUntil('\n');
    value = value_txt.toInt();
    Sound[sound_id].sustain = value;

    value_txt = file.readStringUntil('\n');
    value = value_txt.toInt();
    Sound[sound_id].release = value;

    value_txt = file.readStringUntil('\n');
    value = value_txt.toInt();
    Sound[sound_id].gain = value;

    Serial.println(F("ArchivingManager::Copy_Sound_from_SD_to_RAM - Done."));
}

bool ArchivingManager::Is_SD_inserted(void)
{
    if (SD.begin(BUILTIN_SDCARD))
    {
        return true;
    }
    return false;
}