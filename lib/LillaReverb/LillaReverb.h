/* LILLA Audio Sampler - coupled four-comb stereo reverb with two all-pass diffusers per channel. */
#pragma once
#include <Arduino.h>
#include <AudioStream.h>

class LillaReverb : public AudioStream
{
public:
    static constexpr unsigned CHANNEL_SAMPLES = 16384;
    FLASHMEM LillaReverb();
    FLASHMEM void Setup(int16_t *left, int16_t *right); // Caller supplies disjoint CHANNEL_SAMPLES buffers with audio interrupts disabled.
    FLASHMEM void Set_parameters(int level, int decay, int damping); // 0..100 percent, 0..50 tenths of a second, 0..100 percent; audio interrupts disabled.
    FLASHMEM void Reset(); // Clear the tail with audio interrupts disabled; preserve requested parameters.
    void Suspend(bool value); // Stop tail processing during Panic buffer clearing.
    void update() override;
private:
    PROGMEM static const unsigned lengths[2][4];
    PROGMEM static const unsigned allpass_lengths[2][2];
    static constexpr float ALLPASS_GAIN = 0.5f;
    static constexpr float CROSS_FEEDBACK = 0.15f;
    audio_block_t *input_queue[2];
    int16_t *storage[2] = {};
    unsigned position[2][4] = {};
    unsigned allpass_position[2][2] = {};
    float filtered[2][4] = {};
    float gain[2][4] = {};
    float gain_target[2][4] = {};
    float injection[2][4] = {};
    float injection_target[2][4] = {};
    static constexpr unsigned WET_LOOKAHEAD = 32;
    float wet_delay[2][WET_LOOKAHEAD + 1] = {};
    float wet_peaks[WET_LOOKAHEAD + 1] = {};
    unsigned wet_position = 0;
    float wet_gain = 1.0f;
    static float Read_sample(const int16_t *buffer, unsigned index);
    static void Write_sample(int16_t *buffer, unsigned index, float value);
    void Limit_wet(float &left, float &right);
    float level = 0.0f;
    float level_target = 0.0f;
    float damping = 0.45f;
    float damping_target = 0.45f;
    int decay_setting = -1;
    bool suspended = false;
};
