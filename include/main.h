/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 */

#pragma once

#include <Arduino.h>
#include <type_traits>
#include "Encoders.h"
#include "SharedSampler.h"
#include "SharedSound.h"
#include "SharedPerformance.h"

struct PatchEditSnapshot
{
    const int patch_id = Patch_id;
    Patch_struct patch;
    struct SavedSound
    {
        int sound_id;
        Sound_struct value;
    };

    // Teensy 4.1 allocates the heap in RAM2; only this small owner lives on the stack.
    SavedSound *sounds = nullptr;
    size_t count = 0;
    size_t capacity = 0;
    bool valid = true;
    PatchEditSnapshot(const PatchEditSnapshot &) = delete;
    PatchEditSnapshot &operator=(const PatchEditSnapshot &) = delete;
    PatchEditSnapshot(void); // Capture the editable model while preserving the caller's audio IRQ state.
    ~PatchEditSnapshot(void);
    bool Capture_sound(int sound_id);
    const Sound_struct *Find_sound(int sound_id) const;
    void Restore(void) const; // Restore the model and note maps after a failed preparation; published presets remain unchanged.
};

// Dependencies still owned by main.cpp and used by extracted pages.
class ArchivingManager;
class PlayersManager;
class DelayManager;
class PointerDelay;
class PointerSampler;
class ShiftRegisters;
class Switches;
class LoopLedSet;
class MidiReader;

extern ArchivingManager Archive;
extern PlayersManager Players_Manager;
extern DelayManager Delay_manager;
extern PointerDelay Pointer_Delay;
extern PointerSampler Pointer_Sampler;
extern ShiftRegisters Shifters_manager;
extern Switches Switches_manager;
extern LoopLedSet Loop_led_set;
extern MidiReader Midi_reader;
extern Encoders Encoders_manager;
extern bool TOOLS_pushbutton;
extern DS_pointer_struct DS_local_pointer;
extern int result;

class PointerMixer;
class AudioControlSGTL5000;
class AmpliOutMuteIn;

extern PointerMixer Pointer_Mixer;
extern AudioControlSGTL5000 Audio_shield;
extern AmpliOutMuteIn MAIN_mixer_out_L;
extern AmpliOutMuteIn MAIN_mixer_out_R;
extern AmpliOutMuteIn PWM_mixer_out_L;
extern AmpliOutMuteIn PWM_mixer_out_R;
extern uint8_t Instrument_id;
extern uint16_t Sound_id;
extern int LS_instrument;

enum DS_state_name
{
    DS_waiting_state,
    DS_pause_state,
    DS_recording_state,
    DS_convert_state,
    DS_export_SD_state
};
extern DS_state_name DS_state; // Direct Sampler state owned by main.cpp.

class LillaClock;
class FlashFileRegisterParser;
extern LillaClock Trigger;
extern FlashFileRegisterParser File_scanner;
extern int first_octave_cache;
extern bool confirmation;

class PointerVCF;
class PointerSound;
class PerformanceLedSet;
extern PointerVCF Pointer_VCF;
extern PointerSound Pointer_Sound;
extern PerformanceLedSet Performance_led_set;
extern uint32_t samples_in_file;
extern uint32_t S_trim_step;
extern bool S_sound_original;
extern S_field_description_struct S_pointer;
extern int LS_sound_id;

class PointerPerformance;
class AudioTables;
extern PointerPerformance Pointer_Performance;
extern AudioTables Audio_tables;
extern int P_menu_max;
extern P_field_description_struct P_pointer;
extern Patch_struct Patch_cache_P;
extern uint8_t Patch_id_old;
extern bool patch_original;
extern bool patch_original_0;
extern uint8_t midi_channel_change;
extern uint8_t from_key_change;
extern uint8_t to_key_change;
extern uint8_t patch_change;
extern bool audio_tables_error_pending;
extern uint8_t Capture_return_patch;
extern bool changed;
extern int action;

