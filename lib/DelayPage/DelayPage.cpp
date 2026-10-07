/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 */

#include "DelayPage.h"
#include "MixerPage.h"
#include <Audio.h>
#include "main.h"
#include "MidiLoopPage.h"
#include "LiveSamplerPage.h"
#include "SetupPage.h"
#include "MidiMonitorPage.h"
#include "UserInterface.h"
#include "Functions.h"
#include "SharedLoop.h"
#include "PlayersManager.h"
#include "DelayManager.h"
#include "PointerDelay.h"
#include "PointerSampler.h"
#include "ShiftRegisters.h"
#include "Switches.h"
#include "LoopLedSet.h"
#include "ArchivingManager.h"
#include "GlobalDisplayCommon.h"
#include "GlobalDisplayDelay.h"
#include "GlobalDisplaySampler.h"

DELAY_element_name DELAY_local_pointer; // Current Delay menu or parameter selection.

void Handle_Delay(void)
{
    if (Lilla_state == DELAY_SETTINGS && Lilla_state_0 != DIRECT_SAMPLING)
    {
        // Change Patch VOLUME
        if (Read_encoder(EN_PB_LineOutVol, volume_patch, PATCH_VOLUME_MAX, 0, 1))
        {
            AudioNoInterrupts();
            Players_Manager.Update_all_Preset_volume(Patch_id, Patch_volume_gain(volume_patch));
            Players_Manager.Broadcast_volume();
            AudioInterrupts();

            Display_Common.P_Patch_volume_value(true);
        }

        // Move pointer
        result = Read_encoder_simple(EN_PB_Select);
        if (result != 0)
        {
            Pointer_Delay.Move_pointer(result);
            DELAY_local_pointer = Pointer_Delay.Get_element_name();

            Clear_UI_events();
        }

        // Change values
        switch (DELAY_local_pointer)
        {
        case value_DELAY_Feedback:
            if (D_Read_value(LOOP_GAIN))
            {
                Display_Delay.D_feedback();
            }
            break;
        case value_DELAY_Delay_time:
            if (D_Read_value(SAMPLES))
            {
                Display_Delay.D_delay_time();
                Display_Delay.D_delay_time_LR(); // The shorter base time may also reduce the stereo offset.
            }
            break;
        case value_DELAY_Delay_time_LR:
            if (D_Read_value(SAMPLES_LR))
            {
                Display_Delay.D_delay_time_LR();
            }
            break;
        case value_DELAY_Modulation_source:
            if (D_Read_value(MODULATION_SOURCE))
            {
                Display_Delay.D_modulation_source();
            }
            else if (Read_pushbutton(EN_PB_Value))
            {
                D_Set_value(MODULATION_SOURCE, 0); // Cancel any pending source selection before displaying NONE.
                Display_Delay.D_modulation_source();
            }
            break;
        case value_DELAY_Modulation_frequency:
            if (D_Read_value(MODULATION_FREQUENCY))
            {
                Display_Delay.D_modulation_frequency();
            }
            break;
        case value_DELAY_Modulation_depth:
            if (D_Read_value(MODULATION_DEPTH))
            {
                Display_Delay.D_modulation_depth();
            }
            else if (Read_pushbutton(EN_PB_Value))
            {
                D_Set_value(MODULATION_DEPTH, 0); // Fade toward zero depth through the shared transition manager.
                Display_Delay.D_modulation_depth();
            }
            break;
        case value_DELAY_Modulation_phase_LR:
            if (D_Read_value(MODULATION_PHASE_LR))
            {
                Display_Delay.D_modulation_phase_LR();
            }
            else if (Read_pushbutton(EN_PB_Value))
            {
                D_Set_value(MODULATION_PHASE_LR, 0);
                Display_Delay.D_modulation_phase_LR();
            }
            break;
        default:
            break;
        }

        // Toggle requested routing bits; the audio callback updates every affected voice.
        for (auto Inst_id = 0; Inst_id < INSTRUMENTS; ++Inst_id)
        {
            if (Read_pushbutton(PB_Sound[Inst_id]))
            {
                const int route = Delay_manager.Get_value(INSTRUMENT_ROUTE);
                const int next_route = Lilla_state_0 == LIVE_SAMPLING ? ((route & 3) != 0 ? route & ~3 : route | 3) : route ^ (1 << Inst_id);
                D_Set_value(INSTRUMENT_ROUTE, next_route); // Live sampling enables or disables both channels together, even when the stored bits differ.
                Display_Delay.D_sounds();
            }
        }
    }

    // Navigation remains available while the Direct Sampler locks the Delay controls.
    if (Lilla_state == DELAY_SETTINGS)
    {
        // Switch Mode
        if (Read_pushbutton(PB_Tools))
        {
            TOOLS_pushbutton = false;
            Shifters_manager.Switch_led(LED_Tools, false);

            switch (Switches_manager.Get_value(SwitchModes))
            {
            case SwModesSampler:
            {
                if (Patch_id < PATCHES_MAX)
                {
                    Require_FRAM(Archive.Save_Delay(Patch_id, Delay_data));
                }
                switch (Lilla_state_0)
                {
                case PERFORMANCE:
                    Switch_to_DIRECT_SAMPLING();
                    break;

                case DIRECT_SAMPLING:
                    Lilla_state = DIRECT_SAMPLING;

                    Display_Sampler.DS_page_upper();
                    Display_Sampler.DS_page_lower(recording);

                    // Menu
                    DS_define_menu();
                    Display_Sampler.DS_menu(); // display the menu and updates DS_menu_max

                    // Pointer
                    Pointer_Sampler.Set_pointer_to_first_menu_element();
                    DS_local_pointer = Pointer_Sampler.Get_pointer();

                    // Display the VU meter
                    Display_Sampler.DS_bar(0, 0);
                    Display_Sampler.DS_bar(1, 0);

                    Clear_UI_events();
                    break;

                case LIVE_SAMPLING:
                    Switch_from_LIVE_SAMPLING_to_DIRECT_SAMPLING();
                    break;

                case MIDI_LOOP:
                    // Esci da MIDI_LOOP

                    AudioNoInterrupts();
                    // Ferma i track running
                    for (auto local_track = 0; local_track < TRACKS; ++local_track) // true --> il track va suonato
                    {
                        LOOP_track_run[local_track] = false;
                    }

                    // Ferma i Player dei loop
                    Players_Manager.Release_all_players_loop();
                    AudioInterrupts();

                    Loop_led_set.Request_all_LED_switch_off();

                    Switch_to_DIRECT_SAMPLING();
                    break;

                default:
                    PRINT_ERROR(F("Switch MISSING! "));
                    break;
                }
            }
            break;

            case SwModesLiveSampler:
            {
                if (Patch_id < PATCHES_MAX)
                {
                    Require_FRAM(Archive.Save_Delay(Patch_id, Delay_data));
                }
                switch (Lilla_state_0)
                {
                case PERFORMANCE:
                    Switch_from_PERFORMANCE_to_LIVE_SAMPLING();
                    break;

                case DIRECT_SAMPLING:
                    Switch_from_DIRECT_SAMPLING_to_LIVE_SAMPLING();
                    break;

                case LIVE_SAMPLING:
                    LS_refresh_LS_page();
                    break;

                case MIDI_LOOP:
                    // Esci da MIDI_LOOP

                    AudioNoInterrupts();
                    // Ferma i track running
                    for (auto local_track = 0; local_track < TRACKS; ++local_track) // true --> il track va suonato
                    {
                        LOOP_track_run[local_track] = false;
                    }

                    // Ferma i Player dei track
                    Players_Manager.Release_all_players_loop();
                    AudioInterrupts();

                    Loop_led_set.Request_all_LED_switch_off();

                    Switch_from_PERFORMANCE_to_LIVE_SAMPLING();
                    break;

                default:
                    PRINT_ERROR(F("Switch MISSING! "));
                    break;
                }
            }
            break;

            case SwModesPerformance:
            {
                switch (Lilla_state_0)
                {
                case PERFORMANCE:

                    // Salva il Delay della Patch su FRAM.
                    if (Patch_id < PATCHES_MAX)
                    {
                        Require_FRAM(Archive.Save_Delay(Patch_id, Delay_data));
                    }

                    if (true)
                    {
                        Serial.println();
                        Serial.println(F("main() - Delay_data in RAM:"));
                        Print_Delay_data(Delay_data);
                        Serial.println(F("... has been saved in FRAM."));
                    }

                    Golive_with_PERFORMANCE(Patch_id);
                    break;

                case DIRECT_SAMPLING:
                    Switch_from_DIRECT_SAMPLING_to_PERFORMANCE();
                    break;

                case LIVE_SAMPLING:
                    Switch_from_LIVE_SAMPLING_to_PERFORMANCE();
                    break;

                case MIDI_LOOP:
                    Switch_from_MIDI_LOOP_to_PERFORMANCE();
                    break;

                default:
                    PRINT_ERROR(F("Switch MISSING! "));
                    break;
                }
            }
            break;

            case SwModesMidiLoop:
            {
                if (Patch_id < PATCHES_MAX)
                {
                    Require_FRAM(Archive.Save_Delay(Patch_id, Delay_data));
                }
                Golive_with_MIDI_LOOP(false);
            }
            break;
            }
        }

        // Switch Tool
        if (Switches_manager.Get_change(SwitchTools))
        {
            switch (Switches_manager.Get_value(SwitchTools))
            {
            case SwToolsMixer:
            {
                if (Patch_id < PATCHES_MAX)
                {
                    Require_FRAM(Archive.Save_Delay(Patch_id, Delay_data));
                }
                Switch_to_MIXER();
            }
            break;

            case SwToolsDelay:
                break;

            case SwToolsSetup:
            {
                if (Patch_id < PATCHES_MAX)
                {
                    Require_FRAM(Archive.Save_Delay(Patch_id, Delay_data));
                }
                Golive_SETUP();
            }
            break;

            case SwToolsTest:
            {
                if (Patch_id < PATCHES_MAX)
                {
                    Require_FRAM(Archive.Save_Delay(Patch_id, Delay_data));
                }
                Golive_MIDI_MONITOR();
            }
            break;
            }
        }
    }
}

