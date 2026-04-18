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
    static constexpr int Loop_HEAD_R = 5;
    static constexpr int Loop_HEAD_C = 3;
    static constexpr int Loop_LOOPS_X = 11;
    static constexpr int Loop_LOOP_TIME = 20;
    int8_t Loop_menu_position_0 = 0;
    int8_t Loop_X_position_menu_0 = 0;
    int8_t Loop_dimension_voice_menu_0 = 0;

public:
    DisplayMidiLoop() {}

    static constexpr int Loop_LED_Y = 151;
    static constexpr int Loop_LED_X = 70;
    static constexpr int Loop_LED_DY = 11;
    void Loop_show_Loop_page(void);
    void Loop_loop_id(void);
    void Loop_show_midi_loop_title(void);
    void Loop_track_data(int track);
    void Loop_time_stretched(void);
    void Loop_REC_advice(int track, bool on);
    void Loop_led(int track, int instrument_id, bool on);
    void Loop_led_metronomo(int Xled, int Yled, bool ONled);
    void Loop_menu(void);
    void Loop_show_frame_menu(int position, bool fresh);
    void Loop_Delete_all_frame_menu(void);
};
