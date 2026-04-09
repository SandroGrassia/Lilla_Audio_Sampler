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
    static constexpr float MX_X0 = 9;
    static constexpr float MX_Y0 = 5;
    static constexpr float MX_column_row_Source[sources][2] =
    {
        {MX_X0 - 1, 5},
        {MX_X0 + 4, 5},
        {MX_X0 + 9, 5},
        {MX_X0 + 14, 5},
        {MX_X0 + 19, 5},
        {MX_X0 + 24, 5},
        {MX_X0 + 29, 5},
        {MX_X0 + 34, 5},
        {MX_X0 + 39, 5},
    };

    static constexpr float MX_column_row_Mute_Gain[sources][2]=
    {
        {MX_X0 - 1, 6},
        {MX_X0 + 4, 6},
        {MX_X0 + 9, 6},
        {MX_X0 + 14, 6},
        {MX_X0 + 19, 6},
        {MX_X0 + 24, 6},
        {MX_X0 + 29, 6},
        {MX_X0 + 34, 6},
        {MX_X0 + 39, 6},
    };

    static constexpr float MX_column_row_Pan[sources][2]=
    {
        {MX_X0 - 1, 8},
        {MX_X0 + 4, 8},
        {MX_X0 + 9, 8},
        {MX_X0 + 14, 8},
        {MX_X0 + 19, 8},
        {MX_X0 + 24, 8},
        {MX_X0 + 29, 8},
        {MX_X0 + 34, 8},
        {MX_X0 + 39, 8},
    };

    static constexpr float MX_column_row_Lineout[sources][2]=
    {
        {MX_X0 - 1, 9},
        {MX_X0 + 4, 9},
        {MX_X0 + 9, 9},
        {MX_X0 + 14, 9},
        {MX_X0 + 19, 9},
        {MX_X0 + 24, 9},
        {MX_X0 + 29, 9},
        {MX_X0 + 34, 9},
        {MX_X0 + 39, 9},
    };

    static constexpr float MX_column_row_Monitor[sources][2]=
    {
        {MX_X0 - 1, 10},
        {MX_X0 + 4, 10},
        {MX_X0 + 9, 10},
        {MX_X0 + 14, 10},
        {MX_X0 + 19, 10},
        {MX_X0 + 24, 10},
        {MX_X0 + 29, 10},
        {MX_X0 + 34, 10},
        {MX_X0 + 39, 10},
    };

    static constexpr int MX_frame_chars_high_Source[2] = {4, 6};
    static constexpr int MX_frame_chars_high_Mute_Gain[2] = {4, 2};
    static constexpr int MX_frame_chars_high_Pan[2] = {4, 1};
    static constexpr int MX_frame_chars_high_Lineout[2] = {4, 1};
    static constexpr int MX_frame_chars_high_Monitor[2] = {4, 1};

    public:
    DisplayMixer () {}

    void MX_page(void);
    void MX_source_values(int source);
    void MX_source_values_write(int source);
    void MX_source_values_edit(int source);
    void MX_source_values_jump(int old_source, int new_source);
    void MX_show_pointer_frame(MX_pointer_struct pointer, bool show);
    
};