/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h> // https://learn.adafruit.com/adafruit-gfx-graphics-library/graphics-primitives
#include <ILI9341_t3n.h>
#include "GlobalDisplayCommon.h"
#include "SharedElements.h"
#include "DisplayPrimitives.h"
#include "SharedDelay.h"

/*

enum DELAY_element_name
{
    value_DELAY_Feedback,
    value_DELAY_Lpf_cutoff,
    value_DELAY_Compressor,
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
    static constexpr int DEL_ROW_SOUND = 5;
    static constexpr int DEL_COL_REVERB = 28;

    static constexpr float DELAY_column_row_VOLUME[2] = {41, 0};

    static constexpr float DELAY_column_row_FEEDBACK[2] = {0, DEL_ROW_SOUND + 1.5};
    static constexpr float DELAY_column_row_COMPRESSOR[2] = {0, DEL_ROW_SOUND + 3.5};
    static constexpr float DELAY_column_row_FEEDBACK_LPF[2] = {0, DEL_ROW_SOUND + 2.5};
    static constexpr float DELAY_column_row_DELAY_TIME[2] = {0, DEL_ROW_SOUND + 4.5};
    static constexpr float DELAY_column_row_DELAY_TIME_LR[2] = {0, DEL_ROW_SOUND + 5.5};
    static constexpr float DELAY_column_row_MODULATION_SOURCE[2] = {0, DELAY_column_row_DELAY_TIME_LR[1] + 1};
    static constexpr float DELAY_column_row_MODULATION_FREQUENCY[2] = {0, DELAY_column_row_DELAY_TIME_LR[1] + 2};
    static constexpr float DELAY_column_row_MODULATION_DEPTH[2] = {0, DELAY_column_row_DELAY_TIME_LR[1] + 3};
    static constexpr float DELAY_column_row_MODULATION_PHASE_LR[2] = {0, DELAY_column_row_DELAY_TIME_LR[1] + 4};
    static constexpr float DELAY_column_row_REVERB_LEVEL[2] = {DEL_COL_REVERB, DEL_ROW_SOUND + 1.5};
    static constexpr float DELAY_column_row_REVERB_DECAY[2] = {DEL_COL_REVERB, DELAY_column_row_REVERB_LEVEL[1] + 1};
    static constexpr float DELAY_column_row_REVERB_DAMPING[2] = {DEL_COL_REVERB, DELAY_column_row_REVERB_LEVEL[1] + 2};

    static constexpr float DELAY_column_row_sounds[2] = {8, DEL_ROW_SOUND};
    static constexpr float DELAY_column_row_feedback[2] = {8, DEL_ROW_SOUND + 1.5};
    static constexpr float DELAY_column_row_compressor[2] = {11, DEL_ROW_SOUND + 3.5};
    static constexpr float DELAY_column_row_feedback_lpf[2] = {13, DEL_ROW_SOUND + 2.5};
    static constexpr float DELAY_column_row_delay_time[2] = {10, DEL_ROW_SOUND + 4.5};
    static constexpr float DELAY_column_row_delay_time_LR[2] = {20, DEL_ROW_SOUND + 5.5};
    static constexpr float DELAY_column_row_modulation_source[2] = {DELAY_column_row_MODULATION_SOURCE[0] + 11, DELAY_column_row_MODULATION_SOURCE[1]};
    static constexpr float DELAY_column_row_modulation_frequency[2] = {DELAY_column_row_MODULATION_FREQUENCY[0] + 14, DELAY_column_row_MODULATION_FREQUENCY[1]};
    static constexpr float DELAY_column_row_modulation_depth[2] = {DELAY_column_row_MODULATION_DEPTH[0] + 10, DELAY_column_row_MODULATION_DEPTH[1]};
    static constexpr float DELAY_column_row_modulation_phase_LR[2] = {DELAY_column_row_MODULATION_PHASE_LR[0] + 14, DELAY_column_row_MODULATION_PHASE_LR[1]};
    static constexpr float DELAY_column_row_reverb_level[2] = {DELAY_column_row_REVERB_LEVEL[0] + sizeof("REVERB LEVEL"), DELAY_column_row_REVERB_LEVEL[1]};
    static constexpr float DELAY_column_row_reverb_decay[2] = {DELAY_column_row_REVERB_DECAY[0] + sizeof("REVERB DECAY"), DELAY_column_row_REVERB_DECAY[1]};
    static constexpr float DELAY_column_row_reverb_damping[2] = {DELAY_column_row_REVERB_DAMPING[0] + sizeof("REVERB DUMPING"), DELAY_column_row_REVERB_DAMPING[1]};

    static constexpr int DELAY_chars_sounds = 23;
    static constexpr int DELAY_chars_feedback = 3;
    static constexpr int DELAY_chars_feedback_compressor = 6;
    static constexpr int DELAY_chars_feedback_lpf = 7;
    static constexpr int DELAY_chars_delay_time = 6;
    static constexpr int DELAY_chars_delay_time_LR = 7;
    static constexpr int DELAY_chars_modulation_source = 6;
    static constexpr int DELAY_chars_modulation_frequency = 7;
    static constexpr int DELAY_chars_modulation_depth = 6;
    static constexpr int DELAY_chars_modulation_phase_LR = 6;

    static constexpr int DELAY_chars_reverb_level = 4;
    static constexpr int DELAY_chars_reverb_decay = 4;
    static constexpr int DELAY_chars_reverb_damping = 4;

    // pointer
    static constexpr float DELAY_column_row_element[DELAY_element_names][2] = {
        {DELAY_column_row_feedback[0], DELAY_column_row_feedback[1]},
        {DELAY_column_row_feedback_lpf[0], DELAY_column_row_feedback_lpf[1]},
        {DELAY_column_row_compressor[0], DELAY_column_row_compressor[1]},
        {DELAY_column_row_delay_time[0], DELAY_column_row_delay_time[1]},
        {DELAY_column_row_delay_time_LR[0], DELAY_column_row_delay_time_LR[1]},
        {DELAY_column_row_modulation_source[0], DELAY_column_row_modulation_source[1]},
        {DELAY_column_row_modulation_frequency[0], DELAY_column_row_modulation_frequency[1]},
        {DELAY_column_row_modulation_depth[0], DELAY_column_row_modulation_depth[1]},
        {DELAY_column_row_modulation_phase_LR[0], DELAY_column_row_modulation_phase_LR[1]},
        {DELAY_column_row_reverb_level[0], DELAY_column_row_reverb_level[1]},
        {DELAY_column_row_reverb_decay[0], DELAY_column_row_reverb_decay[1]},
        {DELAY_column_row_reverb_damping[0], DELAY_column_row_reverb_damping[1]}};

    static constexpr int DELAY_chars_element[DELAY_element_names] = {
        DELAY_chars_feedback,
        DELAY_chars_feedback_lpf,
        DELAY_chars_feedback_compressor,
        DELAY_chars_delay_time,
        DELAY_chars_delay_time_LR,
        DELAY_chars_modulation_source,
        DELAY_chars_modulation_frequency,
        DELAY_chars_modulation_depth,
        DELAY_chars_modulation_phase_LR,
        DELAY_chars_reverb_level,
        DELAY_chars_reverb_decay,
        DELAY_chars_reverb_damping};

public:
    DisplayDelay() {}

    void D_disabled(void);
    void D_show_page(void);
    void D_sounds(void); // Display the requested setting, independently of intermediate DSP values.
    void D_feedback(void);             // feedback
    void D_feedback_compressor(void);
    void D_reverb(void);
    void D_feedback_lpf(void);           // Feedback cutoff in integer Hz within seven characters.
    void D_delay_time(void);           // Display the requested setting, independently of intermediate DSP values.
    void D_delay_time_LR(void);        // Display the requested setting, independently of intermediate DSP values.
    void D_modulation_source(void);    // Display the requested setting, independently of intermediate DSP values.
    void D_modulation_frequency(void); // Display the requested setting, independently of intermediate DSP values.
    void D_modulation_depth(void);     // Display the requested setting, independently of intermediate DSP values.
    void D_modulation_phase_LR(void);  // Display the requested setting, independently of intermediate DSP values.

    // pointer
    void DELAY_show_pointer_frame(const DELAY_element_name pointer, const bool show);
};