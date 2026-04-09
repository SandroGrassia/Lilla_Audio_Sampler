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
    static constexpr int MX_X0 = 8;
    static constexpr int MX_Y0 = 5;
    static constexpr float MX_column_row_Source[sources][2] =
    {
        {7, 5},
        {12, 5},
        {17, 5},
        {22, 5},
        {27, 5},
        {32, 5},
        {37, 5},
        {42, 5},
        {47, 5},
    };

    static constexpr float MX_column_row_Mute_Gain[sources][2]=
    {
        {7, 6},
        {12, 6},
        {17, 6},
        {22, 6},
        {27, 6},
        {32, 6},
        {37, 6},
        {42, 6},
        {47, 6},
    };

    static constexpr float MX_column_row_Pan[sources][2]=
    {
        {7, 8},
        {12, 8},
        {17, 8},
        {22, 8},
        {27, 8},
        {32, 8},
        {37, 8},
        {42, 8},
        {47, 8},
    };

    static constexpr float MX_column_row_Lineout[sources][2]=
    {
        {7, 9},
        {12, 9},
        {17, 9},
        {22, 9},
        {27, 9},
        {32, 9},
        {37, 9},
        {42, 9},
        {47, 9},
    };

    static constexpr float MX_column_row_Monitor[sources][2]=
    {
        {7, 10},
        {12, 10},
        {17, 10},
        {22, 10},
        {27, 10},
        {32, 10},
        {37, 10},
        {42, 10},
        {47, 10},
    };

    static constexpr int MX_frame_column_row_Source[2] = {4, 6};
    static constexpr int MX_frame_column_row_Mute_Gain[2] = {4, 2};
    static constexpr int MX_frame_column_row_Pan[2] = {4, 1};
    static constexpr int MX_frame_column_row_Lineout[2] = {4, 1};
    static constexpr int MX_frame_column_row_Monitor[2] = {4, 1};

    public:
    DisplayMixer () {}
    void MX_page(void);
    void MX_source_values(int source);
    void MX_source_values_write(int source);
    void MX_source_values_edit(int source);
    void MX_source_values_jump(int old_source, int new_source);
    
};