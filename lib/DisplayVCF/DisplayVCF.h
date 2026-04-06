/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>     // https://learn.adafruit.com/adafruit-gfx-graphics-library/graphics-primitives
#include <Adafruit_ILI9341.h> // 1.5.12 version - Hardware-specific library
#include <AudioStream.h>      // solo per definizione AUDIO_SAMPLE_RATE
#include "SharedElements.h"
#include "DisplayPrimitives.h"
#include "GlobalDisplayManager.h"
#include "SharedVCF.h"

class DisplayVCF
{
private:
    // solo (constant value on this page)
    static constexpr float VCF_column_row_Solo[2] = {21.5, 5};
    static constexpr int VCF_chars_Solo = 8;

    // values (changed with pointer)
    static constexpr float VCF_column_row_Menu[2] = {0, 1};
    static constexpr float VCF_column_row_Gain[2] = {47.5, 0};
    static constexpr float VCF_column_row_FilterType[2] = {12, 8};
    static constexpr float VCF_column_row_Cutoff[2] = {19, 9};
    static constexpr float VCF_column_row_Resonance[2] = {10, 10};
    static constexpr float VCF_column_row_LFO_modulation_source[2] = {18, 11};
    static constexpr float VCF_column_row_LFO_frequancy_time[2] = {14, 12};
    static constexpr float VCF_column_row_LFO_modulation_depth[2] = {10, 13};

    static constexpr float VCF_column_row_value_element[VCF_value_names][2] =
        {
            {VCF_column_row_Menu[0], VCF_column_row_Menu[1]},
            {VCF_column_row_Gain[0], VCF_column_row_Gain[1]},
            {VCF_column_row_FilterType[0], VCF_column_row_FilterType[1]},
            {VCF_column_row_Cutoff[0], VCF_column_row_Cutoff[1]},
            {VCF_column_row_Resonance[0], VCF_column_row_Resonance[1]},
            {VCF_column_row_LFO_modulation_source[0], VCF_column_row_LFO_modulation_source[1]},
            {VCF_column_row_LFO_frequancy_time[0], VCF_column_row_LFO_frequancy_time[1]},
            {VCF_column_row_LFO_modulation_depth[0], VCF_column_row_LFO_modulation_depth[1]}};
    
    static constexpr int VCF_chars_Menu = 6;
    static constexpr int VCF_chars_Gain = 4;
    static constexpr int VCF_chars_FilterType = 8;
    static constexpr int VCF_chars_Cutoff = 7;
    static constexpr int VCF_chars_Resonance = 4;
    static constexpr int VCF_chars_LfoModulation = 7;
    static constexpr int VCF_chars_ModFreqTime = 10;
    static constexpr int VCF_chars_ModDepth = 4;
  
    static constexpr int VCF_chars_value_element[VCF_value_names] = {
        VCF_chars_Menu,
        VCF_chars_Gain,
        VCF_chars_FilterType,
        VCF_chars_Cutoff,
        VCF_chars_Resonance,
        VCF_chars_LfoModulation,
        VCF_chars_ModFreqTime,
        VCF_chars_ModDepth};

public:
    DisplayVCF() {}

    void VCF_show_VCF_page(int patch_id, int instrument_id);
    void VCF_show_pointer_frame(int pointer, bool show);
    void VCF_show_solo_value(void);

    void VCF_show_sound_gain_value(int sound_id);
    void VCF_show_filter_type_value(int instrument_id);
    void VCF_show_cutoff_value(int instrument_id);
    void VCF_show_resonance_value(int instrument_id);
    void VCF_show_LFO_modulation_source(int instrument_id);
    void VCF_show_LFO_freq_time(int instrument_id);
    void VCF_show_LFO_modulation_depth(int instrument_id);
};