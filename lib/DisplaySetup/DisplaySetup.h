/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#pragma once

#include "DisplayPrimitives.h"

// Setup page and MIDI Control Change assignments.
class DisplaySetup
{
private:
    static constexpr int Setup_Control_change_X = 13; // Character column.

public:
    DisplaySetup() {}

    void SETUP_show_SETUP_page(void);
    void SETUP_show_Key_step_value(void);
    void SETUP_show_First_octave_value(void);
    void SETUP_show_frame(int8_t value);
    void CC_show_ControlChange_page(void);
    void CC_show_all_sound_gains(void);
    void CC_show_sound_gain(int value);
    void CC_show_lowpass_filter_value(void);
    void CC_show_frame_menu(int value);
};
