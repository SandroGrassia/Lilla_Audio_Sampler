#include "SdStorage.h"
#include "SdSoundSource.h"
#include <cstring>
#include <cstddef>

SdSoundSource::Loaded SdSoundSource::loaded[17];
uint8_t SdSoundSource::next_slot = 1;
int SdSoundSource::next_identity = SdSoundSource::FIRST;

bool SdSoundSource::Is_sd(int file)
{
    return file >= FIRST && file <= PREVIEW;
}

SdSoundSource::Loaded *SdSoundSource::Find(int file)
{
    for (auto &item : loaded)
    {
        if (item.file == file)
        {
            return &item;
        }
    }
    return nullptr;
}

FLASHMEM SdSoundSource::SourceStatus SdSoundSource::Diagnose(int file)
{
    if (!SD.mediaPresent())
    {
        return SourceStatus::MissingCard;
    }
    File directory = SD.open("/AUDIO_FILES");
    bool has_audio = false;
    while (directory)
    {
        File entry = directory.openNextFile();
        if (!entry)
        {
            break;
        }
        const char *extension = strrchr(entry.name(), '.');
        if (!entry.isDirectory() && extension != nullptr && strcasecmp(extension, ".raw") == 0 && entry.size() > 0)
        {
            has_audio = true;
            break;
        }
    }
    if (!SD.mediaPresent())
    {
        return SourceStatus::MissingCard;
    }
    if (!has_audio)
    {
        return SourceStatus::EmptyLibrary;
    }
    // Preview selection can fail before a new filename has been assigned.
    if (file == PREVIEW)
    {
        return SourceStatus::Unavailable;
    }
    Reference reference;
    const auto *item = Find(file);
    if (item != nullptr)
    {
        reference = item->reference;
    }
    else
    {
        char path[64];
        Reference_path(file, path);
        File input = SD.open(path);
        if (!input || input.size() != sizeof(reference) || input.read(&reference, sizeof(reference)) != sizeof(reference) || reference.magic != 0x3153444C || reference.checksum != Checksum(reference) || memchr(reference.name, 0, sizeof(reference.name)) == nullptr || strchr(reference.name, '/') != nullptr || strchr(reference.name, '\\') != nullptr)
        {
            return SourceStatus::MissingReference;
        }
    }
    char path[sizeof("/AUDIO_FILES/") + 256];
    snprintf(path, sizeof(path), "/AUDIO_FILES/%s", reference.name);
    return SD.exists(path) ? SourceStatus::Unavailable : SourceStatus::MissingFile;
}

FLASHMEM uint32_t SdSoundSource::Checksum(const Reference &reference)
{
    uint32_t hash = 2166136261u;
    const auto *bytes = reinterpret_cast<const uint8_t *>(&reference);
    for (size_t i = 0; i < offsetof(Reference, checksum); ++i)
    {
        hash = (hash ^ bytes[i]) * 16777619u;
    }
    return hash;
}

FLASHMEM void SdSoundSource::Reference_path(int file, char (&path)[64])
{
    snprintf(path, sizeof(path), "/LILLA_PRIVATE/.lilla-sources/%05d.ref", file);
}

FLASHMEM bool SdSoundSource::Read_header(File &index, UserAudioImport::Header &header)
{
    return index && index.read(&header, sizeof(header)) == sizeof(header) && header.magic == 0x4C524157 && header.version == 1 && header.entries <= UserAudioImport::CAPACITY && header.entry_bytes == sizeof(UserAudioImport::Entry) && header.sample_rate == 44100 && header.channels == 1 && header.bits == 16 && index.size() == sizeof(header) + header.entries * sizeof(UserAudioImport::Entry);
}

FLASHMEM uint32_t SdSoundSource::Catalog_count()
{
    if (!SdStorage::Begin())
    {
        return 0;
    }
    File index = SD.open(UserAudioImport::INDEX_PATH);
    UserAudioImport::Header header;
    return Read_header(index, header) ? header.entries : 0;
}

FLASHMEM bool SdSoundSource::Catalog_entry(uint32_t position, UserAudioImport::Entry &entry)
{
    File index = SD.open(UserAudioImport::INDEX_PATH);
    UserAudioImport::Header header;
    if (!Read_header(index, header) || position >= header.entries || !index.seek(sizeof(header) + position * sizeof(entry)) || index.read(&entry, sizeof(entry)) != sizeof(entry))
    {
        return false;
    }
    return memchr(entry.name, 0, sizeof(entry.name)) != nullptr && entry.name[0] != 0 && strchr(entry.name, '/') == nullptr && strchr(entry.name, '\\') == nullptr && entry.bytes > 0 && (entry.bytes & 1u) == 0 && entry.samples == entry.bytes / 2;
}

FLASHMEM bool SdSoundSource::Find_catalog(const char *name, uint32_t count, uint32_t &position)
{
    uint32_t first = 0;
    while (first < count)
    {
        const uint32_t middle = first + (count - first) / 2;
        UserAudioImport::Entry entry;
        if (!Catalog_entry(middle, entry))
        {
            return false;
        }
        const int order = strcasecmp(entry.name, name);
        if (order == 0)
        {
            position = middle;
            return true;
        }
        if (order < 0)
        {
            first = middle + 1;
        }
        else
        {
            count = middle;
        }
    }
    return false;
}

