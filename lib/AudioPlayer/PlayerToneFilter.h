#pragma once

#include <cstdint>

// Experimental output-domain piano compensation, enabled by the instrument LOCK field.
class PlayerToneFilter
{
public:
    static float Cutoff(float pitch);
    void Reset();
    void Process(int16_t *samples, int count, float pitch, bool enabled, float sample_rate);

private:
    float z1 = 0.0f, z2 = 0.0f;
    float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    float cutoff = 0.0f;
    float cached_pitch = 0.0f;
    float target_cutoff = 20000.0f;
    float wet = 0.0f;
};
