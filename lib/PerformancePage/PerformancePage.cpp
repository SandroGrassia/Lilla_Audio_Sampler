/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 */

#include <Audio.h>
#include "PerformancePage.h"
#include "main.h"
#include "DelayPage.h"
#include "MixerPage.h"
#include "MidiMonitorPage.h"
#include "SetupPage.h"
#include "UserInterface.h"
#include "Functions.h"
#include "SharedElements.h"
#include "SharedPerformance.h"
#include "SharedSound.h"
#include "SharedDelay.h"
#include "CaptureSources.h"
#include "PlayersManager.h"
#include "AudioTables.h"
#include "ArchivingManager.h"
#include "PointerPerformance.h"
#include "PointerSound.h"
#include "PerformanceLedSet.h"
#include "Switches.h"
#include "ShiftRegisters.h"
#include "GlobalDisplayCommon.h"
#include "GlobalDisplayPerformance.h"
#include "GlobalDisplaySound.h"
#include "DisplayPrimitives.h"

static inline void P_UpdatePatchOriginalAndMenu(void)
{
    patch_original_0 = patch_original;
    patch_original = P_Verify_is_Patch_original(Patch_id);
    if (patch_original != patch_original_0)
    {
        P_Select_menu_elements();
        Display_Performance.P_show_Performance_menu();
    }
}

