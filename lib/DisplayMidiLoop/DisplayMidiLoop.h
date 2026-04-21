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
    static constexpr int Loop_column_row_PATCH[2] = {30, 0};
    static constexpr int Loop_column_row_VOLUME[2] = {41, 0};
    static constexpr int Loop_column_row_METRO[2] = {2, 4};
    static constexpr int Loop_column_row_TRACK[2] = {2, 5};
    static constexpr int Loop_column_row_SLIDE[2] = {2, 6};
    static constexpr int Loop_column_row_TRANSP[2] = {1, 7};
    static constexpr int Loop_column_row_LEVEL[2] = {2, 8};
    static constexpr int Loop_column_row_SOUND[2] = {2, 9};

    static constexpr int Loop_column_row_loop[2] = {22, 0};
    static constexpr int Loop_column_row_patch[2] = {36, 0};
    static constexpr float Loop_column_row_volume[2] = {47.5, 0};

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
    static constexpr int Loop_column_row_pitch[TRACKS][2] = {
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
    
    static const int Loop_chars_total_time = 7;
    static const int Loop_chars_loop = 3;
    static const int Loop_chars_patch = 3;
    static const int Loop_chars_volume = 4;
    static const int Loop_chars_track_number = 1;
    static const int Loop_chars_slide = 6;
    static const int Loop_chars_pitch = 7;
    static const int Loop_chars_level = 6;


public:
    DisplayMidiLoop() {}
    void Loop_show_Loop_page(void);
    void Loop_loop_id(void);
    void Loop_patch_id(void);
    void Loop_volume(void);
    void Loop_show_midi_loop_title(void);
    void Loop_track_data(int track);
    void Loop_total_time(void);
    void Loop_REC_advice(int track, bool on);
    void Loop_led(int track, int instrument_id, bool on);
    void Loop_led_metronomo(int Xled, int Yled, bool ONled);
    void Loop_menu(void);
 
    void Loop_show_frame_menu(int position);
    void Loop_Delete_all_frame_menu(void);

    // pointer
    void Loop_show_pointerMenu(const LOOP_menu_element_name pointer, const bool show);
    void Loop_show_pointerMain(const LOOP_main_value_name pointer, const bool show);
    void Loop_show_pointerTrack(const int track, const LOOP_track_value_name pointer, const bool show);
};
