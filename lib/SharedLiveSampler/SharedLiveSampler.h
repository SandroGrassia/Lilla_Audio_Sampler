/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#pragma once

#include <Arduino.h>
#include "config.h"
#include "SharedElements.h"


// Virtual tape
/*
Dalla PSRAM totale si esclude spazio per i puntatori; lo spazio utile deve poter essere divisibile per  2 (L/R) poi per 256 (128 samples)
PSRAM_16MB --> 16777216 byte; escludiamo spazio per i puntatori (4*AUDIO_BLOCK_SAMPLES) --> 16776704 byte pari a 190sec; dedichiamo 1724*256=441344 byte (circa 5sec) per ciascun canale del Delay, restano 15894016 byte per Live Sampling
*/
static constexpr int LS_MONO_SAMPLES = 7946752;
static constexpr uint32_t LS_MONO_BYTES = LS_MONO_SAMPLES << 1; // 0xf28400 - 15.893.504
static constexpr int LS_STEREO_SAMPLES = 3973376; 
static constexpr uint32_t LS_STEREO_BYTES = LS_STEREO_SAMPLES << 1; // 0x794200 - decimale 7.946.752
static constexpr int FIRST_LIVE_SAMPLING_FILE = 320;

// Menu
constexpr int LS_menu_elements = 4;
extern bool Menu_LS[LS_menu_elements];
extern const char Menu_LS_char[LS_menu_elements][12];
extern const uint8_t dimension_voice_Menu_LS[LS_menu_elements];
extern uint8_t X_position_Menu_LS[LS_menu_elements]; // argument is position
extern uint8_t element_Menu_LS[LS_menu_elements];    // argument is position
extern uint8_t position_Menu_LS[LS_menu_elements];   // argument is element
extern int LS_menu_max;

// variabili
constexpr float LS_fbk_table[9] = {0, 0.01, 0.03, 0.07, 0.1, 0.2, 0.4, 0.8, 0.9};
enum LS_States
{
    EMPTY,
    REC,
    PLAYONLY
};
extern LS_States LS_state;
extern int LS_feedback; // feedback interno al Live Sampler
extern bool LS_stereo;
extern int LS_buffer_dim;


/*
    enum LillaPlayModes
    {
        ONCE_FWD,     // 0
        ONCE_REV,     // 1
        LOOP_FWD,     // 2
        LOOP_FWD_REV, // 3
        LOOP_REV_FWD, // 4
        LOOP_REV      // 5
    };
*/
extern LillaPlayModes LS_mode;
extern int LS_window_width; // samples from LS_window_A_sample to LS_window_B_sample
extern int LS_Q_sample; // ultima posizione registrata su LS_buffer_L/R
extern bool LS_XY_lock; // play bloccato sul virtual tape
extern int LS_X_step; // step di avanzamento
extern int LS_X_delta; // distanza tra LS_Q_sample e LS_X_sample
extern int LS_X_sample; // posizione di partenza play
extern int LS_Y_sample; // posizione di fine loop 
extern int LS_XY_delta; // distanza tra LS_X_sample e LS_Y_sample

// waveform
int LS_constrain_position(int value);

// pointer
enum LS_field_name
{
    field_LS_Menu,
    field_LS_Value
};
// constexpr int LS_menu_elements = 4;
enum LS_menu_element_name
{
    value_LS_Recording,
    value_LS_Stop,
    value_LS_MonoStereo,
    value_LS_Erase
};
static constexpr int LS_value_names = 3;
enum LS_value_name
{
    value_LS_Play_mode,
    value_LS_Feedback,
    value_LS_Window
};

struct LS_pointer_struct
{
LS_field_name field_name;
LS_menu_element_name menu_element;
LS_value_name value_element;
};

