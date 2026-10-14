#include "UserAudioImport.h"
#include "Mp3Import.h"
#include <cstring>
#include <strings.h>
#include <cstdlib>
#include <new>

static_assert(sizeof(UserAudioImport::Entry) == 264);
static_assert(sizeof(UserAudioImport::Header) == 24);

FLASHMEM UserAudioImport::Format UserAudioImport::Get_format(const char *name)
{
    const char *extension = strrchr(name, '.');
    if (extension == nullptr)
    {
        return Format::Unknown;
    }
    if (strcasecmp(extension, ".raw") == 0)
    {
        return Format::Raw;
    }
    if (strcasecmp(extension, ".wav") == 0)
    {
        return Format::Wav;
    }
    if (strcasecmp(extension, ".aif") == 0 || strcasecmp(extension, ".aiff") == 0)
    {
        return Format::Aiff;
    }
    if (strcasecmp(extension, ".mp3") == 0)
    {
        return Format::Mp3;
    }
    return Format::Unknown;
}

FLASHMEM bool UserAudioImport::Copy(File &source, const char *destination, Format format, PcmParser wav, PcmParser aiff)
{
    const char *previous = "/LILLA_AUDIO/.user-import.previous";
    if (SD.exists(previous))
    {
        Serial.println("  ERROR: previous interrupted replacement needs recovery: .user-import.previous");
        return false;
    }
    if (source.size() > UINT32_MAX)
    {
        return false;
    }
    uint32_t bytes = source.size();
    uint32_t offset = 0;
    uint16_t channels = 1;
    Mp3Import mp3;
    bool valid = true;
    if (format == Format::Wav)
    {
        valid = wav(source, offset, bytes, channels);
    }
    else if (format == Format::Aiff)
    {
        valid = aiff(source, offset, bytes, channels);
    }
    else if (format == Format::Mp3)
    {
        valid = mp3.Open(source, bytes, channels, 0xFFFFFFFEu);
    }
    if (!valid || bytes < 2 || (bytes & 1u) != 0 || (format != Format::Mp3 && !source.seek(offset)))
    {
        Serial.println("  ERROR: unsupported or invalid audio");
        return false;
    }
    const char *temporary = "/LILLA_AUDIO/.user-import.tmp";
    if (SD.exists(temporary) && !SD.remove(temporary))
    {
        return false;
    }
    File output = SD.open(temporary, FILE_WRITE);
    bool success = static_cast<bool>(output);
    int16_t buffer[1024];
    for (uint32_t written = 0; success && written < bytes;)
    {
        const uint32_t remaining = bytes - written;
        const uint32_t capacity = sizeof(buffer) / channels;
        const uint32_t chunk = remaining < capacity ? remaining : capacity;
        const uint32_t frames = chunk / sizeof(int16_t);
        success = format == Format::Mp3 ? mp3.Read(buffer, frames) : source.read(buffer, chunk * channels) == static_cast<int>(chunk * channels);
        if (!success)
        {
            break;
        }
        if (format == Format::Aiff)
        {
            for (uint32_t i = 0; i < frames * channels; ++i)
            {
                const uint16_t value = static_cast<uint16_t>(buffer[i]);
                buffer[i] = static_cast<int16_t>((value >> 8) | (value << 8));
            }
        }
        if (channels == 2)
        {
            for (uint32_t i = 0; i < frames; ++i)
            {
                buffer[i] = static_cast<int16_t>((static_cast<int32_t>(buffer[i * 2]) + buffer[i * 2 + 1]) / 2);
            }
        }
        success = output.write(reinterpret_cast<const uint8_t *>(buffer), chunk) == chunk;
        written += chunk;
    }
    output.flush();
    success = success && output.size() == bytes;
    output.close();
    bool backed_up = false;
    if (success && SD.exists(destination))
    {
        Serial.println("  overwriting existing destination");
        success = SD.rename(destination, previous);
        backed_up = success;
    }
    if (success)
    {
        success = SD.rename(temporary, destination);
        if (!success && backed_up && !SD.rename(previous, destination))
        {
            Serial.printf("  RECOVERY REQUIRED: rename %s to %s\n", previous, destination);
        }
    }
    if (success && backed_up)
    {
        success = SD.remove(previous);
    }
    if (!success)
    {
        Serial.println("  ERROR: SD read, write or rename failed");
        SD.remove(temporary);
        return false;
    }
    Serial.printf("  OK: %lu bytes, %lu samples, mono 16-bit 44100 Hz\n", static_cast<unsigned long>(bytes), static_cast<unsigned long>(bytes / 2));
    return true;
}

