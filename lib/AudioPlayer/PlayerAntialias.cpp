#include "PlayerAntialias.h"

bool PlayerAntialias::Enabled(uint8_t setting, float base_pitch)
{
    return (setting & Enable_flag) != 0 && (setting & 0x7f) != 0 && base_pitch <= 2.0f;
}

int PlayerAntialias::History(float pitch)
{
    return History_base + static_cast<int>(std::ceil(History_per_pitch * pitch));
}

void PlayerAntialias::Configure(Settings &settings, float pitch)
{
    if (settings.pitch == pitch)
    {
        return;
    }
    settings.pitch = pitch;
    settings.mix = pitch <= 1.0f ? 0.0f : (pitch < 1.25f ? (pitch - 1.0f) * 4.0f : 1.0f);
    if (settings.mix == 0.0f)
    {
        settings.history = 0;
        return;
    }
    settings.history = History(pitch);
    // Fourth-order Butterworth at 0.4/pitch cycles/source sample (17.64 kHz after transposition).
    // Bilinear-transform biquads: https://www.w3.org/TR/audio-eq-cookbook/
    const float k = tanf(3.14159265358979323846f * 0.4f / pitch);
    const float square = k * k;
    const float inverse_q[2] = {1.8477590650f, 0.7653668647f};
    for (int section = 0; section < 2; ++section)
    {
        const float damping = k * inverse_q[section];
        const float scale = 1.0f / (1.0f + damping + square);
        settings.sections[section].b0 = square * scale;
        settings.sections[section].a1 = 2.0f * (square - 1.0f) * scale;
        settings.sections[section].a2 = (1.0f - damping + square) * scale;
    }
}

void PlayerAntialias::Render(const int16_t *source, int count, float position, float pitch, int outputs, const Settings &settings, float *destination)
{
    float z1[2] = {};
    float z2[2] = {};
    float previous = 0.0f;
    const float first_position = position;
    int output = 0;
    int next = static_cast<int>(position) + 1;
    for (int i = 0; i < count; ++i)
    {
        const float dry = source[i];
        float value = dry;
        for (int section = 0; section < 2; ++section)
        {
            const Section &c = settings.sections[section];
            const float feed = c.b0 * value;
            value = feed + z1[section];
            z1[section] = 2.0f * feed - c.a1 * value + z2[section];
            z2[section] = feed - c.a2 * value;
        }
        value = dry + settings.mix * (value - dry);
        if (i == next)
        {
            destination[output] = previous + (position - (next - 1)) * (value - previous);
            ++output;
            if (output == outputs)
            {
                return;
            }
            position = first_position + output * pitch;
            next = static_cast<int>(position) + 1;
        }
        previous = value;
    }
}
