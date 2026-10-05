#pragma once

#include <SD.h>

struct Mp3ImportDecoder;

class Mp3Import
{
public:
    Mp3Import() = default;
    ~Mp3Import();
    Mp3Import(const Mp3Import &) = delete;
    Mp3Import &operator=(const Mp3Import &) = delete;
    // Report mono output bytes at 44100 Hz; channels describes Read's interleaving (one after resampling).
    bool Open(File &file, uint32_t &bytes, uint16_t &channels, uint32_t maximum_bytes);
    bool Read(int16_t *samples, uint32_t frames);

private:
    Mp3ImportDecoder *decoder = nullptr;
};
