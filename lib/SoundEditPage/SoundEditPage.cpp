/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 */

#include <Audio.h>
#include "SoundEditPage.h"
#include "main.h"
#include "DelayPage.h"
#include "MixerPage.h"
#include "MidiMonitorPage.h"
#include "SetupPage.h"
#include "MidiLoopPage.h"
#include "UserInterface.h"
#include "Functions.h"
#include "SharedElements.h"
#include "PlayersManager.h"
#include "PerformanceLedSet.h"
#include "Switches.h"
#include "ShiftRegisters.h"
#include "PointerSound.h"
#include "GlobalDisplaySound.h"
#include "DisplayPrimitives.h"
#include "SharedVCF.h"
#include "AudioTables.h"
#include "AudioPlayer.h"
#include "PointerPerformance.h"
#include "PointerVCF.h"
#include "GlobalDisplayVCF.h"

static int S_slicing_window;            // Selected waveform slicing window.
constexpr int MIN_SNIPPET = 100; // Minimum playback snippet length in samples.

void Handle_Sound_edit(void)
{
    // *************************************************************
    // ********************    SOUND EDIT   ************************
    // *************************************************************

    static bool sound_edit_was_active = false;
    const bool entering_sound_edit = !sound_edit_was_active && Lilla_state == SOUND_EDIT;
    sound_edit_was_active = Lilla_state == SOUND_EDIT;
    if (Lilla_state == SOUND_EDIT)
    {
        S_Refresh_source_limits(entering_sound_edit); // Check only while this page is active and redraw immediately after re-entry.
        const Sound_struct sound_before_edit = Sound[Sound_id];
        // Change volume_patch
        if (Read_encoder(EN_PB_LineOutVol, volume_patch, PATCH_VOLUME_MAX, 0, 1))
        {
            AudioNoInterrupts();
            Players_Manager.Update_all_Preset_volume(Patch_id, Patch_volume_gain(volume_patch));
            Players_Manager.Broadcast_volume();
            AudioInterrupts();
        }

        // Move pointer
        result = Read_encoder_simple(EN_PB_Select);
        if (result != 0)
        {
            Pointer_Sound.Move_pointer(result, S_menu_max);
            S_pointer = Pointer_Sound.Get_pointer();

            Clear_UI_events();
        }

        // Change values
        switch (S_pointer.field_name)
        {
        case field_S_Menu:
        {
            if (Read_pushbutton(EN_PB_Select))
            {
                switch (S_element_menu[S_pointer.menu_element])
                {
                case value_S_Return: // keep changes and exit from SOUND EDIT
                {
                    S_Set_Sound_SOLO_OFF();

                    if (Lilla_state_0 == MIDI_LOOP)
                    {
                        Golive_with_MIDI_LOOP(false);
                    }
                    else
                    {
                        Golive_with_PERFORMANCE(Patch_id);
                    }
                }
                break;

                case value_S_Clone:
                {
                    AudioNoInterrupts();
                    PatchEditSnapshot previous;
                    int new_instrument = 0;
                    bool cloned = false;
                    if (previous.valid && S_Clone_Instrument(Instrument_id, new_instrument, previous))
                    {
                        if (S_Fill_tables(new_instrument))
                        {
                            P_Update_all_maps_Instrument_for_notes();
                            cloned = true;
                        }
                        else
                        {
                            previous.Restore();
                        }
                    }
                    else
                    {
                        previous.Restore();
                    }
                    AudioInterrupts();

                    S_Set_Sound_SOLO_OFF();
                    Golive_with_PERFORMANCE(Patch_id);
                    if (cloned)
                    {
                        Pointer_Performance.Set_pointer_to_RootKey(new_instrument);
                        P_pointer = Pointer_Performance.Get_pointer();
                    }
                }
                break;

                case value_S_Drop:
                {
                    AudioNoInterrupts();
                    Players_Manager.Release_all_players_for_instrument(Instrument_id);
                    P_Delete_one_map_Instrument_for_notes(Instrument_id);
                    S_Drop_Instrument(Instrument_id); // instruments is decremented by 1
                    AudioInterrupts();

                    S_Set_Sound_SOLO_OFF();
                    Golive_with_PERFORMANCE(Patch_id);
                }
                break;

                default:
                    break;
                }
            }
        }
        break;

        case field_S_Value:
        {
            switch (S_pointer.value_element)
            {
            case value_S_File:
            {
                int result = Read_encoder_simple(EN_PB_Value);
                if (result != 0)
                {
                    int S_file_change;
                    if (result == 1)
                    {
                        S_file_change = Get_next_raw_file_in_flash(Sound[Sound_id].file);
                    }
                    else
                    {
                        S_file_change = Get_previous_raw_file_in_flash(Sound[Sound_id].file);
                    }
                    if (S_file_change != Sound[Sound_id].file)
                    {
                        Sound[Sound_id].file = S_file_change;
                        samples_in_file = Get_samples_in_raw_file(Sound[Sound_id].file);
                        Sound[Sound_id].pitch = 0;
                        Sound[Sound_id].A = 0;
                        Sound[Sound_id].B = (samples_in_file > 0 ? samples_in_file - 1 : 0);
                        if (!slicing_mode)
                        {
                            S_slicing_window = Sound[Sound_id].B - Sound[Sound_id].A + 1;
                        }
                        Noclick_max = S_Calc_Noclick_max(((Sound[Sound_id].B - Sound[Sound_id].A + 1) <= BLOCK_MIN));
                        if (Sound[Sound_id].Noclick > Noclick_max)
                        {
                            Sound[Sound_id].Noclick = Noclick_max;
                        }

                        AudioNoInterrupts();

                        if (!S_Fill_tables(Instrument_id))
                        {
                            Sound[Sound_id] = sound_before_edit;
                            Noclick_max = S_Calc_Noclick_max(Preset[Instrument_id].use_Wavetable);
                            samples_in_file = Get_samples_in_raw_file(Sound[Sound_id].file);
                            if (!slicing_mode)
                            {
                                S_slicing_window = Sound[Sound_id].B - Sound[Sound_id].A + 1;
                            }
                            Noclick_max = S_Calc_Noclick_max(((Sound[Sound_id].B - Sound[Sound_id].A + 1) <= BLOCK_MIN));
                        }
                        AudioInterrupts();

                        S_trim_step = S_Calc_trim_step(trim_speed);

                        Display_Sound.Show_File_value(Instrument_id);
                        Display_Sound.Show_wave(Instrument_id);

                        auto sound_original_0 = S_sound_original;
                        S_sound_original = S_Verify_is_Sound_original(Sound_id);
                        if (sound_original_0 != S_sound_original)
                        {
                            if (Lilla_state_0 != MIDI_LOOP)
                            {
                                S_Select_menu_elements();
                            }
                            Display_Sound.Show_SOUND_menu();
                        }
                    }
                }

                // SOLO
                if (Read_pushbutton(EN_PB_Select))
                {
                    if (!solo_flag)
                    {
                        solo_flag = true;

                        AudioNoInterrupts();
                        Players_Manager.Release_all_players_for_instrument_solo(Instrument_id);
                        S_Map_one_Instrument_for_all_notes(Instrument_id);
                        AudioInterrupts();
                    }
                    else
                    {
                        S_Set_Sound_SOLO_OFF();
                    }
                    Display_Sound.Show_wave(Instrument_id);
                }
            }
            break;

            case value_S_Midi:
            {
                int result = Read_encoder_simple(EN_PB_Value);
                if (result != 0)
                {
                    midi_channel_change = S_Get_midi_channel_from_Sound(Sound_id);
                    if (result == 1)
                    {
                        if (midi_channel_change < 15)
                        {
                            midi_channel_change++;
                        }
                    }
                    else
                    {
                        if (midi_channel_change > 0)
                        {
                            midi_channel_change--;
                        }
                    }
                    if (midi_channel_change != S_Get_midi_channel_from_Sound(Sound_id))
                    {
                        AudioNoInterrupts();
                        Players_Manager.Multicast_release_players(Sound_id);
                        P_Reset_map_Instrument_for_notes(Instrument_id);
                        S_Set_midi_channel_for_Sound(Sound_id, midi_channel_change);
                        Update_map_Instrument_for_notes(Patch[Patch_id].Instrument[Instrument_id].from_note, Patch[Patch_id].Instrument[Instrument_id].to_note, Instrument_id);
                        Players_Manager.Update_Preset_midi_channel(Patch_id, Instrument_id);
                        AudioInterrupts();

                        Display_Sound.Show_Midi_channel_value(Instrument_id);

                        auto sound_original_0 = S_sound_original;
                        S_sound_original = S_Verify_is_Sound_original(Sound_id);
                        if (sound_original_0 != S_sound_original)
                        {
                            if (Lilla_state_0 != MIDI_LOOP)
                            {
                                S_Select_menu_elements();
                            }
                            Display_Sound.Show_SOUND_menu();
                        }
                    }
                }
            }
            break;

            case value_S_Pitch:
            {
                result = Read_encoder_simple(EN_PB_Value);
                if (result != 0)
                {
                    changed = false;
                    if (result == 1)
                    {
                        if (Sound[Sound_id].pitch < 96)
                        {
                            ++Sound[Sound_id].pitch;
                            changed = true;
                        }
                    }
                    else
                    {
                        if (Sound[Sound_id].pitch > -96)
                        {
                            Sound[Sound_id].pitch--;
                            changed = true;
                        }
                    }
                    if (changed)
                    {
                        AudioNoInterrupts();
                        Players_Manager.Update_Preset_pitch(Patch_id, Instrument_id);
                        Players_Manager.Multicast_pitch_for_sound_edit(Instrument_id);
                        AudioInterrupts();

                        Display_Sound.Show_Pitch_value(Instrument_id);

                        auto sound_original_0 = S_sound_original;
                        S_sound_original = S_Verify_is_Sound_original(Sound_id);
                        if (sound_original_0 != S_sound_original)
                        {
                            if (Lilla_state_0 != MIDI_LOOP)
                            {
                                S_Select_menu_elements();
                            }
                            Display_Sound.Show_SOUND_menu();
                        }
                    }
                }

                else if (Read_pushbutton(EN_PB_Value))
                {
                    if (Sound[Sound_id].pitch == 0)
                    {
                        break;
                    }

                    Sound[Sound_id].pitch = 0;

                    AudioNoInterrupts();
                    Players_Manager.Update_Preset_pitch(Patch_id, Instrument_id);
                    Players_Manager.Multicast_pitch_for_sound_edit(Instrument_id);
                    AudioInterrupts();

                    Display_Sound.Show_Pitch_value(Instrument_id);

                    auto sound_original_0 = S_sound_original;
                    S_sound_original = S_Verify_is_Sound_original(Sound_id);
                    if (sound_original_0 != S_sound_original)
                    {
                        if (Lilla_state_0 != MIDI_LOOP)
                        {
                            S_Select_menu_elements();
                        }
                        Display_Sound.Show_SOUND_menu();
                    }
                }

                // Analyze the selected loop and tune its strongest component.
                else if (Read_pushbutton(EN_PB_Select))
                {
                    const char *auto_tune_error = S_Auto_tune_pitch(Sound_id);
                    if (auto_tune_error == nullptr)
                    {
                        AudioNoInterrupts();
                        Players_Manager.Update_Preset_pitch(Patch_id, Instrument_id);
                        Players_Manager.Multicast_pitch_for_sound_edit(Instrument_id);
                        AudioInterrupts();

                        Display_Sound.Show_Pitch_value(Instrument_id);

                        auto sound_original_0 = S_sound_original;
                        S_sound_original = S_Verify_is_Sound_original(Sound_id);
                        if (sound_original_0 != S_sound_original)
                        {
                            if (Lilla_state_0 != MIDI_LOOP)
                            {
                                S_Select_menu_elements();
                            }
                            Display_Sound.Show_SOUND_menu();
                        }
                        Show_popup_text("AUTO TUNE", ILI9341_BLACK, ILI9341_GREEN);
                        delay(800);
                        Display_Sound.Show_SOUND_page(Patch_id, Instrument_id);
                        Display_Sound.Show_SOUND_menu();
                        Display_Sound.Show_wave(Instrument_id);
                        Pointer_Sound.Display_pointer();
                        Clear_UI_events();
                    }
                    else
                    {
                        Show_popup_text(auto_tune_error, ILI9341_WHITE, ILI9341_RED, 0);
                        delay(800);
                        Display_Sound.Show_SOUND_page(Patch_id, Instrument_id);
                        Display_Sound.Show_SOUND_menu();
                        Display_Sound.Show_wave(Instrument_id);
                        Pointer_Sound.Display_pointer();
                        Clear_UI_events();
                    }
                }
            }
            break;

            case value_S_Gain:
            {
                if (Read_encoder(EN_PB_Value, Sound[Sound_id].gain, 40, 0, 1))
                {
                    AudioNoInterrupts();
                    Players_Manager.Update_Preset_volume(Patch_id, Instrument_id, Patch_volume_gain(volume_patch));
                    Players_Manager.Multicast_volume_for_instrument_edit(Instrument_id);
                    AudioInterrupts();

                    Display_Sound.Show_Gain_value(Patch_id, Instrument_id);
                    Display_Sound.Show_wave(Instrument_id);

                    auto sound_original_0 = S_sound_original;
                    S_sound_original = S_Verify_is_Sound_original(Sound_id);
                    if (sound_original_0 != S_sound_original)
                    {
                        if (Lilla_state_0 != MIDI_LOOP)
                        {
                            S_Select_menu_elements();
                        }
                        Display_Sound.Show_SOUND_menu();
                    }
                }

                // SOLO
                if (Read_pushbutton(EN_PB_Select))
                {
                    if (!solo_flag)
                    {
                        solo_flag = true;

                        AudioNoInterrupts();
                        Players_Manager.Release_all_players_for_instrument_solo(Instrument_id);
                        S_Map_one_Instrument_for_all_notes(Instrument_id);
                        AudioInterrupts();
                    }
                    else
                    {
                        S_Set_Sound_SOLO_OFF();
                    }
                    Display_Sound.Show_wave(Instrument_id);
                }
            }
            break;

            case value_S_Pan:
            {
                if (Read_encoder(EN_PB_Value, Sound[Sound_id].pan, 16, -16, 1))
                {
                    AudioNoInterrupts();
                    Players_Manager.Update_Preset_pan(Patch_id, Instrument_id);
                    Players_Manager.Multicast_pan(Instrument_id);
                    AudioInterrupts();

                    Display_Sound.Show_Pan_value(Instrument_id);

                    auto sound_original_0 = S_sound_original;
                    S_sound_original = S_Verify_is_Sound_original(Sound_id);
                    if (sound_original_0 != S_sound_original)
                    {
                        if (Lilla_state_0 != MIDI_LOOP)
                        {
                            S_Select_menu_elements();
                        }
                        Display_Sound.Show_SOUND_menu();
                    }
                }

                // Set PAN to center
                else if (Read_pushbutton(EN_PB_Value))
                {
                    Sound[Sound_id].pan = 0;

                    AudioNoInterrupts();
                    Players_Manager.Update_Preset_pan(Patch_id, Instrument_id);
                    Players_Manager.Multicast_pan(Instrument_id);
                    AudioInterrupts();

                    Display_Sound.Show_Pan_value(Instrument_id);

                    auto sound_original_0 = S_sound_original;
                    S_sound_original = S_Verify_is_Sound_original(Sound_id);
                    if (sound_original_0 != S_sound_original)
                    {
                        if (Lilla_state_0 != MIDI_LOOP)
                        {
                            S_Select_menu_elements();
                        }
                        Display_Sound.Show_SOUND_menu();
                    }
                }
            }
            break;

            case value_S_Attack:
            {
                if (Read_encoder(EN_PB_Value, Sound[Sound_id].attack, 255, 0, 1))
                {
                    AudioNoInterrupts();
                    Players_Manager.Update_Preset_attack(Patch_id, Instrument_id);
                    AudioInterrupts();

                    Display_Sound.Show_Attack_value(Instrument_id);

                    auto sound_original_0 = S_sound_original;
                    S_sound_original = S_Verify_is_Sound_original(Sound_id);
                    if (sound_original_0 != S_sound_original)
                    {
                        if (Lilla_state_0 != MIDI_LOOP)
                        {
                            S_Select_menu_elements();
                        }
                        Display_Sound.Show_SOUND_menu();
                    }
                }

                // toggle Attack curve
                else if (Read_pushbutton(EN_PB_Value))
                {
                    AudioNoInterrupts();
                    bitWrite(Sound[Sound_id].data, 0, !bitRead(Sound[Sound_id].data, 0));
                    Players_Manager.Update_Preset_attack_type(Patch_id, Instrument_id);
                    AudioInterrupts();

                    Display_Sound.Show_Attack_value(Instrument_id);

                    auto sound_original_0 = S_sound_original;
                    S_sound_original = S_Verify_is_Sound_original(Sound_id);
                    if (sound_original_0 != S_sound_original)
                    {
                        if (Lilla_state_0 != MIDI_LOOP)
                        {
                            S_Select_menu_elements();
                        }
                        Display_Sound.Show_SOUND_menu();
                    }
                }
            }
            break;

            case value_S_Decay:
            {
                if (Read_encoder(EN_PB_Value, Sound[Sound_id].decay, 255, 0, 1))
                {
                    AudioNoInterrupts();
                    Players_Manager.Update_Preset_decay(Patch_id, Instrument_id);
                    AudioInterrupts();

                    Display_Sound.Show_Decay_value(Instrument_id);

                    auto sound_original_0 = S_sound_original;
                    S_sound_original = S_Verify_is_Sound_original(Sound_id);
                    if (sound_original_0 != S_sound_original)
                    {
                        if (Lilla_state_0 != MIDI_LOOP)
                        {
                            S_Select_menu_elements();
                        }
                        Display_Sound.Show_SOUND_menu();
                    }
                }
            }
            break;

            case value_S_Sustain:
            {
                if (Read_encoder(EN_PB_Value, Sound[Sound_id].sustain, 46, 4, 5))
                {
                    AudioNoInterrupts();
                    Players_Manager.Update_Preset_sustain(Patch_id, Instrument_id);
                    AudioInterrupts();

                    Display_Sound.Show_Sustain_value(Instrument_id);

                    auto sound_original_0 = S_sound_original;
                    S_sound_original = S_Verify_is_Sound_original(Sound_id);
                    if (sound_original_0 != S_sound_original)
                    {
                        if (Lilla_state_0 != MIDI_LOOP)
                        {
                            S_Select_menu_elements();
                        }
                        Display_Sound.Show_SOUND_menu();
                    }
                }
            }
            break;

            case value_S_Release:
            {
                if (Read_encoder(EN_PB_Value, Sound[Sound_id].release, 50, 0, 1))
                {
                    AudioNoInterrupts();
                    Players_Manager.Update_Preset_release(Patch_id, Instrument_id);
                    AudioInterrupts();

                    Display_Sound.Show_Release_value(Instrument_id);

                    auto sound_original_0 = S_sound_original;
                    S_sound_original = S_Verify_is_Sound_original(Sound_id);
                    if (sound_original_0 != S_sound_original)
                    {
                        if (Lilla_state_0 != MIDI_LOOP)
                        {
                            S_Select_menu_elements();
                        }
                        Display_Sound.Show_SOUND_menu();
                    }
                }
            }
            break;

            case value_S_PlayMode:
            {
                if (Read_encoder(EN_PB_Value, Sound[Sound_id].mode, 5, 0, 1))
                {
                    AudioNoInterrupts();
                    // Capture the banks referenced before preparing the edit.
                    uint8_t player_banks_before = 0;
                    for (uint8_t player_id = 0; player_id < PLAYERS; ++player_id)
                    {
                        player_banks_before |= Player[player_id].Get_tables_reference_mask();
                    }

                    // Check the updated preset against the tables prepared before this edit.
                    const Preset_struct candidate = Players_Manager.Build_Preset(Patch_id, Instrument_id, Patch_volume_gain(volume_patch));
                    const bool tables_matched_before = Audio_tables.Get_active_pointers(Instrument_id, candidate).bank_mask != 0;

                    const bool tables_rebuilt = S_Fill_tables(Instrument_id);

                    if (!tables_rebuilt)
                    {
                        Sound[Sound_id] = sound_before_edit;
                        Noclick_max = S_Calc_Noclick_max(Preset[Instrument_id].use_Wavetable);
                    }
                    const uint8_t active_bank_mask = tables_rebuilt ? Audio_tables.Get_active_pointers(Instrument_id, Preset[Instrument_id]).bank_mask : 0;

                    if (tables_rebuilt)
                    {
                        Players_Manager.Multicast_main_settings_editing(Patch_id, Instrument_id);
                    }

                    // Capture current and pending references before audio updates resume.
                    uint8_t player_banks_queued = 0;
                    for (uint8_t player_id = 0; player_id < PLAYERS; ++player_id)
                    {
                        player_banks_queued |= Player[player_id].Get_tables_reference_mask();
                    }
                    AudioInterrupts();

                    Serial.print(F("AudioTables player refs, before: 0x"));
                    Serial.print(player_banks_before, HEX);
                    Serial.print(F(", queued: 0x"));
                    Serial.println(player_banks_queued, HEX);
                    // Report both checks after restoring audio interrupts.
                    Serial.print(F("AudioTables edit preset match, before: "));
                    Serial.print(tables_matched_before);
                    Serial.print(F(", after: "));
                    Serial.println(active_bank_mask != 0);

                    // Report the result after restoring audio interrupts.
                    if (tables_rebuilt)
                    {
                        Serial.print(F("AudioTables edit activated, bank mask: 0x"));
                        Serial.println(active_bank_mask, HEX);
                    }
                    else
                    {
                        Serial.println(F("AudioTables edit tables not activated"));
                    }

                    Display_Sound.Show_Play_mode_value(Instrument_id);

                    auto sound_original_0 = S_sound_original;
                    S_sound_original = S_Verify_is_Sound_original(Sound_id);
                    if (sound_original_0 != S_sound_original)
                    {
                        if (Lilla_state_0 != MIDI_LOOP)
                        {
                            S_Select_menu_elements();
                        }
                        Display_Sound.Show_SOUND_menu();
                    }

                    Serial.print("Change MODE: ");
                    Serial.println(Sound[Sound_id].mode);
                }
            }
            break;

            case value_S_Noclick:
            {
                result = Read_encoder_simple(EN_PB_Value);
                auto changed = false;

                if (result != 0)
                {
                    if (result == 1)
                    {
                        if (Sound[Sound_id].Noclick < Noclick_max)
                        {
                            if (Sound[Sound_id].Noclick >= 90)
                            {
                                Sound[Sound_id].Noclick += 10;
                            }
                            else if (Sound[Sound_id].Noclick >= 30)
                            {
                                Sound[Sound_id].Noclick += 4;
                            }
                            else
                            {
                                Sound[Sound_id].Noclick += 2;
                            }
                            changed = true;
                        }
                    }
                    else
                    {
                        if (Sound[Sound_id].Noclick > 0)
                        {
                            if (Sound[Sound_id].Noclick <= 30)
                            {
                                Sound[Sound_id].Noclick -= 2;
                            }
                            else if (Sound[Sound_id].Noclick <= 90)
                            {
                                Sound[Sound_id].Noclick -= 4;
                            }
                            else
                            {
                                Sound[Sound_id].Noclick -= 10;
                            }
                            changed = true;
                        }
                    }
                    if (changed)
                    {
                        AudioNoInterrupts();

                        if (!S_Fill_tables(Instrument_id))
                        {
                            Sound[Sound_id] = sound_before_edit;
                            Noclick_max = S_Calc_Noclick_max(Preset[Instrument_id].use_Wavetable);
                        }
                        else
                        {
                            Players_Manager.Multicast_main_settings_editing(Patch_id, Instrument_id);
                        }
                        AudioInterrupts();

                        S_sound_original = S_Verify_is_Sound_original(Sound_id);

                        Display_Sound.Show_Noclick_value(Instrument_id, true);
                        Display_Sound.Show_wave(Instrument_id);

                        int sound_original_0 = S_sound_original;
                        S_sound_original = S_Verify_is_Sound_original(Sound_id);
                        if (sound_original_0 != S_sound_original)
                        {
                            if (Lilla_state_0 != MIDI_LOOP)
                            {
                                S_Select_menu_elements();
                            }
                            Display_Sound.Show_SOUND_menu();
                        }
                    }
                }
            }
            break;
            }
        }
        break;
        }

        // Change trim speed
        if (Read_encoder(EN_PB_Step, trim_speed, 5, 0, 1))
        {
            S_trim_step = S_Calc_trim_step(trim_speed);
            Display_Sound.Show_Trim_step_value();
        }

        // Toggle TO/SLICE mode with the TO encoder button.
        if (Read_pushbutton(EN_PB_To))
        {
            slicing_mode = !slicing_mode;
            if (!slicing_mode)
            {
                S_slicing_window = Sound[Sound_id].B - Sound[Sound_id].A + 1;
            }
            Display_Sound.Show_wave(Instrument_id);
        }

        if (Read_pushbutton(EN_PB_Step))
        {
            trim_speed = 5;
            S_trim_step = S_Calc_trim_step(trim_speed);
            Display_Sound.Show_Trim_step_value();
        }

        // Change A
        result = Read_encoder_simple(EN_PB_From);
        if (result != 0)
        {
            uint32_t So_A_change = Sound[Sound_id].A;

            if (result == 1)
            {
                if (slicing_mode)
                {
                    if ((Sound[Sound_id].B - Sound[Sound_id].A + 1) >= (S_trim_step + MIN_SNIPPET))
                    {
                        So_A_change = Sound[Sound_id].A + S_trim_step;
                    }
                    else
                    {
                        So_A_change = Sound[Sound_id].B - MIN_SNIPPET + 1;
                    }
                }

                else
                {
                    if ((Sound[Sound_id].A + S_slicing_window + S_trim_step) <= samples_in_file)
                    {
                        So_A_change = Sound[Sound_id].A + S_trim_step;
                    }
                    else
                    {
                        So_A_change = (samples_in_file - S_slicing_window);
                    }
                }
            }

            else
            {
                if (Sound[Sound_id].A >= S_trim_step)
                {
                    So_A_change = Sound[Sound_id].A - S_trim_step;
                }
                else
                {
                    So_A_change = 0;
                }
            }

            if (So_A_change != Sound[Sound_id].A)
            {
                Sound[Sound_id].A = So_A_change;

                AudioNoInterrupts();
                if (!slicing_mode) // slicing A-Samples
                {
                    Sound[Sound_id].B = Sound[Sound_id].A + S_slicing_window - 1;
                }
                if (trim_speed == 5)
                {
                    S_trim_step = S_Calc_trim_step(5);
                }
                // Clamp the candidate crossfade before preparing the replacement bank.

                Noclick_max = S_Calc_Noclick_max(((Sound[Sound_id].B - Sound[Sound_id].A + 1) <= BLOCK_MIN));
                if (Sound[Sound_id].Noclick > Noclick_max)
                {
                    Sound[Sound_id].Noclick = Noclick_max;
                }
                // Publish the replacement tables before queuing the live edit.
                if (!S_Fill_tables(Instrument_id))
                {
                    Sound[Sound_id] = sound_before_edit;
                    Noclick_max = S_Calc_Noclick_max(Preset[Instrument_id].use_Wavetable);
                }
                else
                {
                    Players_Manager.Multicast_main_settings_editing(Patch_id, Instrument_id);
                }
                AudioInterrupts();

                Display_Sound.Show_players_Pitch_max_value(Instrument_id);
                Display_Sound.Show_wave(Instrument_id);

                int sound_original_0 = S_sound_original;
                S_sound_original = S_Verify_is_Sound_original(Sound_id);
                if (sound_original_0 != S_sound_original)
                {
                    if (Lilla_state_0 != MIDI_LOOP)
                    {
                        S_Select_menu_elements();
                    }
                    Display_Sound.Show_SOUND_menu();
                }
            }
        }

        // Set A = 0
        if (Read_pushbutton(EN_PB_From))
        {
            if (Sound[Sound_id].A != 0)
            {
                Sound[Sound_id].A = 0;

                AudioNoInterrupts();
                if (!slicing_mode) // slicing A-Samples
                {
                    Sound[Sound_id].B = Sound[Sound_id].A + S_slicing_window - 1;
                }
                if (trim_speed == 5)
                {
                    S_trim_step = S_Calc_trim_step(5);
                }
                // Clamp the candidate crossfade before preparing the replacement bank.

                Noclick_max = S_Calc_Noclick_max(((Sound[Sound_id].B - Sound[Sound_id].A + 1) <= BLOCK_MIN));
                if (Sound[Sound_id].Noclick > Noclick_max)
                {
                    Sound[Sound_id].Noclick = Noclick_max;
                }

                // Publish the replacement tables before queuing the live edit.
                if (!S_Fill_tables(Instrument_id))
                {
                    Sound[Sound_id] = sound_before_edit;
                    Noclick_max = S_Calc_Noclick_max(Preset[Instrument_id].use_Wavetable);
                }
                else
                {
                    Players_Manager.Multicast_main_settings_editing(Patch_id, Instrument_id);
                }
                AudioInterrupts();

                Display_Sound.Show_players_Pitch_max_value(Instrument_id);
                Display_Sound.Show_wave(Instrument_id);

                int sound_original_0 = S_sound_original;
                S_sound_original = S_Verify_is_Sound_original(Sound_id);
                if (sound_original_0 != S_sound_original)
                {
                    if (Lilla_state_0 != MIDI_LOOP)
                    {
                        S_Select_menu_elements();
                    }
                    Display_Sound.Show_SOUND_menu();
                }
            }
        }

        // Change B by rotating the TO encoder.
        result = Read_encoder_simple(EN_PB_To);
        if (result != 0)
        {
            uint32_t So_B_change;
            if (result == 1)
            {
                if ((Sound[Sound_id].B + 1 + S_trim_step) <= samples_in_file)
                {
                    So_B_change = Sound[Sound_id].B + S_trim_step;
                }
                else
                {
                    So_B_change = samples_in_file - 1;
                }
            }
            else
            {
                if ((Sound[Sound_id].B - Sound[Sound_id].A + 1) >= (MIN_SNIPPET + S_trim_step))
                {
                    So_B_change = Sound[Sound_id].B - S_trim_step;
                }
                else
                {
                    So_B_change = Sound[Sound_id].A + MIN_SNIPPET - 1;
                }
            }

            if (So_B_change != Sound[Sound_id].B)
            {
                Sound[Sound_id].B = So_B_change;

                AudioNoInterrupts();
                if (trim_speed == 5)
                {
                    S_trim_step = S_Calc_trim_step(5);
                }

                // Clamp the candidate crossfade before preparing the replacement bank.

                Noclick_max = S_Calc_Noclick_max(((Sound[Sound_id].B - Sound[Sound_id].A + 1) <= BLOCK_MIN));
                if (Sound[Sound_id].Noclick > Noclick_max)
                {
                    Sound[Sound_id].Noclick = Noclick_max;
                }

                // Publish the replacement tables before queuing the live edit.
                const bool tables_rebuilt = S_Fill_tables(Instrument_id);
                if (!tables_rebuilt)
                {
                    Sound[Sound_id] = sound_before_edit;
                    Noclick_max = S_Calc_Noclick_max(Preset[Instrument_id].use_Wavetable);
                }
                const uint8_t active_bank_mask = tables_rebuilt ? Audio_tables.Get_active_pointers(Instrument_id, Preset[Instrument_id]).bank_mask : 0;
                const int edited_B = Preset[Instrument_id].B;

                if (tables_rebuilt)
                {
                    Players_Manager.Multicast_main_settings_editing(Patch_id, Instrument_id);
                }

                // Capture current and pending references before audio updates resume.
                uint8_t player_banks_queued = 0;
                for (uint8_t player_id = 0; player_id < PLAYERS; ++player_id)
                {
                    player_banks_queued |= Player[player_id].Get_tables_reference_mask();
                }
                AudioInterrupts();

                // Report the trim result after restoring audio interrupts.
                Serial.print(F("AudioTables trim B: "));
                Serial.print(edited_B);
                Serial.print(F(", ready: "));
                Serial.print(tables_rebuilt && active_bank_mask != 0);
                Serial.print(F(", bank: 0x"));
                Serial.print(active_bank_mask, HEX);
                Serial.print(F(", refs: 0x"));
                Serial.println(player_banks_queued, HEX);

                if (!slicing_mode)
                {
                    S_slicing_window = Sound[Sound_id].B - Sound[Sound_id].A + 1;
                }

                Display_Sound.Show_players_Pitch_max_value(Instrument_id);
                Display_Sound.Show_wave(Instrument_id);

                int sound_original_0 = S_sound_original;
                S_sound_original = S_Verify_is_Sound_original(Sound_id);
                if (sound_original_0 != S_sound_original)
                {
                    if (Lilla_state_0 != MIDI_LOOP)
                    {
                        S_Select_menu_elements();
                    }
                    Display_Sound.Show_SOUND_menu();
                }
            }
        }

        // Switch Sound or INSTRUMENT_EDIT
        for (auto Inst_id = 0; Inst_id < INSTRUMENTS; ++Inst_id)
        {
            if (Read_pushbutton(PB_Sound[Inst_id]))
            {
                if (Inst_id == Instrument_id)
                {
                    Lilla_state = INSTRUMENT_VCF;

                    Display_VCF.VCF_show_VCF_page(Patch_id, Instrument_id);

                    // pointer
                    Pointer_VCF.Set_pointer_to_FilterType();

                    Clear_UI_events();

                    // restore all LED
                    Performance_led_set.Restore_all_LED();
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

                    Sound_id = Get_sound_id(Patch_id, Instrument_id);

                    samples_in_file = Get_samples_in_raw_file(Sound[Sound_id].file);
                    Noclick_max = S_Calc_Noclick_max(((Sound[Sound_id].B - Sound[Sound_id].A + 1) <= BLOCK_MIN));
                    S_trim_step = S_Calc_trim_step(trim_speed);

                    Display_Sound.Show_SOUND_page(Patch_id, Instrument_id);

                    S_sound_original = S_Verify_is_Sound_original(Sound_id);
                    S_Select_menu_elements(); // updates "SO_menu_max" used by encoder_menu
                    Display_Sound.Show_SOUND_menu();

                    // Restore the pointer previously selected in SOUND_EDIT
                    S_pointer = Pointer_Sound.Get_pointer();
                    Pointer_Sound.Display_pointer();

                    Performance_led_set.Restore_all_LED();

                    Display_Sound.Show_wave(Instrument_id);

                    Clear_UI_events();

                    // Report
                    Serial.print("Editing Instrument: ");
                    Serial.print(Instrument_id);
                    Print_Sound(Sound_id);
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

        // Switch verso un TOOL
        if (Read_pushbutton(PB_Tools))
        {
            TOOLS_pushbutton = true;
            Shifters_manager.Switch_led(LED_Tools, true);

            switch (Switches_manager.Get_value(SwitchTools))
            {
            case SwToolsMixer:
            {
                S_Set_Sound_SOLO_OFF();
                Switch_to_MIXER();
            }
            break;

            case SwToolsDelay:
            {
                S_Set_Sound_SOLO_OFF();
                Golive_DELAY_SETTINGS();
            }
            break;

            case SwToolsSetup:
            {
                S_Set_Sound_SOLO_OFF();
                Golive_SETUP();
            }
            break;

            case SwToolsTest:
            {
                S_Set_Sound_SOLO_OFF();
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
                S_Set_Sound_SOLO_OFF();
                Switch_to_DIRECT_SAMPLING();
            }
            break;

            case SwModesLiveSampler:
            {
                S_Set_Sound_SOLO_OFF();
                Switch_from_PERFORMANCE_to_LIVE_SAMPLING();
            }
            break;

            case SwModesPerformance:
            {
                S_Set_Sound_SOLO_OFF();
                Golive_with_PERFORMANCE(Patch_id);
            }
            break;

            case SwModesMidiLoop:
            {
                S_Set_Sound_SOLO_OFF();

                if (Lilla_state_0 == MIDI_LOOP)
                {
                    Golive_with_MIDI_LOOP(false);
                }
                else
                {
                    Switch_from_PERFORMANCE_to_MIDI_LOOP();
                }
            }
            break;
            }
        }
    }

}