bool Read_pushbutton(int element);      // Consume a pending press/change event for the specified pushbutton.
int Read_encoder_simple(int element);   // Consume encoder rotation and return -1, 0 or 1 for its direction.
void Clear_UI_events(void);             // Discard all pending encoder rotation and pushbutton press events without resetting the controllers' internal states.
void Require_FRAM(byte result); // Halt further operations and display an error if a FRAM access failed.
void DS_define_menu(void);                            // Enable Direct Sampler menu entries for the current recording state.
void Switch_to_DIRECT_SAMPLING(void);                      // Prepare the temporary recording patch and enter Direct Sampler.
void Switch_from_LIVE_SAMPLING_to_DIRECT_SAMPLING(void);   // Handle Live Sampler recording exit before entering Direct Sampler.
void Switch_from_PERFORMANCE_to_LIVE_SAMPLING(void);       // Prepare the temporary Live Sampler patch and enter its page.
void Switch_from_DIRECT_SAMPLING_to_LIVE_SAMPLING(void);   // Handle Direct Sampler exit before preparing Live Sampler.
void LS_refresh_LS_page(void);                                                      // Redraw Live Sampler, restore its controls and discard notices from the previous page.
void Golive_with_PERFORMANCE(int patch_id);                // Enter the Performance page for the requested patch.
void Switch_from_DIRECT_SAMPLING_to_PERFORMANCE(void);     // Handle Direct Sampler exit and restore the previous Performance patch.
void Switch_from_LIVE_SAMPLING_to_PERFORMANCE(void);       // Handle recording exit and restore the previous Performance patch.
void Switch_from_MIDI_LOOP_to_PERFORMANCE(void);           // Stop loop tracks and return to Performance.
void Golive_with_MIDI_LOOP(bool restart = false);          // Enter MIDI Loop; preserve running tracks unless restart is requested.

void Switch_from_PERFORMANCE_to_MIDI_LOOP(void);           // Enter MIDI Loop while retaining the current Performance patch.

void Switch_from_DIRECT_SAMPLING_to_MIDI_LOOP(void);       // Leave Direct Sampler and restore the previous patch for MIDI Loop.

void Switch_from_LIVE_SAMPLING_to_MIDI_LOOP(void);         // Handle recording exit and restore the previous patch for MIDI Loop.

void Switch_from_MIDI_LOOP_to_DIRECT_SAMPLING(void);       // Stop loop tracks and enter Direct Sampler.

void Switch_from_MIDI_LOOP_to_LIVE_SAMPLING(void);         // Stop loop tracks, prepare the Live Sampler patch and enter its page.

void LOOP_stop_all_midi_tracks(void);                          // Stop every MIDI Loop track; call with audio interrupts disabled.

void Calc_pitch_from_note(const int &key_step); // Recalculate the note pitch multipliers for the selected keyboard scale.

bool SET_Copy_audio_files_from_SD_to_Flash(bool &flash_changed); // Import audio files from SD and report whether Flash contents changed.

bool P_Quiesce_audio_players(void);                                   // Stop control callbacks and drain players before replacing file or patch metadata.

void VFS_Make_VFS(void);                              // Ask for the recording-area size and create the erasable Flash packet files.

void DS_seed_all_Recordings(void);                    // Initialize empty recording metadata and save it to FRAM.

void Reload_system_state(void);          // Reload persistent settings and model data into the running system.

bool S_Fill_all_tables(void);                                         // Prepare all used instruments from the model with audio interrupts disabled.

bool BACKUP_Restore(bool *config_error = nullptr);    // Restore an SD backup and optionally distinguish configuration errors from other failures.

bool BACKUP_Export(void);                             // Export configuration and recording audio to an SD backup.

void Factory_setup_FRAM(void);  // Initialize persistent configuration with factory defaults.

void S_Set_Sound_SOLO_OFF(void);                                                                                                                               // Disable Sound solo mode and restore normal instrument note routing.

void S_Map_one_Instrument_for_all_notes(const int instrument_id);                                                                                              // Map the selected instrument across the note range for Sound editing.

void P_Update_all_maps_Instrument_for_notes(void);                                               // Rebuild the mapping from MIDI channel and note to patch instruments.

int Get_samples_in_raw_file(int value);            // Return the sample count for a RAW file or Direct Sampler channel.

uint16_t S_Calc_Noclick_max(bool use_Wavetable);                      // Return the maximum click-suppression setting for the current source type.

uint32_t S_Calc_trim_step(int value);                                                                                                                          // Calculate the sample increment for the selected trimming speed.

bool S_Verify_is_Sound_original(int sound_id);                                                                                                                 // Compare a Sound with its reference metadata.

void S_Select_menu_elements(void); // Enable Sound menu entries according to the selected Sound and editing state.

void Golive_with_LIVE_SAMPLING(void);                      // Enter and redraw the Live Sampler page with its controls and waveform.

