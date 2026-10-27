#pragma once

#include <stdint.h>

namespace PlayerInterpolator
{
constexpr int Taps = 8;
constexpr int Left = 3;
constexpr int Phases = 256;
extern const float coefficients[Phases + 1][Taps];

float Mix(float pitch); // Return the low-pitch interpolation blend.

float Read(const int16_t *samples, float position, float mix); // Interpolate a sample using the configured linear and sinc blend.
}
