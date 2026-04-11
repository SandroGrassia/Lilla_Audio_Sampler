/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>     // https://learn.adafruit.com/adafruit-gfx-graphics-library/graphics-primitives
#include <Adafruit_ILI9341.h> // 1.5.12 version - Hardware-specific library
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
static constexpr int Delay_ROW_BASE = 6;

static constexpr float DELAY_column_row_FEEDBACK[2] = {, };
static constexpr float DELAY_column_row_DELAY_TIME[2] = {, };
static constexpr float DELAY_column_row_DELAY_TIME_LR[2] = {, };
static constexpr float DELAY_column_row_MODULATION_SOURCE[2] = {, };
static constexpr float DELAY_column_row_MODULATION_FREQUENCY[2] = {, };
static constexpr float DELAY_column_row_MODULATION_DEPTH[2] = {, };
static constexpr float DELAY_column_row_MODULATION_PHASE_LR[2] = {, };

static constexpr float DELAY_column_row_feedback[2] = {8.5, Delay_ROW_BASE + 2};
static constexpr float DELAY_column_row_delay_time[2] = {10, Delay_ROW_BASE + 3};
static constexpr float DELAY_column_row_delay_time_LR[2] = {10, Delay_ROW_BASE + 4};
static constexpr float DELAY_column_row_modulation_source[2] = {30, Delay_ROW_BASE + 2};
static constexpr float DELAY_column_row_modulation_frequency[2] = {30, Delay_ROW_BASE + 3};
static constexpr float DELAY_column_row_modulation_depth[2] = {30, Delay_ROW_BASE + 4};
static constexpr float DELAY_column_row_modulation_phase_LR[2] = {30, Delay_ROW_BASE + 5};

static constexpr int DELAY_chars_feedback = 8;
static constexpr int DELAY_chars_delay_time = 9;
static constexpr int DELAY_chars_delay_time_LR = 12;
static constexpr int DELAY_chars_modulation_source = 6;
static constexpr int DELAY_chars_modulation_frequency = 7;
static constexpr int DELAY_chars_modulation_depth = 8;
static constexpr int DELAY_chars_modulation_phase_LR = 7;

  
public:
    DisplayDelay() {}
    
    void D_disabled(void);
    void D_show_page(void);
    void D_sounds(void);

    void D_feedback(void); // feedback
    void D_delay_time(void);
    void D_delay_time_LR(void);
    void D_modulation_source(void);
    void D_modulation_frequency(void);
    void D_modulation_depth(void); // index
    void D_modulation_phase_LR(void);
    
};