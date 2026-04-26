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
    static constexpr int Loop_column_row_SHIFT[2] = {2, 6};
    static constexpr int Loop_column_row_TRANSP[2] = {1, 7};
    static constexpr int Loop_column_row_LEVEL[2] = {2, 8};
    static constexpr int Loop_column_row_SOUND[2] = {2, 9};

    static constexpr int Loop_column_row_loop[2] = {23, 0};
    static constexpr int Loop_column_row_patch[2] = {36, 0};
    static constexpr float Loop_column_row_volume[2] = {47.5, 0};
    static constexpr int Loop_column_row_total_time[2] = {20, 4};

    static constexpr int Loop_column_distance = 10;

    static constexpr int Loop_column_row_track[TRACKS][2] = {
        {11, 5},
        {11 + Loop_column_distance, 5},
        {11 + 2 * Loop_column_distance, 5},
        {11 + 3 * Loop_column_distance, 5},
    };
    static constexpr int Loop_column_row_slide[TRACKS][2] = {
        {11, 6},
        {11 + Loop_column_distance, 6},
        {11 + 2 * Loop_column_distance, 6},
        {11 + 3 * Loop_column_distance, 6},
    };
    static constexpr int Loop_column_row_pitch[TRACKS][2] = {
        {11, 7},
        {11 + Loop_column_distance, 7},
        {11 + 2 * Loop_column_distance, 7},
        {11 + 3 * Loop_column_distance, 7},
    };
    static constexpr int Loop_column_row_level[TRACKS][2] = {
        {11, 8},
        {11 + Loop_column_distance, 8},
        {11 + 2 * Loop_column_distance, 8},
        {11 + 3 * Loop_column_distance, 8},
    };

    static constexpr float Loop_column_row_sound_id_0[2] = {6, 9.8};
    static constexpr float Loop_coefficient_row_sound_id = 0.7; // Loop_column_row_sound_id_N[2] = {6, 9.8 + 0.7 * N}

    static constexpr float Loop_column_row_sound_LED_0[2] = {11, 9.8};
    static constexpr float Loop_coefficients_column_row_sound_LED[2] = {Loop_column_distance, 0.7};

    static const int Loop_chars_total_time = 7;
    static const int Loop_chars_loop = 3;
    static const int Loop_chars_patch = 3;
    static const int Loop_chars_volume = 4;
    static const int Loop_chars_track_number = 5;
    static const int Loop_chars_shift = 6;
    static const int Loop_chars_transpose = 6;
    static const int Loop_chars_level = 3;

public:
    DisplayMidiLoop() {}
    void Show_Loop_page(void);
    void Show_loop_id(void);
    void Show_patch_id(void);
    void Show_volume(void);
    void Show_MIDI_LOOP(void);
    void Show_track_all_data(const int track);
    void Loop_total_time(void);
    void Loop_REC_advice(const int track, const bool on);
    void Loop_led(const int track, const int instrument_id, const bool on);
    void Loop_led_metronomo(const int Xled, const int Yled, const bool ONled);
    void Show_menu(void);

    // pointer
    void Loop_show_pointerMenu(const LOOP_menu_element_name pointer, const bool show);
    void Loop_show_pointerTrack(const int track, const LOOP_track_value_name pointer, const bool show);
};
