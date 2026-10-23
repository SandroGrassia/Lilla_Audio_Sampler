#pragma once

#include <SD.h>

class WavImport
{
public:
    struct Info // Validated source layout and mono 44100 Hz output size.
    {
        uint32_t offset = 0, frames = 0, rate = 0, bytes = 0; // Input offset, frame count, sample rate and converted byte count.
        uint16_t encoding = 0, channels = 0, bits = 0, alignment = 0; // Validated input sample representation.
    };
    WavImport() = default; // Allocate conversion buffers only when opened.
    FLASHMEM ~WavImport(); // Release offline conversion buffers.
    WavImport(const WavImport &) = delete; // Keep ownership of decoder buffers unique.
    WavImport &operator=(const WavImport &) = delete; // Prevent shared ownership of decoder buffers.
    FLASHMEM static bool Inspect(File &file, Info &info); // Validate PCM/float WAV chunks, including the known RIFF +4 discrepancy.
    FLASHMEM bool Open(File &file, uint32_t &bytes, uint16_t &channels); // Prepare streaming conversion to mono 16-bit 44100 Hz.
    FLASHMEM bool Read(int16_t *samples, uint32_t frames); // Read the next requested output frames.
private:
    struct State; // Heap-backed input history and resampling state.
    State *state = nullptr; // Exclusive conversion state, absent until Open.
    FLASHMEM static bool Read_exact(File &file, void *destination, uint32_t bytes); // Handle short reads and reject failed or truncated input.
    FLASHMEM static uint32_t Little(const uint8_t *bytes, uint32_t count); // Decode up to four little-endian bytes.
    FLASHMEM bool Prepare_filter(); // Build a bounded polyphase antialias filter.
    FLASHMEM bool Fill_history(uint32_t last); // Decode and mix input through the requested frame.
};
