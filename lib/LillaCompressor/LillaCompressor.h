/* LILLA Audio Sampler - shared stereo Live Sampler and Delay dynamics. */
#pragma once

#include <Arduino.h>
#include <AudioStream.h>
#include <array>
#include "StereoLiveSampler.h"

class LillaCompressor : public AudioStream
{
public:
    static constexpr int THRESHOLD_OFF = -2; // Encoder position after -3 dBFS; not a physical threshold.
    enum class Profile { LiveSampler, Delay };

private:
    static constexpr unsigned MAX_LOOKAHEAD = 32;
    static constexpr unsigned PEAK_QUEUE_SIZE = MAX_LOOKAHEAD + 2;
    static constexpr float CEILING = 29204.0f; // Approximately -1 dBFS.
    PROGMEM static const std::array<std::array<float, 258>, 22> gain_table;
    int threshold_db = -6;
    float threshold_position = 18.0f;
    float threshold_step = 0.0f;
    unsigned threshold_remaining = 0;
    const Profile profile;
    const unsigned lookahead;
    const float attack_step;
    audio_block_t *input_queue[4]; // Line L/R, followed by feedback L/R.
    int32_t delayed_left[MAX_LOOKAHEAD] = {};
    int32_t delayed_right[MAX_LOOKAHEAD] = {};
    int32_t peak_values[PEAK_QUEUE_SIZE] = {};
    uint32_t peak_times[PEAK_QUEUE_SIZE] = {};
    uint32_t sample_clock = 0;
    unsigned delay_position = 0;
    unsigned peak_head = 0;
    unsigned peak_count = 0;
    int32_t feedback_gain = 0;
    int32_t feedback_target = 0;
    float compressor_gain = 1.0f;
    float bypass_position = 0.0f;
    bool enabled = false;
    bool processing = false;

    float Required_gain(int32_t peak);
    void Process_sample(int32_t left, int32_t right, int16_t &output_left, int16_t &output_right);
    static int16_t To_sample(float sample);

public:
    explicit LillaCompressor(Profile selected_profile);
    void Reset(void); // Clear history with audio interrupts disabled; preserve the requested enable state.
    StereoLiveSampler *Recorder_ptr = nullptr; // Connected at startup before recording can begin.
    void update(void) override;
    void Set_feedback(float feedback); // LiveSampler profile only; caller supplies the table range [0, 1].
    void Set_enabled(bool value); // Call with audio interrupts disabled.
    bool Is_enabled(void) const;
    void Set_threshold(int db); // Caller supplies [-24, -3] dBFS or THRESHOLD_OFF with audio interrupts disabled.
    int Get_threshold(void) const;
};
