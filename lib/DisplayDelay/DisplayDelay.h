/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>     // https://learn.adafruit.com/adafruit-gfx-graphics-library/graphics-primitives
#include <ILI9341_t3n.h>
#include "GlobalDisplayManager.h"
#include "SharedElements.h"
#include "DisplayPrimitives.h"
#include "SharedDelay.h"

/*

enum DELAY_element_name
{
    value_DELAY_Feedback,
    value_DELAY_Delay_time,
    value_DELAY_Delay_time_LR,
    value_DELAY_Modulation_source,
    value_DELAY_Modulation_frequency,
    value_DELAY_Modulation_depth,
    value_DELAY_Modulation_phase_LR
};

*/

class DisplayDelay
{
private:
    static constexpr int DEL_ROW_SOUND = 6;
    static constexpr int DEL_COL_MOD_SOURCE = 28;

    static constexpr float DELAY_column_row_VOLUME[2] = {41, 0};
    static constexpr float DELAY_column_row_FEEDBACK[2] = {0, DEL_ROW_SOUND + 2};
    static constexpr float DELAY_column_row_DELAY_TIME[2] = {0, DEL_ROW_SOUND + 3};
    static constexpr float DELAY_column_row_DELAY_TIME_LR[2] = {0, DEL_ROW_SOUND + 4};
    static constexpr float DELAY_column_row_MODULATION_SOURCE[2] = {DEL_COL_MOD_SOURCE, DEL_ROW_SOUND + 2};
    static constexpr float DELAY_column_row_MODULATION_FREQUENCY[2] = {DEL_COL_MOD_SOURCE, DEL_ROW_SOUND + 3};
    static constexpr float DELAY_column_row_MODULATION_DEPTH[2] = {DEL_COL_MOD_SOURCE, DEL_ROW_SOUND + 4};
    static constexpr float DELAY_column_row_MODULATION_PHASE_LR[2] = {DEL_COL_MOD_SOURCE, DEL_ROW_SOUND + 5};

    static constexpr float DELAY_column_row_sounds[2] = {8, DEL_ROW_SOUND};
    static constexpr int DELAY_chars_sounds = 23;

    static constexpr float DELAY_column_row_feedback[2] = {9, DEL_ROW_SOUND + 2};
    static constexpr float DELAY_column_row_delay_time[2] = {11, DEL_ROW_SOUND + 3};
    static constexpr float DELAY_column_row_delay_time_LR[2] = {15, DEL_ROW_SOUND + 4};
    static constexpr float DELAY_column_row_modulation_source[2] = {DEL_COL_MOD_SOURCE + 11, DEL_ROW_SOUND + 2};
    static constexpr float DELAY_column_row_modulation_frequency[2] = {DEL_COL_MOD_SOURCE + 14, DEL_ROW_SOUND + 3};
    static constexpr float DELAY_column_row_modulation_depth[2] = {DEL_COL_MOD_SOURCE + 10, DEL_ROW_SOUND + 4};
    static constexpr float DELAY_column_row_modulation_phase_LR[2] = {DEL_COL_MOD_SOURCE + 14, DEL_ROW_SOUND + 5};

    static constexpr int DELAY_chars_feedback = 6;
    static constexpr int DELAY_chars_delay_time = 7;
    static constexpr int DELAY_chars_delay_time_LR = 10;
    static constexpr int DELAY_chars_modulation_source = 6;
    static constexpr int DELAY_chars_modulation_frequency = 7;
    static constexpr int DELAY_chars_modulation_depth = 6;
    static constexpr int DELAY_chars_modulation_phase_LR = 6;

    // pointer
    static constexpr float DELAY_column_row_element[DELAY_element_names][2] = {
        {DELAY_column_row_feedback[0], DELAY_column_row_feedback[1]},
        {DELAY_column_row_delay_time[0], DELAY_column_row_delay_time[1]},
        {DELAY_column_row_delay_time_LR[0], DELAY_column_row_delay_time_LR[1]},
        {DELAY_column_row_modulation_source[0], DELAY_column_row_modulation_source[1]},
        {DELAY_column_row_modulation_frequency[0], DELAY_column_row_modulation_frequency[1]},
        {DELAY_column_row_modulation_depth[0], DELAY_column_row_modulation_depth[1]},
        {DELAY_column_row_modulation_phase_LR[0], DELAY_column_row_modulation_phase_LR[1]}};

    static constexpr int DELAY_chars_element[DELAY_element_names] = {
        DELAY_chars_feedback,
        DELAY_chars_delay_time,
        DELAY_chars_delay_time_LR,
        DELAY_chars_modulation_source,
        DELAY_chars_modulation_frequency,
        DELAY_chars_modulation_depth,
        DELAY_chars_modulation_phase_LR};

public:
    DisplayDelay() {}

    void D_disabled(void);
    void D_show_page(void);
    void D_sounds(void); // Display the requested setting, independently of intermediate DSP values.

    void D_feedback(void);             // feedback
    void D_delay_time(void);           // Display the requested setting, independently of intermediate DSP values.
    void D_delay_time_LR(void);        // Display the requested setting, independently of intermediate DSP values.
    void D_modulation_source(void);    // Display the requested setting, independently of intermediate DSP values.
    void D_modulation_frequency(void); // Display the requested setting, independently of intermediate DSP values.
    void D_modulation_depth(void);     // Display the requested setting, independently of intermediate DSP values.
    void D_modulation_phase_LR(void);  // Display the requested setting, independently of intermediate DSP values.

    // pointer
    void DELAY_show_pointer_frame(const DELAY_element_name pointer, const bool show);
};