void Golive_DELAY_SETTINGS(void)
{
    Lilla_state = DELAY_SETTINGS;

    Display_Delay.D_show_page();
    if (Lilla_state_0 != DIRECT_SAMPLING)
    {
        Pointer_Delay.Set_pointer_to_Feedback();
        DELAY_local_pointer = Pointer_Delay.Get_element_name();
    }

    Clear_UI_events();
}

void Switch_from_LIVE_SAMPLING_to_DELAY(void)
{
    Lilla_state_0 = LIVE_SAMPLING;

    if (Delay_values.instrument_route[0] || Delay_values.instrument_route[1])
    {
        Delay_values.instrument_route[0] = true;
        Delay_values.instrument_route[1] = true;
    }

    Golive_DELAY_SETTINGS();
}

void D_Set_value(int item, int value) // Publish one UI request through the same parameter owner used by patch changes.
{
    const bool enabled = NVIC_IS_ENABLED(IRQ_SOFTWARE) != 0;

    AudioNoInterrupts();
    Delay_manager.Set_value(item, value); // Retarget only this parameter; the audio callback applies the change.
    if (enabled)
    {
        AudioInterrupts();
    }
}

bool D_Read_value(int item) // Edit a requested value locally so the encoder never writes intermediate DSP state.
{
    int value = Delay_manager.Get_value(item);
    const int offset_limit = item == SAMPLES_LR ? Calc_delay_samples_LR_limit(Delay_manager.Get_value(SAMPLES)) : 0;
    const int lowest = item == SAMPLES_LR ? -offset_limit : (item < DELAY_LPF_ITEMS ? Delay_data_limits[item][0] : 0);
    const int highest = item == SAMPLES_LR ? offset_limit : (item < DELAY_LPF_ITEMS ? Delay_data_limits[item][1] : 2);
    if (!Read_encoder(EN_PB_Value, value, highest, lowest, 1))
    {
        return false;
    }
    D_Set_value(item, value); // Submit the encoder result with audio interrupts disabled.
    return true;
}
