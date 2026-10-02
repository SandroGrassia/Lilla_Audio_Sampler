#include "FileNameRegistry.h"
#include "GlobalFRAM.h"
#include <cstring>
#include <cstdio>

DMAMEM FileNameRegistry::Bank FileNameRegistry::current{};
DMAMEM FileNameRegistry::Bank FileNameRegistry::scratch{};
int FileNameRegistry::active_bank = -1;

FLASHMEM
bool FileNameRegistry::Read(uint32_t address, void *data, uint32_t bytes)
{
    auto *destination = static_cast<uint8_t *>(data);
    for (uint32_t offset = 0; offset < bytes; offset += 128)
    {
        const uint32_t count = bytes - offset < 128 ? bytes - offset : 128;
        if (LillaFram.readArray(address + offset, count, destination + offset) != LillaFRAM_2x512::ERROR_0)
        {
            return false;
        }
    }
    return true;
}

FLASHMEM
bool FileNameRegistry::Write(uint32_t address, const void *data, uint32_t bytes)
{
    const auto *source = static_cast<const uint8_t *>(data);
    uint8_t buffer[128];
    for (uint32_t offset = 0; offset < bytes; offset += sizeof(buffer))
    {
        const uint32_t count = bytes - offset < sizeof(buffer) ? bytes - offset : sizeof(buffer);
        memcpy(buffer, source + offset, count);
        if (LillaFram.writeArray(address + offset, count, buffer) != LillaFRAM_2x512::ERROR_0)
        {
            return false;
        }
    }
    return true;
}

FLASHMEM
uint32_t FileNameRegistry::Crc(const Bank &bank)
{
    uint32_t crc = 0xFFFFFFFFUL;
    const auto *bytes = reinterpret_cast<const uint8_t *>(&bank);
    for (size_t offset = 0; offset < offsetof(Bank, crc32); ++offset)
    {
        crc ^= bytes[offset];
        for (uint8_t bit = 0; bit < 8; ++bit)
        {
            crc = (crc >> 1) ^ ((crc & 1U) ? 0xEDB88320UL : 0UL);
        }
    }
    return crc ^ 0xFFFFFFFFUL;
}

FLASHMEM
bool FileNameRegistry::Valid_name(const char *filename)
{
    const size_t length = strlen(filename);
    if (length < 5 || length >= FILENAME_BYTES || strcmp(filename + length - 4, ".raw") != 0)
    {
        return false;
    }
    bool packet = filename[0] == 'P' && length > 5;
    for (size_t i = 0; i < length - 4; ++i)
    {
        const unsigned char ch = filename[i];
        if (ch < 32 || ch == '/' || ch == '\\' || ch == ':' || ch == 127)
        {
            return false;
        }
        if (i > 0 && (ch < '0' || ch > '9'))
        {
            packet = false;
        }
    }
    return !packet; // P<number>.raw belongs to the recording packet namespace.
}

FLASHMEM
bool FileNameRegistry::Valid(const Bank &bank)
{
    if (bank.magic != 0x314E464CUL || bank.crc32 != Crc(bank) || bank.names[0][0] != '0' || bank.names[0][1] != 0)
    {
        return false;
    }
    for (uint16_t id = 0; id < FILES; ++id)
    {
        if (memchr(bank.names[id], 0, BASENAME_BYTES) == nullptr)
        {
            return false;
        }
        if (bank.names[id][0] == 0)
        {
            continue;
        }
        char filename[FILENAME_BYTES];
        snprintf(filename, sizeof(filename), "%s.raw", bank.names[id]);
        if (!Valid_name(filename))
        {
            return false;
        }
        for (uint16_t earlier = 0; earlier < id; ++earlier)
        {
            if (strcmp(bank.names[id], bank.names[earlier]) == 0)
            {
                return false;
            }
        }
    }
    return true;
}

FLASHMEM
bool FileNameRegistry::Load()
{
    active_bank = -1;
    for (int bank = 0; bank < 2; ++bank)
    {
        if (!Read(ADDRESS + bank * BANK_BYTES, &scratch, sizeof(scratch)))
        {
            active_bank = -1;
            return false;
        }
        if (Valid(scratch) && (active_bank < 0 || static_cast<int32_t>(scratch.generation - current.generation) > 0))
        {
            current = scratch;
            active_bank = bank;
        }
    }
    return active_bank >= 0;
}

FLASHMEM
bool FileNameRegistry::Save()
{
    const int target = active_bank == 0 ? 1 : 0;
    const uint32_t address = ADDRESS + target * BANK_BYTES;
    current.magic = 0x314E464CUL;
    ++current.generation;
    current.crc32 = Crc(current);
    const uint32_t invalid = 0;
    if (!Write(address, &invalid, sizeof(invalid)) || !Write(address + 4, reinterpret_cast<const uint8_t *>(&current) + 4, sizeof(current) - 4) || !Write(address, &current.magic, sizeof(current.magic)) || !Read(address, &scratch, sizeof(scratch)) || memcmp(&current, &scratch, sizeof(current)) != 0)
    {
        Load(); // Discard staged changes; an interrupted write keeps the previous bank readable.
        return false;
    }
    active_bank = target;
    return true;
}

FLASHMEM
void FileNameRegistry::Clear()
{
    memset(current.names, 0, sizeof(current.names));
    strcpy(current.names[0], "0");
    if (active_bank < 0)
    {
        current.generation = 0;
    }
}

FLASHMEM
int FileNameRegistry::Find(const char *filename)
{
    if (!Valid_name(filename))
    {
        return -1;
    }
    const size_t length = strlen(filename) - 4;
    for (uint16_t id = 0; id < FILES; ++id)
    {
        if (strlen(current.names[id]) == length && memcmp(current.names[id], filename, length) == 0)
        {
            return id;
        }
    }
    return -1;
}

FLASHMEM
int FileNameRegistry::Add(const char *filename)
{
    if (!Valid_name(filename))
    {
        return -1;
    }
    const int existing = Find(filename);
    if (existing >= 0)
    {
        return existing;
    }
    for (uint16_t id = 1; id < FILES; ++id)
    {
        if (!Assigned(id))
        {
            memcpy(current.names[id], filename, strlen(filename) - 4);
            current.names[id][strlen(filename) - 4] = 0;
            return id;
        }
    }
    return -1;
}

FLASHMEM
bool FileNameRegistry::Assigned(uint16_t id)
{
    return current.names[id][0] != 0;
}

FLASHMEM
bool FileNameRegistry::Numeric_available(uint16_t id)
{
    char filename[FILENAME_BYTES];
    snprintf(filename, sizeof(filename), "%u.raw", static_cast<unsigned int>(id));
    return !Assigned(id) && Find(filename) < 0;
}

FLASHMEM
bool FileNameRegistry::Bind_numeric(uint16_t id)
{
    char filename[FILENAME_BYTES];
    snprintf(filename, sizeof(filename), "%u.raw", static_cast<unsigned int>(id));
    if (Find(filename) == id)
    {
        return true;
    }
    if (!Numeric_available(id))
    {
        return false;
    }
    snprintf(current.names[id], BASENAME_BYTES, "%u", static_cast<unsigned int>(id));
    return true;
}

FLASHMEM
const char *FileNameRegistry::Filename(uint16_t id, char (&name)[FILENAME_BYTES])
{
    if (Assigned(id))
    {
        snprintf(name, sizeof(name), "%s.raw", current.names[id]);
    }
    else
    {
        snprintf(name, sizeof(name), "%u.raw", static_cast<unsigned int>(id)); // Proposed name for a new sampler file.
    }
    return name;
}