FLASHMEM bool UserAudioImport::Build_index(uint32_t &entries)
{
    entries = 0;
    Entry *list = new (std::nothrow) Entry[CAPACITY]{};
    if (list == nullptr)
    {
        Serial.println("INDEX ERROR: not enough RAM");
        return false;
    }
    File directory = SD.open("/LILLA_AUDIO");
    bool valid = directory && directory.isDirectory();
    while (valid)
    {
        File file = directory.openNextFile();
        if (!file)
        {
            break;
        }
        if (!file.isDirectory() && Get_format(file.name()) == Format::Raw)
        {
            if (entries == CAPACITY || strlen(file.name()) >= sizeof(Entry::name) || file.size() < 2 || file.size() > UINT32_MAX || (file.size() & 1u) != 0)
            {
                Serial.printf("INDEX ERROR: capacity, filename or invalid RAW: %s\n", file.name());
                valid = false;
            }
            else
            {
                auto &entry = list[entries++];
                strcpy(entry.name, file.name());
                entry.bytes = file.size();
                entry.samples = entry.bytes / 2;
            }
        }
        file.close();
    }
    directory.close();
    valid = valid && SD.mediaPresent();
    const char *temporary = "/LILLA_AUDIO/.lilla-index.tmp";
    if (valid)
    {
        qsort(list, entries, sizeof(Entry), [](const void *left, const void *right) -> int
        {
            const char *a = static_cast<const Entry *>(left)->name;
            const char *b = static_cast<const Entry *>(right)->name;
            const int order = strcasecmp(a, b);
            return order != 0 ? order : strcmp(a, b);
        });
        valid = !SD.exists(temporary) || SD.remove(temporary);
        File index;
        if (valid)
        {
            index = SD.open(temporary, FILE_WRITE);
        }
        Header header;
        header.entries = entries;
        const uint32_t size = entries * sizeof(Entry);
        valid = index && index.write(reinterpret_cast<const uint8_t *>(&header), sizeof(header)) == sizeof(header) && index.write(reinterpret_cast<const uint8_t *>(list), size) == size;
        index.flush();
        valid = valid && index.size() == sizeof(header) + size;
        index.close();
        valid = valid && SD.rename(temporary, INDEX_PATH);
    }
    delete[] list;
    if (!valid)
    {
        SD.remove(temporary);
        Serial.println("INDEX ERROR: index not published");
    }
    else
    {
        Serial.printf("INDEX OK: %lu alphabetically sorted RAW files\n", static_cast<unsigned long>(entries));
    }
    return valid;
}

FLASHMEM UserAudioImport::Report UserAudioImport::Run(PcmParser wav, PcmParser aiff)
{
    Report report;
    Serial.println("USER AUDIO: /USER_AUDIO -> /LILLA_AUDIO");
    if (!SD.begin(BUILTIN_SDCARD))
    {
        Serial.println("ERROR: SD card unavailable");
        ++report.errors;
        return report;
    }
    File directory = SD.open("/USER_AUDIO");
    if (!directory || !directory.isDirectory())
    {
        Serial.println("ERROR: /USER_AUDIO directory unavailable");
        ++report.errors;
        return report;
    }
    if ((!SD.exists("/LILLA_AUDIO") && !SD.mkdir("/LILLA_AUDIO")) || (SD.exists(INDEX_PATH) && !SD.remove(INDEX_PATH)))
    {
        Serial.println("ERROR: cannot prepare /LILLA_AUDIO or invalidate old index");
        ++report.errors;
        return report;
    }
    while (true)
    {
        File source = directory.openNextFile();
        if (!source)
        {
            break;
        }
        const Format format = Get_format(source.name());
        if (source.isDirectory() || format == Format::Unknown)
        {
            Serial.printf("SKIP: %s\n", source.name());
            ++report.skipped;
            source.close();
            continue;
        }
        char destination[sizeof("/LILLA_AUDIO/") + 256];
        const size_t stem = strrchr(source.name(), '.') - source.name();
        if (stem == 0 || stem + sizeof(".raw") > 256)
        {
            Serial.printf("ERROR: invalid destination name: %s\n", source.name());
            ++report.errors;
            source.close();
            continue;
        }
        snprintf(destination, sizeof(destination), "/LILLA_AUDIO/%.*s.raw", static_cast<int>(stem), source.name());
        Serial.printf("%s: /USER_AUDIO/%s -> %s\n", format == Format::Raw ? "COPY" : "CONVERT", source.name(), destination);
        if (Copy(source, destination, format, wav, aiff))
        {
            ++report.copied;
        }
        else
        {
            ++report.errors;
        }
        source.close();
    }
    directory.close();
    report.index_ready = Build_index(report.indexed);
    if (!report.index_ready)
    {
        ++report.errors;
    }
    Serial.printf("USER AUDIO RESULT: copied/converted=%lu, skipped=%lu, errors=%lu, indexed=%lu, index=%s\n", static_cast<unsigned long>(report.copied), static_cast<unsigned long>(report.skipped), static_cast<unsigned long>(report.errors), static_cast<unsigned long>(report.indexed), report.index_ready ? "READY" : "UNAVAILABLE");
    return report;
}
