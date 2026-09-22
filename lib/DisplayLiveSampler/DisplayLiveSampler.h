/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>     // https://learn.adafruit.com/adafruit-gfx-graphics-library/graphics-primitives
#include <ILI9341_t3n.h>
#include "DisplayPrimitives.h"
#include <AudioStream.h> // solo per definizione AUDIO_SAMPLE_RATE
#include "SharedLiveSampler.h"
#include "SharedMixer.h"
#include "GlobalInfoMaster.h"

// Handles all display rendering for the Live Sampler page on the ILI9341 TFT.
// Responsibilities:
//   - Full page layout: title, parameter labels and values, menu row.
//   - Waveform canvas: draws min/max sample envelopes with colour-coded age gradient, X/Y playback markers, REC LED blink, and stereo label.
//   - Pointer frame: draws or erases a highlight rectangle around the currently selected menu item or parameter value.
//   - LED indicator: reflects mute state and recording state.

class DisplayLiveSampler
{
private:
    float LS_K_wave_color;             // samples-per-pixel ratio for the waveform canvas
    float LS_wave_poit_distance_0 = 0; // previous pixel distance from write head, used for colour gradient
    int LS_window_A_sample;            // first sample index of the visible window
    int LS_window_B_sample;            // last  sample index of the visible window

    elapsedMillis LS_blink_timer; // drives the 500 ms REC LED blink period
    bool LS_blink_ON = false;     // current blink phase

    // canvas top-left corner coordinates on the TFT
    static constexpr int LS_CANVAS_X = 5;
    static constexpr int LS_CANVAS_Y = 132;

    static constexpr uint16_t LS_WAVE_COLOR = 0xE08A;      // default waveform colour
    static constexpr uint16_t LS_WAVE_ZERO_COLOR = 0x7BCF; // colour used when sample value is zero
    static constexpr uint16_t LS_WAVE_BOARD = 0xFE40;      // canvas background colour
    static constexpr uint16_t LS_X_COLOR = ILI9341_GREEN;  // X (play) position marker
    static constexpr uint16_t LS_Y_COLOR = ILI9341_WHITE;  // Y (loop end) position marker

    // Draws the X (play point) and Y (loop end) vertical lines on the canvas.
    void Draw_XY_lines(void);

    // Erases all menu item highlight frames.
    void Delete_menu_frames(void);

    // Renders the "LIVE SAMPLER" title bar with red background.
    void Page_title(void);

    // pointer
    static constexpr float LS_ROW_MENU = 1;
    static constexpr float LS_ROW_VALUES = 2.5;

    static constexpr float LS_column_row_BUFFER[2] = {20, 0};
    static constexpr float LS_column_row_VOLUME[2] = {41, 0};

    static constexpr float LS_column_row_PLAY_MODE[2] = {0, LS_ROW_VALUES};
    static constexpr float LS_column_row_FEEDBACK[2] = {0, LS_ROW_VALUES + 1};
    static constexpr float LS_column_row_WINDOW[2] = {0, LS_ROW_VALUES + 2};

    static constexpr float LS_column_row_START_POINT[2] = {0, LS_ROW_VALUES + 3};
    static constexpr float LS_column_row_LOOP[2] = {29, LS_ROW_VALUES + 3};
    static constexpr float LS_column_row_STEP[2] = {0, LS_ROW_VALUES + 4};

    static constexpr float LS_column_row_buffer[2] = {27, 0};
    static constexpr float LS_column_row_volume[2] = {48, 0};

    static constexpr float LS_column_row_play_mode[2] = {10, LS_ROW_VALUES};
    static constexpr float LS_column_row_feedback[2] = {9, LS_ROW_VALUES + 1};
    static constexpr float LS_column_row_window[2] = {7, LS_ROW_VALUES + 2};

    static constexpr float LS_column_row_loop_time_mode_2[2] = {23, LS_ROW_VALUES};
    static constexpr float LS_column_row_loop_time_mode_3[2] = {23, LS_ROW_VALUES};

    static constexpr float LS_column_row_start_point[2] = {12, LS_ROW_VALUES + 3};
    static constexpr float LS_column_row_loop_time[2] = {34, LS_ROW_VALUES + 3};
    static constexpr float LS_column_row_step[2] = {5, LS_ROW_VALUES + 4};

    static constexpr float LS_column_row_element[LS_value_names][2] = {
        {LS_column_row_play_mode[0], LS_column_row_play_mode[1]},
        {LS_column_row_feedback[0], LS_column_row_feedback[1]},
        {LS_column_row_window[0], LS_column_row_window[1]}};

    int LS_chars_play_mode;
    int LS_chars_feedback;
    int LS_chars_window;

    static constexpr int LS_chars_loop_time = 9;
    static constexpr int LS_chars_start_point = 15;
    static constexpr int LS_chars_step = 16;

    // elapsedMicros localtimer;
    //  int memo[2];

public:
    DisplayLiveSampler() {}

    // Turns the LIVE SAMPLING LED on/off. Colour reflects mute state (green = active, red = muted).
    void Led_LIVE_SAMPLING(bool on);

    // Draws the "STOP RECORDING?" confirmation popup.
    void Confirm_EXIT_from_LS(void);

    // Renders the full Live Sampler page (title, all labels and values).
    void Page(void);

    // Redraws the Feedback parameter value.
    void Feedback(void);

    // Redraws the Step parameter value (in samples).
    void Step(void);

    // Redraws the Buffer size value (in seconds).
    void Buffer(void);

    // Redraws the Volume value.
    void Volume(void);

    // Redraws the Play Mode value (mode name, optional loop prefix).
    void Play_mode(void);

    // Redraws the Window width value (in seconds).
    void Window(void);

    // Redraws the Loop Time value; shows "--" when looping is inactive.
    void Loop_time(void);

    // Redraws the Start Point value, expressing it as DELAY or ADVANCE relative to the current write head, or as FIXED when XY lock is on.
    void Start_point(void);

    // Redraws the entire menu row, recalculating each visible item's pixel position and updating element_Menu_LS / position_Menu_LS tables.
    void Menu(void);

    // Draws a highlight frame around the menu item at the given display position.
    void Menu_frame(const int position);

    // Renders the waveform canvas for the given sound channel (min/max envelope, colour gradient by buffer age, X/Y markers, REC LED, stereo label), then blits the canvas to the TFT.
    void Show_wave(const int sound_id);

    // Returns the RGB565 colour for a canvas pixel at column `point',
    // encoding buffer age as a green channel gradient.
    uint16_t Get_wave_color(const int point);

    // Redraws the REC LED blink state on the canvas (500 ms period).
    void Update_REC_LED(void);

    // Draws or erases the pointer highlight frame at the position described
    // by `pointer` (either a menu item or a parameter value cell).
    void LS_show_pointer_frame(const LS_pointer_struct pointer, const bool show);
};