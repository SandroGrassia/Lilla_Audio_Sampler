/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "SharedElements.h"

// PSRAM timing
elapsedMicros audio_update_time_micros;
volatile uint32_t audio_update_cycle = 0;

// SETUP
int key_step;
int8_t first_octave;

// GESTIONE DELLA MEMORIA FLASH ESTERNA
int verified_flash_memory_MB;

// LILLA STATE
uint8_t Lilla_state;
uint8_t Lilla_state_0;

// PATCH
DMAMEM Patch_struct Patch[PATCHES_MAX + 1]; // Operational metadata in RAM2; initialized explicitly at startup.
DMAMEM Sound_struct Sound[SOUNDS_MAX + 2];

// PERFORMANCE
uint8_t Patch_id = 0;
int volume_patch = 50; // Initial patch volume: gain 0.5 on the nonlinear curve.

float Patch_volume_gain(int volume)
{
    if (volume <= 0)
    {
        return 0.0f;
    }
    if (volume >= PATCH_VOLUME_MAX)
    {
        return 2.0f;
    }
    const int scaled = volume * 40;
    const int index = scaled / PATCH_VOLUME_MAX;
    const float fraction = static_cast<float>(scaled % PATCH_VOLUME_MAX) / PATCH_VOLUME_MAX;
    return Volume_float[index] + (Volume_float[index + 1] - Volume_float[index]) * fraction;
}
uint8_t map_instrument_for_note[16][NOTE_NUMBERS] = {0};
bool key_state[16][NOTE_NUMBERS] = {0}; // usato solo a fini statistici; key premuti su ciascun canale midi; rilevato attaverso il conteggio dei NoteOn
int8_t P_line_of_instrument[INSTRUMENTS];
float pitch_from_note[NOTE_NUMBERS] = {0};
bool display_instrument_volume_flag = false;
uint8_t instrument_volume_changed = 0;



uint8_t P_choice_menu;
bool Menu_P[5];

// Tuning tone
int tuning_tone_volume = 10;
uint8_t tuning_tone_last_note = 0;
bool tuning_tone_flag = false;
bool TT_playing = false;
bool TT_led_flag = false;

// SOUND EDIT
uint8_t trim_speed = 5;
uint16_t Noclick_max;
bool solo_flag = false;
bool slicing_mode = true; // true: slicing AB  - false: slicing A-Samples
const char name_mode[6][8] = {{"FWD"}, {"REV"}, {"FWD"}, {"FWD-REV"}, {"REV-FWD"}, {"REV"}};
char loop_mode[6][5] = {{"once"}, {"once"}, {"loop"}, {"loop"}, {"loop"}, {"loop"}};

// array compilati al setup()
float m_exp_table[10];
float m_sin_table[10];
float m_decay_table[10];
float m_release_table[10];
float pan_gain_L_table[33];
float pan_gain_R_table[33];


// PRESET
Preset_struct Preset[INSTRUMENTS];

// PLAYER
volatile uint32_t audio_player_emergency_stops = 0;

// funzioni
void Update_map_Instrument_for_notes(int from_note, int to_note, int instrument_id) // aggiorna la mappatura tra tutte Instrument e le coppie midi_channel/note_number e relative
{
    for (auto note = 0; note < NOTE_NUMBERS; ++note)
    {
        if (note >= from_note && note <= to_note)
        {
            bitWrite(map_instrument_for_note[Get_midi_channel(Patch_id, instrument_id)][note], instrument_id, 1);
        }
        else
        {
            bitWrite(map_instrument_for_note[Get_midi_channel(Patch_id, instrument_id)][note], instrument_id, 0);
        }
    }
}

void Reset_keys_state()
{
    for (auto a = 0; a < 16; ++a)
    {
        for (auto b = 0; b < NOTE_NUMBERS; ++b)
        {
            key_state[a][b] = false; // true: key pressed down
        }
    }
}

// LPF FILTER RESOLUTION DOWNSAMPLING
bool lowpass_flag;
bool lowpass_direction;
int lowpass;
int lowpass_target;
bool display_lowpass_flag;
int resolution;   // [0, 79] 0: risoluzione 16bit
int downsampling; // n. of repeated samples  1 = 44.1ksps

// PITCH BEND - AFTER TOUCH - VIBRATO
float pitch_bend_value[16] = {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
float after_touch_channel_value[16] = {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};

// CONTROL CHANGE
uint8_t CC_Sound_gain[INSTRUMENTS] = {0};
uint8_t CC_lowpass_filter_value;
uint8_t CC_midi_controller;

// STAMPA
void PRINT(String who, String what, float value)
{
    Serial.print(who);
    Serial.print(" ");
    Serial.print(what);
    Serial.print(": ");
    Serial.println(value);
}
