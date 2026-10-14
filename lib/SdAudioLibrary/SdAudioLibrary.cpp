#include "SdAudioLibrary.h"
#include "GlobalFRAM.h"
#include <strings.h>
#include <cstring>
#include <cstdlib>
#include <new>

FLASHMEM
bool SdAudioReader::Wav_data(File &file, uint32_t &data_offset, uint32_t &data_length, uint16_t &channels)
{
    const auto read16 = [&file](uint16_t &value) -> bool
    {
        uint8_t bytes[2];
        if (file.read(bytes, sizeof(bytes)) != sizeof(bytes))
        {
            return false;
        }
        value = static_cast<uint16_t>(bytes[0]) | static_cast<uint16_t>(bytes[1]) << 8;
        return true;
    };
    const auto read32 = [&file](uint32_t &value) -> bool
    {
        uint8_t bytes[4];
        if (file.read(bytes, sizeof(bytes)) != sizeof(bytes))
        {
            return false;
        }
        value = static_cast<uint32_t>(bytes[0]) | static_cast<uint32_t>(bytes[1]) << 8 | static_cast<uint32_t>(bytes[2]) << 16 | static_cast<uint32_t>(bytes[3]) << 24;
        return true;
    };
    char id[4];
    uint32_t riff_length;
    if (!file.seek(0) || file.read(id, sizeof(id)) != sizeof(id) || memcmp(id, "RIFF", 4) != 0 || !read32(riff_length) || file.read(id, sizeof(id)) != sizeof(id) || memcmp(id, "WAVE", 4) != 0)
    {
        return false;
    }
    const uint64_t file_length = file.size();
    if (riff_length < 4 || riff_length > UINT32_MAX - 8 || static_cast<uint64_t>(riff_length) + 8 > file_length)
    {
        return false;
    }
    const uint32_t riff_end = riff_length + 8;
    bool format_found = false;
    bool data_found = false;
    uint32_t position = 12;
    while (position <= riff_end && riff_end - position >= 8)
    {
        uint32_t chunk_length;
        if (!file.seek(position) || file.read(id, sizeof(id)) != sizeof(id) || !read32(chunk_length))
        {
            return false;
        }
        const uint32_t chunk_start = position + 8;
        if (chunk_length > riff_end - chunk_start || (chunk_length & 1U) > riff_end - chunk_start - chunk_length)
        {
            return false;
        }
        if (memcmp(id, "fmt ", 4) == 0)
        {
            uint16_t encoding;
            uint32_t sample_rate;
            uint32_t byte_rate;
            uint16_t block_align;
            uint16_t bits;
            if (format_found || chunk_length < 16 || !read16(encoding) || !read16(channels) || !read32(sample_rate) || !read32(byte_rate) || !read16(block_align) || !read16(bits) || encoding != 1 || (channels != 1 && channels != 2) || sample_rate != 44100 || byte_rate != 88200U * channels || block_align != 2U * channels || bits != 16)
            {
                return false;
            }
            format_found = true;
        }
        else if (memcmp(id, "data", 4) == 0)
        {
            if (data_found || chunk_length < 2 || (chunk_length & 1U) != 0)
            {
                return false;
            }
            data_offset = chunk_start;
            data_length = chunk_length;
            data_found = true;
        }
        position = chunk_start + chunk_length + (chunk_length & 1U);
    }
    if (!format_found || !data_found || data_length % (sizeof(int16_t) * channels) != 0)
    {
        return false;
    }
    // Report the mono output size for Flash allocation and the import summary.
    data_length /= channels;
    return true;
}

