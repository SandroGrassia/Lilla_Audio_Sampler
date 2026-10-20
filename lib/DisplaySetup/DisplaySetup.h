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
    static constexpr int Convert_popup_lines = 5;
    static constexpr int Import_info_popup_lines = 4;
    static constexpr int Setup_Control_change_X = 13; // Character column.

public:
    DisplaySetup() {}

    void SETUP_show_SETUP_page(void);
    void SETUP_show_Key_step_value(void);
    void SETUP_show_First_octave_value(void);
    void SETUP_show_frame(int8_t value);
    FLASHMEM void SETUP_show_convert_popup(void);
    FLASHMEM void SETUP_show_import_info_popup(void);
    FLASHMEM void SETUP_show_import_info_frame(int value);
    FLASHMEM void SETUP_show_convert_frame(int value);
    void CC_show_ControlChange_page(void);
    void CC_show_all_sound_gains(void);
    void CC_show_sound_gain(int value);
    void CC_show_lowpass_filter_value(void);
    void CC_show_frame_menu(int value);
};
