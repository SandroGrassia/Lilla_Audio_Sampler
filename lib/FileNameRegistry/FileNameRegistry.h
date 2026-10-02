#pragma once

#include <stdint.h>
#include <stddef.h>

// Persistent RAW identities. Callers stop playback before loading or changing the registry.
class FileNameRegistry
{
public:
    static constexpr uint16_t FILES = 260;
    static constexpr size_t BASENAME_BYTES = 32;
    static constexpr size_t FILENAME_BYTES = BASENAME_BYTES + 4;
    static constexpr uint32_t ADDRESS = 0x10000;
    static constexpr uint32_t BANK_BYTES = 12 + FILES * BASENAME_BYTES;
    static constexpr uint32_t END_ADDRESS = ADDRESS + 2 * BANK_BYTES;

    static bool Load();
    static bool Save();
    static void Clear(); // Stage a new registry, retaining the generation of the last loaded copy.
    static bool Valid_name(const char *filename);
    static int Find(const char *filename);
    static int Add(const char *filename); // Stage an identity; -1 means invalid name or no free slot.
    static bool Bind_numeric(uint16_t id); // Reserve the legacy or sampler identity id -> "id.raw".
    static bool Numeric_available(uint16_t id);
    static bool Assigned(uint16_t id);
    static const char *Filename(uint16_t id, char (&name)[FILENAME_BYTES]);

private:
    struct Bank
    {
        uint32_t magic;
        uint32_t generation;
        char names[FILES][BASENAME_BYTES];
        uint32_t crc32;
    };
    static_assert(sizeof(Bank) == BANK_BYTES);
    static Bank current;
    static Bank scratch;
    static int active_bank;
    static uint32_t Crc(const Bank &bank);
    static bool Valid(const Bank &bank);
    static bool Read(uint32_t address, void *data, uint32_t bytes);
    static bool Write(uint32_t address, const void *data, uint32_t bytes);
};
