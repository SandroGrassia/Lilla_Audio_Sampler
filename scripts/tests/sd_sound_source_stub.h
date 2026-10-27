#pragma once
#include <cstdint>
// Legacy subsystem fixtures have no SD repository. Dedicated SD tests use the real implementation.
struct SdSoundSource
{
    static constexpr int FIRST = 1024;
    static constexpr int PREVIEW = 32767;
    static bool Is_sd(int file); // Identify fixture SD file IDs.
    static inline bool available = false;
    static inline const int16_t *bound = nullptr;
    static uint32_t Samples(int); // Return the available fixture sample count.
    static bool Prepare(int); // Report whether fixture audio is available.
    static bool Load(int, int16_t *destination); // Copy fixture audio into the supplied buffer.
    static bool Read(int, int16_t *, int, int); // Report that streamed reads are unsupported by this fixture.
    static void Bind(int, const int16_t *data); // Record the fixture sample buffer.
    static void Unbind_all(); // Clear the fixture sample buffer.
    static const char *Name(int); // Return the fixture missing-source label.
};