FLASHMEM
bool SdAudioReader::Aiff_data(File &file, uint32_t &data_offset, uint32_t &data_length, uint16_t &channels)
{
    const auto read16 = [&file](uint16_t &value) -> bool
    {
        uint8_t bytes[2];
        if (file.read(bytes, sizeof(bytes)) != sizeof(bytes))
        {
            return false;
        }
        value = static_cast<uint16_t>(bytes[0]) << 8 | static_cast<uint16_t>(bytes[1]);
        return true;
    };
    const auto read32 = [&file](uint32_t &value) -> bool
    {
        uint8_t bytes[4];
        if (file.read(bytes, sizeof(bytes)) != sizeof(bytes))
        {
            return false;
        }
        value = static_cast<uint32_t>(bytes[0]) << 24 | static_cast<uint32_t>(bytes[1]) << 16 | static_cast<uint32_t>(bytes[2]) << 8 | static_cast<uint32_t>(bytes[3]);
        return true;
    };
    char id[4];
    uint32_t form_length;
    if (!file.seek(0) || file.read(id, sizeof(id)) != sizeof(id) || memcmp(id, "FORM", 4) != 0 || !read32(form_length) || file.read(id, sizeof(id)) != sizeof(id) || memcmp(id, "AIFF", 4) != 0)
    {
        return false;
    }
    if (form_length < 4 || form_length > UINT32_MAX - 8 || static_cast<uint64_t>(form_length) + 8 > file.size())
    {
        return false;
    }
    const uint32_t form_end = form_length + 8;
    bool format_found = false;
    bool data_found = false;
    uint32_t frames = 0;
    uint32_t position = 12;
    while (position <= form_end && form_end - position >= 8)
    {
        uint32_t chunk_length;
        if (!file.seek(position) || file.read(id, sizeof(id)) != sizeof(id) || !read32(chunk_length))
        {
            return false;
        }
        const uint32_t chunk_start = position + 8;
        if (chunk_length > form_end - chunk_start || (chunk_length & 1U) > form_end - chunk_start - chunk_length)
        {
            return false;
        }
        if (memcmp(id, "COMM", 4) == 0)
        {
            uint16_t bits;
            uint8_t sample_rate[10];
            // Canonical 80-bit extended representation of 44100 Hz.
            static constexpr uint8_t rate_44100[10] = {0x40, 0x0E, 0xAC, 0x44, 0, 0, 0, 0, 0, 0};
            if (format_found || chunk_length != 18 || !read16(channels) || !read32(frames) || !read16(bits) || file.read(sample_rate, sizeof(sample_rate)) != sizeof(sample_rate) || (channels != 1 && channels != 2) || bits != 16 || frames == 0 || memcmp(sample_rate, rate_44100, sizeof(sample_rate)) != 0)
            {
                return false;
            }
            format_found = true;
        }
        else if (memcmp(id, "SSND", 4) == 0)
        {
            uint32_t offset;
            uint32_t block_size;
            if (data_found || chunk_length < 8 || !read32(offset) || !read32(block_size) || offset > chunk_length - 8)
            {
                return false;
            }
            // Alignment bytes precede the first frame; trailing block padding is not audio.
            data_offset = chunk_start + 8 + offset;
            data_length = chunk_length - 8 - offset;
            data_found = true;
        }
        position = chunk_start + chunk_length + (chunk_length & 1U);
    }
    if (position != form_end || !format_found || !data_found || static_cast<uint64_t>(frames) * channels * sizeof(int16_t) > data_length)
    {
        return false;
    }
    data_length = frames * sizeof(int16_t);
    return true;
}


FLASHMEM
SdAudioReader::~SdAudioReader()
{
    Close();
}

FLASHMEM
bool SdAudioReader::Audio_name(const char *name)
{
    const char *extension = strrchr(name, '.');
    return extension != nullptr && (strcasecmp(extension, ".raw") == 0 || strcasecmp(extension, ".wav") == 0 || strcasecmp(extension, ".aif") == 0 || strcasecmp(extension, ".aiff") == 0 || strcasecmp(extension, ".mp3") == 0);
}

FLASHMEM
void SdAudioReader::Close()
{
    delete mp3;
    mp3 = nullptr;
    file.close();
    samples = 0;
    position = 0;
}

