#include "SdStorage.h"
#include <cstring>

FLASHMEM bool SdStorage::Begin()
{
    if (!SD.begin(BUILTIN_SDCARD))
    {
        return false;
    }
    if (!SD.exists("/LILLA_PRIVATE") && SD.exists("/LILLA_HIDDEN") && !SD.rename("/LILLA_HIDDEN", "/LILLA_PRIVATE"))
    {
        return false;
    }
    // Move metadata before exposing the legacy audio directory. Never overwrite an existing destination.
    const char *old_paths[] = {"/LILLA_PRIVATE/AUDIO_FILES/.lilla-sources", "/LILLA_PRIVATE/AUDIO_FILES/.lilla-index", "/LILLA_PRIVATE/AUDIO_FILES", "/SD_AUDIO_FILES", "/FLASH_AUDIO_FILES"};
    const char *new_paths[] = {"/LILLA_PRIVATE/.lilla-sources", "/LILLA_PRIVATE/.lilla-index", "/AUDIO_FILES", "/AUDIO_FILES", "/UPLOAD_AUDIO_FILES"};
    const bool migrate_audio = !SD.exists("/AUDIO_FILES");
    for (size_t i = 0; i < sizeof(old_paths) / sizeof(old_paths[0]); ++i)
    {
        if (i < 3 && !migrate_audio)
        {
            continue;
        }
        if (!SD.exists(new_paths[i]) && SD.exists(old_paths[i]) && !SD.rename(old_paths[i], new_paths[i]))
        {
            return false;
        }
    }
    return !SD.exists("/LILLA_PRIVATE") || Hide();
}

FLASHMEM bool SdStorage::Ensure()
{
    if (!SD.exists("/LILLA_PRIVATE") && !SD.mkdir("/LILLA_PRIVATE"))
    {
        return false;
    }
    return Hide();
}

bool SdStorage::Hide_entry(uint8_t *entry, size_t bytes, bool exfat)
{
    if (!exfat)
    {
        if (bytes != 32 || entry[0] == 0 || entry[0] == 0xE5 || (entry[11] & 0x18) != 0x10)
        {
            return false;
        }
        entry[11] |= 0x06;
        return true;
    }
    if (bytes < 96 || entry[0] != 0x85 || bytes != (static_cast<size_t>(entry[1]) + 1) * 32 || (entry[4] & 0x10) == 0 || entry[32] != 0xC0)
    {
        return false;
    }
    uint16_t checksum = 0;
    for (size_t i = 0; i < bytes; ++i)
    {
        if (i != 2 && i != 3)
        {
            checksum = static_cast<uint16_t>(((checksum & 1) ? 0x8000 : 0) + (checksum >> 1) + entry[i]);
        }
    }
    if (checksum != static_cast<uint16_t>(entry[2] | (entry[3] << 8)))
    {
        return false;
    }
    entry[4] |= 0x06;
    checksum = 0;
    for (size_t i = 0; i < bytes; ++i)
    {
        if (i != 2 && i != 3)
        {
            checksum = static_cast<uint16_t>(((checksum & 1) ? 0x8000 : 0) + (checksum >> 1) + entry[i]);
        }
    }
    entry[2] = checksum & 0xFF;
    entry[3] = checksum >> 8;
    return true;
}

FLASHMEM bool SdStorage::Hide()
{
    FsFile directory = SD.sdfs.open("/LILLA_PRIVATE", O_RDONLY);
    if (!directory || !directory.isDir())
    {
        return false;
    }
    const uint32_t offset = directory.dirIndex() * 32u;
    directory.close();
    FsFile root = SD.sdfs.open("/", O_RDONLY);
    uint8_t entries[608];
    if (!root || !root.seekSet(offset) || root.read(entries, 32) != 32)
    {
        return false;
    }
    fspos_t position;
    root.fgetpos(&position);
    const bool exfat = SD.sdfs.fatType() == FAT_TYPE_EXFAT;
    const size_t bytes = exfat ? (static_cast<size_t>(entries[1]) + 1) * 32 : 32;
    uint8_t original[32];
    memcpy(original, entries, sizeof(original));
    if (bytes > sizeof(entries) || (bytes > 32 && root.read(entries + 32, bytes - 32) != static_cast<int>(bytes - 32)) || !Hide_entry(entries, bytes, exfat))
    {
        return false;
    }
    root.close();
    if (memcmp(original, entries, sizeof(original)) == 0)
    {
        return true;
    }
    uint32_t sector;
    if (SD.sdfs.fatType() == FAT_TYPE_FAT16)
    {
        FatVolume volume;
        if (!volume.begin(SD.sdfs.card(), false, 1) && !volume.begin(SD.sdfs.card(), false, 0))
        {
            return false;
        }
        sector = volume.rootDirStart() + offset / 512;
    }
    else
    {
        if (position.cluster < 2)
        {
            return false;
        }
        const uint32_t sectors_per_cluster = SD.sdfs.sectorsPerCluster();
        sector = SD.sdfs.dataStartSector() + (position.cluster - 2) * sectors_per_cluster + (offset / 512) % sectors_per_cluster;
    }
    uint8_t buffer[512];
    auto *card = SD.sdfs.card();
    if (!card->readSector(sector, buffer) || memcmp(buffer + offset % 512, original, sizeof(original)) != 0)
    {
        return false;
    }
    memcpy(buffer + offset % 512, entries, 32);
    // All file handles are closed. Invalidate the clean filesystem cache before the single-sector metadata write.
    SD.sdfs.vol()->end();
    const bool written = card->writeSector(sector, buffer) && card->syncDevice();
    uint8_t verified[512];
    const bool checked = written && card->readSector(sector, verified) && memcmp(buffer, verified, sizeof(buffer)) == 0;
    const bool mounted = SD.begin(BUILTIN_SDCARD);
    return checked && mounted;
}