void P_Select_menu_elements(void); // Enable Performance menu entries according to the current patch state.

void P_Read_all_Patches(void);                       // Load patch metadata from FRAM.

void P_Update_Patches_number(void);                  // Recount the patch slots currently in use.

uint8_t P_Get_next_Patch_id_existing(void);          // Find the next existing patch relative to the current selection.

uint8_t P_Get_previous_Patch_id_existing(void);      // Find the previous existing patch relative to the current selection.

int P_Ask_if_change_Patch(void);                     // Show the patch-change dialog and return the selected action.

bool P_Ask_if_delete_this_Patch(void);               // Ask whether to delete the current patch; return true if confirmed.

bool P_Verify_is_Patch_original(const int patch_id); // Compare a patch and its used instruments with the reference metadata.

void Update_instruments_leds(void);                                                              // Refresh instrument LEDs for the current operating mode.

void P_Update_line_of_all_instruments(void);                                                     // Recalculate the display row assigned to each instrument.

void P_Macro_Instrument_editing(const int patch_id, const int instrument_id, const int element); // Apply an instrument edit and publish the related playback changes.

void P_Reset_map_Instrument_for_notes(const int instrument_id);                                  // Clear one instrument's note mappings before rebuilding its range.

void S_Copy_all_Sound_to_Sound_cache_P(void);                                                                                                                  // Save current Sound metadata as the reference for editing and discard.

bool S_Save_all_Sounds_changed(void);                                                                                                                          // Save modified Sound metadata to FRAM; report whether the operation succeeded.

bool S_Pull_all_Sound_from_Sound_cache_P(PatchEditSnapshot *snapshot = nullptr);                                                                               // Restore Sound metadata from the reference, optionally using an edit snapshot.

uint8_t S_Get_midi_channel_from_Sound(int sound_id);                                                                                                           // Decode the MIDI channel stored in a Sound's packed metadata.

void S_Set_midi_channel_for_Sound(int sound_id, int midi_channel);                                                                                             // Update the MIDI channel bits in a Sound's packed metadata.

bool P_Jump_to_Patch(uint8_t next_patch);                  // Publish the destination patch only after its presets and tables are ready.

bool P_Save_current_patch_as_new(void);                    // Prepare the cloned patch before saving its sounds and metadata.

void Print_Patch(int patch_id);                                        // Print the selected patch's metadata to Serial.

void Print_Sound(int sound_id);                                        // Print the selected Sound's metadata to Serial.

FLASHMEM void LS_Capture_collect(void);                                             // Release unreferenced capture sources and caches while preserving references held by players.

FLASHMEM void LS_Capture_finish_save(void);                                         // Clear successfully saved capture sources and release caches no longer needed after saving.

template <class T>
bool Read_encoder(const int encoder, T &value, const int highest, const int lowest, const int increment)
{
    auto R = Encoders_manager.Get_rotation(encoder);
    if (R == 0)
    {
        return false;
    }
    if constexpr (std::is_enum_v<T>)
    {
        auto v = static_cast<int>(value);
        if (R == -1)
        {
            if (v > lowest)
            {
                value = static_cast<T>(v - increment);
                return true;
            }
            return false;
        }
        else
        {
            if (v < highest)
            {
                value = static_cast<T>(v + increment);
                return true;
            }
            return false;
        }
    }
    else
    {
        if (R == -1)
        {
            if (value > lowest)
            {
                value = value - increment;
                return true;
            }
            return false;
        }
        else
        {
            if (value < highest)
            {
                value = value + increment;
                return true;
            }
            return false;
        }
    }
}


template <class T>
bool Read_encoder_inverse(const int encoder, T &value, const int highest, const int lowest, const int increment)
{
    auto R = Encoders_manager.Get_rotation(encoder);
    if (R == 0)
    {
        return false;
    }
    if constexpr (std::is_enum_v<T>)
    {
        auto v = static_cast<int>(value);
        if (R == 1)
        {
            if (v > lowest)
            {
                value = static_cast<T>(v - increment);
                return true;
            }
            return false;
        }
        else
        {
            if (v < highest)
            {
                value = static_cast<T>(v + increment);
                return true;
            }
            return false;
        }
    }
    else
    {
        if (R == 1)
        {
            if (value > lowest)
            {
                value = value - increment;
                return true;
            }
            return false;
        }
        else
        {
            if (value < highest)
            {
                value = value + increment;
                return true;
            }
            return false;
        }
    }
}