FLASHMEM
bool SdAudioReader::Open(const char *name)
{
    Close();
    char path[sizeof("/LILLA_AUDIO/") + 256];
    if (!Audio_name(name) || strlen(name) > 255 || strchr(name, '/') != nullptr || strchr(name, '\\') != nullptr)
    {
        return false;
    }
    snprintf(path, sizeof(path), "/LILLA_AUDIO/%s", name);
    file = SD.open(path);
    if (!file || file.isDirectory() || file.size() > UINT32_MAX)
    {
        Close();
        return false;
    }
    const char *extension = strrchr(name, '.');
    format = strcasecmp(extension, ".wav") == 0 ? Format::Wav : strcasecmp(extension, ".mp3") == 0 ? Format::Mp3 : strcasecmp(extension, ".raw") == 0 ? Format::Raw : Format::Aiff;
    uint32_t bytes = file.size();
    offset = 0;
    channels = 1;
    bool valid = bytes >= 2 && (bytes & 1u) == 0;
    if (format == Format::Wav)
    {
        valid = Wav_data(file, offset, bytes, channels);
    }
    else if (format == Format::Aiff)
    {
        valid = Aiff_data(file, offset, bytes, channels);
    }
    else if (format == Format::Mp3)
    {
        mp3 = new (std::nothrow) Mp3Import;
        valid = mp3 != nullptr && mp3->Open(file, bytes, channels, 0x7FFFFFFEu);
    }
    samples = bytes / 2;
    if (!valid || samples == 0 || !Seek(0))
    {
        Close();
        return false;
    }
    return true;
}

FLASHMEM
bool SdAudioReader::Seek(uint32_t sample)
{
    if (sample > samples || !(mp3 != nullptr ? mp3->Seek(sample) : file.seek(offset + sample * 2u * channels)))
    {
        return false;
    }
    position = sample;
    return true;
}

FLASHMEM
bool SdAudioReader::Read(int16_t *destination, uint32_t count)
{
    if (position > samples || count > samples - position)
    {
        return false;
    }
    int16_t input[512];
    while (count != 0)
    {
        const uint32_t chunk = count < 256 ? count : 256;
        if (!(mp3 != nullptr ? mp3->Read(input, chunk) : file.read(input, chunk * channels * 2) == static_cast<size_t>(chunk * channels * 2)))
        {
            return false;
        }
        for (uint32_t i = 0; i < chunk; ++i)
        {
            int32_t sum = 0;
            for (uint16_t channel = 0; channel < channels; ++channel)
            {
                const uint16_t value = static_cast<uint16_t>(input[i * channels + channel]);
                sum += format == Format::Aiff ? static_cast<int16_t>((value >> 8) | (value << 8)) : static_cast<int16_t>(value);
            }
            destination[i] = sum / channels;
        }
        destination += chunk;
        position += chunk;
        count -= chunk;
    }
    return true;
}

DMAMEM SdAudioLibrary::Recipe SdAudioLibrary::recipes[FileNameRegistry::FILES] = {};
bool SdAudioLibrary::available[FileNameRegistry::FILES] = {};
uint32_t SdAudioLibrary::generation = 0;
uint16_t SdAudioLibrary::count = 0;
const char *SdAudioLibrary::error = nullptr;

FLASHMEM
uint32_t SdAudioLibrary::Crc(const Recipe &recipe)
{
    uint32_t crc = 0xFFFFFFFFu;
    const auto *bytes = reinterpret_cast<const uint8_t *>(&recipe);
    for (size_t i = 0; i < offsetof(Recipe, crc); ++i)
    {
        crc ^= bytes[i];
        for (uint8_t bit = 0; bit < 8; ++bit)
        {
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
        }
    }
    return ~crc;
}

