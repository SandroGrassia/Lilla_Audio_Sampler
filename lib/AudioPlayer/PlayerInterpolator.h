#pragma once

#include <stdint.h>

namespace PlayerInterpolator
{
constexpr int Taps = 8;
constexpr int Left = 3;
constexpr int Phases = 256;
extern const float coefficients[Phases + 1][Taps];

inline float Mix(float pitch)
{
    return pitch < 1.0f ? (pitch <= 0.75f ? 1.0f : (1.0f - pitch) * 4.0f) : 0.0f;
}

inline float Read(const int16_t *samples, float position, float mix)
{
    const int index = static_cast<int>(position);
    const float fraction = position - index;
    if (mix == 0.0f)
    {
        return samples[index] + fraction * (samples[index + (fraction > 0.0f)] - samples[index]);
    }
    const float phase = fraction * Phases;
    const int row = static_cast<int>(phase);
    const float delta = phase - row;
    const float *low = coefficients[row];
    const float *high = coefficients[row + 1];
    const int16_t *window = samples + index - Left;
    float filtered = 0.0f;
    for (int tap = 0; tap < Taps; ++tap)
    {
        filtered += window[tap] * (low[tap] + delta * (high[tap] - low[tap]));
    }
    if (mix == 1.0f)
    {
        return filtered;
    }
    const float linear = samples[index] + fraction * (samples[index + (fraction > 0.0f)] - samples[index]);
    return linear + mix * (filtered - linear);
}
}