FLASHMEM bool SdSoundSource::Select(uint32_t index, uint32_t capacity)
{
    UserAudioImport::Entry entry;
    if (!SD.mediaPresent() || !Catalog_entry(index, entry))
    {
        return false;
    }
    loaded[0] = {};
    loaded[0].file = PREVIEW;
    auto &reference = loaded[0].reference;
    strcpy(reference.name, entry.name);
    reference.original_bytes = entry.bytes;
    reference.samples = entry.samples < capacity ? entry.samples : capacity;
    return true;
}

FLASHMEM bool SdSoundSource::Prepare(int file)
{
    if (Find(file) != nullptr)
    {
        return true;
    }
    if (!SdStorage::Begin())
    {
        return false;
    }
    char path[64];
    Reference_path(file, path);
    File input = SD.open(path);
    Reference reference;
    if (!input || input.size() != sizeof(reference) || input.read(&reference, sizeof(reference)) != sizeof(reference) || reference.magic != 0x3153444C || reference.checksum != Checksum(reference) || memchr(reference.name, 0, sizeof(reference.name)) == nullptr || strchr(reference.name, '/') != nullptr || strchr(reference.name, '\\') != nullptr || reference.samples == 0 || reference.first > reference.original_bytes / 2 || reference.samples > reference.original_bytes / 2 - reference.first)
    {
        return false;
    }
    // Never evict a descriptor whose cache is still bound to audio readers.
    for (int attempt = 0; attempt < 16; ++attempt)
    {
        const uint8_t slot = next_slot;
        next_slot = next_slot == 16 ? 1 : next_slot + 1;
        if (loaded[slot].data == nullptr)
        {
            loaded[slot] = {};
            loaded[slot].file = file;
            loaded[slot].reference = reference;
            return true;
        }
    }
    return false;
}

uint32_t SdSoundSource::Samples(int file)
{
    const auto *item = Find(file);
    return item == nullptr ? 0 : item->reference.samples;
}

const char *SdSoundSource::Name(int file)
{
    const auto *item = Find(file);
    return item == nullptr ? "" : item->reference.name;
}

FLASHMEM bool SdSoundSource::Load(int file, int16_t *destination)
{
    auto *item = Find(file);
    if (item == nullptr)
    {
        return false;
    }
    const auto &reference = item->reference;
    char path[sizeof("/AUDIO_FILES/") + 256];
    snprintf(path, sizeof(path), "/AUDIO_FILES/%s", reference.name);
    File input = SD.open(path);
    if (!input || input.size() != reference.original_bytes || !input.seek(reference.first * 2))
    {
        return false;
    }
    uint32_t remaining = reference.samples;
    while (remaining != 0)
    {
        const uint32_t count = remaining < 2048u ? remaining : 2048u;
        if (input.read(destination, count * 2) != static_cast<int>(count * 2))
        {
            return false;
        }
        destination += count;
        remaining -= count;
    }
    return SD.mediaPresent();
}

FLASHMEM bool SdSoundSource::Read(int file, int16_t *destination, int first, int count)
{
    const auto *item = Find(file);
    if (item == nullptr || item->data == nullptr || first < 0 || count < 0 || static_cast<uint32_t>(first) > item->reference.samples || static_cast<uint32_t>(count) > item->reference.samples - first)
    {
        memset(destination, 0, count > 0 ? count * sizeof(int16_t) : 0);
        return false;
    }
    memcpy(destination, item->data + first, count * sizeof(int16_t));
    return true;
}

FLASHMEM void SdSoundSource::Bind(int file, const int16_t *data)
{
    auto *item = Find(file);
    if (item != nullptr)
    {
        item->data = data;
    }
}

FLASHMEM void SdSoundSource::Unbind_all()
{
    for (auto &item : loaded)
    {
        item.data = nullptr;
    }
}

FLASHMEM int SdSoundSource::Save(uint32_t first, uint32_t last, bool (*identity_used)(int))
{
    const auto *preview = Find(PREVIEW);
    if (preview == nullptr || preview->data == nullptr || first > last || last >= preview->reference.samples || (!SD.exists("/LILLA_PRIVATE/.lilla-sources") && !SD.mkdir("/LILLA_PRIVATE/.lilla-sources")))
    {
        return -1;
    }
    char path[64];
    while (next_identity < PREVIEW)
    {
        Reference_path(next_identity, path);
        if (!SD.exists(path) && (identity_used == nullptr || !identity_used(next_identity)))
        {
            break;
        }
        ++next_identity;
    }
    if (next_identity == PREVIEW)
    {
        return -1;
    }
    Reference reference = preview->reference;
    reference.first += first;
    reference.samples = last - first + 1;
    reference.checksum = Checksum(reference);
    const char *temporary = "/LILLA_PRIVATE/.lilla-sources/.new";
    if (SD.exists(temporary) && !SD.remove(temporary))
    {
        return -1;
    }
    File output = SD.open(temporary, FILE_WRITE);
    bool valid = output && output.write(reinterpret_cast<const uint8_t *>(&reference), sizeof(reference)) == sizeof(reference);
    output.flush();
    valid = valid && output.size() == sizeof(reference);
    output.close();
    if (!valid || !SD.rename(temporary, path))
    {
        SD.remove(temporary);
        return -1;
    }
    return next_identity++;
}
