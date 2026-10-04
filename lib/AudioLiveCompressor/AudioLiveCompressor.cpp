/* LILLA Audio Sampler - stereo Live Sampler feedback and recording dynamics. */
#include "AudioLiveCompressor.h"
#include <algorithm>
#include <cstdlib>

namespace
{
constexpr std::array<float, 258> Make_gain_table()
{
    std::array<float, 258> table = {};
    constexpr float threshold = 16384.0f; // Compression starts at -6.02 dBFS.
    constexpr float ceiling = 29204.0f;
    constexpr float knee_end = 2.0f * ceiling - threshold;
    for (unsigned index = 0; index < table.size(); ++index)
    {
        const float input = static_cast<float>(index * 256);
        float output = input;
        if (input >= knee_end)
        {
            output = ceiling;
        }
        else if (input > threshold)
        {
            const float excess = input - threshold;
            output = input - excess * excess / (4.0f * (ceiling - threshold));
        }
        table[index] = index == 0 ? 1.0f : output / input;
    }
    return table;
}
}

// Constant-initialized LUT: no logarithms or curve construction in the audio interrupt.
const std::array<float, 258> AudioLiveCompressor::gain_table = Make_gain_table();

FLASHMEM
AudioLiveCompressor::AudioLiveCompressor() : AudioStream(4, input_queue)
{
}

FLASHMEM
void AudioLiveCompressor::Set_feedback(float feedback)
{
    feedback_target = (static_cast<int32_t>(feedback * 65536.0f) / 128) * 128;
}

FLASHMEM
void AudioLiveCompressor::Set_enabled(bool value)
{
    enabled = value; // Retarget the running fade; never reset audio history or envelopes.
}

FLASHMEM
bool AudioLiveCompressor::Is_enabled(void) const
{
    return enabled;
}

float AudioLiveCompressor::Required_gain(int32_t peak)
{
    // Keep the largest peak in [now - LOOKAHEAD, now], including the sample being emitted.
    while (peak_count != 0 && static_cast<uint32_t>(sample_clock - peak_times[peak_head]) > LOOKAHEAD)
    {
        peak_head = (peak_head + 1) % PEAK_QUEUE_SIZE;
        --peak_count;
    }
    while (peak_count != 0 && peak_values[(peak_head + peak_count - 1) % PEAK_QUEUE_SIZE] <= peak)
    {
        --peak_count;
    }
    const unsigned tail = (peak_head + peak_count) % PEAK_QUEUE_SIZE;
    peak_values[tail] = peak;
    peak_times[tail] = sample_clock;
    ++peak_count;
    ++sample_clock;
    const int32_t maximum = peak_values[peak_head];
    const unsigned index = static_cast<unsigned>(maximum) >> 8;
    const float fraction = static_cast<float>(maximum & 255) / 256.0f;
    float target = gain_table[index] + fraction * (gain_table[index + 1] - gain_table[index]);
    if (maximum > CEILING)
    {
        target = std::min(target, CEILING / static_cast<float>(maximum)); // Bound interpolation error at the ceiling.
    }
    return target;
}

int16_t AudioLiveCompressor::To_sample(float sample)
{
    return static_cast<int16_t>(std::max(-32768.0f, std::min(32767.0f, sample)));
}

void AudioLiveCompressor::Process_sample(int32_t left, int32_t right, int16_t &output_left, int16_t &output_right)
{
    const float target = Required_gain(std::max(std::abs(left), std::abs(right)));
    if (target < compressor_gain)
    {
        // A full-scale gain reduction takes at most LOOKAHEAD samples, before the peak arrives.
        compressor_gain = std::max(target, compressor_gain - 1.0f / static_cast<float>(LOOKAHEAD));
    }
    else
    {
        constexpr float release_step = 1.0f / (0.100f * AUDIO_SAMPLE_RATE_EXACT);
        compressor_gain += (target - compressor_gain) * release_step;
    }

    constexpr float bypass_step = 1.0f / (0.010f * AUDIO_SAMPLE_RATE_EXACT);
    if (enabled)
    {
        bypass_position = std::min(1.0f, bypass_position + bypass_step);
    }
    else
    {
        bypass_position = std::max(0.0f, bypass_position - bypass_step);
    }
    const float wet = bypass_position * bypass_position * (3.0f - 2.0f * bypass_position);
    const int32_t delayed_l = delayed_left[delay_position];
    const int32_t delayed_r = delayed_right[delay_position];
    delayed_left[delay_position] = left;
    delayed_right[delay_position] = right;
    delay_position = (delay_position + 1) % LOOKAHEAD;

    // Delay both dry and wet paths, including bypass: toggling never moves the recording timeline.
    const float dry_l = To_sample(static_cast<float>(delayed_l));
    const float dry_r = To_sample(static_cast<float>(delayed_r));
    output_left = To_sample(dry_l + wet * (static_cast<float>(delayed_l) * compressor_gain - dry_l));
    output_right = To_sample(dry_r + wet * (static_cast<float>(delayed_r) * compressor_gain - dry_r));
}

void AudioLiveCompressor::update(void)
{
    if (Recorder_ptr == nullptr || !Recorder_ptr->Is_writing())
    {
        for (unsigned channel = 0; channel < 4; ++channel)
        {
            audio_block_t *input = receiveReadOnly(channel);
            if (input != nullptr)
            {
                release(input);
            }
        }
        processing = false;
        return; // No output blocks and no DSP when the recorder is inactive.
    }
    if (!processing)
    {
        std::fill(std::begin(delayed_left), std::end(delayed_left), 0);
        std::fill(std::begin(delayed_right), std::end(delayed_right), 0);
        delay_position = 0;
        peak_head = 0;
        peak_count = 0;
        sample_clock = 0;
        compressor_gain = 1.0f;
        bypass_position = 0.0f;
        feedback_gain = feedback_target;
        processing = true;
    }
    audio_block_t *inputs[4];
    for (unsigned channel = 0; channel < 4; ++channel)
    {
        inputs[channel] = receiveReadOnly(channel);
    }
    audio_block_t *output_l = allocate();
    audio_block_t *output_r = allocate();
    for (unsigned sample = 0; sample < AUDIO_BLOCK_SAMPLES; ++sample)
    {
        if (feedback_gain < feedback_target)
        {
            ++feedback_gain;
        }
        else if (feedback_gain > feedback_target)
        {
            --feedback_gain;
        }
        const int32_t line_l = inputs[0] == nullptr ? 0 : inputs[0]->data[sample];
        const int32_t line_r = inputs[1] == nullptr ? 0 : inputs[1]->data[sample];
        const int32_t return_l = inputs[2] == nullptr ? 0 : inputs[2]->data[sample];
        const int32_t return_r = inputs[3] == nullptr ? 0 : inputs[3]->data[sample];
        const int32_t sum_l = line_l + ((feedback_gain * return_l) >> 16);
        const int32_t sum_r = line_r + ((feedback_gain * return_r) >> 16);
        int16_t result_l;
        int16_t result_r;
        Process_sample(sum_l, sum_r, result_l, result_r);
        if (output_l != nullptr && output_r != nullptr)
        {
            output_l->data[sample] = result_l;
            output_r->data[sample] = result_r;
        }
    }
    if (output_l != nullptr && output_r != nullptr)
    {
        transmit(output_l, 0);
        transmit(output_r, 1);
    }
    if (output_l != nullptr)
    {
        release(output_l);
    }
    if (output_r != nullptr)
    {
        release(output_r);
    }
    for (audio_block_t *input : inputs)
    {
        if (input != nullptr)
        {
            release(input);
        }
    }
}
