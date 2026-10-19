/* LILLA Audio Sampler - two four-comb networks with stereo feedback and all-pass diffusion. */
#include "LillaReverb.h"
#include <algorithm>
#include <cmath>
#include <cstring>

PROGMEM const unsigned LillaReverb::lengths[2][4] = {{1493, 1601, 1867, 2081}, {1559, 1699, 1999, 2203}};
PROGMEM const unsigned LillaReverb::allpass_lengths[2][2] = {{211, 337}, {223, 349}};
static_assert(sizeof(float) == 2 * sizeof(int16_t));
static_assert(1493 + 1601 + 1867 + 2081 + 211 + 337 <= LillaReverb::CHANNEL_SAMPLES / 2);
static_assert(1559 + 1699 + 1999 + 2203 + 223 + 349 <= LillaReverb::CHANNEL_SAMPLES / 2);

FLASHMEM LillaReverb::LillaReverb() : AudioStream(2, input_queue)
{
}

FLASHMEM void LillaReverb::Setup(int16_t *left, int16_t *right)
{
    storage[0] = left;
    storage[1] = right;
    Reset();
}

FLASHMEM void LillaReverb::Set_parameters(int requested_level, int requested_decay, int requested_damping)
{
    level_target = requested_level * 0.01f;
    damping_target = requested_damping * 0.009f;
    if (requested_decay != decay_setting)
    {
        decay_setting = requested_decay;
        for (unsigned channel = 0; channel < 2; ++channel)
        {
            for (unsigned comb = 0; comb < 4; ++comb)
            {
                gain_target[channel][comb] = requested_decay == 0 ? 0.0f : powf(0.001f, lengths[channel][comb] / (AUDIO_SAMPLE_RATE_EXACT * requested_decay * 0.1f));
                const float g = gain_target[channel][comb];
                constexpr float feedback_energy = (1.0f - CROSS_FEEDBACK) * (1.0f - CROSS_FEEDBACK) + CROSS_FEEDBACK * CROSS_FEEDBACK;
                injection_target[channel][comb] = sqrtf(1.0f - feedback_energy * g * g); // Normalize decorrelated feedback energy instead of DC gain.
            }
        }
    }
}

FLASHMEM void LillaReverb::Reset()
{
    for (unsigned channel = 0; channel < 2; ++channel)
    {
        unsigned used_samples = 0;
        for (unsigned length : lengths[channel])
        {
            used_samples += length;
        }
        for (unsigned length : allpass_lengths[channel])
        {
            used_samples += length;
        }
        std::fill(storage[channel], storage[channel] + 2 * used_samples, 0); // Reset work follows the DSP histories, not the reserved capacity.
        std::fill(std::begin(allpass_position[channel]), std::end(allpass_position[channel]), 0u);
        for (unsigned comb = 0; comb < 4; ++comb)
        {
            position[channel][comb] = 0;
            filtered[channel][comb] = 0.0f;
            gain[channel][comb] = gain_target[channel][comb];
            injection[channel][comb] = injection_target[channel][comb];
        }
    }
    for (unsigned channel = 0; channel < 2; ++channel)
    {
        std::fill(std::begin(wet_delay[channel]), std::end(wet_delay[channel]), 0.0f);
    }
    std::fill(std::begin(wet_peaks), std::end(wet_peaks), 0.0f);
    wet_position = 0;
    wet_gain = 1.0f;
    level = level_target;
    damping = damping_target;
}

void LillaReverb::Suspend(bool value)
{
    suspended = value;
}

