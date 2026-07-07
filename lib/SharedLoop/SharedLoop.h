/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#pragma once

#include <Arduino.h>
#include "config.h"

static constexpr int NEW_LOOP = -1;
static constexpr int NO_TRACK = -1; // track virtuale per note suonate da tastiera nello stato MIDI_LOOP
static constexpr int MASTER_TRACK = 0;
static constexpr int LOOP_EVENTS = 40; // numero massimo di eventi in un track
static constexpr int LOOP_UI_A = 1;    // primo encoder prima fila
static constexpr int LOOP_UI_B = 9;    // primo encoder seconda fila
static constexpr int LOOP_UI_C = 17;   // primo encoder terza fila
static constexpr int LOOP_metro_leds = 4; // 4 = 4/4
static constexpr int MIDI_LOOP_FILES = 1000;

struct LOOP_struct // verificata 12 byte 
{
    int time;    // ms
    uint8_t midi_channel;
    uint8_t note_number;
    uint8_t velocity;
    bool note_on;
};
extern LOOP_struct LOOP_element[TRACKS][LOOP_EVENTS];
extern byte LOOP_events[TRACKS];
extern int LOOP_slide[TRACKS]; // slittamento temporale in ms
extern int LOOP_pitch_int[TRACKS]; // slittamento pitch -400....0....+400
extern float LOOP_stretch; // stretch comune a tutti i track

// LOOP play/stop
extern bool LOOP_track_run[TRACKS]; // se "true" il track e' in esecuzione
extern int LOOP_play_event[TRACKS]; // indice del prossimo evento da eseguire
extern uint32_t LOOP_play_time[TRACKS]; // (ms) istante di esecuzione del prossimo evento da eseguire rispetto a LOOP_Clock
extern float LOOP_volume[TRACKS];
extern uint16_t LOOP_time; // (ms) durata del track master (0) comune a tutti i track

// LOOP learn
extern int LOOP_learning_track;
extern bool LOOP_learn_flag;
extern int LOOP_elements;
extern elapsedMillis LOOP_learn_clock; // utilizzato per calcolare la durata di track learn
extern int LOOP_clock_memo;
extern int LOOP_last_event;

// Metronomo
extern bool LOOP_metronomo_run; // se "true" i led del metronomo sono visualizzati

// Metronomo, richieste da MidiReader a Main
extern bool LOOP_metronomo_flag_IN[2]; // accendi led_0, switch led del metronomo

// Menu
static constexpr int LOOP_menu_values = 4;
extern int LOOP_menu_max;
extern const char Menu_LOOP_char[LOOP_menu_values][12];
extern const uint8_t dimension_voice_Menu_LOOP[LOOP_menu_values];
extern uint8_t X_position_Menu_LOOP[LOOP_menu_values]; // argument is position
extern bool Menu_LOOP[LOOP_menu_values];
extern uint8_t element_Menu_LOOP[LOOP_menu_values]; // argument is position
extern uint8_t position_Menu_LOOP[LOOP_menu_values]; // argument is element

// Functions
unsigned long LOOP_Clock_time_from_virtual_time(int T_evento); // definita in main.cpp
unsigned long LOOP_Clock(void); // definita in main.cpp

// Save to SD
extern int LOOP_id; // loop_id actually displayed

// Pointer
enum LOOP_field_name
{
    field_LOOP_Menu,
    field_LOOP_TrackValues
};

enum LOOP_menu_element_name : int
{
    value_LOOP_Menu_none = -1,
    value_LOOP_New = 0,
    value_LOOP_Save = 1,
    value_LOOP_SaveAsNew = 2,
    value_LOOP_Delete = 3
};

static constexpr int LOOP_track_values = 3;
enum LOOP_track_value_name : int
{
    value_LOOP_Track_none = -1,
    value_LOOP_shift = 0,
    value_LOOP_pitch = 1,
    value_LOOP_level = 2
};

struct LOOP_field_description_struct
{
LOOP_field_name field_name;
LOOP_menu_element_name menu_element;
LOOP_track_value_name track_value_element;
};