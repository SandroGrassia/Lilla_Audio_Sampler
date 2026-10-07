/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 */

#include <Audio.h>
#include "MixerPage.h"
#include "main.h"
#include "LiveSamplerPage.h"
#include "SetupPage.h"
#include "MidiMonitorPage.h"
#include "DelayPage.h"
#include "UserInterface.h"
#include "Functions.h"
#include "SharedElements.h"
#include "SharedMixer.h"
#include "SharedSampler.h"
#include "PlayersManager.h"
#include "AmpliOutMuteIn.h"
#include "PointerMixer.h"
#include "PointerSampler.h"
#include "Switches.h"
#include "ShiftRegisters.h"
#include "GlobalDisplayMixer.h"
#include "GlobalDisplaySampler.h"

static MX_pointer_struct MX_local_pointer;          // Current Mixer channel or parameter selection.

void Handle_Mixer(void)
{
    if (Lilla_state == MIXER)
    {
        if (Lilla_state_0 == PERFORMANCE || (Lilla_state_0 == DIRECT_SAMPLING && DS_state == DS_waiting_state) || Lilla_state_0 == LIVE_SAMPLING)
        {
            if (Read_encoder(EN_PB_LineOutVol, volume_patch, PATCH_VOLUME_MAX, 0, 1))
            {
                AudioNoInterrupts();
                Players_Manager.Update_all_Preset_volume(Patch_id, Patch_volume_gain(volume_patch));
                Players_Manager.Broadcast_volume();
                AudioInterrupts();
            }
        }

        // Move pointer
        result = Read_encoder_simple(EN_PB_Select);
        if (result != 0)
        {
            Pointer_Mixer.Move_pointer(result);
            MX_local_pointer = Pointer_Mixer.Get_pointer();

            Clear_UI_events();

            Instrument_id = (MX_local_pointer.source < LINE_IN_source ? MX_local_pointer.source : 0);
            Sound_id = Get_sound_id(Patch_id, Instrument_id);
        }

        // Change values
        if (MX_local_pointer.field_name == field_MX_Source)
        {
            // Enter inside
            if (Read_pushbutton(EN_PB_Select))
            {
                Pointer_Mixer.Move_pointer_to_field_MX_Elements();
                MX_local_pointer = Pointer_Mixer.Get_pointer();

                Clear_UI_events();
            }
        }

        else if (MX_local_pointer.field_name == field_MX_Elements)
        {
            // Exit to Source
            if (Read_pushbutton(EN_PB_Select))
            {
                Pointer_Mixer.Move_pointer_to_field_MX_Source();
                MX_local_pointer = Pointer_Mixer.Get_pointer();

                Clear_UI_events();
            }

            switch (MX_local_pointer.element)
            {
            case value_MX_Mute_Gain:
            {
                if (MX_local_pointer.source == LINE_IN_source) // MX_source == LINE_IN_CHANNEL
                {
                    if (Read_encoder(EN_PB_Value, Line_in_gain, 15, 0, 1))
                    {
                        Audio_shield.lineInLevel(Line_in_gain);

                        Display_Mixer.MX_source_values_edit(LINE_IN_source);
                    }
                }
                else
                {
                    if (Read_encoder(EN_PB_Value, Sound[Sound_id].gain, 40, 0, 1))
                    {
                        AudioNoInterrupts();
                        Players_Manager.Update_Preset_volume(Patch_id, Instrument_id, Patch_volume_gain(volume_patch));
                        Players_Manager.Multicast_volume_for_instrument_edit(Instrument_id);
                        AudioInterrupts();

                        Display_Mixer.MX_source_values_edit(Instrument_id);
                    }
                }

                if (Read_pushbutton(EN_PB_Value)) // Mute source
                {
                    MX_mute[MX_local_pointer.source] = !MX_mute[MX_local_pointer.source];

                    if (MX_local_pointer.source == LINE_IN_source)
                    {
                        if (MX_mute[MX_local_pointer.source])
                        {
                            MAIN_mixer_out_L.Mute(1);
                            MAIN_mixer_out_R.Mute(1);
                            PWM_mixer_out_L.Mute(1);
                            PWM_mixer_out_R.Mute(1);
                        }
                        else
                        {
                            MAIN_mixer_out_L.unmute(1);
                            MAIN_mixer_out_R.unmute(1);
                            PWM_mixer_out_L.unmute(1);
                            PWM_mixer_out_R.unmute(1);
                        }
                    }
                    else
                    {
                        AudioNoInterrupts();
                        Players_Manager.Update_Preset_volume(Patch_id, Instrument_id, Patch_volume_gain(volume_patch));
                        Players_Manager.Multicast_volume_for_instrument_edit(Instrument_id);
                        AudioInterrupts();
                    }

                    Display_Mixer.MX_source_values_edit(MX_local_pointer.source);
                }
            }
            break;

            case value_MX_Pan:
            {
                if (MX_local_pointer.source == LINE_IN_source)
                {
                    // not supported
                }
                else
                {
                    if (Read_encoder(EN_PB_Value, Sound[Sound_id].pan, 16, -16, 1))
                    {
                        AudioNoInterrupts();
                        Players_Manager.Update_Preset_pan(Patch_id, Instrument_id);
                        Players_Manager.Multicast_pan(Instrument_id);
                        AudioInterrupts();

                        Display_Mixer.MX_source_values_edit(Instrument_id);
                    }
                }
            }
            break;

            case value_MX_Lineout:
            {
                if (Read_pushbutton(EN_PB_Value))
                {
                    if (MX_routing_source[MX_local_pointer.source] == 0) // era tutto muto --> solo MAIN
                    {
                        MX_routing_source[MX_local_pointer.source] = 2;
                    }
                    else if (MX_routing_source[MX_local_pointer.source] == 1) // era solo MONITOR --> MONITOR e MAIN
                    {
                        MX_routing_source[MX_local_pointer.source] = 3;
                    }
                    else if (MX_routing_source[MX_local_pointer.source] == 2) // era solo MAIN --> tutto muto
                    {
                        MX_routing_source[MX_local_pointer.source] = 0;
                    }
                    else // era 3 (MONITOR e MAIN) --> solo MONITOR
                    {
                        MX_routing_source[MX_local_pointer.source] = 1;
                    }

                    if (MX_local_pointer.source == LINE_IN_source)
                    {
                        switch (MX_routing_source[MX_local_pointer.source])
                        {
                        case 0:
                            MAIN_mixer_out_L.Mute(1);
                            MAIN_mixer_out_R.Mute(1);
                            break;

                        case 1:
                            MAIN_mixer_out_L.Mute(1);
                            MAIN_mixer_out_R.Mute(1);
                            break;

                        case 2:
                            MAIN_mixer_out_L.unmute(1);
                            MAIN_mixer_out_R.unmute(1);
                            break;

                        case 3:
                            MAIN_mixer_out_L.unmute(1);
                            MAIN_mixer_out_R.unmute(1);
                            break;

                        default:
                            PRINT_ERROR(F("Switch MISSING! "));
                            break;
                        }
                    }
                    else
                    {
                        AudioNoInterrupts();
                        Players_Manager.MX_multicast_change_routing(Instrument_id);
                        AudioInterrupts();
                    }

                    Display_Mixer.MX_source_values_edit(MX_local_pointer.source);
                }
            }
            break;

            case value_MX_Monitor:
            {
                if (Read_pushbutton(EN_PB_Value))
                {
                    if (MX_routing_source[MX_local_pointer.source] == 0) // era tutto muto --> solo MONITOR
                    {
                        MX_routing_source[MX_local_pointer.source] = 1;
                    }
                    else if (MX_routing_source[MX_local_pointer.source] == 1) // era solo MONITOR --> tutto muto
                    {
                        MX_routing_source[MX_local_pointer.source] = 0;
                    }
                    else if (MX_routing_source[MX_local_pointer.source] == 2) // era solo MAIN --> MONITOR e MAIN
                    {
                        MX_routing_source[MX_local_pointer.source] = 3;
                    }
                    else // era 3 (MONITOR e MAIN) --> solo MAIN
                    {
                        MX_routing_source[MX_local_pointer.source] = 2;
                    }

                    if (MX_local_pointer.source == LINE_IN_source)
                    {
                        switch (MX_routing_source[MX_local_pointer.source])
                        {
                        case 0:
                            PWM_mixer_out_L.Mute(1);
                            PWM_mixer_out_R.Mute(1);
                            break;

                        case 1:
                            PWM_mixer_out_L.unmute(1);
                            PWM_mixer_out_R.unmute(1);
                            break;

                        case 2:
                            PWM_mixer_out_L.Mute(1);
                            PWM_mixer_out_R.Mute(1);
                            break;

                        case 3:
                            PWM_mixer_out_L.unmute(1);
                            PWM_mixer_out_R.unmute(1);
                            break;

                        default:
                            PRINT_ERROR(F("Switch MISSING! "));
                            break;
                        }
                    }

                    else
                    {
                        AudioNoInterrupts();
                        Players_Manager.MX_multicast_change_routing(Instrument_id);
                        AudioInterrupts();
                    }

                    Display_Mixer.MX_source_values_edit(MX_local_pointer.source);
                }
            }
            break;

            default:
                break;
            }
        }

        // Switch verso un TOOL
        if (Switches_manager.Get_change(SwitchTools))
        {
            switch (Switches_manager.Get_value(SwitchTools))
            {
            case SwToolsMixer:
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
            {
                Golive_MIDI_MONITOR();
            }
            break;
            }
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
                    Golive_with_PERFORMANCE(Patch_id);
                    break;

                case DIRECT_SAMPLING:
                    Switch_from_DIRECT_SAMPLING_to_PERFORMANCE();
                    break;

                case LIVE_SAMPLING:
                    Switch_from_LIVE_SAMPLING_to_PERFORMANCE();
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

void Switch_to_MIXER()
{
    // Se non si sta editando, si parte dal primo Instrument esistente
    if (Lilla_state_0 != PERFORMANCE && Lilla_state_0 != SOUND_EDIT && Lilla_state_0 != INSTRUMENT_VCF && Lilla_state_0 != LIVE_SAMPLING)
    {
        for (auto instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
        {
            if (Patch[Patch_id].Instrument[instrument_id].used)
            {
                Instrument_id = instrument_id;
                break;
            }
        }
        Sound_id = Get_sound_id(Patch_id, Instrument_id);
    }

    else if (Lilla_state == LIVE_SAMPLING)
    {
        Instrument_id = LS_instrument;
        Sound_id = Get_sound_id(Patch_id, Instrument_id);
    }

    Golive_MIXER();
}

void Golive_MIXER(void)
{
    Lilla_state = MIXER;

    Display_Mixer.MX_page();

    Clear_UI_events();

    for (auto source = 0; source < MX_sources; ++source)
    {
        Display_Mixer.MX_source_values(source, (source == 0 ? true : false));
    }

    Pointer_Mixer.Set_pointer_to_source(0);
    MX_local_pointer = Pointer_Mixer.Get_pointer();

    Instrument_id = 0;
    Sound_id = Get_sound_id(Patch_id, Instrument_id);
}