void LillaReverb::update()
{
    audio_block_t *input_l = receiveReadOnly(0);
    audio_block_t *input_r = receiveReadOnly(1);
    audio_block_t *output_l = allocate();
    audio_block_t *output_r = allocate();
    constexpr float smoothing = 1.0f / (0.050f * AUDIO_SAMPLE_RATE_EXACT);
    for (unsigned sample = 0; sample < AUDIO_BLOCK_SAMPLES; ++sample)
    {
        const float dry[2] = {input_l == nullptr ? 0.0f : static_cast<float>(input_l->data[sample]), input_r == nullptr ? 0.0f : static_cast<float>(input_r->data[sample])};
        float wet[2] = {};
        level += (level_target - level) * smoothing;
        damping += (damping_target - damping) * smoothing;
        if (storage[0] != nullptr && !suspended)
        {
            unsigned offset[2] = {};
            for (unsigned comb = 0; comb < 4; ++comb)
            {
                float delayed[2];
                for (unsigned channel = 0; channel < 2; ++channel)
                {
                    delayed[channel] = Read_sample(storage[channel], offset[channel] + position[channel][comb]);
                    filtered[channel][comb] += (1.0f - damping) * (delayed[channel] - filtered[channel][comb]);
                    gain[channel][comb] += (gain_target[channel][comb] - gain[channel][comb]) * smoothing;
                    injection[channel][comb] += (injection_target[channel][comb] - injection[channel][comb]) * smoothing;
                    wet[channel] += filtered[channel][comb] * 0.5f; // Damping also softens the first reflections, before all-pass diffusion.
                }
                for (unsigned channel = 0; channel < 2; ++channel)
                {
                    const float feedback = (1.0f - CROSS_FEEDBACK) * filtered[channel][comb] + CROSS_FEEDBACK * filtered[1 - channel][comb];
                    const float g = gain[channel][comb];
                    const float value = injection[channel][comb] * dry[channel] + g * feedback;
                    Write_sample(storage[channel], offset[channel] + position[channel][comb], value);
                    if (++position[channel][comb] == lengths[channel][comb])
                    {
                        position[channel][comb] = 0;
                    }
                    offset[channel] += lengths[channel][comb];
                }
            }
            // Schroeder all-pass sections use the free tail of each existing float buffer.
            for (unsigned stage = 0; stage < 2; ++stage)
            {
                for (unsigned channel = 0; channel < 2; ++channel)
                {
                    const unsigned index = offset[channel] + allpass_position[channel][stage];
                    const float input = wet[channel];
                    const float delayed = Read_sample(storage[channel], index);
                    wet[channel] = delayed - ALLPASS_GAIN * input;
                    Write_sample(storage[channel], index, input + ALLPASS_GAIN * wet[channel]);
                    if (++allpass_position[channel][stage] == allpass_lengths[channel][stage])
                    {
                        allpass_position[channel][stage] = 0;
                    }
                    offset[channel] += allpass_lengths[channel][stage];
                }
            }
        }
        Limit_wet(wet[0], wet[1]);
        if (output_l != nullptr && output_r != nullptr)
        {
            const float mix = suspended || (level_target == 0.0f && level < 0.00001f) ? 0.0f : level;
            output_l->data[sample] = static_cast<int16_t>(std::max(-32768.0f, std::min(32767.0f, dry[0] + mix * (wet[0] - dry[0]))));
            output_r->data[sample] = static_cast<int16_t>(std::max(-32768.0f, std::min(32767.0f, dry[1] + mix * (wet[1] - dry[1]))));
        }
    }
    if (output_l != nullptr && output_r != nullptr)
    {
        transmit(output_l, 0);
        transmit(output_r, 1);
    }
    for (audio_block_t *block : {input_l, input_r, output_l, output_r})
    {
        if (block != nullptr)
        {
            release(block);
        }
    }
}

float LillaReverb::Read_sample(const int16_t *buffer, unsigned index)
{
    float value;
    memcpy(&value, buffer + 2 * index, sizeof(value)); // Preserve float precision in the existing reserved PSRAM storage without aliasing.
    return value;
}

void LillaReverb::Write_sample(int16_t *buffer, unsigned index, float value)
{
    memcpy(buffer + 2 * index, &value, sizeof(value));
}

void LillaReverb::Limit_wet(float &left, float &right)
{
    wet_delay[0][wet_position] = left;
    wet_delay[1][wet_position] = right;
    wet_peaks[wet_position] = std::max(std::abs(left), std::abs(right));
    float peak = 30000.0f;
    for (float value : wet_peaks)
    {
        peak = std::max(peak, value);
    }
    const float target = 30000.0f / peak;
    if (target < wet_gain)
    {
        wet_gain = std::max(target, wet_gain - 1.0f / WET_LOOKAHEAD);
    }
    else
    {
        wet_gain += (target - wet_gain) / (0.100f * AUDIO_SAMPLE_RATE_EXACT);
    }
    wet_position = (wet_position + 1) % (WET_LOOKAHEAD + 1);
    const float emitted_peak = std::max(std::abs(wet_delay[0][wet_position]), std::abs(wet_delay[1][wet_position]));
    const float applied_gain = emitted_peak > 30000.0f ? std::min(wet_gain, 30000.0f / emitted_peak) : wet_gain;
    left = wet_delay[0][wet_position] * applied_gain;
    right = wet_delay[1][wet_position] * applied_gain;
}
