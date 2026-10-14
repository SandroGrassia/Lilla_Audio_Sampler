#include "SdAudioLibrary.h"
#include "GlobalFRAM.h"
#include <strings.h>
#include <cstring>
#include <cstdlib>
#include <new>
#include <cmath>
#include "SharedElements.h"

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
    position = UINT32_MAX;
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
    if (sample == position)
    {
        return true;
    }
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

SdAudioReader SdAudioLibrary::cache_reader;
int SdAudioLibrary::read_file = -1;
uint32_t SdAudioLibrary::read_next = 0;
uint32_t SdAudioLibrary::read_generation = 0;

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
    Suspend_read();
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
        const char *temporary = "/LILLA_AUDIO/.lilla-index.tmp";
        SD.remove(temporary);
        File index = SD.open(temporary, FILE_WRITE);
        const size_t bytes = static_cast<size_t>(count) * NAME_BYTES;
        if (!index || index.write(reinterpret_cast<const uint8_t *>(names), bytes) != bytes)
        {
            error = "SD INDEX: WRITE ERROR";
        }
        index.flush();
        const bool complete = index && index.size() == bytes;
        index.close();
        if (error == nullptr && (!complete || (SD.exists(INDEX_PATH) && !SD.remove(INDEX_PATH)) || !SD.rename(temporary, INDEX_PATH)))
        {
            error = "SD INDEX: PUBLISH ERROR";
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
    File source = SD.open(INDEX_PATH);
    const bool success = source && source.seek(static_cast<uint32_t>(index) * NAME_BYTES) && source.read(name, NAME_BYTES) == NAME_BYTES && memchr(name, 0, NAME_BYTES) != nullptr;
    source.close();
    return success;
}

FLASHMEM
bool SdAudioLibrary::Assigned(int file)
{
    return SdPlayerCache::Assigned(file) || (file > 0 && file < FileNameRegistry::FILES && recipes[file].samples != 0);
}

FLASHMEM
const char *SdAudioLibrary::Source_name(int file)
{
    return SdPlayerCache::Assigned(file) ? SdPlayerCache::Name() : recipes[file].name;
}

FLASHMEM
uint32_t SdAudioLibrary::Samples(int file)
{
    return SdPlayerCache::Assigned(file) ? SdPlayerCache::Samples() : (Assigned(file) && available[file] ? recipes[file].samples : 0);
}

