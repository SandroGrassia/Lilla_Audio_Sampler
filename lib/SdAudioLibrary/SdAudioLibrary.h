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
    static constexpr uint32_t INDEX_ADDRESS = ADDRESS + FileNameRegistry::FILES * sizeof(Recipe);
    static constexpr uint16_t INDEX_CAPACITY = (0x20000 - INDEX_ADDRESS) / NAME_BYTES;
    static_assert(sizeof(Recipe) == 80);
    static_assert(INDEX_ADDRESS == 0x19438);
    static bool Begin();
    static uint16_t Count();
    static bool Name(uint16_t index, char (&name)[NAME_BYTES]);
    static bool Assigned(int file);
    static uint32_t Samples(int file);
    static const char *Source_name(int file);
    static bool Save(int file, const char *name, uint32_t first, uint32_t samples);
    static bool Read(int file, int16_t *destination, uint32_t first, uint32_t samples);
    static const char *Error();
private:
    static Recipe recipes[FileNameRegistry::FILES];
    static bool available[FileNameRegistry::FILES];
    static uint32_t generation;
    static uint16_t count;
    static const char *error;
    static uint32_t Crc(const Recipe &recipe);
};

class SdAudioPreview : public AudioStream
{
public:
    SdAudioPreview();
    bool Open(const char *name);
    bool Trim(uint32_t first, uint32_t last);
    void Stop();
    bool Service();
    void update() override;
    uint32_t Samples() const { return reader.Samples(); }
private:
    static constexpr uint32_t BUFFER_SAMPLES = 8192;
    static int16_t buffer[BUFFER_SAMPLES];
    volatile uint32_t written = 0;
    volatile uint32_t consumed = 0;
    volatile bool running = false;
    uint32_t first = 0;
    uint32_t last = 0;
    uint32_t position = 0;
    SdAudioReader reader;
};
