#pragma once

#include <SD.h>
#include "UserAudioImport.h"

// Persistent SD identities are independent of the alphabetical catalog position.
class SdSoundSource
{
public:
    static constexpr int FIRST = 1024;
    static constexpr int PREVIEW = 32767;
    struct Reference
    {
        uint32_t magic = 0x3153444C;
        uint32_t first = 0;
        uint32_t samples = 0;
        uint32_t original_bytes = 0;
        char name[256] = {};
        uint32_t checksum = 0;
    };
    static bool Is_sd(int file);
    static uint32_t Catalog_count();
    static bool Catalog_entry(uint32_t index, UserAudioImport::Entry &entry);
    static bool Find_catalog(const char *name, uint32_t count, uint32_t &position);
    static bool Select(uint32_t index, uint32_t capacity);
    static bool Prepare(int file);
    static uint32_t Samples(int file);
    static const char *Name(int file);
    static bool Load(int file, int16_t *destination);
    static bool Read(int file, int16_t *destination, int first, int count);
    static void Bind(int file, const int16_t *data);
    static void Unbind_all();
    static int Save(uint32_t first, uint32_t last, bool (*identity_used)(int) = nullptr);
private:
    struct Loaded
    {
        int file = -1;
        Reference reference;
        const int16_t *data = nullptr;
    };
    static Loaded loaded[17];
    static uint8_t next_slot;
    static int next_identity;
    static Loaded *Find(int file);
    static uint32_t Checksum(const Reference &reference);
    static void Reference_path(int file, char (&path)[64]);
    static bool Read_header(File &index, UserAudioImport::Header &header);
};
