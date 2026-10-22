/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#pragma once

#include "DisplayPrimitives.h"
#include "SharedControlChange.h"

class DisplayControlChange
{
private:
    // Coordinates in character columns/rows and widths in characters.
    static constexpr float CC_column_row_Title[2] = {0, 0};
    static constexpr float CC_column_row_Return[2] = {0, 1};
    static constexpr float CC_column_row_FirstSoundGain[2] = {13, 3};
    static constexpr float CC_column_row_LowpassFilter[2] = {13, 11};
    static constexpr int CC_column_label = 0;
    static constexpr int CC_sound_row_step = 1;
    static constexpr int CC_chars_Return = 6;
    static constexpr int CC_chars_Value = 3;

public:
    FLASHMEM void CC_show_ControlChange_page(void);
    FLASHMEM void CC_show_all_sound_gains(void);
    FLASHMEM void CC_show_sound_gain(int instrument);
    FLASHMEM void CC_show_lowpass_filter_value(void);
    FLASHMEM void Show_pointer_frame(CC_field_description_struct description, bool show);
};
