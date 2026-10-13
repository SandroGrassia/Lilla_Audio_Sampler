/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>     // https://learn.adafruit.com/adafruit-gfx-graphics-library/graphics-primitives
#include <ILI9341_t3n.h>
#include "SharedElements.h"
#include "SharedSampler.h"
#include "DisplayPrimitives.h"
#include "SharedMixer.h"

class DisplayMixer
{
private:
    static constexpr float MX_column_Sound = 9;
    static constexpr float MX_row_Sound = 5;
    static constexpr float MX_column_row_Source[MX_sources][2] =
        {
            {MX_column_Sound - 1, 5},
            {MX_column_Sound + 4, 5},
            {MX_column_Sound + 9, 5},
            {MX_column_Sound + 14, 5},
            {MX_column_Sound + 19, 5},
            {MX_column_Sound + 24, 5},
            {MX_column_Sound + 29, 5},
            {MX_column_Sound + 34, 5},
            {MX_column_Sound + 39, 5},
    };

    static constexpr float MX_column_row_Mute[MX_sources][2] =
        {
            {MX_column_Sound, 6},
            {MX_column_Sound + 5, 6},
            {MX_column_Sound + 10, 6},
            {MX_column_Sound + 15, 6},
            {MX_column_Sound + 20, 6},
            {MX_column_Sound + 25, 6},
            {MX_column_Sound + 30, 6},
            {MX_column_Sound + 35, 6},
            {MX_column_Sound + 40, 6},
    };

    static constexpr float MX_column_row_Gain[MX_sources][2] =
        {
            {MX_column_Sound - 1, 7},
            {MX_column_Sound + 4, 7},
            {MX_column_Sound + 9, 7},
            {MX_column_Sound + 14, 7},
            {MX_column_Sound + 19, 7},
            {MX_column_Sound + 24, 7},
            {MX_column_Sound + 29, 7},
            {MX_column_Sound + 34, 7},
            {MX_column_Sound + 39, 7},
    };

    static constexpr float MX_column_row_Pan[MX_sources][2] =
        {
            {MX_column_Sound - 1, 8},
            {MX_column_Sound + 4, 8},
            {MX_column_Sound + 9, 8},
            {MX_column_Sound + 14, 8},
            {MX_column_Sound + 19, 8},
            {MX_column_Sound + 24, 8},
            {MX_column_Sound + 29, 8},
            {MX_column_Sound + 34, 8},
            {MX_column_Sound + 39, 8},
    };

    static constexpr float MX_column_row_Lineout[MX_sources][2] =
        {
            {MX_column_Sound, 9},
            {MX_column_Sound + 5, 9},
            {MX_column_Sound + 10, 9},
            {MX_column_Sound + 15, 9},
            {MX_column_Sound + 20, 9},
            {MX_column_Sound + 25, 9},
            {MX_column_Sound + 30, 9},
            {MX_column_Sound + 35, 9},
            {MX_column_Sound + 40, 9},
    };

    static constexpr float MX_column_row_Monitor[MX_sources][2] =
        {
            {MX_column_Sound, 10},
            {MX_column_Sound + 5, 10},
            {MX_column_Sound + 10, 10},
            {MX_column_Sound + 15, 10},
            {MX_column_Sound + 20, 10},
            {MX_column_Sound + 25, 10},
            {MX_column_Sound + 30, 10},
            {MX_column_Sound + 35, 10},
            {MX_column_Sound + 40, 10},
    };

    static constexpr float MX_frame_wide_high_Source[2] = {4, 6};
    static constexpr float MX_frame_wide_high_Mute[2] = {2, 0.7};
    static constexpr float MX_frame_wide_high_Gain[2] = {4, 0.7};
    static constexpr float MX_frame_wide_high_Pan[2] = {4, 0.7};
    static constexpr float MX_frame_wide_high_Lineout[2] = {2, 0.7};
    static constexpr float MX_frame_wide_high_Monitor[2] = {2, .7};

public:
    DisplayMixer() {}

    void MX_page(void);
    void MX_source_values(const int source, const bool bright);
    void MX_source_values_write(const int source, const bool bright);
    void MX_source_values_edit(const int source);
    void MX_source_values_jump(const int old_source, const int new_source);
    void MX_show_pointer_frame(const MX_pointer_struct pointer, const bool show);
};