#include "PlayerToneFilter.h"
#include <cmath>

float PlayerToneFilter::Cutoff(float pitch, uint8_t level)
{
    // Logarithmic sweep from 20 kHz at unity to the selected endpoint at +11 semitones.
    const float endpoint = level == 0 ? 20000.0f : 100.0f * level; // TONE stores 30..80 tenths of kHz.
    const float position = fminf(fmaxf(12.0f * log2f(pitch) / 11.0f, 0.0f), 1.0f);
    return 20000.0f * powf(endpoint / 20000.0f, position);
}

void PlayerToneFilter::Reset()
{
    z1 = 0.0f;
    z2 = 0.0f;
    cutoff = 0.0f;
    wet = 0.0f;
}

void PlayerToneFilter::Process(int16_t *samples, int count, float pitch, uint8_t level, float sample_rate)
{
    const float target_wet = level != 0 && pitch > 1.0f ? 1.0f : 0.0f;
    if (target_wet == 0.0f && wet == 0.0f)
    {
        Reset();
        return;
    }
    if (pitch != cached_pitch || level != cached_level)
    {
        cached_pitch = pitch;
        cached_level = level;
        target_cutoff = Cutoff(pitch, level);
    }
    const float next_cutoff = cutoff == 0.0f ? target_cutoff : cutoff + 0.25f * (target_cutoff - cutoff);
    if (next_cutoff != cutoff)
    {
        cutoff = fabsf(next_cutoff - target_cutoff) < 0.1f ? target_cutoff : next_cutoff;
        const float angle = 6.28318530718f * cutoff / sample_rate;
        const float cosine = cosf(angle);
        const float alpha = sinf(angle) / (2.0f * 0.7071f);
        const float scale = 1.0f / (1.0f + alpha);
        b0 = 0.5f * (1.0f - cosine) * scale;
        b1 = 2.0f * b0;
        b2 = b0;
        a1 = -2.0f * cosine * scale;
        a2 = (1.0f - alpha) * scale;
    }
    const float wet_step = (target_wet - wet) / count;
    for (int sample = 0; sample < count; ++sample)
    {
        const float input = samples[sample];
        const float output = b0 * input + z1;
        z1 = b1 * input - a1 * output + z2;
        z2 = b2 * input - a2 * output;
        wet += wet_step;
        const float mixed = input + wet * (output - input);
        samples[sample] = static_cast<int16_t>(fminf(32767.0f, fmaxf(-32768.0f, mixed)));
    }
    wet = target_wet;
}
