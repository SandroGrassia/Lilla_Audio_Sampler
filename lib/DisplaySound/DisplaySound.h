/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>     // https://learn.adafruit.com/adafruit-gfx-graphics-library/graphics-primitives
#include <ILI9341_t3n.h>
#include <AudioStream.h>      // pick definition of AUDIO_SAMPLE_RATE
#include "SharedElements.h"
#include "DisplayPrimitives.h"
#include "GlobalDisplayCommon.h"
#include "GlobalDisplayMidiLoop.h"
#include "GlobalInfoMaster.h"
#include "SharedSound.h"

class DisplaySound
{
private:
    int menu_first_visible = 0;
    int source_notice_file = -1; // Source identity associated with the last notice.
    uint8_t source_notice_reason = 0; // Distinguish missing media, empty libraries, missing files and load errors.
    uint32_t source_notice_started = 0; // Nonblocking notice start time in milliseconds.
    bool source_notice_visible = false; // Track whether the waveform contains a timed notice.
    // Screen coordinates for each SOUND page value field, expressed in character columns/rows.
    static constexpr float S_column_row_Patch[2] = {32, 0};
    static constexpr float S_column_row_Sound[2] = {22.5, 0};
    static constexpr float S_column_row_File[2] = {41, 0};
    static constexpr float S_column_row_Midi[2] = {49, 4.9};
    static constexpr float S_column_row_Pitch[2] = {23.5, 4.9};
    static constexpr float S_column_row_Gain[2] = {4.5, 4.9};
    static constexpr float S_column_row_Pan[2] = {34.5, 4.9};
    static constexpr float S_column_row_Attack[2] = {3.5, 5.9};
    static constexpr float S_column_row_Decay[2] = {21.5, 5.9};
    static constexpr float S_column_row_Sustain[2] = {34.5, 5.9};
    static constexpr float S_column_row_Release[2] = {45, 5.9};
    static constexpr float S_column_row_PlayMode[2] = {9.5, 6.9};
    static constexpr float S_column_row_NoClick[2] = {38.5, 6.9};

    // Lookup table used by pointer/highlight rendering for SOUND value fields.
    static constexpr float S_column_row_value_element[S_value_names][2] =
        {
            {S_column_row_File[0], S_column_row_File[1]},
            {S_column_row_Gain[0], S_column_row_Gain[1]},
            {S_column_row_Pitch[0], S_column_row_Pitch[1]},
            {S_column_row_Pan[0], S_column_row_Pan[1]},
            {S_column_row_Midi[0], S_column_row_Midi[1]},
            {S_column_row_Attack[0], S_column_row_Attack[1]},
            {S_column_row_Decay[0], S_column_row_Decay[1]},
            {S_column_row_Sustain[0], S_column_row_Sustain[1]},
            {S_column_row_Release[0], S_column_row_Release[1]},
            {S_column_row_PlayMode[0], S_column_row_PlayMode[1]},
            {S_column_row_NoClick[0], S_column_row_NoClick[1]}};

    // Character widths used when clearing and redrawing each value field.
    static constexpr int S_chars_File = 10;
    static constexpr int S_chars_Midi = 2;
    static constexpr int S_chars_Pitch = 5;
    static constexpr int S_chars_Gain = 4;
    static constexpr int S_chars_Pan = 3;
    static constexpr int S_chars_Attack = 12;
    static constexpr int S_chars_Decay = 8;
    static constexpr int S_chars_Sustain = 4;
    static constexpr int S_chars_Release = 7;
    static constexpr int S_chars_PlayMode = 12;
    static constexpr int S_chars_NoClick = 4;

    static constexpr int S_chars_value_element[S_value_names] = {
        S_chars_File,
        S_chars_Gain,
        S_chars_Pitch,
        S_chars_Pan,
        S_chars_Midi,
        S_chars_Attack,
        S_chars_Decay,
        S_chars_Sustain,
        S_chars_Release,
        S_chars_PlayMode,
        S_chars_NoClick};


public:
    void Led_SOUND_EDIT_instrument(int instrument_id, bool on);
    DisplaySound() {}

    FLASHMEM void Show_SOUND_page(int patch_id, int instrument_id);                      // Draws the complete SOUND page for the selected instrument.
    FLASHMEM void Show_pointer_frame(S_field_description_struct description, bool show); // Shows or hides the selection frame requested by PointerSound.
    FLASHMEM void Show_SOUND_menu(void);                                                 // Rebuilds the SOUND menu row using the currently enabled entries.
    void Show_wave(int instrument_id, bool show_notice = true);                                          // Renders the waveform preview and trim information.

    FLASHMEM void Show_source_notice(int file, bool restart = true); // Explain how to restore a source and start a 4.5-second notice.
    FLASHMEM void Show_read_sd_notice(); // Request SD catalog preparation with a nonblocking 4.5-second popup.
    void Update_source_notice(int instrument_id); // Restore the waveform after the notice expires without blocking input.

    FLASHMEM void Show_Attack_value(int instrument_id);  // Prints attack mode and time.
    FLASHMEM void Show_Decay_value(int instrument_id);   // Prints decay time.
    FLASHMEM void Show_Sustain_value(int instrument_id); // Prints sustain level as a percentage.
    FLASHMEM void Show_Release_value(int instrument_id); // Prints release time.

    FLASHMEM void Show_File_value(int instrument_id);                // Prints the File name assigned to the instrument.
    FLASHMEM void Show_Midi_channel_value(int instrument_id);        // Prints the MIDI channel.
    FLASHMEM void Show_Pitch_value(int instrument_id);               // Prints pitch transposition.
    FLASHMEM void Show_Gain_value(int patch_id, int instrument_id);  // Prints playback gain for the routed sound.
    FLASHMEM void Show_Pan_value(int instrument_id);                 // Prints pan direction and amount.
    FLASHMEM void Show_Play_mode_value(int instrument_id);           // Prints loop/playback mode.
    FLASHMEM void Show_Noclick_value(int instrument_id, bool value); // Prints the no-click smoothing amount.
    FLASHMEM void Show_Trim_step_value(void);                        // Prints the current trim encoder step.
    FLASHMEM void Show_players_Pitch_max_value(int instrument_id);   // Prints max pitch and available voices for the selected media.
};
