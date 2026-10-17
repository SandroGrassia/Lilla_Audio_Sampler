/* LILLA Audio Sampler - stereo Delay input and feedback dynamics. */
#pragma once

#include <Arduino.h>
#include <AudioStream.h>
#include <array>

class AudioDelayCompressor : public AudioStream
{
private:
    static constexpr unsigned LOOKAHEAD = 32;
    static constexpr unsigned PEAK_QUEUE_SIZE = LOOKAHEAD + 2;
    static constexpr float CEILING = 29204.0f; // Approximately -1 dBFS.
    static const std::array<float, 258> gain_table;
    audio_block_t *input_queue[4]; // Routed L/R, followed by filtered, gain-scaled feedback L/R.
    int32_t delayed_left[LOOKAHEAD] = {};
    int32_t delayed_right[LOOKAHEAD] = {};
    int32_t peak_values[PEAK_QUEUE_SIZE] = {};
    uint32_t peak_times[PEAK_QUEUE_SIZE] = {};
    uint32_t sample_clock = 0;
    unsigned delay_position = 0;
    unsigned peak_head = 0;
    unsigned peak_count = 0;
    float compressor_gain = 1.0f;
    float bypass_position = 0.0f;
    bool enabled = false;

    float Required_gain(int32_t peak);
    void Process_sample(int32_t left, int32_t right, int16_t &output_left, int16_t &output_right);
    static int16_t To_sample(float sample);

public:
    AudioDelayCompressor();
    void update(void) override;
    void Set_enabled(bool value); // Call with audio interrupts disabled.
    bool Is_enabled(void) const;
    void Reset(void); // Clear lookahead and gain history with audio interrupts disabled.
};