FLASHMEM
bool SdAudioLibrary::Begin()
{
    count = 0;
    ++generation;
    memset(available, 0, sizeof(available));
    error = nullptr;
    for (int id = 0; id < FileNameRegistry::FILES; ++id)
    {
        auto &recipe = recipes[id];
        if (LillaFram.readArray(ADDRESS + id * sizeof(Recipe), sizeof(Recipe), reinterpret_cast<uint8_t *>(&recipe)) != LillaFRAM_2x512::ERROR_0)
        {
            error = "SD LIBRARY: FRAM READ ERROR";
            return false;
        }
        if (id == 0 || !FileNameRegistry::Assigned(id) || recipe.crc != Crc(recipe) || recipe.name[0] == 0 || memchr(recipe.name, 0, NAME_BYTES) == nullptr || !SdAudioReader::Audio_name(recipe.name) || recipe.samples == 0)
        {
            recipe = {};
        }
    }
    if (!SD.begin(BUILTIN_SDCARD))
    {
        return true;
    }
    for (int id = 1; id < FileNameRegistry::FILES; ++id)
    {
        if (!Assigned(id))
        {
            continue;
        }
        char path[sizeof("/LILLA_AUDIO/") + NAME_BYTES];
        snprintf(path, sizeof(path), "/LILLA_AUDIO/%s", recipes[id].name);
        File source = SD.open(path);
        available[id] = source && !source.isDirectory() && source.size() == recipes[id].file_bytes;
        source.close();
    }
    File root = SD.open("/LILLA_AUDIO");
    if (!root || !root.isDirectory())
    {
        return true;
    }
    using NameEntry = char[NAME_BYTES];
    NameEntry *names = new (std::nothrow) NameEntry[INDEX_CAPACITY]{};
    if (names == nullptr)
    {
        error = "SD INDEX: NOT ENOUGH RAM";
        return false;
    }
    while (true)
    {
        File entry = root.openNextFile();
        if (!entry)
        {
            break;
        }
        if (!entry.isDirectory() && SdAudioReader::Audio_name(entry.name()) && entry.size() > 0)
        {
            if (strlen(entry.name()) >= NAME_BYTES || count == INDEX_CAPACITY)
            {
                error = "SD INDEX: NAME / CAPACITY LIMIT";
                entry.close();
                break;
            }
            strcpy(names[count++], entry.name());
        }
        entry.close();
    }
    root.close();
    if (error == nullptr)
    {
        qsort(names, count, sizeof(NameEntry), [](const void *a, const void *b) -> int
        {
            const int order = strcasecmp(static_cast<const char *>(a), static_cast<const char *>(b));
            return order == 0 ? strcmp(static_cast<const char *>(a), static_cast<const char *>(b)) : order;
        });
        for (uint16_t i = 0; i < count; ++i)
        {
            if (LillaFram.writeArray(INDEX_ADDRESS + i * NAME_BYTES, NAME_BYTES, reinterpret_cast<uint8_t *>(names[i])) != LillaFRAM_2x512::ERROR_0)
            {
                error = "SD INDEX: FRAM WRITE ERROR";
                break;
            }
        }
    }
    delete[] names;
    if (error != nullptr)
    {
        count = 0; // Never expose a partial or unsorted repository.
    }
    return error == nullptr;
}

FLASHMEM
uint16_t SdAudioLibrary::Count()
{
    return count;
}

FLASHMEM
bool SdAudioLibrary::Name(uint16_t index, char (&name)[NAME_BYTES])
{
    return LillaFram.readArray(INDEX_ADDRESS + index * NAME_BYTES, NAME_BYTES, reinterpret_cast<uint8_t *>(name)) == LillaFRAM_2x512::ERROR_0;
}

FLASHMEM
bool SdAudioLibrary::Assigned(int file)
{
    return file > 0 && file < FileNameRegistry::FILES && recipes[file].samples != 0;
}

FLASHMEM
const char *SdAudioLibrary::Source_name(int file)
{
    return recipes[file].name;
}

FLASHMEM
uint32_t SdAudioLibrary::Samples(int file)
{
    return Assigned(file) && available[file] ? recipes[file].samples : 0;
}

FLASHMEM
const char *SdAudioLibrary::Error()
{
    return error;
}

FLASHMEM
bool SdAudioLibrary::Save(int file, const char *name, uint32_t first, uint32_t samples)
{
    Recipe recipe = {};
    strcpy(recipe.name, name);
    recipe.first = first;
    recipe.samples = samples;
    char path[sizeof("/LILLA_AUDIO/") + NAME_BYTES];
    snprintf(path, sizeof(path), "/LILLA_AUDIO/%s", name);
    File source = SD.open(path);
    if (!source || source.size() > UINT32_MAX)
    {
        return false;
    }
    recipe.file_bytes = source.size();
    source.close();
    recipe.crc = Crc(recipe);
    const uint32_t address = ADDRESS + file * sizeof(Recipe);
    if (LillaFram.writeArray(address, offsetof(Recipe, crc), reinterpret_cast<uint8_t *>(&recipe)) != LillaFRAM_2x512::ERROR_0 || LillaFram.writeLong(address + offsetof(Recipe, crc), recipe.crc) != LillaFRAM_2x512::ERROR_0)
    {
        return false;
    }
    Recipe verified = {};
    if (LillaFram.readArray(address, sizeof(Recipe), reinterpret_cast<uint8_t *>(&verified)) != LillaFRAM_2x512::ERROR_0 || memcmp(&recipe, &verified, sizeof(Recipe)) != 0)
    {
        return false;
    }
    recipes[file] = recipe;
    available[file] = true;
    ++generation;
    return true;
}

