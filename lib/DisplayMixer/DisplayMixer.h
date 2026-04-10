/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>     // https://learn.adafruit.com/adafruit-gfx-graphics-library/graphics-primitives
#include <Adafruit_ILI9341.h> // 1.5.12 version - Hardware-specific library
#include "SharedElements.h"
#include "SharedDS.h"
#include "DisplayPrimitives.h"
#include "SharedMixer.h"

class DisplayMixer
{
private:
    static constexpr float MX_column_Sound = 9;
    static constexpr float MX_row_Sound = 5;
    static constexpr float MX_column_row_Source[sources][2] =
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

    static constexpr float MX_column_row_Mute_Gain[sources][2] =
        {
            {MX_column_Sound - 1, 6},
            {MX_column_Sound + 4, 6},
            {MX_column_Sound + 9, 6},
            {MX_column_Sound + 14, 6},
            {MX_column_Sound + 19, 6},
            {MX_column_Sound + 24, 6},
            {MX_column_Sound + 29, 6},
            {MX_column_Sound + 34, 6},
            {MX_column_Sound + 39, 6},
    };

    static constexpr float MX_column_row_Pan[sources][2] =
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

    static constexpr float MX_column_row_Lineout[sources][2] =
        {
            {MX_column_Sound - 1, 9},
            {MX_column_Sound + 4, 9},
            {MX_column_Sound + 9, 9},
            {MX_column_Sound + 14, 9},
            {MX_column_Sound + 19, 9},
            {MX_column_Sound + 24, 9},
            {MX_column_Sound + 29, 9},
            {MX_column_Sound + 34, 9},
            {MX_column_Sound + 39, 9},
    };

    static constexpr float MX_column_row_Monitor[sources][2] =
        {
            {MX_column_Sound - 1, 10},
            {MX_column_Sound + 4, 10},
            {MX_column_Sound + 9, 10},
            {MX_column_Sound + 14, 10},
            {MX_column_Sound + 19, 10},
            {MX_column_Sound + 24, 10},
            {MX_column_Sound + 29, 10},
            {MX_column_Sound + 34, 10},
            {MX_column_Sound + 39, 10},
    };

    static constexpr int MX_frame_wide_high_Source[2] = {4, 6};
    static constexpr int MX_frame_wide_high_Mute_Gain[2] = {4, 2};
    static constexpr int MX_frame_wide_high_Pan[2] = {4, 1};
    static constexpr int MX_frame_wide_high_Lineout[2] = {4, 1};
    static constexpr int MX_frame_wide_high_Monitor[2] = {4, 1};

public:
    DisplayMixer() {}

    void MX_page(void);
    void MX_source_values(int source);
    void MX_source_values_write(int source);
    void MX_source_values_edit(int source);
    void MX_source_values_jump(int old_source, int new_source);
    void MX_show_pointer_frame(MX_pointer_struct pointer, bool show);
};