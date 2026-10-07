/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 */

#include <Audio.h>
#include "VCFPage.h"
#include "main.h"
#include "LiveSamplerPage.h"
#include "DelayPage.h"
#include "MixerPage.h"
#include "MidiMonitorPage.h"
#include "SetupPage.h"
#include "UserInterface.h"
#include "Functions.h"
#include "SharedElements.h"
#include "SharedVCF.h"
#include "SharedLiveSampler.h"
#include "SharedDelay.h"
#include "PlayersManager.h"
#include "PointerVCF.h"
#include "PointerSound.h"
#include "PerformanceLedSet.h"
#include "Switches.h"
#include "ShiftRegisters.h"
#include "GlobalDisplayCommon.h"
#include "GlobalDisplayVCF.h"
#include "GlobalDisplaySound.h"
#include "DisplayPrimitives.h"

static void Macro_VCF_filter_on_none(void);
static void Macro_VCF_modulation_none(void);

void Handle_VCF(void)
{
    if (Lilla_state == INSTRUMENT_VCF)
    {

        // Change volume_patch
        if (Read_encoder(EN_PB_LineOutVol, volume_patch, PATCH_VOLUME_MAX, 0, 1))
        {
            AudioNoInterrupts();
            Players_Manager.Update_all_Preset_volume(Patch_id, Patch_volume_gain(volume_patch));
            Players_Manager.Broadcast_volume();
            AudioInterrupts();

            if (Lilla_state_0 == LIVE_SAMPLING)
            {
                Display_Common.P_Patch_volume_value(true); // true: YELLOW
            }
        }

        // Move pointer
        result = Read_encoder_simple(EN_PB_Select);
        if (result != 0)
        {
            Pointer_VCF.Move_pointer(result);

            Clear_UI_events();
        }

        // Change values
        switch (Pointer_VCF.Get_VCF_value_name())
        {

        case value_VCF_Menu:
        {
            if (Read_pushbutton(EN_PB_Select))
            {
                if (Lilla_state_0 == PERFORMANCE)
                {
                    S_Set_Sound_SOLO_OFF();
                    Golive_with_PERFORMANCE(Patch_id);
                }
                else if (Lilla_state_0 == MIDI_LOOP)
                {
                    S_Set_Sound_SOLO_OFF();
                    Golive_with_MIDI_LOOP(false);
                }
                else if (Lilla_state_0 == LIVE_SAMPLING)
                {
                    Golive_with_LIVE_SAMPLING();
                }
            }
        }
        break;

        case value_VCF_Gain_Volume:
        {
            // Volume
            if (Lilla_state_0 == LIVE_SAMPLING)
            {
                if (Read_encoder(EN_PB_Value, volume_patch, PATCH_VOLUME_MAX, 0, 1))
                {
                    AudioNoInterrupts();
                    Players_Manager.Update_all_Preset_volume(Patch_id, Patch_volume_gain(volume_patch));
                    Players_Manager.Broadcast_volume();
                    AudioInterrupts();

                    if (Lilla_state_0 == LIVE_SAMPLING)
                    {
                        Display_Common.P_Patch_volume_value(true); // true: YELLOW
                    }
                }
            }

            // Gain
            else
            {
                if (Read_encoder(EN_PB_Value, Sound[Sound_id].gain, 40, 0, 1))
                {
                    AudioNoInterrupts();
                    Players_Manager.Update_Preset_volume(Patch_id, Instrument_id, Patch_volume_gain(volume_patch));
                    Players_Manager.Multicast_volume_for_instrument_edit(Instrument_id);
                    AudioInterrupts();

                    Display_VCF.VCF_show_sound_gain_value(Sound_id);
                }

                // Solo
                if (Read_pushbutton(EN_PB_Value))
                {
                    AudioNoInterrupts();
                    Players_Manager.Release_all_players_for_instrument_solo(Instrument_id);
                    S_Map_one_Instrument_for_all_notes(Instrument_id);
                    AudioInterrupts();

                    Display_VCF.VCF_show_solo_value();
                }
            }
        }
        break;

        case value_VCF_FilterType:
        {
            if (Read_encoder(EN_PB_Value, Patch[Patch_id].Instrument[Instrument_id].Filter.type, 3, 0, 1))
            {
                if (Lilla_state_0 == LIVE_SAMPLING)
                {
                    Patch[Patch_id].Instrument[1].Filter.type = Patch[Patch_id].Instrument[0].Filter.type;

                    AudioNoInterrupts();
                    Players_Manager.Update_Preset_IF(Patch_id, 0);
                    Players_Manager.Update_Preset_IF(Patch_id, 1);
                    Players_Manager.Multicast_IF_update_filter_type(0);
                    Players_Manager.Multicast_IF_update_filter_type(1);
                    AudioInterrupts();
                }
                else
                {
                    AudioNoInterrupts();
                    Players_Manager.Update_Preset_IF(Patch_id, Instrument_id);
                    Players_Manager.Multicast_IF_update_filter_type(Instrument_id);
                    AudioInterrupts();
                }

                Display_VCF.VCF_show_filter_type_value(Instrument_id);
            }

            // Exclude VCF
            else if (Read_pushbutton(EN_PB_Value) || Read_pushbutton(EN_PB_Select))
            {
                Macro_VCF_filter_on_none();
                Display_VCF.VCF_show_filter_type_value(Instrument_id);
            }
        }
        break;

        case value_VCF_Cutoff:
        {
            if (Read_encoder(EN_PB_Value, Patch[Patch_id].Instrument[Instrument_id].Filter.pivot, 100, 0, 1))
            {
                AudioNoInterrupts();
                Players_Manager.Update_Preset_IF(Patch_id, Instrument_id);
                if (Preset[Instrument_id].Filter.use == 1)
                {
                    Players_Manager.Multicast_IF_pivot(Instrument_id);
                }

                if (Lilla_state_0 == LIVE_SAMPLING)
                {
                    Patch[Patch_id].Instrument[1].Filter.pivot = Patch[Patch_id].Instrument[0].Filter.pivot;
                    Instrument_id = 1;

                    Players_Manager.Update_Preset_IF(Patch_id, Instrument_id);
                    if (Preset[Instrument_id].Filter.use == 1)
                    {
                        Players_Manager.Multicast_IF_pivot(Instrument_id);
                    }

                    Instrument_id = 0;
                }
                AudioInterrupts();

                Display_VCF.VCF_show_cutoff_value(Instrument_id);
            }
        }
        break;

        case value_VCF_Resonance:
        {
            if (Read_encoder(EN_PB_Value, Patch[Patch_id].Instrument[Instrument_id].Filter.resonance, 40, 0, 1))
            {

                AudioNoInterrupts();
                Players_Manager.Update_IF_resonance(Patch_id, Instrument_id);
                if (Lilla_state_0 == LIVE_SAMPLING)
                {
                    Patch[Patch_id].Instrument[1].Filter.resonance = Patch[Patch_id].Instrument[0].Filter.resonance;
                    Instrument_id = 1;
                    Players_Manager.Update_IF_resonance(Patch_id, Instrument_id);
                    Instrument_id = 0;
                }
                AudioInterrupts();

                Display_VCF.VCF_show_resonance_value(Instrument_id);
            }
        }
        break;

        case value_VCF_LfoModulationType:
        {
            if (Read_encoder(EN_PB_Value, Patch[Patch_id].Instrument[Instrument_id].Filter.modulation, 4, 0, 1))
            {
                if (Lilla_state_0 == LIVE_SAMPLING)
                {
                    Patch[Patch_id].Instrument[1].Filter.modulation = Patch[Patch_id].Instrument[0].Filter.modulation;

                    AudioNoInterrupts();
                    Players_Manager.Update_Preset_IF_modulation(Patch_id, 0);
                    Players_Manager.Update_Preset_IF_modulation(Patch_id, 1);
                    AudioInterrupts();
                }
                else
                {
                    AudioNoInterrupts();
                    Players_Manager.Update_Preset_IF_modulation(Patch_id, Instrument_id);
                    AudioInterrupts();
                }

                Display_VCF.VCF_show_LFO_modulation_source(Instrument_id);
            }

            // Exclude VCF
            else if (Read_pushbutton(EN_PB_Value) || Read_pushbutton(EN_PB_Select))
            {
                Macro_VCF_modulation_none();
                Display_VCF.VCF_show_LFO_modulation_source(Instrument_id);
            }
        }
        break;

        case value_VCF_LfoModFreqTime:
        {
            if (Read_encoder(EN_PB_Value, Patch[Patch_id].Instrument[Instrument_id].Filter.frequency_time, 40, 0, 1))
            {
                AudioNoInterrupts();
                Players_Manager.Update_Preset_IF(Patch_id, Instrument_id);
                if ((Preset[Instrument_id].Filter.use == 1) && (Preset[Instrument_id].Filter.modulation > 0) && (Preset[Instrument_id].Filter.periodic == 1))
                {
                    Players_Manager.Multicast_IF_frequency_filter(Instrument_id);
                }

                if (Lilla_state_0 == LIVE_SAMPLING)
                {
                    Patch[Patch_id].Instrument[1].Filter.frequency_time = Patch[Patch_id].Instrument[0].Filter.frequency_time;
                    Instrument_id = 1;
                    Players_Manager.Update_Preset_IF(Patch_id, Instrument_id);
                    if ((Preset[Instrument_id].Filter.use == 1) && (Preset[Instrument_id].Filter.modulation > 0) && (Preset[Instrument_id].Filter.periodic == 1))
                    {
                        Players_Manager.Multicast_IF_frequency_filter(Instrument_id);
                    }

                    Instrument_id = 0;
                }
                AudioInterrupts();

                Display_VCF.VCF_show_LFO_freq_time(Instrument_id);
            }
        }
        break;

        case value_VCF_LfoModDepth:
        {
            if (Read_encoder(EN_PB_Value, Patch[Patch_id].Instrument[Instrument_id].Filter.index, 20, 0, 1))
            {
                AudioNoInterrupts();
                Players_Manager.Update_Preset_IF_index(Patch_id, Instrument_id);

                if (Lilla_state_0 == LIVE_SAMPLING)
                {
                    Patch[Patch_id].Instrument[1].Filter.index = Patch[Patch_id].Instrument[0].Filter.index;
                    Instrument_id = 1;
                    Players_Manager.Update_Preset_IF_index(Patch_id, Instrument_id);
                    Instrument_id = 0;
                }
                AudioInterrupts();

                Display_VCF.VCF_show_LFO_modulation_depth(Instrument_id);
            }
        }
        break;

        default:
            break;
        }

        // Switch Sound or INSTRUMENT_EDIT
        if (Lilla_state_0 == PERFORMANCE)
        {
            for (auto Inst_id = 0; Inst_id < INSTRUMENTS; ++Inst_id)
            {
                if (Read_pushbutton(PB_Sound[Inst_id]))
                {
                    if (Inst_id == Instrument_id)
                    {
                        S_Set_Sound_SOLO_OFF();
                        Golive_with_PERFORMANCE(Patch_id);
                    }

                    else if (Patch[Patch_id].Instrument[Inst_id].used)
                    {
                        AudioNoInterrupts();
                        if (solo_flag)
                        {
                            solo_flag = false;
                            P_Update_all_maps_Instrument_for_notes();
                        }
                        AudioInterrupts();

                        Instrument_id = Inst_id;

                        Lilla_state = SOUND_EDIT;

                        Sound_id = Get_sound_id(Patch_id, Instrument_id);

                        samples_in_file = Get_samples_in_raw_file(Sound[Sound_id].file);
                        Noclick_max = S_Calc_Noclick_max(Preset[Instrument_id].use_Wavetable);
                        S_trim_step = S_Calc_trim_step(trim_speed);

                        Display_Sound.Show_SOUND_page(Patch_id, Instrument_id);

                        S_sound_original = S_Verify_is_Sound_original(Sound_id);
                        S_Select_menu_elements();
                        Display_Sound.Show_SOUND_menu(); // displays the menu and updates "SO_menu_max" used by encoder_menu

                        // Select RETURN when entering SOUND_EDIT.
                        Pointer_Sound.Set_pointer_to_first_menu_element();
                        S_pointer = Pointer_Sound.Get_pointer();
                        Pointer_Sound.Display_pointer();

                        Performance_led_set.Restore_all_LED();

                        Display_Sound.Show_wave(Instrument_id);

                        Clear_UI_events();
                    }

                    else
                    {
                        char message[24];
                        snprintf(message, sizeof(message), "SOUND %u IS NOT USED", static_cast<unsigned int>(Inst_id + 1));
                        Show_popup_text_tight(message, "", ILI9341_WHITE, ILI9341_RED, 114);
                        delay(1000);
                        Show_popup_text_tight(message, "", ILI9341_BLACK, ILI9341_BLACK, 114);
                    }
                }
            }
        }

        else if (Lilla_state_0 == LIVE_SAMPLING)
        {
            if (LS_stereo)
            {
                // VCF left channel
                if (Read_pushbutton(PB_S1))
                {
                    if (Instrument_id == 0)
                    {
                        LS_instrument = 0;        // Left
                        LS_sound_id = SOUNDS_MAX; // Left
                        Golive_with_LIVE_SAMPLING();
                    }

                    else
                    {
                        Instrument_id = 0;
                        Sound_id = SOUNDS_MAX;

                        Display_VCF.VCF_show_VCF_page(Patch_id, Instrument_id);

                        // restore LED
                        Performance_led_set.Restore_all_LED();
                    }
                }

                // VCF right channel
                if (Read_pushbutton(PB_S2))
                {
                    if (Instrument_id == 1)
                    {
                        LS_instrument = 1;            // Right
                        LS_sound_id = SOUNDS_MAX + 1; // Right
                        Golive_with_LIVE_SAMPLING();
                    }

                    else
                    {
                        Instrument_id = 1;
                        Sound_id = SOUNDS_MAX + 1;
                        Display_VCF.VCF_show_VCF_page(Patch_id, Instrument_id);

                        // restore LED
                        Performance_led_set.Restore_all_LED();
                    }
                }
            }

            else
            {
                // VCF left channel
                if (Read_pushbutton(PB_S1))
                {
                    Golive_with_LIVE_SAMPLING();
                }

                // VCF right channel
                else if (Read_pushbutton(PB_S2))
                {
                    Golive_with_LIVE_SAMPLING();
                }
            }
        }

        else if (Lilla_state_0 == MIDI_LOOP)
        {
            for (auto Inst_id = 0; Inst_id < INSTRUMENTS; ++Inst_id)
            {
                if (Read_pushbutton(PB_Sound[Inst_id]))
                {
                    if (Inst_id == Instrument_id)
                    {
                        S_Set_Sound_SOLO_OFF();
                        Golive_with_MIDI_LOOP(false);
                    }

                    else if (Patch[Patch_id].Instrument[Inst_id].used)
                    {
                        AudioNoInterrupts();
                        if (solo_flag)
                        {
                            solo_flag = false;
                            P_Update_all_maps_Instrument_for_notes();
                        }
                        AudioInterrupts();

                        Instrument_id = Inst_id;

                        Lilla_state = SOUND_EDIT;

                        Sound_id = Get_sound_id(Patch_id, Instrument_id);

                        samples_in_file = Get_samples_in_raw_file(Sound[Sound_id].file);
                        Noclick_max = S_Calc_Noclick_max(Preset[Instrument_id].use_Wavetable);
                        S_trim_step = S_Calc_trim_step(trim_speed);

                        Display_Sound.Show_SOUND_page(Patch_id, Instrument_id);

                        S_sound_original = S_Verify_is_Sound_original(Sound_id);
                        S_Select_menu_elements();
                        Display_Sound.Show_SOUND_menu(); // displays the menu and updates "SO_menu_max" used by encoder_menu

                        // Select RETURN when entering SOUND_EDIT.
                        Pointer_Sound.Set_pointer_to_first_menu_element();
                        S_pointer = Pointer_Sound.Get_pointer();
                        Pointer_Sound.Display_pointer();

                        Performance_led_set.Restore_all_LED();

                        Display_Sound.Show_wave(Instrument_id);

                        Clear_UI_events();
                    }

                    else
                    {
                        char message[24];
                        snprintf(message, sizeof(message), "SOUND %u IS NOT USED", static_cast<unsigned int>(Inst_id + 1));
                        Show_popup_text_tight(message, "", ILI9341_WHITE, ILI9341_RED, 114);
                        delay(1000);
                        Show_popup_text_tight(message, "", ILI9341_BLACK, ILI9341_BLACK, 114);
                    }
                }
            }
        }

        // Switch verso un TOOL
        if (Read_pushbutton(PB_Tools))
        {
            TOOLS_pushbutton = true;
            Shifters_manager.Switch_led(LED_Tools, true);

            switch (Switches_manager.Get_value(SwitchTools))
            {
            case SwToolsMixer:
            {
                Switch_to_MIXER();
            }
            break;

            case SwToolsDelay:
            {
                if (Lilla_state_0 == PERFORMANCE)
                {
                    S_Set_Sound_SOLO_OFF();
                }
                else // Lilla_state_0 == LIVE_SAMPLING
                {
                    if (Delay_values.instrument_route[0] || Delay_values.instrument_route[1])
                    {
                        Delay_values.instrument_route[0] = true;
                        Delay_values.instrument_route[1] = true;
                    }
                }

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
        if (Switches_manager.Get_change(SwitchModes))
        {
            switch (Switches_manager.Get_value(SwitchModes))
            {
            case SwModesSampler:
            {
                if (Lilla_state_0 == PERFORMANCE)
                {
                    S_Set_Sound_SOLO_OFF();
                    Switch_to_DIRECT_SAMPLING();
                }
                else // Lilla_state_0 == LIVE_SAMPLING
                {
                    Switch_from_LIVE_SAMPLING_to_DIRECT_SAMPLING();
                }
            }
            break;

            case SwModesLiveSampler:
            {
                if (Lilla_state_0 == PERFORMANCE)
                {
                    S_Set_Sound_SOLO_OFF();
                    Switch_from_PERFORMANCE_to_LIVE_SAMPLING();
                }
                else // Lilla_state_0 == LIVE_SAMPLING
                {
                    Golive_with_LIVE_SAMPLING();
                }
            }
            break;

            case SwModesPerformance:
            {
                if (Lilla_state_0 == PERFORMANCE)
                {
                    // S_Set_Sound_SOLO_OFF();
                    // Golive_with_PERFORMANCE(Patch_id);
                }
                else
                {
                    Switch_from_LIVE_SAMPLING_to_PERFORMANCE();
                }
            }
            break;

            case SwModesMidiLoop:
            {
                S_Set_Sound_SOLO_OFF();
                Switch_from_PERFORMANCE_to_MIDI_LOOP();
            }
            break;
            }
        }

    } // end INSTRUMENT_VCF
}

static void Macro_VCF_filter_on_none(void)
{
    if (!Patch[Patch_id].Instrument[Instrument_id].Filter.use)
    {
        Patch[Patch_id].Instrument[Instrument_id].Filter.use = true;
    }
    else
    {
        Patch[Patch_id].Instrument[Instrument_id].Filter.use = false;
    }

    AudioNoInterrupts();
    Players_Manager.Update_Preset_IF(Patch_id, Instrument_id);
    Players_Manager.Multicast_IF_update_filter_type(Instrument_id);

    if (Lilla_state_0 == LIVE_SAMPLING)
    {
        Patch[Patch_id].Instrument[1].Filter.use = Patch[Patch_id].Instrument[0].Filter.use;
        Instrument_id = 1;

        Players_Manager.Update_Preset_IF(Patch_id, Instrument_id);
        Players_Manager.Multicast_IF_update_filter_type(Instrument_id);

        Instrument_id = 0;
    }
    AudioInterrupts();
}

static void Macro_VCF_modulation_none(void)
{
    Patch[Patch_id].Instrument[Instrument_id].Filter.modulation = 0;

    AudioNoInterrupts();
    Players_Manager.Update_Preset_IF(Patch_id, Instrument_id);

    if (Lilla_state_0 == LIVE_SAMPLING)
    {
        Patch[Patch_id].Instrument[1].Filter.modulation = Patch[Patch_id].Instrument[0].Filter.modulation;
        Instrument_id = 1;

        Players_Manager.Update_Preset_IF(Patch_id, Instrument_id);

        Instrument_id = 0;
    }
    AudioInterrupts();
}
