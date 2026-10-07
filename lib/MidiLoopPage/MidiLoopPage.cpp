/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 */

#include <Audio.h>
#include "MidiLoopPage.h"
#include "main.h"
#include "DelayPage.h"
#include "MixerPage.h"
#include "MidiMonitorPage.h"
#include "SetupPage.h"
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
#include "SharedLoop.h"
#include "AudioPlayer.h"
#include "PlayersStatistics.h"
#include "PointerMidiLoop.h"
#include "LoopLedSet.h"
#include "LoopMetronomo.h"
#include "GlobalDisplayCommon.h"
#include "GlobalDisplayMidiLoop.h"

static LOOP_field_description_struct LOOP_local_pointer; // Current MIDI Loop menu, track or parameter selection.

void Handle_Midi_loop(void)
{
    // *************************************************************
    // ******************       MIDI_LOOP      *********************
    // *************************************************************

    if (Lilla_state == MIDI_LOOP)
    {
        if (Display_MidiLoop.Update_save_failed())
        {
            Pointer_MidiLoop.Show_pointer(true);
        }

        // Change volume_patch
        if (Read_encoder(EN_PB_LineOutVol, volume_patch, PATCH_VOLUME_MAX, 0, 1))
        {
            AudioNoInterrupts();
            Players_Manager.Update_all_Preset_volume(Patch_id, Patch_volume_gain(volume_patch));
            Players_Manager.Broadcast_volume();
            AudioInterrupts();

            Display_Common.P_Patch_volume_value(true);
        }

        // Move pointerMenu
        if (LOOP_events[MASTER_TRACK] > 0)
        {
            result = Read_encoder_simple(EN_PB_Select);
            if (result != 0)
            {
                Pointer_MidiLoop.Move_pointer(result);
                LOOP_local_pointer = Pointer_MidiLoop.Get_pointer();

                Clear_UI_events();
            }
        }

        // Change LOOP_id
        result = Read_encoder_simple(EN_PB_Loop);
        if (result != 0)
        {
            int new_loop_id;
            if (result == +1)
            {
                new_loop_id = LOOP_Get_next_loop_id_in_SD(LOOP_id);
            }
            else if (result == -1)
            {
                new_loop_id = LOOP_Get_previous_loop_id_in_SD(LOOP_id);
            }

            PRINT_CONTROL_POINT(new_loop_id);

            if (new_loop_id == -1)
            {
                LOOP_run_button_state = true;
            }

            else if (new_loop_id != LOOP_id)
            {
                // Validation leaves the current loop untouched; a failed second pass clears it safely.
                const bool loaded = LOOP_Copy_midi_loop_from_SD_to_RAM(new_loop_id);
                if (loaded || LOOP_events[MASTER_TRACK] == 0)
                {
                    if (loaded)
                    {
                        LOOP_id = new_loop_id;
                    }

                    // Show LOOP_id on display
                    Display_MidiLoop.Show_loop_id();

                    // Show LOOP_time on display
                    Display_MidiLoop.Loop_total_time();

                    // Show track infos on display
                    for (auto track = 0; track < TRACKS; ++track)
                    {
                        Display_MidiLoop.Show_track_all_data(track);
                    }

                    // Update menu and pointer
                    Pointer_MidiLoop.Show_pointer(false);
                    LOOP_select_menu_elements();
                    Display_MidiLoop.Show_menu();
                    Pointer_MidiLoop.Set_pointer_to_first_menu_element();
                    LOOP_local_pointer = Pointer_MidiLoop.Get_pointer();

                    Clear_UI_events();

                    // Switch off all tracks LEDs on display
                    Loop_led_set.Request_all_LED_switch_off();

                    if (LOOP_time > 0)
                    {
                        LOOP_metronomo.Led_ON(0);
                        LOOP_metronomo.Setup(LOOP_time);
                    }

                    // restart clock
                    LOOP_restart_clock();

                    // Set first event for each track
                    for (auto track = 0; track < TRACKS; ++track)
                    {
                        LOOP_play_event[track] = 0;
                    }

                    // Sort events by timestamp
                    for (auto track = 0; track < TRACKS; ++track)
                    {
                        LOOP_set_time_order(track);
                    }

                    // Simulate all tracks Start/Stop, with all tracks active
                    LOOP_run_button_state = LOOP_events[MASTER_TRACK] == 0;

                    // Save track states before stopping
                    for (auto track = 0; track < TRACKS; ++track)
                    {
                        LOOP_track_run_memo[track] = LOOP_events[track] > 0;
                        LOOP_track_run[track] = false;
                    }
                }

                // Report
                if (loaded)
                {
                    Serial.println("Loop uploaded; data in RAM:");
                    LOOP_Print_midi_loop_complete_data(LOOP_id);
                }
                else
                {
                    Serial.println(F("Loop import failed."));
                }
            }
        }

        // Use the first track encoder rotation on a menu item to select LEVEL.
        if (LOOP_events[MASTER_TRACK] > 0 && LOOP_local_pointer.field_name == field_LOOP_Menu)
        {
            bool select_level = false;
            for (auto track = 0; track < TRACKS; ++track)
            {
                if (Read_encoder_simple(EN_PB_Track[track]) != 0)
                {
                    select_level = true;
                }
            }
            if (select_level)
            {
                Pointer_MidiLoop.Set_pointer_to_level();
                LOOP_local_pointer = Pointer_MidiLoop.Get_pointer();
            }
        }

        // Recording
        for (auto track = 0; track < TRACKS; ++track)
        {
            if (Read_pushbutton(PB_Rec[track]) && LOOP_run_button_state && (track == MASTER_TRACK || LOOP_events[MASTER_TRACK] != 0))
            {
                LOOP_learning_track = track; // LOOP_learning_track e' il nuovo loop

                // High priority

                AudioNoInterrupts();
                if (LOOP_events[LOOP_learning_track] != 0)
                {
                    // se si tratta di MASTER_TRACK si fermano e cancellano tutti i track
                    if (LOOP_learning_track == MASTER_TRACK)
                    {
                        // Interrompi i Player che eseguono note di qualsiasi track
                        Players_Manager.Release_all_players_loop();

                        for (auto local_track = 0; local_track < TRACKS; ++local_track)
                        {
                            // Interrompe la lettura
                            LOOP_track_run[local_track] = false;

                            // Cancella il loop
                            LOOP_events[local_track] = 0;

                            // Resetta slide
                            LOOP_slide[local_track] = 0;

                            // Resetta pitch
                            LOOP_pitch_int[local_track] = 0;
                        }

                        // ferma il metronomo
                        LOOP_metronomo_run = false;
                        // LOOP_metronomo_flag_OUT = false; // da eliminare

                        // Annulla (se ci fosse) l'ultima richiesta di aggiornamento del metronomo proveniente da MidiReader
                        LOOP_metronomo_flag_IN[1] = false;

                        // Nuovo loop
                        LOOP_original = true;
                    }
                    else
                    {
                        if (LOOP_track_run[LOOP_learning_track])
                        {
                            // Interrompi i Player di LOOP_learning_track
                            Players_Manager.Release_all_players_loop(LOOP_learning_track);
                        }
                        // Interrompe la lettura
                        LOOP_track_run[LOOP_learning_track] = false;

                        // Cancella il loop
                        LOOP_events[LOOP_learning_track] = 0; // Loop cancellato

                        // Resetta slide
                        LOOP_slide[LOOP_learning_track] = 0;

                        // Resetta pitch
                        LOOP_pitch_int[LOOP_learning_track] = 0;

                        // loop esistente
                        LOOP_original = false;
                    }
                }
                AudioInterrupts();

                // LED_Rec ON
                Shifters_manager.Switch_led(LED_Rec[LOOP_learning_track], true);

                // Display update, and other low priority procedures
                if (LOOP_learning_track == MASTER_TRACK)
                {
                    // Spegni i led del metronomo
                    LOOP_metronomo.Leds_off(); // va eseguito fuori da AudioNoInterrupt()

                    // nomina loop_id
                    LOOP_id = NEW_LOOP;
                    Display_MidiLoop.Show_loop_id();

                    LOOP_time = 0;
                    Display_MidiLoop.Loop_total_time(); // accanto ai led del metronomo appare il tempo totale 0.0s
                }

                // Se si tratta del track master (0) non ancora esistente, oppure si tratta di un altro track ma con track master esistente
                if (LOOP_learning_track == MASTER_TRACK || LOOP_events[MASTER_TRACK] != 0)
                {
                    // Show (or delete) all tracks infos
                    for (auto track = 0; track < TRACKS; ++track)
                    {
                        Display_MidiLoop.Show_track_all_data(track);

                        if (LOOP_events[track] == 0)
                        {
                            Pointer_MidiLoop.Show_pointerTrack(track, false);
                        }
                    }

                    // Display "n-REC"
                    Display_MidiLoop.Loop_REC_advice(LOOP_learning_track, true);

                    // Switch off all tracks LEDs
                    Loop_led_set.Request_all_LED_switch_off();

                    // Update menu and pointer
                    Pointer_MidiLoop.Show_pointer(false);
                    LOOP_select_menu_elements();
                    Display_MidiLoop.Show_menu();
                    Pointer_MidiLoop.Set_pointer_to_first_menu_element();
                    LOOP_local_pointer = Pointer_MidiLoop.Get_pointer();

                    Clear_UI_events();

                    // Prepare learning
                    LOOP_learn_clock = 0;
                    LOOP_elements = 0;      // ancora nessun evento
                    LOOP_learn_flag = true; // avvia il learning

                    // Feedback
                    Serial.println(F("Learning inizializzato!"));

                    // Learning
                    while (LOOP_learn_flag)
                    {
                        if (Display_MidiLoop.Update_save_failed())
                        {
                            Pointer_MidiLoop.Show_pointer(true);
                        }
                        Shifters_manager.Update();

                        // Stop learning
                        if (Read_pushbutton(PB_Rec[LOOP_learning_track]))
                        {
                            // Chiude il learning
                            LOOP_learn_flag = false;

                            // Feedback
                            Serial.println("Loop correttamente chiuso manualmente!");
                            break;
                        }

                        // After 20 secons without events --> cancel track
                        else if (LOOP_elements == 0 && LOOP_learn_clock > 20000)
                        {
                            // Close learning
                            LOOP_learn_flag = false;

                            // Feedback
                            Serial.println(F("Loop chiuso e cancellato perche' dimenticato aperto!"));
                            break;
                        }

                        // se si tratta del track master (0), MidiReader chiede l'accensione del primo led del metronomo quando riceve il primo NoteOn
                        else if (LOOP_learning_track == MASTER_TRACK && LOOP_metronomo_flag_IN[0])
                        {
                            LOOP_metronomo_flag_IN[0] = false;

                            // Ask metronomo to switch on first led (metronomo is not runnig)
                            LOOP_metronomo.Led_ON(0);
                        }

                        // Continously update LEDs
                        Update_instruments_leds();

                        // se NON si tratta del track master (0) c'e' l'aggiornamento continuo del metronomo
                        if (LOOP_metronomo_flag_IN[1])
                        {
                            LOOP_metronomo_flag_IN[1] = false;
                            LOOP_metronomo.Update();
                            LOOP_metronomo.metro_time += LOOP_metronomo.Read_metro_delta_ms();
                        }
                    }

                    // Learnig closed. From here: LOOP_learn_flag == false
                    LOOP_events[LOOP_learning_track] = LOOP_elements; // se LOOP_events[LOOP_learning_track] == 0 significa che il LOOP_learning_track ÃƒÆ’Ã‚Â¨ vuoto e non viene eseguito

                    Clear_UI_events();

                    // LED_Rec OFF
                    Shifters_manager.Switch_led(LED_Rec[LOOP_learning_track], false);

                    // Feedback
                    Serial.println("Learning closed!");

                    // The new track is valid (contains events)
                    if (LOOP_events[LOOP_learning_track] > 0)
                    {
                        // If MASTER_TRACK
                        if (LOOP_learning_track == MASTER_TRACK)
                        {
                            // Setup di LOOP_time (durata di tutti i loop)
                            LOOP_time = LOOP_learn_clock;

                            // Restart clock
                            LOOP_restart_clock();

                            // Reset stretch
                            LOOP_stretch_int = 100;
                            LOOP_stretch = 1.0;

                            // Report
                            Serial.print("LOOP_time:");
                            Serial.println(LOOP_time);
                        }

                        // aggiungi info di slide
                        LOOP_slide[LOOP_learning_track] = 0; // ms

                        // aggiungi info di pitch
                        LOOP_pitch_int[LOOP_learning_track] = 0; // 0 --> pitch = 1.0

                        // evento di avvio
                        LOOP_play_event[LOOP_learning_track] = 0;

                        // calcolo istante esecuzione evento di avvio (LOOP_play_time) e prossimo switch del metronomo rispetto a LOOP_Clock
                        if (LOOP_learning_track == MASTER_TRACK)
                        {
                            LOOP_play_time[LOOP_learning_track] = 0;

                            // setup metronomo
                            LOOP_metronomo.Setup(LOOP_time);

                            // set next metronomo step
                            LOOP_metronomo.metro_time = 0 + LOOP_metronomo.Read_metro_delta_ms();

                            // metronomo switch-on
                            LOOP_metronomo_run = true;
                        }
                        else
                        {
                            LOOP_play_time[LOOP_learning_track] = LOOP_Clock_time_from_virtual_time(LOOP_element[LOOP_learning_track][0].time);
                        }

                        // effettua l'ordinamento temporale degli eventi
                        LOOP_set_time_order(LOOP_learning_track);

                        // avvio
                        LOOP_track_run[LOOP_learning_track] = true;

                        // update menu
                        if (LOOP_id == NEW_LOOP)
                        {
                            LOOP_original = true;
                        }

                        else if (LOOP_learning_track == MASTER_TRACK && LOOP_original)
                        {
                            LOOP_original = false;
                        }

                        else if (LOOP_learning_track > MASTER_TRACK)
                        {
                            LOOP_original = false;
                        }

                        // Update menu and pointer on display
                        Pointer_MidiLoop.Show_pointer(false);
                        LOOP_select_menu_elements();
                        Display_MidiLoop.Show_menu();

                        // Report
                        Serial.println(" **************** ");
                        Serial.print("eventi:");
                        Serial.println(LOOP_events[LOOP_learning_track]);
                        for (uint32_t event = 0; event < LOOP_events[LOOP_learning_track]; ++event)
                        {
                            Serial.print(event);
                            Serial.print(" time:");
                            Serial.print(LOOP_element[LOOP_learning_track][event].time);
                            Serial.print(" midi_channel:");
                            Serial.print(LOOP_element[LOOP_learning_track][event].midi_channel);
                            Serial.print(" note_number:");
                            Serial.print(LOOP_element[LOOP_learning_track][event].note_number);
                            Serial.print(" velocity:");
                            Serial.print(LOOP_element[LOOP_learning_track][event].velocity);
                            Serial.print(" note_on:");
                            Serial.println(LOOP_element[LOOP_learning_track][event].note_on ? "NoteOn" : "NoteOff");
                        }

                        Serial.print("Ordine temporale degli eventi: ");
                        for (uint32_t event = 0; event < LOOP_events[LOOP_learning_track]; ++event)
                        {
                            Serial.print(LOOP_time_order[LOOP_learning_track][event]);
                            Serial.print(" - ");
                        }
                        Serial.println();
                        Serial.print("Si inizia con l'evento:");
                        Serial.println(LOOP_play_event[LOOP_learning_track]);
                        Serial.print("Tra ms:");
                        Serial.println(LOOP_play_time[LOOP_learning_track] - LOOP_Clock());
                        Serial.println("*************");
                    }

                    else if (LOOP_events[MASTER_TRACK] == 0)
                    {
                        // spegni il primo led se acceso
                        LOOP_metronomo.Leds_off();

                        // cancella la richiesta per il primo led se arrivata (non dovrebbe essere possibile)
                        LOOP_metronomo_flag_IN[0] = false;
                    }

                    Display_MidiLoop.Show_track_all_data(track);

                    // pointer
                    Pointer_MidiLoop.Set_pointer_to_first_menu_element();
                    LOOP_local_pointer = Pointer_MidiLoop.Get_pointer();

                    // Display loop time
                    if (LOOP_learning_track == MASTER_TRACK)
                    {
                        Display_MidiLoop.Loop_total_time();
                    }

                    Clear_UI_events();
                }
            }

            // Change values
            if (LOOP_events[track] > 0)
            {

                // Track start-stop
                if (Read_pushbutton(EN_PB_Track[track]))
                {
                    // Stop track
                    if (LOOP_track_run[track])
                    {
                        LOOP_track_run[track] = false;

                        // interrompi i Player di track

                        AudioNoInterrupts();
                        Players_Manager.Release_all_players_loop(track);
                        AudioInterrupts();

                        // spegni i led del loop
                        Loop_led_set.Request_track_LED_switch_off(track);
                    }

                    // Play track
                    else
                    {
                        // accendi il primo led del metronomo
                        // LOOP_metronomo.Led_ON(0);

                        AudioNoInterrupts();
                        // dopo uno stop a tutti i loop, alla prima ripartenza va azzerato LOOP_clock e va fatto ripartire il metronomo
                        if (!LOOP_metronomo_run)
                        {
                            // se LOOP_run_button_state == false va ripristinato
                            LOOP_run_button_state = true;

                            LOOP_restart_clock();

                            // calcolo prossimo evento metronomo
                            LOOP_metronomo.metro_time = 0 + LOOP_metronomo.Read_metro_delta_ms();

                            // avvia il metronomo
                            LOOP_metronomo_run = true;
                        }
                        LOOP_restart_procedure(track); // Procedura di ripartenza
                        AudioInterrupts();
                    }
                }

                if (LOOP_local_pointer.field_name == field_LOOP_TrackValues)
                {
                    // Change track parameters
                    switch (LOOP_local_pointer.track_value_element)
                    {
                    // Slide temporale
                    case value_LOOP_shift:
                    {
                        result = Read_encoder_simple(EN_PB_Track[track]);
                        if (result != 0)
                        {
                            LOOP_original = false;

                            int jump;
                            if (result == 1)
                            {
                                jump = 100;
                            }
                            else
                            {
                                if (LOOP_time >= 100)
                                {
                                    jump = LOOP_time - 100;
                                }
                                else
                                {
                                    jump = 0;
                                }
                            }

                            // Report
                            Serial.print("Shift ms:");
                            Serial.println(jump);

                            AudioNoInterrupts();
                            for (uint32_t event = 0; event < LOOP_events[track]; ++event)
                            {
                                LOOP_element[track][event].time = (LOOP_element[track][event].time + jump) % LOOP_time;
                            }

                            // Stop Players for this track
                            Players_Manager.Release_all_players_loop(track);

                            // Sort events by timestamp
                            LOOP_set_time_order(track);

                            // Restart procedure
                            LOOP_restart_procedure(track);
                            AudioInterrupts();

                            // Switch off track LEDs
                            Loop_led_set.Request_track_LED_switch_off(track);

                            LOOP_slide[track] = (LOOP_slide[track] + jump) % LOOP_time;

                            Display_MidiLoop.Show_track_all_data(track);
                        }

                        // REMOVED - Cancel time slide
                        /*
                            int jump = LOOP_time - LOOP_slide[track];

                            AudioNoInterrupts();
                            for (uint32_t event = 0; event < LOOP_events[track]; ++event)
                            {
                                LOOP_element[track][event].time = (LOOP_element[track][event].time + jump) % LOOP_time;
                            }

                            // Stop all track Players
                            Players_Manager.Release_all_players_loop(track);

                            // Effettua l'ordinamento temporale degli eventi
                            LOOP_set_time_order(LOOP_learning_track);

                            // Restart procedure
                            LOOP_restart_procedure(track);
                            AudioInterrupts();

                            // Switch off track LEDs
                            Loop_led_set.Request_track_LED_switch_off(track);

                            LOOP_slide[track] = (LOOP_slide[track] + jump) % LOOP_time;
                            Display_MidiLoop.Show_track_all_data(track);
                        */
                    }
                    break;

                    case value_LOOP_pitch:
                    {
                        int next_pitch = LOOP_pitch_int[track];
                        if (Read_encoder(EN_PB_Track[track], next_pitch, 24, -24, 1))
                        {
                            AudioNoInterrupts();
                            Players_Manager.Multicast_stop_players_for_loop_track(track);
                            LOOP_pitch_int[track] = next_pitch;
                            AudioInterrupts();

                            LOOP_original = false;

                            Display_MidiLoop.Show_track_all_data(track);

                            // Report
                            Serial.print("LOOP_pitch_int: ");
                            Serial.println(LOOP_pitch_int[track]);
                        }
                    }
                    break;

                    case value_LOOP_level:
                    {
                        if (Read_encoder(EN_PB_Track[track], LOOP_volume_int[track], 40, 0, 1))
                        {
                            LOOP_original = false;

                            AudioNoInterrupts();
                            LOOP_volume[track] = LOOP_volume_int[track] / 20.0f;
                            Players_Manager.Multicast_volume_for_MIDI_LOOP_running(track, LOOP_volume[track]);
                            AudioInterrupts();

                            Display_MidiLoop.Show_track_all_data(track);

                            // Report
                            Serial.print("LOOP_volume: ");
                            Serial.println(LOOP_volume[track]);
                        }
                    }
                    break;
                    }
                }
            }
        }

        // All track active commands (if MASTER_TRACK exists)
        if (LOOP_events[MASTER_TRACK] > 0)
        {
            // Update metronomo
            if (LOOP_metronomo_flag_IN[1])
            {
                LOOP_metronomo_flag_IN[1] = false;
                LOOP_metronomo.Update();
                LOOP_metronomo.metro_time += LOOP_metronomo.Read_metro_delta_ms();
            }

            // Choose menu item
            if (LOOP_local_pointer.field_name == field_LOOP_Menu)
            {
                if (Read_pushbutton(EN_PB_Select))
                {
                    switch (LOOP_local_pointer.menu_element)
                    {
                    case value_LOOP_New:
                    {
                        LOOP_stop_and_reset_runnig_loop_data(); // LOOP_track_run[track] = false; LOOP_metronomo_run == false; LOOP_metronomo_flag_IN[1] = false;
                        LOOP_id = NEW_LOOP;
                        LOOP_original = true;
                        LOOP_run_button_state = true;

                        Golive_with_MIDI_LOOP(true);
                    }
                    break;

                    case value_LOOP_Save:
                    {
                        const int saved_loop_id = LOOP_id == NEW_LOOP ? LOOP_Get_first_loop_id_free() : LOOP_id;
                        if (saved_loop_id < 0 || !LOOP_Copy_midi_loop_from_RAM_to_SD(saved_loop_id))
                        {
                            Serial.println(F("Loop save failed; RAM loop remains unsaved."));
                            Display_MidiLoop.Show_save_failed();
                            Clear_UI_events();
                            break;
                        }
                        LOOP_id = saved_loop_id;
                        LOOP_original = true;
                        Display_MidiLoop.Show_loop_id();

                        // Update menu and pointerMenu
                        Pointer_MidiLoop.Show_pointer(false);
                        LOOP_select_menu_elements();
                        Display_MidiLoop.Show_menu();
                        Pointer_MidiLoop.Set_pointer_to_first_menu_element();
                        LOOP_local_pointer = Pointer_MidiLoop.Get_pointer();

                        Clear_UI_events();
                    }
                    break;

                    case value_LOOP_SaveAsNew:
                    {
                        result = LOOP_Get_first_loop_id_free();
                        if (result >= 0)
                        {
                            if (!LOOP_Copy_midi_loop_from_RAM_to_SD(result))
                            {
                                Serial.println(F("Loop Save As New failed; loop ID unchanged."));
                                Display_MidiLoop.Show_save_failed();
                                Clear_UI_events();
                                break;
                            }
                            LOOP_id = result;

                            // Update menu
                            LOOP_original = true;

                            // Update menu and pointerMenu
                            Pointer_MidiLoop.Show_pointer(false);
                            LOOP_select_menu_elements();
                            Display_MidiLoop.Show_menu();
                            Pointer_MidiLoop.Set_pointer_to_first_menu_element();
                            LOOP_local_pointer = Pointer_MidiLoop.Get_pointer();

                            // Update loop_id
                            Display_MidiLoop.Show_loop_id();

                            Clear_UI_events();
                        }
                        else
                        {
                            Display_MidiLoop.Show_save_failed();
                            Clear_UI_events();
                        }
                    }
                    break;

                    case value_LOOP_Delete:
                    {
                        if (LOOP_id != NEW_LOOP && !LOOP_Delete_midi_loop_from_SD(LOOP_id))
                        {
                            Serial.println(F("Loop delete failed."));
                            Clear_UI_events();
                            break;
                        }

                        // new
                        LOOP_stop_and_reset_runnig_loop_data(); // LOOP_track_run[track] = false; LOOP_metronomo_run == false; LOOP_metronomo_flag_IN[1] = false;
                        LOOP_id = NEW_LOOP;
                        LOOP_original = true;
                        LOOP_run_button_state = true;

                        Golive_with_MIDI_LOOP(true);
                    }
                    break;
                    }
                }
            }

            // Change tempo
            if (Read_encoder_inverse(EN_PB_Tempo, LOOP_stretch_int, 198, 1, 1))
            {
                AudioNoInterrupts();
                // Memorizza il tempo virtuale attuale
                LOOP_clock_memo = LOOP_Clock();

                // Update LOOP_stretch
                if (LOOP_stretch_int <= 100)
                {
                    LOOP_stretch = LOOP_stretch_int / 100.0;
                }
                else
                {
                    LOOP_stretch = 1.0 / (2.0f - LOOP_stretch_int / 100.0f);
                }

                // Update LOOP_clock
                LOOP_clock = LOOP_clock_memo * LOOP_stretch;
                AudioInterrupts();

                Display_MidiLoop.Loop_total_time();

                // Report
                Serial.print("LOOP_stretch: ");
                Serial.println(LOOP_stretch);
            }

            // Back to original tempo
            if (Read_pushbutton(EN_PB_Tempo))
            {
                AudioNoInterrupts();
                // Memorizza il tempo virtuale attuale
                LOOP_clock_memo = LOOP_Clock();

                // Aggiorna LOOP_stretch
                LOOP_stretch_int = 100;
                LOOP_stretch = 1.0;

                // Ricalcolo LOOP_clock
                LOOP_clock = LOOP_clock_memo;
                AudioInterrupts();

                Display_MidiLoop.Loop_total_time();

                Serial.print("LOOP_stretch: ");
                Serial.println(LOOP_stretch);
            }

            // Start/Stop all tracks
            if (Read_pushbutton(EN_PB_Loop))
            {
                // stop all tracks
                if (LOOP_run_button_state)
                {
                    LOOP_run_button_state = false;

                    AudioNoInterrupts();

                    // memorizza lo stato dei track prima di fermarli
                    for (auto local_track = 0; local_track < TRACKS; ++local_track)
                    {
                        LOOP_track_run_memo[local_track] = LOOP_track_run[local_track];
                        LOOP_track_run[local_track] = false;
                    }

                    // Interrompi i Player di loop
                    Players_Manager.Release_all_players_loop();

                    // Ferma il metronomo
                    LOOP_metronomo_run = false;

                    // Annulla l'ultima richiesta di aggiornamento proveniente da MidiReader
                    LOOP_metronomo_flag_IN[1] = false;

                    AudioInterrupts();

                    // Aggiorna (spegni) tutti i led
                    Loop_led_set.Request_all_LED_switch_off();

                    // Spegni i led del metronomo
                    LOOP_metronomo.Leds_off();

                    // Accendi led_0
                    LOOP_metronomo.Led_ON(0);
                }

                // enable all tracks
                else
                {
                    LOOP_run_button_state = true;

                    AudioNoInterrupts();
                    LOOP_restart_clock();

                    // il led 0 e' gia' acceso, riavvia il metronomo
                    LOOP_metronomo.metro_time = 0 + LOOP_metronomo.Read_metro_delta_ms();
                    LOOP_metronomo_run = true;
                    for (auto local_track = 0; local_track < TRACKS; ++local_track)
                    {
                        if (LOOP_track_run_memo[local_track])
                        {
                            LOOP_restart_procedure(local_track);
                        }
                    }
                    AudioInterrupts();
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
                Lilla_state_0 = MIDI_LOOP;
                Switch_to_MIXER();
            }
            break;

            case SwToolsDelay:
            {
                Lilla_state_0 = MIDI_LOOP;
                Golive_DELAY_SETTINGS();
            }
            break;

            case SwToolsSetup:
            {
                Switch_from_MIDI_LOOP_to_SETUP();
            }
            break;

            case SwToolsTest:
            {
                Switch_from_MIDI_LOOP_to_MIDI_MONITOR();
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
                Switch_from_MIDI_LOOP_to_DIRECT_SAMPLING();
            }
            break;

            case SwModesLiveSampler:
            {
                Switch_from_MIDI_LOOP_to_LIVE_SAMPLING();
            }
            break;

            case SwModesPerformance:
            {
                Switch_from_MIDI_LOOP_to_PERFORMANCE();
            }
            break;

            case SwModesMidiLoop:
                break;
            }
        }

        // Edit Sounds
        for (auto Inst_id = 0; Inst_id < INSTRUMENTS; ++Inst_id)
        {
            if (Read_pushbutton(PB_Sound[Inst_id]))
            {
                if (Patch[Patch_id].Instrument[Inst_id].used)
                {
                    Lilla_state_0 = MIDI_LOOP;
                    Lilla_state = SOUND_EDIT;

                    Instrument_id = Inst_id;
                    Sound_id = Get_sound_id(Patch_id, Instrument_id);

                    samples_in_file = Get_samples_in_raw_file(Sound[Sound_id].file);
                    Noclick_max = S_Calc_Noclick_max(Preset[Instrument_id].use_Wavetable);
                    S_trim_step = S_Calc_trim_step(trim_speed);

                    Display_Sound.Show_SOUND_page(Patch_id, Instrument_id);

                    // Menu
                    S_sound_original = S_Verify_is_Sound_original(Sound_id);
                    S_Select_menu_elements(); // updates "SO_menu_max" used by encoder_menu
                    Display_Sound.Show_SOUND_menu();

                    // Pointer
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
            }
        }
    }
}

void Golive_with_MIDI_LOOP(bool restart)
{
    Lilla_state = MIDI_LOOP;

    Clear_UI_events();

    LOOP_select_menu_elements();
    Display_MidiLoop.Show_Loop_page();

    // Pointer
    Pointer_MidiLoop.Set_pointer_to_first_menu_element();
    LOOP_local_pointer = Pointer_MidiLoop.Get_pointer();

    // LEDs setup
    if (restart)
    {
        Players_statistics.Reset_total_Players_per_track_instrument();
        Loop_led_set.Request_all_LED_switch_off();
    }

    // Update_instruments_leds();
    LOOP_metronomo.Leds_off(); // spegni i LED del metronomo

    // Se esiste loop_0, accendi il metronomo
    if (LOOP_events[0] != 0)
    {
        if (restart)
        {
            LOOP_restart_clock();

            // Accendi primo led metronomo
            LOOP_metronomo.Led_ON(0);

            /*
            // calcolo prossimo evento metronomo
            LOOP_metronomo.metro_time = 0 + LOOP_metronomo.Read_metro_delta_ms();

            // avvia il metronomo
            LOOP_metronomo_run = true
            */
        }
    }
    else
    {
        LOOP_run_button_state = true; // stato pulsante EN_PB_Loop (arresta/riavvia tutti i loop)
    }
}

