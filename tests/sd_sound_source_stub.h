#pragma once
#include <cstdint>
// Legacy subsystem fixtures have no SD repository. Dedicated SD tests use the real implementation.
struct SdSoundSource
{
    static constexpr int FIRST = 1024;
    static constexpr int PREVIEW = 32767;
    static bool Is_sd(int file) { return file >= FIRST && file <= PREVIEW; }
    static inline bool available = false;
    static inline const int16_t *bound = nullptr;
    static uint32_t Samples(int) { return available ? 4 : 0; }
    static bool Prepare(int) { return available; }
    static bool Load(int, int16_t *destination) { if (!available) { return false; } for (int i=0; i<4; ++i) { destination[i]=100+i; } return true; }
    static bool Read(int, int16_t *, int, int) { return false; }
    static void Bind(int, const int16_t *data) { bound=data; }
    static void Unbind_all() { bound=nullptr; }
    static const char *Name(int) { return "SD AUDIO UNAVAILABLE"; }
};
