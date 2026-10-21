#pragma once

#include <SD.h>

// Offline SD preparation only. The caller suspends audio and MIDI for the entire Run.
class UserAudioImport
{
public:
    using PcmParser = bool (*)(File &, uint32_t &, uint32_t &, uint16_t &);
    static constexpr uint32_t CAPACITY = 4096;
    static constexpr const char *INDEX_PATH = "/LILLA_AUDIO/.lilla-index";
    struct Entry
    {
        char name[256];
        uint32_t bytes;
        uint32_t samples;
    };
    struct Header
    {
        uint32_t magic = 0x4C524157;
        uint32_t version = 1;
        uint32_t entries = 0;
        uint32_t entry_bytes = sizeof(Entry);
        uint32_t sample_rate = 44100;
        uint16_t channels = 1;
        uint16_t bits = 16;
    };
    struct Report
    {
        uint32_t copied = 0;
        uint32_t skipped = 0;
        uint32_t errors = 0;
        uint32_t indexed = 0;
        bool index_ready = false;
    };
    FLASHMEM static Report Run(PcmParser wav, PcmParser aiff);
private:
    enum class Format { Raw, Wav, Aiff, Mp3, Unknown };
    FLASHMEM static Format Get_format(const char *name);
    FLASHMEM static bool Copy(File &source, const char *destination, Format format, PcmParser wav, PcmParser aiff);
    FLASHMEM static bool Merge_index(const char *source, const char *destination, uint32_t entries, uint32_t width);
    FLASHMEM static bool Build_index(uint32_t &entries);
};
