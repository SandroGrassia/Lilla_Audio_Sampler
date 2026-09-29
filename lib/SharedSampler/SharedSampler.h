/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#pragma once

#include <Arduino.h>

// DIRECT SAMPLING

// VU-meter
static constexpr int BAR_ELEMENTS = 50; // barre del VU-meter stereo

// Menu
static constexpr int DS_menu_elements = 12; // elementi di Menu_DS[]
extern bool Menu_DS[DS_menu_elements];
extern const char Menu_DS_char[DS_menu_elements][19];
extern const uint8_t dimension_voice_Menu_DS[DS_menu_elements]; // dimensione degli elementi di Menu_DS[] 
extern uint8_t X_position_Menu_DS[DS_menu_elements]; 
extern uint8_t Y_position_Menu_DS[DS_menu_elements]; 
extern uint8_t element_Menu_DS[DS_menu_elements]; 
extern uint8_t position_Menu_DS[DS_menu_elements];
extern uint8_t choice_DS_menu;
extern int DS_menu_max;

extern int recordings;
extern int recording;
extern elapsedMillis DS_blink_timer;
extern bool DS_blink_ON;
extern bool DS_recording_led_visible; // Whether the current description reserves space for the playback LED.
extern bool DS_recording_led_redraw; // Request a refresh after the recording description is drawn.

// Pointer
enum DS_field_name
{
    field_DS_Menu,
    field_DS_Value
};

enum DS_menu_element_name
{
    value_DS_CancelRecording,
    value_DS_PauseRec,
    value_DS_MonoRec,
    value_DS_StereoRec,
    value_DS_Stop,
    value_DS_MakeRaw,
    value_DS_Cancel,
    value_DS_MakeMono,
    value_DS_MakeLeft,
    value_DS_MakeRight,
    value_DS_MakeBoth,
    value_DS_ExportWavToSD
};

static constexpr int DS_value_names = 2;
enum DS_value_name
{
    value_DS_Recording,
    value_DS_Gain
};

struct DS_pointer_struct
{
DS_field_name field_name;
DS_menu_element_name menu_element;
DS_value_name value_element;
};