FLASHMEM
bool SdAudioLibrary::Check(int file)
{
    char path[sizeof("/LILLA_AUDIO/") + NAME_BYTES];
    snprintf(path, sizeof(path), "/LILLA_AUDIO/%s", recipes[file].name);
    File source = SD.mediaPresent() ? SD.open(path) : File();
    available[file] = source && !source.isDirectory() && source.size() == recipes[file].file_bytes;
    source.close();
    return available[file];
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
void SdAudioLibrary::Suspend_read()
{
    cache_reader.Close();
    read_file = -1;
}

FLASHMEM
bool SdAudioLibrary::Read(int file, int16_t *destination, uint32_t first, uint32_t samples)
{
    if (SdPlayerCache::Assigned(file))
    {
        return SdPlayerCache::Read_foreground(destination, first, samples);
    }
    const Recipe &recipe = recipes[file];
    if (first > recipe.samples || samples > recipe.samples - first)
    {
        return false;
    }
    if (read_file != file || read_generation != generation)
    {
        read_file = -1;
        if (!cache_reader.Open(recipe.name) || recipe.first > cache_reader.Samples() || recipe.samples > cache_reader.Samples() - recipe.first)
        {
            return false;
        }
        read_file = file;
        read_generation = generation;
        read_next = 0;
        if (!cache_reader.Seek(recipe.first))
        {
            read_file = -1;
            return false;
        }
    }
    if ((read_next != first && !cache_reader.Seek(recipe.first + first)) || !cache_reader.Read(destination, samples))
    {
        read_file = -1;
        return false;
    }
    read_next = first + samples;
    if (read_next == recipe.samples)
    {
        cache_reader.Close();
        read_file = -1;
    }
    return true;
}

SdPlayerCache::Page SdPlayerCache::pages[PAGES];
int16_t *SdPlayerCache::data = nullptr;
SdAudioReader SdPlayerCache::reader;
int SdPlayerCache::file_id = -1;
char SdPlayerCache::name[SdAudioLibrary::NAME_BYTES] = {};
volatile bool SdPlayerCache::failed = false;

FLASHMEM bool SdPlayerCache::Open(int file, const char *filename)
{
    Close();
    data = new (std::nothrow) int16_t[PAGES * PAGE_SAMPLES];
    if (data == nullptr || !reader.Open(filename))
    {
        Close();
        failed = true;
        return false;
    }
    strcpy(name, filename);
    file_id = file;
    return true;
}

FLASHMEM void SdPlayerCache::Close()
{
    file_id = -1;
    reader.Close();
    delete[] data;
    data = nullptr;
    failed = false;
    for (auto &page : pages)
    {
        page.index = -1;
        page.ready = false;
        page.loading = false;
        page.cycle = 0;
    }
}

bool SdPlayerCache::Assigned(int file) { return file >= 0 && file == file_id; }
int SdPlayerCache::File_id() { return file_id; }
uint32_t SdPlayerCache::Samples() { return reader.Samples(); }
const char *SdPlayerCache::Name() { return name; }
bool SdPlayerCache::Failed() { return failed; }

FLASHMEM bool SdPlayerCache::Read_foreground(int16_t *destination, uint32_t first, uint32_t count)
{
    return reader.Seek(first) && reader.Read(destination, count);
}

int SdPlayerCache::Find(int index)
{
    for (uint8_t id = 0; id < PAGES; ++id)
    {
        if (pages[id].index == index)
        {
            return id;
        }
    }
    return -1;
}

bool SdPlayerCache::Require(int first, int count, uint32_t cycle)
{
    bool ready = !failed;
    const int end = first + count;
    for (int index = first / PAGE_SAMPLES; index <= (end - 1) / static_cast<int>(PAGE_SAMPLES) && count > 0; ++index)
    {
        int id = Find(index);
        if (id < 0)
        {
            uint32_t oldest = 0;
            for (uint8_t candidate = 0; candidate < PAGES; ++candidate)
            {
                const auto &page = pages[candidate];
                const uint32_t age = cycle - page.cycle;
                if (!page.loading && (page.index < 0 || (age > 2 && age > oldest)))
                {
                    id = candidate;
                    oldest = age;
                    if (page.index < 0)
                    {
                        break;
                    }
                }
            }
            if (id < 0)
            {
                ready = false;
                continue;
            }
            pages[id].ready = false;
            pages[id].index = index;
        }
        pages[id].cycle = cycle;
        ready &= pages[id].ready;
    }
    return ready;
}

// Cover every interpolation/IIR read at pitch <= 4, including outgoing crossfades.
bool SdPlayerCache::Require_geometry(int first, int last, int crossfade, uint8_t mode, float position, uint32_t cycle)
{
    constexpr int history = 128;
    constexpr int ahead = 640;
    const int current = static_cast<int>(floorf(position));
    if (mode == ONCE_FWD || mode == ONCE_REV)
    {
        const int center = mode == ONCE_REV ? 2 * last - current : current;
        const int low = center - (mode == ONCE_REV ? ahead : history);
        const int high = center + (mode == ONCE_REV ? history : ahead);
        const int begin = low < first ? first : low;
        const int end = high > last ? last : high;
        return end < begin || Require(begin, end - begin + 1, cycle);
    }
    const bool pingpong = mode == LOOP_FWD_REV || mode == LOOP_REV_FWD;
    const int span = last - first + 1;
    const int fade = pingpong ? 0 : crossfade;
    const int period = pingpong ? 2 * (span - 1) : span - fade;
    const int base = mode == LOOP_REV ? last - fade : first + fade;
    const int position_phase = mode == LOOP_FWD && current < base ? 0 : current - base;
    int phase = ((position_phase - history) % period + period) % period;
    int remaining = history + ahead + 1;
    bool ready = true;
    while (remaining > 0)
    {
        const int raw = span - 2 * fade;
        const bool reverse = pingpong ? phase >= span : mode == LOOP_REV;
        const bool from_fade = !pingpong && phase >= raw;
        const int boundary = pingpong ? (reverse ? period : span) : (from_fade ? period : raw);
        const int count = remaining < boundary - phase ? remaining : boundary - phase;
        if (!from_fade)
        {
            const int source = pingpong ? (reverse ? first + period - phase : first + phase) : (reverse ? last - fade - phase : first + fade + phase);
            ready &= Require(reverse ? source - count + 1 : source, count, cycle);
        }
        remaining -= count;
        phase = (phase + count) % period;
    }
    return ready;
}

FLASHMEM bool SdPlayerCache::Service()
{
    int16_t temporary[PAGE_SAMPLES];
    for (uint8_t id = 0, loads = 0; id < PAGES && loads < 8; ++id)
    {
        auto &page = pages[id];
        const bool enabled = NVIC_IS_ENABLED(IRQ_SOFTWARE) != 0;
        NVIC_DISABLE_IRQ(IRQ_SOFTWARE);
        const int index = page.index;
        const bool load = index >= 0 && !page.ready && !page.loading;
        if (load)
        {
            page.loading = true;
        }
        if (enabled)
        {
            NVIC_ENABLE_IRQ(IRQ_SOFTWARE);
        }
        if (!load)
        {
            continue;
        }
        ++loads;
        const uint32_t first = index * PAGE_SAMPLES;
        const uint32_t count = min(PAGE_SAMPLES, Samples() - first);
        if (!Read_foreground(temporary, first, count))
        {
            failed = true;
            page.loading = false;
            return false;
        }
        memcpy(data + id * PAGE_SAMPLES, temporary, count * sizeof(int16_t));
        __asm__ volatile("dmb" ::: "memory");
        page.ready = true;
        page.loading = false;
    }
    return !failed;
}

void SdPlayerCache::Read_cached(int16_t *destination, int first, int count)
{
    while (count > 0)
    {
        const int index = first / PAGE_SAMPLES;
        const int offset = first % PAGE_SAMPLES;
        const int amount = min(count, static_cast<int>(PAGE_SAMPLES) - offset);
        const int id = Find(index);
        if (id < 0 || !pages[id].ready)
        {
            memset(destination, 0, amount * sizeof(int16_t));
            failed = true; // A missing preflight range is a reader fault, never an IRQ SD read.
        }
        else
        {
            memcpy(destination, data + id * PAGE_SAMPLES + offset, amount * sizeof(int16_t));
        }
        first += amount;
        destination += amount;
        count -= amount;
    }
}
