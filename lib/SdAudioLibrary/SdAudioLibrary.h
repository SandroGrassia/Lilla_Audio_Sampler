#pragma once

#include <SD.h>
#include <AudioStream.h>
#include "Mp3Import.h"
#include "FileNameRegistry.h"

// Foreground-only SD access. Audio interrupts consume only the preview ring.
class SdAudioReader
{
public:
    enum class Format : uint8_t { Raw, Wav, Aiff, Mp3 };
    ~SdAudioReader();
    static bool Audio_name(const char *name);
    bool Open(const char *name);
    void Close();
    bool Seek(uint32_t sample);
    bool Read(int16_t *destination, uint32_t samples);
    uint32_t Samples() const { return samples; }
private:
    File file;
    Mp3Import *mp3 = nullptr;
    Format format = Format::Raw;
    uint32_t offset = 0;
    uint32_t samples = 0;
    uint32_t position = 0;
    uint16_t channels = 1;
    static bool Wav_data(File &file, uint32_t &offset, uint32_t &bytes, uint16_t &channels);
    static bool Aiff_data(File &file, uint32_t &offset, uint32_t &bytes, uint16_t &channels);
};

class SdAudioLibrary
{
public:
    static constexpr uint32_t ADDRESS = 0x142F8;
    static constexpr size_t NAME_BYTES = 64;
    struct Recipe
    {
        char name[NAME_BYTES];
        uint32_t first;
        uint32_t samples;
        uint32_t file_bytes;
        uint32_t crc;
    };
    static constexpr uint32_t SOURCE_RECORDS_END = ADDRESS + FileNameRegistry::FILES * sizeof(Recipe);
    static constexpr uint16_t INDEX_CAPACITY = 1000;
    static constexpr const char *INDEX_PATH = "/LILLA_AUDIO/.lilla-index";
    static_assert(sizeof(Recipe) == 80);
    static_assert(SOURCE_RECORDS_END == 0x19438);
    static bool Begin();
    static uint16_t Count();
    static bool Name(uint16_t index, char (&name)[NAME_BYTES]);
    static bool Assigned(int file);
    static uint32_t Samples(int file);
    static bool Check(int file);
    static const char *Source_name(int file);
    static bool Save(int file, const char *name, uint32_t first, uint32_t samples);
    static void Suspend_read();
    static bool Read(int file, int16_t *destination, uint32_t first, uint32_t samples);
    static const char *Error();
private:
    static SdAudioReader cache_reader;
    static int read_file;
    static uint32_t read_next;
    static uint32_t read_generation;
    static Recipe recipes[FileNameRegistry::FILES];
    static bool available[FileNameRegistry::FILES];
    static uint32_t generation;
    static uint16_t count;
    static const char *error;
    static uint32_t Crc(const Recipe &recipe);
};

// One ephemeral SD source, shared by the ordinary Players. SD I/O is foreground-only.
class SdPlayerCache
{
public:
    static constexpr uint32_t PAGE_SAMPLES = 768;
    static constexpr uint8_t PAGES = 64;
    static constexpr float MAX_PITCH = 4.0f;
    static bool Open(int file, const char *name);
    static void Close(); // Caller has drained every Player before releasing this source.
    static bool Assigned(int file);
    static int File_id();
    static uint32_t Samples();
    static const char *Name();
    static bool Read_foreground(int16_t *destination, uint32_t first, uint32_t count);
    static bool Service();
    static bool Require(int first, int count, uint32_t cycle);
    static bool Require_geometry(int first, int last, int crossfade, uint8_t mode, float position, uint32_t cycle);
    static void Read_cached(int16_t *destination, int first, int count);
    static bool Failed();
private:
    struct Page
    {
        int index = -1;
        volatile bool ready = false;
        volatile bool loading = false;
        volatile uint32_t cycle = 0;
    };
    static Page pages[PAGES];
    static int16_t *data;
    static SdAudioReader reader;
    static int file_id;
    static char name[SdAudioLibrary::NAME_BYTES];
    static volatile bool failed;
    static int Find(int index);
};
