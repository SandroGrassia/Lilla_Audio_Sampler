/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#pragma once

#include "DisplayPrimitives.h"

// Shared patch headers, effects and confirmation frames.
class DisplayCommon
{
private:
    // Shared patch header
    static constexpr float P_column_PATCH = 27;
    static constexpr float P_column_VOLUME = 41;
    static constexpr float P_column_Volume_value = 47.5;
public:
    static constexpr float P_column_Patch_id = 33;

    DisplayCommon() {}

    void Confirm_no_yes_popup_frame(int value);
    void Show_all_effects(void);
    void Resolution(void);
    void Downsampling(void);
    void Lowpass_filter(void);
    void P_show_PERFORMANCE_title(void);
    void P_show_Patch_number(bool change_patch);
    void P_Patch_VOLUME(bool change_vol); // shows VOLUME <value>
    void P_Patch_volume_value(bool change_vol);
    void Patch_volume_color(bool change_patch, bool change_vol);
};
