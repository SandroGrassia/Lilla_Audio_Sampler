/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>     // https://learn.adafruit.com/adafruit-gfx-graphics-library/graphics-primitives
#include <Adafruit_ILI9341.h> // 1.5.12 version - Hardware-specific library
#include "DisplayPrimitives.h"
#include <AudioStream.h> // solo per definizione AUDIO_SAMPLE_RATE
#include "SharedLS.h"
#include "SharedMixer.h"
#include "InfoMaster.h"

class DisplayLiveSampler
{
private:
    InfoMaster &Info;

    float LS_K_wave_color;
    float LS_wave_poit_distance_0 = 0;
    int LS_window_A_sample;
    int LS_window_B_sample;

    elapsedMillis LS_blink_timer;
    bool LS_blink_ON = false;
    
    // canvas top-left corner coordinates 
    static constexpr int LS_CANVAS_X = 5;
    static constexpr int LS_CANVAS_Y = 132;

    static constexpr uint16_t LS_WAVE_COLOR = 0xE08A;
    static constexpr uint16_t LS_WAVE_ZERO_COLOR = 0x7BCF;
    static constexpr uint16_t LS_WAVE_BOARD = 0xFE40;     // 0xA514
    static constexpr uint16_t LS_X_COLOR = ILI9341_GREEN; // Live Sampling linee verticali di esecuzione
    static constexpr uint16_t LS_Y_COLOR = ILI9341_WHITE; // Live Sampling linee verticali di esecuzione

    void Draw_XY_lines(void);
    void Delete_menu_frames(void);
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
    static constexpr float LS_column_row_LOOP[2] = {35, LS_ROW_VALUES + 3};
    static constexpr float LS_column_row_STEP[2] = {0, LS_ROW_VALUES + 4};
    
    static constexpr float LS_column_row_buffer[2] = {27, 0};
    static constexpr float LS_column_row_volume[2] = {47.5, 0};

    static constexpr float LS_column_row_play_mode[2] = {9.5, LS_ROW_VALUES};
    static constexpr float LS_column_row_feedback[2] = {8.5, LS_ROW_VALUES + 1};
    static constexpr float LS_column_row_window[2] = {6.5, LS_ROW_VALUES + 2};

    static constexpr float LS_column_row_loop_time_mode_2[2] = {23, LS_ROW_VALUES};
    static constexpr float LS_column_row_loop_time_mode_3[2] = {23, LS_ROW_VALUES};

    static constexpr float LS_column_row_start_point[2] = {12, LS_ROW_VALUES + 3};
    static constexpr float LS_column_row_loop_time[2] = {39.5, LS_ROW_VALUES + 3};
    static constexpr float LS_column_row_step[2] = {6.5, LS_ROW_VALUES + 4};   

    static constexpr float LS_column_row_element[LS_value_names][2] = {
        {LS_column_row_play_mode[0], LS_column_row_play_mode[1]},
        {LS_column_row_feedback[0], LS_column_row_feedback[1]},
        {LS_column_row_window[0], LS_column_row_window[1]}
    };

    static constexpr int LS_chars_play_mode = 12;
    static constexpr int LS_chars_feedback = 5;
    static constexpr int LS_chars_window = 8;

    static constexpr int LS_chars_loop_time = 12;
    
    static constexpr int LS_chars_start_point = 15;
    static constexpr int LS_chars_step = 16;

    static constexpr int LS_chars_element[LS_value_names] = {LS_chars_play_mode, LS_chars_feedback, LS_chars_window};

    // elapsedMicros localtimer;
    //  int memo[2];

public:
    DisplayLiveSampler(InfoMaster &Obj) : Info(Obj) {}

    // Gestione LED
    void Led_LIVE_SAMPLING(bool on);

    void Confirm_EXIT_from_LS(void);
    void Page(void);
    void Feedback(void);
    void Step(void);
    void Buffer(void);
    void Volume(void);
    void Play_mode(void);
    void Window(void);
    void Loop_time(void);
    void Start_point(void);
    void Menu(void);
    void Menu_frame(const int position);
    void Show_wave(const int sound_id);
    uint16_t Get_wave_color(const int point);
    void Update_REC_LED(void);

    // pointer
    void LS_show_pointer_frame(const LS_pointer_struct pointer, const bool show);
};