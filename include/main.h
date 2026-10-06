/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 */

#pragma once

#include <Arduino.h>
#include <type_traits>
#include "Encoders.h"
#include "SharedSampler.h"

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
void Golive_SETUP(void);                                   // Enter and initialize the Setup page.
void Golive_MIDI_MONITOR(void);                            // Enter the MIDI Monitor page and initialize its display.

void Switch_from_PERFORMANCE_to_MIDI_LOOP(void);           // Enter MIDI Loop while retaining the current Performance patch.

void Switch_from_DIRECT_SAMPLING_to_MIDI_LOOP(void);       // Leave Direct Sampler and restore the previous patch for MIDI Loop.

void Switch_from_LIVE_SAMPLING_to_MIDI_LOOP(void);         // Handle recording exit and restore the previous patch for MIDI Loop.

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

