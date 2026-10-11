#pragma once

#include <cstdint>

// Experimental output-domain piano compensation, enabled by the instrument HIFI field.
class PlayerToneFilter
{
public:
    static float Cutoff(float pitch, uint8_t level);
    void Reset();
    void Process(int16_t *samples, int count, float pitch, uint8_t level, float sample_rate);

private:
    float z1 = 0.0f, z2 = 0.0f;
    float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    float cutoff = 0.0f;
    float cached_pitch = 0.0f;
    uint8_t cached_level = 0;
    float target_cutoff = 20000.0f;
    float wet = 0.0f;
};