FLASHMEM
bool SdAudioLibrary::Read(int file, int16_t *destination, uint32_t first, uint32_t samples)
{
    static SdAudioReader reader;
    static int opened = -1;
    static uint32_t next = 0;
    static uint32_t opened_generation = 0;
    const Recipe &recipe = recipes[file];
    if (first > recipe.samples || samples > recipe.samples - first)
    {
        return false;
    }
    if (opened != file || opened_generation != generation)
    {
        opened = -1;
        if (!reader.Open(recipe.name) || recipe.first > reader.Samples() || recipe.samples > reader.Samples() - recipe.first)
        {
            return false;
        }
        opened = file;
        opened_generation = generation;
        next = 0;
        if (!reader.Seek(recipe.first))
        {
            opened = -1;
            return false;
        }
    }
    if ((next != first && !reader.Seek(recipe.first + first)) || !reader.Read(destination, samples))
    {
        opened = -1;
        return false;
    }
    next = first + samples;
    if (next == recipe.samples)
    {
        reader.Close();
        opened = -1;
    }
    return true;
}

DMAMEM int16_t SdAudioPreview::buffer[BUFFER_SAMPLES];

SdAudioPreview::SdAudioPreview() : AudioStream(0, nullptr) {}

FLASHMEM
void SdAudioPreview::Stop()
{
    const bool audio_enabled = NVIC_IS_ENABLED(IRQ_SOFTWARE) != 0;
    NVIC_DISABLE_IRQ(IRQ_SOFTWARE);
    running = false;
    written = 0;
    consumed = 0;
    if (audio_enabled)
    {
        NVIC_ENABLE_IRQ(IRQ_SOFTWARE);
    }

    reader.Close();
}

FLASHMEM
bool SdAudioPreview::Open(const char *name)
{
    Stop();
    return reader.Open(name) && Trim(0, reader.Samples() - 1);
}

FLASHMEM
bool SdAudioPreview::Trim(uint32_t start, uint32_t end)
{
    const bool audio_enabled = NVIC_IS_ENABLED(IRQ_SOFTWARE) != 0;
    NVIC_DISABLE_IRQ(IRQ_SOFTWARE);
    running = false;
    written = 0;
    consumed = 0;
    if (audio_enabled)
    {
        NVIC_ENABLE_IRQ(IRQ_SOFTWARE);
    }

    first = start;
    last = end;
    position = start;
    if (!reader.Seek(first))
    {
        return false;
    }
    running = true;
    return Service();
}

FLASHMEM
bool SdAudioPreview::Service()
{
    if (!running)
    {
        return true;
    }
    for (uint32_t reads = 0; reads < 32 && written - consumed <= BUFFER_SAMPLES - 256; ++reads)
    {
        if (position > last)
        {
            if (!reader.Seek(first))
            {
                Stop();
                return false;
            }
            position = first;
        }
        const uint32_t remaining = last - position + 1;
        const uint32_t contiguous = BUFFER_SAMPLES - written % BUFFER_SAMPLES;
        const uint32_t chunk = min(256u, min(remaining, contiguous));
        if (!reader.Read(buffer + written % BUFFER_SAMPLES, chunk))
        {
            Stop();
            return false;
        }
        __asm__ volatile("dmb" ::: "memory");
        written = written + chunk;
        position += chunk;
    }
    return true;
}

void SdAudioPreview::update()
{
    if (!running || written - consumed < AUDIO_BLOCK_SAMPLES)
    {
        return;
    }
    audio_block_t *block = allocate();
    if (block == nullptr)
    {
        return;
    }
    for (uint32_t i = 0; i < AUDIO_BLOCK_SAMPLES; ++i)
    {
        block->data[i] = buffer[(consumed + i) % BUFFER_SAMPLES];
    }
    __asm__ volatile("dmb" ::: "memory");
    consumed = consumed + AUDIO_BLOCK_SAMPLES;
    transmit(block);
    release(block);
}
