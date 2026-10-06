/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 */

#pragma once

#include <Arduino.h>
#include "SharedDelay.h"
#include "StereoDelay.h"
#include "WaveLFO.h"
#include "AudioGain.h"
#include "PlayersManager.h"
#include "config.h"

class DelayManager
{
private:
    static constexpr int steps = 30;
    static constexpr double n0 = 0.02748805251448725;
    static constexpr double n1 = 0.0178446670423745;
    static constexpr double d1 = 1.6107672804431383;
    static constexpr double d2 = -0.6561000000000001;
    double x[DELAY_LPF_ITEMS] = {};
    double x_1[DELAY_LPF_ITEMS] = {};
    double y_1[DELAY_LPF_ITEMS] = {};
    double y_2[DELAY_LPF_ITEMS] = {};
    double lower[DELAY_LPF_ITEMS] = {};
    double upper[DELAY_LPF_ITEMS] = {};
    uint8_t remaining[DELAY_LPF_ITEMS] = {};
    bool flag[DELAY_ITEMS] = {};
    void Start_LPF(int item, double current, double target); // Restart only the edited parameter from its currently applied value.
    double New_value(int item); // Advance a bounded filter and return the exact target on its final step.
    void Apply_delay_times(void); // Send both channel targets together; StereoDelay owns the only delay-time ramp.

public:
    DelayManager(void) = default; // Start with no pending parameter changes.
    StereoDelay *Delay_L_ptr = nullptr;
    StereoDelay *Delay_R_ptr = nullptr;
    WaveLFO *LFO_D_ptr[2] = {};
    AudioGain *D_gain_L_feedback_ptr = nullptr;
    AudioGain *D_gain_R_feedback_ptr = nullptr;
    PlayersManager *Players_Manager_ptr = nullptr;
    void Update(void); // Apply pending parameters from the audio callback without serial output.
    bool New_values(const Delay_data_struct *data); // Submit a complete patch target with audio interrupts disabled.
    bool Set_value(int item, int value); // Replace one UI target without cancelling other transitions; disable audio interrupts first.
    int Get_value(int item) const; // Read the requested value used by UI and persistence, not an intermediate filter value.
    void Stop(void); // Finish pending manager transitions at their exact targets; disable audio interrupts first.
    void Silence_feedback(void); // Preserve the requested setting, cancel its transition and ramp both gains to zero; disable audio interrupts first.
    void Restore_feedback(void); // Ramp back to the preserved setting after clearing buffers; disable audio interrupts first.
};
