/* LILLA Audio Sampler - shared stereo Live Sampler and Delay dynamics. */
#include "LillaCompressor.h"
#include <algorithm>
#include <cstdlib>

namespace
{
constexpr std::array<float, 258> Make_gain_table(float ceiling, float threshold)
{
    std::array<float, 258> table = {};
    const float knee_end = 2.0f * ceiling - threshold;
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

// One curve per dB from -24 to -3; amplitudes are 16384 * 10^((dB + 6) / 20), preserving the original -6 dB curve.
// Constant-initialized LUTs: no logarithms or curve construction in the audio interrupt.
PROGMEM const std::array<std::array<float, 258>, 22> LillaCompressor::gain_table = {Make_gain_table(CEILING, 2062.6233947f), Make_gain_table(CEILING, 2314.3015131f), Make_gain_table(CEILING, 2596.6890065f), Make_gain_table(CEILING, 2913.5329854f), Make_gain_table(CEILING, 3269.0377768f), Make_gain_table(CEILING, 3667.9207134f), Make_gain_table(CEILING, 4115.4747294f), Make_gain_table(CEILING, 4617.6385946f), Make_gain_table(CEILING, 5181.0757184f), Make_gain_table(CEILING, 5813.2625692f), Make_gain_table(CEILING, 6522.5878823f), Make_gain_table(CEILING, 7318.4639738f), Make_gain_table(CEILING, 8211.4516357f), Make_gain_table(CEILING, 9213.4002719f), Make_gain_table(CEILING, 10337.6051320f), Make_gain_table(CEILING, 11598.9837313f), Make_gain_table(CEILING, 13014.2737977f), Make_gain_table(CEILING, 14602.2553704f), Make_gain_table(CEILING, 16384.0000000f), Make_gain_table(CEILING, 18383.1503553f), Make_gain_table(CEILING, 20626.2339468f), Make_gain_table(CEILING, 23143.0151311f)};

FLASHMEM LillaCompressor::LillaCompressor(Profile selected_profile) : AudioStream(4, input_queue), profile(selected_profile), lookahead(MAX_LOOKAHEAD), attack_step(1.0f / static_cast<float>(lookahead))
{
}

FLASHMEM void LillaCompressor::Set_feedback(float feedback)
{
    feedback_target = (static_cast<int32_t>(feedback * 65536.0f) / 128) * 128;
}

FLASHMEM void LillaCompressor::Set_enabled(bool value)
{
    enabled = value; // Retarget the running fade; never reset audio history or envelopes.
}

FLASHMEM bool LillaCompressor::Is_enabled(void) const
{
    return enabled;
}

FLASHMEM void LillaCompressor::Set_threshold(int db)
{
    enabled = db != THRESHOLD_OFF;
    if (db == threshold_db)
    {
        return;
    }
    threshold_db = db;
    if (!enabled)
    {
        threshold_remaining = 0;
        return;
    }
    threshold_remaining = static_cast<unsigned>(0.050f * AUDIO_SAMPLE_RATE_EXACT);
    threshold_step = (static_cast<float>(db + 24) - threshold_position) / threshold_remaining;
}

FLASHMEM int LillaCompressor::Get_threshold(void) const
{
    return threshold_db;
}

float LillaCompressor::Required_gain(int32_t peak)
{
    // Keep the largest peak in [now - lookahead, now], including the sample being emitted.
    while (peak_count != 0 && static_cast<uint32_t>(sample_clock - peak_times[peak_head]) > lookahead)
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
    if (threshold_remaining != 0)
    {
        --threshold_remaining;
        threshold_position = threshold_remaining == 0 ? static_cast<float>(threshold_db + 24) : threshold_position + threshold_step;
    }
    // Interpolate adjacent curves sample by sample, continuing from the current position on retarget.
    const unsigned curve = std::min(20u, static_cast<unsigned>(threshold_position));
    const float blend = threshold_position - static_cast<float>(curve);
    const float lower = gain_table[curve][index] + fraction * (gain_table[curve][index + 1] - gain_table[curve][index]);
    const float upper = gain_table[curve + 1][index] + fraction * (gain_table[curve + 1][index + 1] - gain_table[curve + 1][index]);
    float target = lower + blend * (upper - lower);
    if (maximum > CEILING)
    {
        target = std::min(target, CEILING / static_cast<float>(maximum)); // Bound interpolation error at the ceiling.
    }
    return target;
}

int16_t LillaCompressor::To_sample(float sample)
{
    return static_cast<int16_t>(std::max(-32768.0f, std::min(32767.0f, sample)));
}

void LillaCompressor::Process_sample(int32_t left, int32_t right, int16_t &output_left, int16_t &output_right)
{
    const float target = Required_gain(std::max(std::abs(left), std::abs(right)));
    if (target < compressor_gain)
    {
        // A full-scale gain reduction takes at most lookahead samples, before the peak arrives.
        compressor_gain = std::max(target, compressor_gain - attack_step);
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
    if (++delay_position == lookahead)
    {
        delay_position = 0;
    }

    // Delay both dry and wet paths, including bypass: toggling never moves the recording timeline.
    const float dry_l = To_sample(static_cast<float>(delayed_l));
    const float dry_r = To_sample(static_cast<float>(delayed_r));
    output_left = To_sample(dry_l + wet * (static_cast<float>(delayed_l) * compressor_gain - dry_l));
    output_right = To_sample(dry_r + wet * (static_cast<float>(delayed_r) * compressor_gain - dry_r));
}

FLASHMEM void LillaCompressor::Reset(void)
{
    std::fill(std::begin(delayed_left), std::end(delayed_left), 0);
    std::fill(std::begin(delayed_right), std::end(delayed_right), 0);
    delay_position = 0;
    peak_head = 0;
    peak_count = 0;
    sample_clock = 0;
    compressor_gain = 1.0f;
    threshold_position = static_cast<float>(std::min(threshold_db, -3) + 24);
    threshold_remaining = 0;
    bypass_position = enabled ? 1.0f : 0.0f;
    feedback_gain = feedback_target;
    processing = true;
}

void LillaCompressor::update(void)
{
    if (profile == Profile::LiveSampler && (Recorder_ptr == nullptr || !Recorder_ptr->Is_writing()))
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
        Reset();
        if (profile == Profile::LiveSampler)
        {
            bypass_position = 0.0f; // Preserve the Live Sampler fade-in at the start of each recording.
        }
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
        if (profile == Profile::LiveSampler && feedback_gain < feedback_target)
        {
            ++feedback_gain;
        }
        else if (profile == Profile::LiveSampler && feedback_gain > feedback_target)
        {
            --feedback_gain;
        }
        const int32_t line_l = inputs[0] == nullptr ? 0 : inputs[0]->data[sample];
        const int32_t line_r = inputs[1] == nullptr ? 0 : inputs[1]->data[sample];
        const int32_t return_l = inputs[2] == nullptr ? 0 : inputs[2]->data[sample];
        const int32_t return_r = inputs[3] == nullptr ? 0 : inputs[3]->data[sample];
        const int32_t sum_l = line_l + (profile == Profile::LiveSampler ? (feedback_gain * return_l) >> 16 : return_l);
        const int32_t sum_r = line_r + (profile == Profile::LiveSampler ? (feedback_gain * return_r) >> 16 : return_r);
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