bool Handle_Performance(void)
{
    if (Lilla_state == PERFORMANCE)
    {

        // Change volume_patch
        if (Read_encoder(EN_PB_LineOutVol, volume_patch, PATCH_VOLUME_MAX, 0, 1)) // LINE OUT VOLUME
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
            Pointer_Performance.Move_pointer(result, P_menu_max);
            P_pointer = Pointer_Performance.Get_pointer();

            Clear_UI_events();
        }

        // Change values
        switch (P_pointer.field_name)
        {
        case field_P_Menu:
        {
            if (Read_pushbutton(EN_PB_Select))
            {
                switch (P_element_menu[P_pointer.element])
                {
                case value_P_Exit: // drop Sound changes
                {
                    if (Patch_id == Capture_new_patch)
                    {
                        PatchEditSnapshot previous;
                        if (!previous.valid)
                        {
                            break;
                        }
                        Patch[Patch_id] = Patch_cache_P;
                        if (!S_Pull_all_Sound_from_Sound_cache_P(&previous) || !P_Jump_to_Patch(Capture_return_patch))
                        {
                            previous.Restore();
                        }
                        break;
                    }
                    AudioNoInterrupts();
                    PatchEditSnapshot previous;
                    if (!previous.valid)
                    {
                        audio_tables_error_pending = true;
                        AudioInterrupts();
                        break;
                    }
                    Patch[Patch_id] = Patch_cache_P;
                    const bool sounds_restored = S_Pull_all_Sound_from_Sound_cache_P(&previous);
                    P_Update_all_maps_Instrument_for_notes();

                    const bool tables_rebuilt = sounds_restored && S_Fill_all_tables();
                    if (!tables_rebuilt)
                    {
                        previous.Restore();
                        AudioInterrupts();
                        break;
                    }
                    uint8_t active_bank_mask = 0;

                    // Capture the active bank mask before restoring audio interrupts.
                    if (tables_rebuilt)
                    {
                        for (uint8_t instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
                        {
                            active_bank_mask |= Audio_tables.Get_active_pointers(instrument_id).bank_mask;
                        }
                    }
                    AudioInterrupts();

                    // Report the result after restoring audio interrupts.
                    if (tables_rebuilt)
                    {
                        Serial.print(F("AudioTables restore activated, bank mask: 0x"));
                        Serial.println(active_bank_mask, HEX);
                    }
                    else
                    {
                        Serial.println(F("AudioTables restore tables not activated"));
                    }

                    LS_Capture_collect();
                    Pointer_Performance.Delete_pointer();
                    patch_original = true;
                    P_Select_menu_elements();
                    P_Update_line_of_all_instruments();

                    Display_Performance.P_show_Performance_menu(); // Draw the menu and update its navigation layout.
                    Display_Performance.P_show_all_instruments(Patch_id);
                    Performance_led_set.Restore_all_LED();
                    Update_instruments_leds();

                    Pointer_Performance.Set_pointer_to_Patch();
                    P_pointer = Pointer_Performance.Get_pointer();

                    Clear_UI_events();

                    Print_Patch(Patch_id);
                }
                break;

                case value_P_Save: // Save this Patch
                    if (!S_Save_all_Sounds_changed())
                    {
                        Golive_with_PERFORMANCE(Patch_id);
                        break;
                    }
                    Require_FRAM(Archive.Save_Delay(Patch_id, Delay_data));
                    Require_FRAM(Archive.Save_Patch(Patch_id));
                    LS_Capture_finish_save();
                    Archive.Copy_Patch_from_RAM_to_SD(Patch_id);

                    Patch_cache_P = Patch[Patch_id];
                    S_Copy_all_Sound_to_Sound_cache_P();

                    Golive_with_PERFORMANCE(Patch_id);
                    break;

                case value_P_Clone:
                case value_P_SaveAsNew:
                    if (!P_Save_current_patch_as_new())
                    {
                        Serial.println(F("AudioTables clone not activated; current patch retained"));
                    }
                    break;

                case value_P_DropPatch:
                    // Drop patch and go back to PERFORMANCE "P_Get_first_Patch_id_existing()"

                    // YES, drop
                    if (P_Ask_if_delete_this_Patch())
                    {
                        const uint8_t deleted_patch = Patch_id;
                        int next_patch = -1;
                        for (int candidate = 0; candidate < PATCHES_MAX; ++candidate)
                        {
                            if (candidate != deleted_patch && Patch[candidate].used)
                            {
                                next_patch = candidate;
                                break;
                            }
                        }
                        if (next_patch < 0 || !P_Jump_to_Patch(next_patch))
                        {
                            break;
                        }
                        Lilla_state = PERFORMANCE;

                        Patch[deleted_patch].used = false;
                        for (auto instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
                        {
                            if (Patch[deleted_patch].Instrument[instrument_id].used)
                            {
                                Sound[Get_sound_id(deleted_patch, instrument_id)].used = false;
                            }
                        }

                        if (!S_Save_all_Sounds_changed())
                        {
                            Golive_with_PERFORMANCE(Patch_id);
                            break;
                        }
                        Require_FRAM(Archive.Save_Patch(deleted_patch));
                        LS_Capture_finish_save();
                        Archive.Copy_Patch_from_RAM_to_SD(deleted_patch);

                        P_Read_all_Patches();
                        P_Update_Patches_number();

                        S_Copy_all_Sound_to_Sound_cache_P();
                        LS_Capture_collect();
                    }

                    // NO, don't drop the patch
                    else
                    {
                        P_Select_menu_elements();
                        Display_Performance.P_show_PERFORMANCE_page(false, true);
                        Performance_led_set.Restore_all_LED();
                        Update_instruments_leds();

                        Pointer_Performance.Set_pointer_to_Patch();
                        P_pointer = Pointer_Performance.Get_pointer();
                    }
                    break;

                default:
                    PRINT_ERROR(F("ERROR: switch MISSING! "));
                    break;
                }
            }
        }
        break;

        case field_P_Patch:
        {
            result = Read_encoder_simple(EN_PB_Value);
            if (result != 0)
            {
                if (result == +1)
                {
                    patch_change = P_Get_next_Patch_id_existing();
                }
                else
                {
                    patch_change = P_Get_previous_Patch_id_existing();
                }

                if (patch_change != Patch_id)
                {
                    if (!P_Verify_is_Patch_original(Patch_id)) // Patch_id NOT original
                    {
                        action = P_Ask_if_change_Patch();

                        if (action == 0) // Exit: remain in this patch_id
                        {
                            Golive_with_PERFORMANCE(Patch_id);
                        }

                        else // change patch_id
                        {
                            PatchEditSnapshot previous;
                            if (!previous.valid)
                            {
                                audio_tables_error_pending = true;
                                return false;
                            }
                            if (action == 1) // No: discharge changings and switch patch_id
                            {
                                Patch[Patch_id] = Patch_cache_P;
                                if (!S_Pull_all_Sound_from_Sound_cache_P(&previous))
                                {
                                    previous.Restore();
                                    return false;
                                }
                            }

                            else if (action == 2) // Yes: save changings and switch patch_id
                            {
                                if (!S_Save_all_Sounds_changed())
                                {
                                    Golive_with_PERFORMANCE(Patch_id);
                                    return false;
                                }
                                Require_FRAM(Archive.Save_Delay(Patch_id, Delay_data));
                                Require_FRAM(Archive.Save_Patch(Patch_id));
                                LS_Capture_finish_save();
                                Archive.Copy_Patch_from_RAM_to_SD(Patch_id);
                            }

                            if (!P_Jump_to_Patch(patch_change))
                            {
                                previous.Restore();
                                return false;
                            }

                            Patch_id_old = Patch_id;
                        }
                    }

                    else // Patch_id IS original
                    {
                        if (!P_Jump_to_Patch(patch_change))
                        {
                            return false;
                        }

                        Patch_id_old = Patch_id;
                    }
                }
            }
        }
        break;

        case field_P_Instrument:
        {
            // Enter Instrument_inside area
            if (Read_pushbutton(EN_PB_Select))
            {
                Pointer_Performance.Move_pointer_from_Instrument_to_inside();
                P_pointer = Pointer_Performance.Get_pointer();

                Clear_UI_events();
            }
        }
        break;

        case field_P_Instrument_inside:
        {
            const int instrument_id = static_cast<int>(P_pointer.instrument_id); // static_cast<int>(Pointer_Performance.Get_pointer().instrument_id);
            const int element = P_pointer.element;
            const int sound_id = Get_sound_id(Patch_id, instrument_id);

            // Exit from Instrument_inside area
            if (Read_pushbutton(EN_PB_Select))
            {
                Pointer_Performance.Move_pointer_from_inside_to_Instrument();
                P_pointer = Pointer_Performance.Get_pointer();

                Clear_UI_events();
            }

            switch (Pointer_Performance.Get_pointer().element)
            {
            case value_P_Lock: // Lock
            {
                result = Read_encoder_simple(EN_PB_Value);

                if (result == 1)
                {
                    AudioNoInterrupts();
                    Patch[Patch_id].Instrument[instrument_id].lock = true;
                    Players_Manager.Update_Preset_lock(Patch_id, instrument_id);
                    Players_Manager.Multicast_reset_pitch_bend_effects(instrument_id);
                    AudioInterrupts();

                    P_Macro_Instrument_editing(Patch_id, instrument_id, element);
                    P_UpdatePatchOriginalAndMenu();
                }

                else if (result == -1)
                {
                    AudioNoInterrupts();
                    Patch[Patch_id].Instrument[instrument_id].lock = false;
                    Players_Manager.Update_Preset_lock(Patch_id, instrument_id);
                    Players_Manager.Broadcast_restore_pitch_bend_and_effects(instrument_id, pitch_bend_value[Get_midi_channel(Patch_id, instrument_id)]);
                    AudioInterrupts();

                    P_Macro_Instrument_editing(Patch_id, instrument_id, element);
                    P_UpdatePatchOriginalAndMenu();
                }
            }
            break;

            case value_P_Precedence: // Precedence
            {
                result = Read_encoder_simple(EN_PB_Value);
                if (result == 1)
                {
                    AudioNoInterrupts();
                    Patch[Patch_id].Instrument[instrument_id].precedence = true;
                    Players_Manager.Update_Preset_precedence(Patch_id, instrument_id);
                    AudioInterrupts();

                    P_Macro_Instrument_editing(Patch_id, instrument_id, element);
                    P_UpdatePatchOriginalAndMenu();
                }
                else if (result == -1)
                {
                    AudioNoInterrupts();
                    Patch[Patch_id].Instrument[instrument_id].precedence = false;
                    Players_Manager.Update_Preset_precedence(Patch_id, instrument_id);
                    AudioInterrupts();

                    P_Macro_Instrument_editing(Patch_id, instrument_id, element);
                    P_UpdatePatchOriginalAndMenu();
                }
            }
            break;

            case value_P_Midi: // Midi (channel)
            {
                result = Read_encoder_simple(EN_PB_Value);
                if (result != 0)
                {
                    midi_channel_change = S_Get_midi_channel_from_Sound(sound_id);

                    if (result == 1)
                    {
                        if (midi_channel_change < 15)
                        {
                            ++midi_channel_change;
                        }
                    }
                    else // -1
                    {
                        if (midi_channel_change > 0)
                        {
                            --midi_channel_change;
                        }
                    }
                    if (midi_channel_change != S_Get_midi_channel_from_Sound(sound_id))
                    {
                        AudioNoInterrupts();
                        Players_Manager.Multicast_release_players(sound_id);
                        P_Reset_map_Instrument_for_notes(instrument_id);
                        S_Set_midi_channel_for_Sound(sound_id, midi_channel_change);
                        Update_map_Instrument_for_notes(Patch[Patch_id].Instrument[instrument_id].from_note, Patch[Patch_id].Instrument[instrument_id].to_note, instrument_id);
                        Players_Manager.Update_Preset_midi_channel(Patch_id, instrument_id);
                        AudioInterrupts();

                        P_Macro_Instrument_editing(Patch_id, instrument_id, element);
                        P_UpdatePatchOriginalAndMenu();
                    }
                }
            }
            break;

            case value_P_RootKey: // Root key
                changed = Read_encoder(EN_PB_Value, Patch[Patch_id].Instrument[instrument_id].root_key, 127, 0, 1);
                if (Read_pushbutton(EN_PB_Value))
                {
                    changed = (changed || Patch[Patch_id].Instrument[instrument_id].root_key != 60);
                    Patch[Patch_id].Instrument[instrument_id].root_key = 60; // Restore middle C; use the normal edit path to retune active players.
                }
                if (changed)
                {
                    AudioNoInterrupts();
                    Players_Manager.Multicast_change_players_notes(Patch_id, instrument_id);
                    AudioInterrupts();

                    P_Macro_Instrument_editing(Patch_id, instrument_id, element);
                    P_UpdatePatchOriginalAndMenu();
                }
                break;

            case value_P_FromKey: // From Key
            {
                result = Read_encoder_simple(EN_PB_Value);
                const bool reset_key = Read_pushbutton(EN_PB_Value); // Consume the click even when rotation occurs in the same loop.
                if (result != 0 || reset_key)
                {
                    changed = false;
                    if (reset_key)
                    {
                        from_key_change = 0; // Extend the lower note boundary through the existing mapping update.
                        changed = Patch[Patch_id].Instrument[instrument_id].from_note != from_key_change;
                    }
                    else if (result == 1 && Patch[Patch_id].Instrument[instrument_id].from_note < Patch[Patch_id].Instrument[instrument_id].to_note)
                    {
                        from_key_change = Patch[Patch_id].Instrument[instrument_id].from_note + 1;
                        changed = true;
                    }
                    else if (result == -1 && Patch[Patch_id].Instrument[instrument_id].from_note > 0)
                    {
                        from_key_change = Patch[Patch_id].Instrument[instrument_id].from_note - 1;
                        changed = true;
                    }
                    if (changed)
                    {
                        AudioNoInterrupts();
                        Players_Manager.Change_from_key(Patch_id, instrument_id, from_key_change);
                        AudioInterrupts();

                        P_Macro_Instrument_editing(Patch_id, instrument_id, element);
                        P_UpdatePatchOriginalAndMenu();
                    }
                }
            }
            break;

            case value_P_ToKey: // To key
            {
                result = Read_encoder_simple(EN_PB_Value);
                const bool reset_key = Read_pushbutton(EN_PB_Value); // Consume the click even when rotation occurs in the same loop.
                if (result != 0 || reset_key)
                {
                    changed = false;
                    if (reset_key)
                    {
                        to_key_change = 127; // Extend the upper note boundary through the existing mapping update.
                        changed = Patch[Patch_id].Instrument[instrument_id].to_note != to_key_change;
                    }
                    else if (result == 1 && Patch[Patch_id].Instrument[instrument_id].to_note < 127)
                    {
                        to_key_change = Patch[Patch_id].Instrument[instrument_id].to_note + 1;
                        changed = true;
                    }

                    else if (result == -1 && Patch[Patch_id].Instrument[instrument_id].to_note > Patch[Patch_id].Instrument[instrument_id].from_note)
                    {
                        to_key_change = Patch[Patch_id].Instrument[instrument_id].to_note - 1;
                        changed = true;
                    }

                    if (changed)
                    {
                        AudioNoInterrupts();
                        Players_Manager.Change_to_key(Patch_id, instrument_id, to_key_change);
                        AudioInterrupts();

                        P_Macro_Instrument_editing(Patch_id, instrument_id, element);
                        P_UpdatePatchOriginalAndMenu();
                    }
                }
            }
            break;

            case value_P_Pan: // Pan
                if (Read_encoder(EN_PB_Value, Sound[sound_id].pan, 16, -16, 1))
                {
                    AudioNoInterrupts();
                    Players_Manager.Update_Preset_pan(Patch_id, instrument_id);
                    Players_Manager.Multicast_pan(instrument_id);
                    AudioInterrupts();

                    P_Macro_Instrument_editing(Patch_id, instrument_id, element);
                    P_UpdatePatchOriginalAndMenu();
                }

                // Set PAN to center
                else if (Read_pushbutton(EN_PB_Value))
                {
                    Sound[sound_id].pan = 0;

                    AudioNoInterrupts();
                    Players_Manager.Update_Preset_pan(Patch_id, instrument_id);
                    Players_Manager.Multicast_pan(instrument_id);
                    AudioInterrupts();

                    P_Macro_Instrument_editing(Patch_id, instrument_id, element);
                    P_UpdatePatchOriginalAndMenu();
                }

                break;

            case value_P_Gain: // Gain
                if (Read_encoder(EN_PB_Value, Sound[sound_id].gain, 40, 0, 1))
                {
                    AudioNoInterrupts();
                    Players_Manager.Update_Preset_volume(Patch_id, instrument_id, Patch_volume_gain(volume_patch));
                    Players_Manager.Multicast_volume_for_instrument_edit(instrument_id);
                    AudioInterrupts();

                    P_Macro_Instrument_editing(Patch_id, instrument_id, element);
                    P_UpdatePatchOriginalAndMenu();
                }
                break;
            }
        }
        break;
        }

        // Instrument volume changed from MIDI CC
        if (display_instrument_volume_flag)
        {
            Display_Performance.P_show_Instrument_description(Patch_id, instrument_volume_changed, true);

            // restore LED
            if (Performance_led_set.Read_LED_activity(instrument_volume_changed) > 0)
            {
                Display_Performance.Led_PERFORMANCE_instrument(instrument_volume_changed, true);
            }
            else
            {
                Display_Performance.Led_PERFORMANCE_instrument(instrument_volume_changed, false);
            }
            display_instrument_volume_flag = false;
        }

        // Switch to SOUND_EDIT
        for (auto Inst_id = 0; Inst_id < INSTRUMENTS; ++Inst_id)
        {
            if (Read_pushbutton(PB_Sound[Inst_id]))
            {
                if (Patch[Patch_id].Instrument[Inst_id].used)
                {
                    Instrument_id = Inst_id;

                    Lilla_state_0 = PERFORMANCE;
                    Lilla_state = SOUND_EDIT;

                    Sound_id = Get_sound_id(Patch_id, Instrument_id);

                    samples_in_file = Get_samples_in_raw_file(Sound[Sound_id].file);
                    Noclick_max = S_Calc_Noclick_max(Preset[Instrument_id].use_Wavetable);
                    S_trim_step = S_Calc_trim_step(trim_speed);

                    Display_Sound.Show_SOUND_page(Patch_id, Instrument_id);

                    S_sound_original = S_Verify_is_Sound_original(Sound_id);
                    S_Select_menu_elements();
                    Display_Sound.Show_SOUND_menu(); // displays the menu and updates "SO_menu_max" used by encoder_menu

                    // Reset pointer
                    Pointer_Sound.Set_pointer_to_first_menu_element();
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
                    Show_popup_text(message, ILI9341_WHITE, ILI9341_RED, display_coordinate_y(7));
                    delay(1000);
                    Show_popup_text(message, ILI9341_BLACK, ILI9341_BLACK, display_coordinate_y(7));
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
                Lilla_state_0 = PERFORMANCE;
                Patch_id_old = Patch_id;
                Switch_to_MIXER();
            }
            break;

            case SwToolsDelay:
            {
                Lilla_state_0 = PERFORMANCE;
                Patch_id_old = Patch_id;
                Golive_DELAY_SETTINGS();
            }
            break;

            case SwToolsSetup:
            {
                Lilla_state_0 = PERFORMANCE;
                Patch_id_old = Patch_id;
                Golive_SETUP();
            }
            break;

            case SwToolsTest:
            {
                Lilla_state_0 = PERFORMANCE;
                Patch_id_old = Patch_id;
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
                Patch_id_old = Patch_id;
                Switch_to_DIRECT_SAMPLING();
            }
            break;

            case SwModesLiveSampler:
            {
                Patch_id_old = Patch_id;
                Switch_from_PERFORMANCE_to_LIVE_SAMPLING();
            }
            break;

            case SwModesPerformance:
                break;

            case SwModesMidiLoop:
            {
                Switch_from_PERFORMANCE_to_MIDI_LOOP();
            }
            break;
            }
        }
    }
    return true;
}
