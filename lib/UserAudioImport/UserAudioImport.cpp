#include "UserAudioImport.h"
#include "Mp3Import.h"
#include <cstring>
#include <strings.h>

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
    if (success)
    {
        success = SD.rename(temporary, destination);
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

FLASHMEM bool UserAudioImport::Merge_index(const char *source, const char *destination, uint32_t entries, uint32_t width)
{
    if (SD.exists(destination) && !SD.remove(destination))
    {
        return false;
    }
    File left = SD.open(source);
    File right = SD.open(source);
    File output = SD.open(destination, FILE_WRITE);
    bool valid = left && right && output;
    Entry a;
    Entry b;
    for (uint32_t start = 0; valid && start < entries; start += 2 * width)
    {
        uint32_t left_count = entries - start < width ? entries - start : width;
        const uint32_t middle = start + left_count;
        uint32_t right_count = entries - middle < width ? entries - middle : width;
        valid = left.seek(start * sizeof(Entry)) && left.read(&a, sizeof(a)) == sizeof(a);
        if (valid && right_count != 0)
        {
            valid = right.seek(middle * sizeof(Entry)) && right.read(&b, sizeof(b)) == sizeof(b);
        }
        while (valid && (left_count != 0 || right_count != 0))
        {
            int order = 0;
            if (left_count != 0 && right_count != 0)
            {
                order = strcasecmp(a.name, b.name);
                if (order == 0)
                {
                    order = strcmp(a.name, b.name);
                }
            }
            const bool take_left = right_count == 0 || (left_count != 0 && order <= 0);
            const Entry &entry = take_left ? a : b;
            valid = output.write(reinterpret_cast<const uint8_t *>(&entry), sizeof(entry)) == sizeof(entry);
            if (take_left)
            {
                --left_count;
                if (valid && left_count != 0)
                {
                    valid = left.read(&a, sizeof(a)) == sizeof(a);
                }
            }
            else
            {
                --right_count;
                if (valid && right_count != 0)
                {
                    valid = right.read(&b, sizeof(b)) == sizeof(b);
                }
            }
        }
    }
    output.flush();
    valid = valid && output.size() == entries * sizeof(Entry) && SD.mediaPresent();
    output.close();
    left.close();
    right.close();
    return valid;
}

FLASHMEM bool UserAudioImport::Build_index(uint32_t &entries)
{
    entries = 0;
    const char *source = "/LILLA_AUDIO/.lilla-index.tmp";
    const char *destination = "/LILLA_AUDIO/.lilla-index.sort";
    bool valid = (!SD.exists(INDEX_PATH) || SD.remove(INDEX_PATH)) && (!SD.exists(source) || SD.remove(source)) && (!SD.exists(destination) || SD.remove(destination));
    File records;
    if (valid)
    {
        records = SD.open(source, FILE_WRITE);
    }
    File directory = SD.open("/LILLA_AUDIO");
    valid = valid && records && directory && directory.isDirectory();
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
                Entry entry{};
                strcpy(entry.name, file.name());
                entry.bytes = file.size();
                entry.samples = entry.bytes / 2;
                valid = records.write(reinterpret_cast<const uint8_t *>(&entry), sizeof(entry)) == sizeof(entry);
                if (valid)
                {
                    ++entries;
                }
            }
        }
        file.close();
    }
    directory.close();
    records.flush();
    valid = valid && records.size() == entries * sizeof(Entry) && SD.mediaPresent();
    records.close();
    if (valid)
    {
        Serial.printf("INDEX: sorting %lu RAW files using SD temporary files\n", static_cast<unsigned long>(entries));
    }
    // Merge runs on SD; only the two current records need to reside in RAM.
    for (uint32_t width = 1; valid && width < entries; width *= 2)
    {
        valid = Merge_index(source, destination, entries, width);
        const char *previous = source;
        source = destination;
        destination = previous;
    }
    if (valid)
    {
        valid = !SD.exists(destination) || SD.remove(destination);
        File index;
        File sorted;
        if (valid)
        {
            sorted = SD.open(source);
            index = SD.open(destination, FILE_WRITE);
        }
        Header header;
        header.entries = entries;
        const uint32_t size = entries * sizeof(Entry);
        valid = sorted && index && index.write(reinterpret_cast<const uint8_t *>(&header), sizeof(header)) == sizeof(header);
        for (uint32_t i = 0; valid && i < entries; ++i)
        {
            Entry entry;
            valid = sorted.read(&entry, sizeof(entry)) == sizeof(entry) && index.write(reinterpret_cast<const uint8_t *>(&entry), sizeof(entry)) == sizeof(entry);
        }
        index.flush();
        valid = valid && index.size() == sizeof(header) + size;
        index.close();
        sorted.close();
        valid = valid && SD.mediaPresent() && SD.rename(destination, INDEX_PATH);
    }
    SD.remove(source);
    SD.remove(destination);
    if (!valid)
    {
        entries = 0;
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
    if (!SD.exists("/LILLA_AUDIO") && !SD.mkdir("/LILLA_AUDIO"))
    {
        Serial.println("ERROR: cannot prepare /LILLA_AUDIO");
        ++report.errors;
        return report;
    }
    if (!SD.exists(INDEX_PATH))
    {
        Serial.println("INDEX: creating initial catalog of /LILLA_AUDIO");
        report.index_ready = Build_index(report.indexed);
        if (!report.index_ready)
        {
            Serial.println("USER AUDIO: initial index failed; import not started");
            ++report.errors;
            return report;
        }
    }
    else
    {
        Serial.println("INDEX: existing catalog retained until final update");
    }
    File directory = SD.open("/USER_AUDIO");
    if (!directory || !directory.isDirectory())
    {
        Serial.println("ERROR: /USER_AUDIO directory unavailable");
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
        if (SD.exists(destination))
        {
            Serial.printf("SKIP: /USER_AUDIO/%s -> %s already exists; unchanged\n", source.name(), destination);
            ++report.skipped;
            source.close();
            continue;
        }
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
    Serial.println("INDEX: updating final catalog of /LILLA_AUDIO");
    report.index_ready = Build_index(report.indexed);
    if (!report.index_ready)
    {
        ++report.errors;
    }
    Serial.printf("USER AUDIO RESULT: copied/converted=%lu, skipped=%lu, errors=%lu, indexed=%lu, index=%s\n", static_cast<unsigned long>(report.copied), static_cast<unsigned long>(report.skipped), static_cast<unsigned long>(report.errors), static_cast<unsigned long>(report.indexed), report.index_ready ? "READY" : "UNAVAILABLE");
    return report;
}
