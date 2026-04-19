/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "DisplayPrimitives.h"
#include "SharedElements.h"
#include "SharedLoop.h"
#include "SharedPerformance.h"
#include "GlobalInfoMaster.h"
#include "GraphicElements.h"
#include "GlobalDisplayManager.h"

// Handles all display rendering for the MIDI Loop page on the ILI9341 TFT.

class DisplayMidiLoop
{
private:
    static constexpr int Loop_column_row_LOOP[2] = {18, 0};
    static constexpr int Loop_column_row_METRO[2] = {2, 4};
    static constexpr int Loop_column_row_TRACK[2] = {2, 5};
    static constexpr int Loop_column_row_SLIDE[2] = {2, 6};
    static constexpr int Loop_column_row_TRANSP[2] = {1, 7};
    static constexpr int Loop_column_row_LEVEL[2] = {2, 8};
    static constexpr int Loop_column_row_SOUND[2] = {2, 9};

    static constexpr int Loop_column_row_total_time[2] = {20, 4};
    static constexpr int Loop_column_row_track[TRACKS][2] = {
        {11, 5},
        {18, 5},
        {25, 5},
        {32, 5},
    };
    static constexpr int Loop_column_row_slide[TRACKS][2] = {
        {11, 6},
        {18, 6},
        {25, 6},
        {32, 6},
    };
    static constexpr int Loop_column_row_transpose[TRACKS][2] = {
        {11, 7},
        {18, 7},
        {25, 7},
        {32, 7},
    };
    static constexpr int Loop_column_row_level[TRACKS][2] = {
        {11, 8},
        {18, 8},
        {25, 8},
        {32, 8},
    };

    static constexpr float Loop_column_row_sound_id_0[2] = {6, 9.8};
    static constexpr float Loop_coefficient_row_sound_id = 0.7; // Loop_column_row_sound_id_N[2] = {6, 9.8 + 0.7 * N}

    static constexpr float Loop_column_row_sound_LED_0[2] = {11, 9.8};
    static constexpr float Loop_coefficients_column_row_sound_LED[2] = {7, 0.7};

    int8_t Loop_menu_position_0 = 0;
    int8_t Loop_X_position_menu_0 = 0;
    int8_t Loop_dimension_voice_menu_0 = 0;

public:
    DisplayMidiLoop() {}
    void Loop_show_Loop_page(void);
    void Loop_loop_id(void);
    void Loop_show_midi_loop_title(void);
    void Loop_track_data(int track);
    void Loop_total_time(void);
    void Loop_REC_advice(int track, bool on);
    void Loop_led(int track, int instrument_id, bool on);
    void Loop_led_metronomo(int Xled, int Yled, bool ONled);
    void Loop_menu(void);
    void Loop_show_frame_menu(int position, bool fresh);
    void Loop_Delete_all_frame_menu(void);
};
