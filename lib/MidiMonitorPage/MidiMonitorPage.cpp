/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 */

#include <Audio.h>
#include "MidiMonitorPage.h"
#include "main.h"
#include "LiveSamplerPage.h"
#include "SetupPage.h"
#include "DelayPage.h"
#include "MixerPage.h"
#include "UserInterface.h"
#include "Functions.h"
#include "SharedElements.h"
#include "SharedMM.h"
#include "PlayersManager.h"
#include "Switches.h"
#include "ShiftRegisters.h"
#include "PointerSampler.h"
#include "GlobalDisplayDiagnostics.h"
#include "GlobalDisplaySampler.h"

void Handle_Midi_monitor(void)
{
    if (Lilla_state == MIDI_MONITOR)
    {
        // Change Patch VOLUME
        if (Read_encoder(EN_PB_LineOutVol, volume_patch, PATCH_VOLUME_MAX, 0, 1))
        {
            AudioNoInterrupts();
            Players_Manager.Update_all_Preset_volume(Patch_id, Patch_volume_gain(volume_patch));
            Players_Manager.Broadcast_volume();
            AudioInterrupts();
        }

        // Visualizza incoming MIDI
        if (display_wait)
        {
            switch (midi_message_received)
            {
            case 0: // no message received
                break;
            case 1: // note ON
                Display_Diagnostics.Midi_monitor_data(MM_midi_channel, 0, MM_note_number, MM_velocity, -1, -1);
                break;
            case 2: // note OFF
                Display_Diagnostics.Midi_monitor_data(MM_midi_channel, 1, MM_note_number, MM_velocity, -1, -1);
                break;
            case 3: // pitch bend
                Display_Diagnostics.Midi_monitor_data(MM_midi_channel, 2, -1, -1, (MM_pitch_bend_most << 7) + MM_pitch_bend_least, -1);
                break;
            case 4: // after touch poly
                Display_Diagnostics.Midi_monitor_data(MM_midi_channel, 3, MM_least_bits, -1, MM_most_bits, -1);
                break;
            case 5: // control change
                Display_Diagnostics.Midi_monitor_data(MM_midi_channel, 4, -1, -1, MM_midi_value, MM_midi_controller);
                break;
            case 6: // program change
                Display_Diagnostics.Midi_monitor_data(MM_midi_channel, 5, -1, -1, -1, MM_least_bits);
                break;
            case 7: // After Touch Channel
                Display_Diagnostics.Midi_monitor_data(MM_midi_channel, 6, -1, -1, MM_least_bits, -1);
                break;
            case 8: // System Exclusive
                Display_Diagnostics.Midi_monitor_data(MM_midi_channel, 7, -1, -1, -1, -1);
                break;
            default:
                PRINT_ERROR(F("Switch MISSING! "));
                break;
            }
            display_wait = false;
        }

        // Switch TOOL
        switch (Switches_manager.Get_value(SwitchTools))
        {
        case SwToolsMixer:
        {
            Switch_to_MIXER();
        }
        break;

        case SwToolsDelay:
        {
            Golive_DELAY_SETTINGS();
        }
        break;

        case SwToolsSetup:
        {
            Golive_SETUP();
        }
        break;

        case SwToolsTest:
            break;
        }

        // Switch Mode
        if (Read_pushbutton(PB_Tools))
        {
            TOOLS_pushbutton = false;
            Shifters_manager.Switch_led(LED_Tools, false);
            switch (Switches_manager.Get_value(SwitchModes))
            {
            case SwModesSampler:
            {
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
                    Switch_from_MIDI_LOOP_to_DIRECT_SAMPLING();
                    break;

                default:
                    PRINT_ERROR(F("Switch MISSING! "));
                    break;
                }
            }
            break;

            case SwModesLiveSampler:
            {
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
                    Switch_from_MIDI_LOOP_to_LIVE_SAMPLING();
                    break;

                default:
                    PRINT_ERROR(F("Switch MISSING! "));
                    break;
                };
            }
            break;

            case SwModesPerformance:
            {
                switch (Lilla_state_0)
                {
                case PERFORMANCE:
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
                switch (Lilla_state_0)
                {
                case PERFORMANCE:
                    Switch_from_PERFORMANCE_to_MIDI_LOOP();
                    break;

                case DIRECT_SAMPLING:
                    Switch_from_DIRECT_SAMPLING_to_MIDI_LOOP();
                    break;

                case LIVE_SAMPLING:
                    Switch_from_LIVE_SAMPLING_to_MIDI_LOOP();
                    break;

                case MIDI_LOOP:
                    Golive_with_MIDI_LOOP(false);
                    break;

                default:
                    PRINT_ERROR(F("Switch MISSING! "));
                    break;
                }
            }
            break;
            }
        }
    }
}

void Golive_MIDI_MONITOR(void)
{

    AudioNoInterrupts();
    Players_Manager.Stop_all_players();
    AudioInterrupts();

    Lilla_state = MIDI_MONITOR;

    display_wait = false;
    Display_Diagnostics.Midi_monitor_page();

    Clear_UI_events();
}

void Switch_from_MIDI_LOOP_to_MIDI_MONITOR(void)
{
    AudioNoInterrupts();
    LOOP_stop_all_midi_tracks();
    AudioInterrupts();

    Golive_MIDI_MONITOR();
}
