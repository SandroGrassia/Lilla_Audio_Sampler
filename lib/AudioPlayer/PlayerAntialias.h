#pragma once

#include <cmath>
#include <stdint.h>

namespace PlayerAntialias
{
constexpr int Basket_capacity = 5000;
constexpr int History_base = 64;
constexpr int History_per_pitch = 12;

inline int History(float pitch)
{
    return History_base + static_cast<int>(std::ceil(History_per_pitch * pitch));
}

struct Section
{
    float b0 = 0.0f;
    float a1 = 0.0f;
    float a2 = 0.0f;
};

struct Settings
{
    float pitch = 0.0f;
    float mix = 0.0f;
    int history = 0;
    Section sections[2];
};

void Configure(Settings &settings, float pitch);
// Source includes History() preceding samples and one interpolation neighbour after the last output.
// Reconstruct state from source context for each segment: no stale state across seeks, loops or crossfades.
void Render(const int16_t *source, int count, float position, float pitch, int outputs, const Settings &settings, float *destination);
}
