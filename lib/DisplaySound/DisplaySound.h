/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>     // https://learn.adafruit.com/adafruit-gfx-graphics-library/graphics-primitives
#include <Adafruit_ILI9341.h> // 1.5.12 version - Hardware-specific library
#include <AudioStream.h>      // pick definition of AUDIO_SAMPLE_RATE
#include "SharedElements.h"
#include "DisplayPrimitives.h"
#include "GlobalDisplayManager.h"
#include "GlobalDisplayMidiLoop.h"
#include "GlobalInfoMaster.h"
#include "SharedSound.h"

class DisplaySound
{
private:
    // Screen coordinates for each SOUND page value field, expressed in character columns/rows.
    static constexpr float S_column_row_File[2] = {43, 0};
    static constexpr float S_column_row_Midi[2] = {12.5, 4.9};
    static constexpr float S_column_row_Pitch[2] = {23.5, 4.9};
    static constexpr float S_column_row_Gain[2] = {35.5, 4.9};
    static constexpr float S_column_row_Pan[2] = {45.5, 4.9};
    static constexpr float S_column_row_Attack[2] = {3.5, 5.9};
    static constexpr float S_column_row_Decay[2] = {21.5, 5.9};
    static constexpr float S_column_row_Sustain[2] = {34.5, 5.9};
    static constexpr float S_column_row_Release[2] = {45.5, 5.9};
    static constexpr float S_column_row_PlayMode[2] = {9.5, 6.9};
    static constexpr float S_column_row_NoClick[2] = {38.5, 6.9};

    // Lookup table used by pointer/highlight rendering for SOUND value fields.
    static constexpr float S_column_row_value_element[S_value_names][2] =
        {
            {S_column_row_File[0], S_column_row_File[1]},
            {S_column_row_Midi[0], S_column_row_Midi[1]},
            {S_column_row_Pitch[0], S_column_row_Pitch[1]},
            {S_column_row_Gain[0], S_column_row_Gain[1]},
            {S_column_row_Pan[0], S_column_row_Pan[1]},
            {S_column_row_Attack[0], S_column_row_Attack[1]},
            {S_column_row_Decay[0], S_column_row_Decay[1]},
            {S_column_row_Sustain[0], S_column_row_Sustain[1]},
            {S_column_row_Release[0], S_column_row_Release[1]},
            {S_column_row_PlayMode[0], S_column_row_PlayMode[1]},
            {S_column_row_NoClick[0], S_column_row_NoClick[1]}};

    // Character widths used when clearing and redrawing each value field.
    static constexpr int S_chars_File = 7;
    static constexpr int S_chars_Midi = 2;
    static constexpr int S_chars_Pitch = 6;
    static constexpr int S_chars_Gain = 4;
    static constexpr int S_chars_Pan = 2;
    static constexpr int S_chars_Attack = 10;
    static constexpr int S_chars_Decay = 7;
    static constexpr int S_chars_Sustain = 4;
    static constexpr int S_chars_Release = 6;
    static constexpr int S_chars_PlayMode = 13;
    static constexpr int S_chars_NoClick = 4;

    static constexpr int S_chars_value_element[S_value_names] = {
        S_chars_File,
        S_chars_Midi,
        S_chars_Pitch,
        S_chars_Gain,
        S_chars_Pan,
        S_chars_Attack,
        S_chars_Decay,
        S_chars_Sustain,
        S_chars_Release,
        S_chars_PlayMode,
        S_chars_NoClick};

    // Resolves the Sound_id assigned to the selected patch instrument.
    int Sound_Id(int patch_id, int instrument_id);

public:
    DisplaySound() {}

    void S_show_SOUND_page(int patch_id, int instrument_id);                      // Draws the complete SOUND page for the selected instrument.
    void S_show_pointer_frame(S_field_description_struct description, bool show); // Shows or hides the selection frame requested by PointerSound.
    void S_show_SOUND_menu(void);                                                 // Rebuilds the SOUND menu row using the currently enabled entries.
    void S_show_menu_frame(int position);                                         // Highlights the active menu item.
    void S_Delete_all_menu_frame(void);                                           // Clears every SOUND menu highlight frame.
    void S_show_wave(int instrument_id);                                          // Renders the waveform preview and trim information.

    void S_show_Attack_value(int instrument_id);  // Prints attack mode and time.
    void S_show_Decay_value(int instrument_id);   // Prints decay time.
    void S_show_Sustain_value(int instrument_id); // Prints sustain level as a percentage.
    void S_show_Release_value(int instrument_id); // Prints release time.

    void S_show_File_value(int instrument_id);                // Prints the File name assigned to the instrument.
    void S_show_Midi_channel_value(int instrument_id);        // Prints the MIDI channel.
    void S_show_Pitch_value(int instrument_id);               // Prints pitch transposition.
    void S_show_Gain_value(int patch_id, int instrument_id);  // Prints playback gain for the routed sound.
    void S_show_Pan_value(int instrument_id);                 // Prints pan direction and amount.
    void S_show_Play_mode_value(int instrument_id);           // Prints loop/playback mode.
    void S_show_Noclick_value(int instrument_id, bool value); // Prints the no-click smoothing amount.
    void S_show_Trim_step_value(void);                        // Prints the current trim encoder step.
    void S_show_players_Pitch_max_value(int instrument_id);   // Prints max pitch and available voices for the selected media.
};