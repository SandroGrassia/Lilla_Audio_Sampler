/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 */

#include <Audio.h>
#include "LiveSamplerPage.h"
#include "main.h"
#include "DelayPage.h"
#include "MixerPage.h"
#include "MidiMonitorPage.h"
#include "SetupPage.h"
#include "UserInterface.h"
#include "SharedElements.h"
#include "SharedLiveSampler.h"
#include "PlayersManager.h"
#include "AudioPlayer.h"
#include "StereoLiveSampler.h"
#include "AudioLiveCompressor.h"
#include "MidiReader.h"
#include "PointerLiveSampler.h"
#include "PerformanceLedSet.h"
#include "Switches.h"
#include "ShiftRegisters.h"
#include "GlobalInfoMaster.h"
#include "GlobalDisplayLiveSampler.h"

bool Handle_Live_sampler(void)
{
    if (Lilla_state == LIVE_SAMPLING)
    {
        // Consume audio notices and update the popup only on the Live Sampler page.

        AudioNoInterrupts();
        const bool live_unrecorded_notice = AudioPlayer::Take_live_unrecorded_notice();
        AudioInterrupts();

        Display_LiveSampler.Update_no_recorded_audio(live_unrecorded_notice);

        for (int Inst_id = 0; Inst_id < INSTRUMENTS; ++Inst_id)
        {
            if (Read_pushbutton(PB_Sound[Inst_id]))
            {
                LS_Capture_sound(Inst_id);
                return false;
            }
        }

        /*
        Live Sampling (LIVE SAMPLER) consente la registrazione sia Mono che Stereo. Prevede l'uso della Patch PATCHES_MAX.

        Se la registrazione ÃƒÆ’Ã‚Â¨ mono, PATCHES_MAX comprende 1 Instrument e il Sound SOUNDS_MAX:
        - Patch[PATCHES_MAX].Instrument[0].sound_id == PATCHES_MAX

        L'Instrument ha:
        from_note = 0
        to_note = 127
        root_key = 60
        midi_ch = 0 (midi channel 1)

        Il Sound ÃƒÆ’Ã‚Â¨ associato al file Mono.liv:
        Sound[SOUNDS_MAX].file = FIRST_LIVE_SAMPLING_FILE;


        Se la registrazione ÃƒÆ’Ã‚Â¨ mono, PATCHES_MAX comprende 1 Instrument e il Sound SOUNDS_MAX:
        - Patch[PATCHES_MAX].Instrument[0].sound_id == SOUNDS_MAX --> associato a ch. Left
        - Patch[PATCHES_MAX].Instrument[1].sound_id == SOUNDS_MAX + 1 --> associato a ch. Right

        Entrambi gli Instrument hanno:
        from_note = 0
        to_note = 127
        root_key = 60
        midi_ch = 0 (midi channel 1)


        I due Sound sono associati ai file .liv:
        Sound[SOUNDS_MAX].file = FIRST_LIVE_SAMPLING_FILE + 1 (Left.liv)
        Sound[SOUNDS_MAX + 1].file = FIRST_LIVE_SAMPLING_FILE + 2 (Right.liv)

        Fisicamente, i campioni sono salvati su due buffer "virtual tape" (int16_t LS_buffer_L e int16_t LS_buffer_R) istanziati dinamicamente
        nei PSRAM chip; entrambi i buffer comprendono LS_buffer_dim campioni, con indirizzo da 0 a (LS_buffer_dim -1):
        0......................................................................................(LS_buffer_dim -1)

        L'ultimo campione scritto e' Q_sample; esempio di prima scrittura del buffer:
                                                                         Q_sample
        0>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>..................(LS_buffer_dim -1)

        La porzione di LS_buffer_L/R visualizzata (window) mostra la waveform registrata; con riferimento agli elementi di LS_buffer_L
        i samples visualizzati vanno da LS_window_A_sample a LS_window_B_sample; l'ampiezza della window e' LS_window_width.


        LS_X_sample - LS_Y_sample ÃƒÆ’Ã‚Â¨ l'intervallo di esecuzione:
        - FWD e REV : parte da LS_X_sample
        - Loop FWD e Loop FWD/REV : da LS_X_sample a LS_Y_sample.


        LS_X_sample e' sempre al centro della window; al primo accesso a LIVE SAMPLER ÃƒÆ’Ã‚Â¨ sul sample 0:
        .................................(LS_X_sample).........................................(LS_buffer_dim -1)
                  (LS_window_A_sample)+++++++++|+++++++(LS_window_B_sample)

        LS_Y_sample e' definito come LS_X_sample + LS_XY_delta. LS_Y_sample puo' essere anche esterno alla window:

        .................................(LS_X_sample)..........(LS_Y_sample)..................(LS_buffer_dim -1)
                  (LS_window_A_sample)+++++++++|+++++++(LS_window_B_sample)

        VINCOLI
        0 <= LS_window_A_sample <= (LS_buffer_dim -1)
        LS_window_A_sample < LS_window_B_sample (LS_buffer_L/R viene comunque letto come un ring tape)
        0 <= LS_X_sample <= (LS_buffer_dim -1)
        LS_Y_sample > LS_X_sample (da verificare)
        LS_XY_delta spazia da valori negativi a positivi (per consentire qualsiasi posizionamento a LS_Y_sample): -(LS_buffer_dim -1) < LS_XY_delta < (LS_buffer_dim -1)

        Live Sampler prevede che i punti di lettura LS_X_sample/LS_Y_sample possano essere:
        1) fissi su un punto del buffer (se fosse un tape sono solidali al tape, solidali ai campioni registrati): LS_XY_lock == true.
        2) spostarsi lungo il buffer (se fosse un tape sono solidali con la testa di registrazione, i campioni sottostanti cambiano con continuita'): LS_XY_lock == false

        In entrambi i casi, con il NoteOn le posizioni di partenza (modi FWD e REV) e di arrivo (modi loop FWD, loop FWD/REV) sono congelate sul buffer (non sono piÃƒÆ’Ã‚Â¹ mobili). Importante notare
        che nel modo loop il suono sambia se il segmento di buffer LS_X_sample/LS_Y_sample viene riscritto.

        Calcolo degli estremi della window
        LS_window_A_sample = LS_X_sample - (LS_window_width - 1)/2 (poi scalato ad un valore positivo: 0 <= LS_window_A_sample <= (LS_buffer_dim -1))
        LS_window_B_sample = LS_window_A_sample + LS_window_width - 1 (NON scalato se supera (LS_buffer_dim -1))

        1) Caso LS_XY_lock == true
        LS_X_sample ÃƒÆ’Ã‚Â¨ fisso su una certa posizione del buffer; la waveform cresce verso DESTRA (nuovi campioni a DESTRA)
        0 <= LS_X_sample <= (LS_buffer_dim -1)
        LS_Y_sample = LS_X_sample + LS_XY_delta

        2) LS_XY_lock == false
        LS_X_sample segue un determinato campione registrato, quindi scorre lungo il buffer; la waveform cresce verso SINISTRA (vecchi campioni a SINISTRA)
        Tutti i valori operativi sono in movimento, LS_X_sample e LS_Y_sample vanno continuamente aggiornati:
        LS_X_sample = LS_Q_sample + LS_X_delta (poi scalato per rispettare il vincolo 0 <= LS_X_sample <= (LS_buffer_dim -1)
        LS_Y_sample = LS_X_sample + LS_XY_delta

        Modifica dei parametri
        Gli step di avanzamento di:
        LS_window_width
        LS_X_sample
        LS_X_delta

        sono sempre PROPORZIONALI a LS_window_width.
        */

        // Change volume_patch
        if (Read_encoder(EN_PB_LineOutVol, volume_patch, PATCH_VOLUME_MAX, 0, 1))
        {
            AudioNoInterrupts();
            Players_Manager.Update_all_Preset_volume(Patch_id, Patch_volume_gain(volume_patch));
            Players_Manager.Broadcast_volume();
            AudioInterrupts();

            Display_LiveSampler.Volume();
        }

        // Move pointer
        result = Read_encoder_simple(EN_PB_Select);
        if (result != 0)
        {
            Pointer_LiveSampler.Move_pointer(result);
            LS_local_pointer = Pointer_LiveSampler.Get_pointer();

            Clear_UI_events();
        }

        // Change values
        if (LS_local_pointer.field_name == field_LS_Menu)
        {
            if (Read_pushbutton(EN_PB_Select))
            {
                switch (LS_local_pointer.menu_element)
                {
                case value_LS_Recording:
                {
                    LS_state = REC;

                    LS_update_menu_elements();
                    Display_LiveSampler.Menu(); // displays the menu and updates "Value_Max_encoder.LS_menu" used by encoder_menu

                    Pointer_LiveSampler.Set_pointer_to_first_menu_element();
                    LS_local_pointer = Pointer_LiveSampler.Get_pointer();

                    Clear_UI_events();

                    LiveSampler.Start(LS_stereo);
                    LS_wave_refresh_timer = 0;
                    delay(10);
                }
                break;
                case value_LS_Stop:
                {
                    LS_state = PLAYONLY;
                    LiveSampler.Stop();

                    LS_update_menu_elements();
                    Display_LiveSampler.Menu(); // displays the menu and updates "Value_Max_encoder.LS_menu" used by encoder_menu

                    Pointer_LiveSampler.Set_pointer_to_first_menu_element();
                    LS_local_pointer = Pointer_LiveSampler.Get_pointer();

                    Clear_UI_events();

                    delay(20);
                    if (!LS_XY_lock)
                    {
                        LS_update_both_X_Y_samples();
                    }
                    else // altrimenti e' gia' stato calcolato
                    {
                        LS_update_Q_sample();
                    }
                    Display_LiveSampler.Show_wave(LS_sound_id);
                }
                break;
                case value_LS_MonoStereo:
                {
                    Midi_reader.Stop(); // NON sostituire con AudioNoInterrupts!

                    AudioNoInterrupts();
                    Players_Manager.Stop_all_players();
                    AudioInterrupts();

                    LS_stereo = !LS_stereo;
                    LS_buffer_dim = (LS_stereo ? LS_CACHE_STEREO_SAMPLES : LS_CACHE_MONO_SAMPLES);
                    LS_window_width = LS_buffer_dim;
                    LS_window_step = LS_window_width / 8;
                    LS_Reset_buffer();
                    LS_state = EMPTY;
                    LS_setup_LS_Patch(LS_stereo);

                    AudioNoInterrupts();
                    Players_Manager.Update_all_Preset(Patch_id, Patch_volume_gain(volume_patch));
                    AudioInterrupts();

                    P_Update_all_maps_Instrument_for_notes();
                    Print_Patch(Patch_id);
                    LS_sound_id = SOUNDS_MAX; // mostra sempre il primo Sound
                    LS_instrument = 0;
                    LS_X_delta = 0;
                    LS_X_sample = 0;
                    LS_XY_delta = AUDIO_SAMPLE_RATE;
                    LS_Y_sample = LS_X_sample + LS_XY_delta;
                    LS_X_step = LS_window_width / LS_COMB;

                    LS_refresh_LS_page();

                    Pointer_LiveSampler.Set_pointer_to_first_menu_element();
                    LS_local_pointer = Pointer_LiveSampler.Get_pointer();

                    Clear_UI_events();

                    Midi_reader.Start();
                }
                break;
                case value_LS_Erase:
                {
                    AudioNoInterrupts();
                    Players_Manager.Stop_all_players();
                    AudioInterrupts();

                    LS_Reset_buffer();
                    LS_state = EMPTY;
                    LS_sound_id = SOUNDS_MAX; // mostra sempre il primo Sound
                    LS_instrument = 0;
                    LS_window_width = LS_buffer_dim; // LS_window_width = 441001;
                    LS_window_step = LS_window_width / 8;
                    LS_X_sample = 0;
                    LS_X_delta = 0;
                    LS_XY_delta = AUDIO_SAMPLE_RATE;
                    LS_Y_sample = LS_X_sample + LS_XY_delta;
                    LS_X_step = LS_window_width / LS_COMB;

                    Display_LiveSampler.Page();

                    // restore all LED
                    Performance_led_set.Restore_all_LED();

                    LS_update_menu_elements();
                    Display_LiveSampler.Menu();

                    Pointer_LiveSampler.Set_pointer_to_first_menu_element();
                    LS_local_pointer = Pointer_LiveSampler.Get_pointer();

                    Clear_UI_events();

                    if (!LS_XY_lock)
                    {
                        LS_update_both_X_Y_samples();
                    }
                    else // altrimenti e' gia' stato calcolato
                    {
                        LS_update_Q_sample();
                    }

                    Display_LiveSampler.Show_wave(LS_sound_id);
                }
                break;
                }
            }
        }

        else if (LS_local_pointer.field_name == field_LS_Value)
        {
            switch (LS_local_pointer.value_element)
            {
            case value_LS_Gain:
            {
                if (Read_encoder(EN_PB_Value, Line_in_gain, 15, 0, 1))
                {
                    Audio_shield.lineInLevel(Line_in_gain);
                    Display_LiveSampler.Gain();
                }
            }
            break;

            case value_LS_Play_mode:
            {
                if (Read_encoder(EN_PB_Value, LS_mode, LOOP_FWD_REV, 0, 1))
                {
                    AudioNoInterrupts();
                    Sound[SOUNDS_MAX].mode = LS_mode;
                    Players_Manager.Update_Preset_mode(Patch_id, 0);
                    Players_Manager.Multicast_main_settings_editing(Patch_id, 0);
                    if (LS_stereo)
                    {
                        Sound[SOUNDS_MAX + 1].mode = LS_mode;
                        Players_Manager.Update_Preset_mode(Patch_id, 1);
                        Players_Manager.Multicast_main_settings_editing(Patch_id, 1);
                    }
                    AudioInterrupts();

                    Pointer_LiveSampler.Show_pointer(false);
                    Display_LiveSampler.Play_mode();
                    Pointer_LiveSampler.Show_pointer(true);

                    Display_LiveSampler.Loop_time();
                    if (LS_state != REC)
                    {
                        if (!LS_XY_lock)
                        {
                            LS_update_both_X_Y_samples();
                        }
                        else // altrimenti e' gia' stato calcolato
                        {
                            LS_update_Q_sample();
                        }
                        Display_LiveSampler.Show_wave(LS_sound_id);
                    }
                }
            }
            break;
            case value_LS_Feedback:
            {
                if (Read_encoder(EN_PB_Value, LS_feedback, 8, 0, 1))
                {
                    AudioNoInterrupts();
                    LS_Compressor.Set_feedback(LS_fbk_table[LS_feedback]);
                    AudioInterrupts();

                    Pointer_LiveSampler.Show_pointer(false);
                    Display_LiveSampler.Feedback();
                    Pointer_LiveSampler.Show_pointer(true);

                    Serial.println(LS_fbk_table[LS_feedback]);
                }
            }
            break;
            case value_LS_Compressor:
            {
                if (Read_pushbutton(EN_PB_Select))
                {
                    AudioNoInterrupts();
                    const bool compressor_enabled = !LS_Compressor.Is_enabled();
                    LS_Compressor.Set_enabled(compressor_enabled);
                    AudioInterrupts();

                    Display_LiveSampler.Compressor(compressor_enabled);
                }
            }
            break;
            case value_LS_Window:
            {
                result = Read_encoder_simple(EN_PB_Value);
                if (result != 0)
                {
                    Info.LS_restart_antiflicker();
                    if (result == 1)
                    {
                        LS_window_width -= LS_window_width / 8;
                    }
                    else
                    {
                        LS_window_width += LS_window_width / 8;
                    }

                    LS_window_width = constrain(LS_window_width, 20 * AUDIO_BLOCK_SAMPLES, LS_buffer_dim); // constrain(LS_window_width, 200 * AUDIO_BLOCK_SAMPLES, LS_buffer_dim);
                    if (LS_state != REC)
                    {
                        if (!LS_XY_lock)
                        {
                            LS_update_both_X_Y_samples();
                        }
                        else // altrimenti e' gia' stato calcolato
                        {
                            LS_update_Q_sample();
                        }
                        Display_LiveSampler.Show_wave(LS_sound_id);
                    }
                    LS_X_step = LS_window_width / LS_COMB;
                    // Display_LiveSampler.Step();

                    Pointer_LiveSampler.Show_pointer(false);
                    Display_LiveSampler.Window();
                    Pointer_LiveSampler.Show_pointer(true);
                }

                // Set window_width to "ALL TAPE"
                if (Read_pushbutton(EN_PB_Value))
                {
                    Info.LS_restart_antiflicker();
                    LS_window_width = LS_buffer_dim;
                    if (LS_state != REC)
                    {
                        if (!LS_XY_lock)
                        {
                            LS_update_both_X_Y_samples();
                        }
                        else // altrimenti e' gia' stato calcolato
                        {
                            LS_update_Q_sample();
                        }
                        Display_LiveSampler.Show_wave(LS_sound_id);
                    }
                    LS_X_step = LS_window_width / LS_COMB;
                    // Display_LiveSampler.Step();

                    Pointer_LiveSampler.Show_pointer(false);
                    Display_LiveSampler.Window();
                    Pointer_LiveSampler.Show_pointer(true);
                }
            }
            }
        }

        // Change LS_X_sample o LS_X_delta
        result = Read_encoder_simple(EN_PB_From);
        if (result != 0)
        {
            // si usa LS_X_sample
            if (LS_XY_lock)
            {
                if (result == 1)
                {
                    LS_X_sample += LS_X_step;
                }
                else
                {
                    LS_X_sample -= LS_X_step;
                }

                LS_X_sample = LS_constrain_position(LS_X_sample);
                Display_LiveSampler.Start_point();

                Serial.print(F("LS_X_sample: "));
                Serial.println(LS_X_sample);

                LS_Y_sample = LS_X_sample + LS_XY_delta;
                Serial.print(F("LS_Y_sample: "));
                Serial.println(LS_Y_sample);
            }

            // si usa LS_X_delta
            else
            {
                if (result == 1)
                {
                    LS_X_delta += LS_X_step;
                }
                else
                {
                    LS_X_delta -= LS_X_step;
                }

                LS_X_delta = LS_constrain_position(LS_X_delta);
                Display_LiveSampler.Start_point();

                Serial.print(F("LS_X_delta: "));
                Serial.println(LS_X_delta);
            }

            if (LS_state > REC)
            {
                AudioNoInterrupts();
                Players_Manager.Multicast_main_settings_editing(Patch_id, 0);
                if (LS_stereo)
                {
                    Players_Manager.Multicast_main_settings_editing(Patch_id, 1);
                }
                AudioInterrupts();
            }

            if (LS_state != REC)
            {
                if (!LS_XY_lock)
                {
                    LS_update_both_X_Y_samples();
                }
                else // altrimenti e' gia' stato calcolato
                {
                    LS_update_Q_sample();
                }
                Display_LiveSampler.Show_wave(LS_sound_id);
            }
        }

        // toggle LS_XY_lock/!LS_XY_lock
        if (Read_pushbutton(EN_PB_Step))
        {
            if (LS_XY_lock)
            {
                // passando a LS_XY_lock non si deve riassegnare
                AudioNoInterrupts();
                Players_Manager.Stop_all_players();
                AudioInterrupts();

                LS_XY_lock = false;
                LS_X_delta = 0;
                Info.LS_restart_antiflicker();
            }
            else
            {
                // passando a !LS_XY_lock si deve riassegnare LS_X_sample
                LS_lock_X_sample();
                Serial.print(F("LS_X_sample "));
                Serial.println(LS_X_sample);
            }

            Display_LiveSampler.Start_point();
            Serial.print(F("LS_XY_lock: "));
            Serial.println(LS_XY_lock);

            if (LS_state != REC)
            {
                if (!LS_XY_lock)
                {
                    LS_update_both_X_Y_samples();
                }
                else // altrimenti e' gia' stato calcolato
                {
                    LS_update_Q_sample();
                }
                Display_LiveSampler.Show_wave(LS_sound_id);
            }
        }

        // Change "Loop Width" (LS_XY_delta)
        result = Read_encoder_simple(EN_PB_To);
        if (result != 0)
        {
            if (result == 1)
            {
                LS_XY_delta += LS_X_step;
            }
            else
            {
                LS_XY_delta -= LS_X_step;
            }

            LS_XY_delta = constrain(LS_XY_delta, LS_XY_DELTA_MIN, LS_buffer_dim - 1);

            AudioNoInterrupts();
            Players_Manager.Multicast_main_settings_editing(Patch_id, 0);
            if (LS_stereo)
            {
                Players_Manager.Multicast_main_settings_editing(Patch_id, 1);
            }
            AudioInterrupts();

            if (LS_XY_lock)
            {
                LS_Y_sample = LS_X_sample + LS_XY_delta;
            }
            Display_LiveSampler.Loop_time();

            Serial.print(F("LS_XY_delta: "));
            Serial.println(LS_XY_delta);

            if (LS_state != REC)
            {
                if (!LS_XY_lock)
                {
                    LS_update_both_X_Y_samples();
                }
                else // altrimenti e' gia' stato calcolato
                {
                    LS_update_Q_sample();
                }
                Display_LiveSampler.Show_wave(LS_sound_id);
            }
        }

        // Change "Step" (LS_X_step)
        result = Read_encoder_simple(EN_PB_Step);
        if (result != 0)
        {
            if (result == 1)
            {
                LS_COMB = LS_COMB / 2;
            }

            else // Aumenta LS_X_step
            {
                LS_COMB = 2 * LS_COMB;
            }

            LS_COMB = constrain(LS_COMB, 8, 1024);
            LS_X_step = LS_window_width / LS_COMB;
            Display_LiveSampler.Step();
        }

        // Update wave
        if (LS_state == REC) // Open
        {
            if (LS_wave_refresh_timer >= LS_REFRESH) // ms
            {
                LS_wave_refresh_timer = 0;

                if (!LS_XY_lock)
                {
                    LS_update_both_X_Y_samples();
                }
                else // altrimenti e' gia' stato calcolato
                {
                    LS_update_Q_sample();
                }

                Display_LiveSampler.Show_wave(LS_sound_id);
            }
        }

        // Toggle wave Left/Right and VCF
        if (LS_stereo)
        {
            // Display Left wave or VCF
            if (Read_pushbutton(EN_PB_From))
            {
                if (LS_instrument == 1) // Right
                {
                    LS_instrument = 0;        // Left
                    LS_sound_id = SOUNDS_MAX; // Left
                    if (LS_state != REC)
                    {
                        if (!LS_XY_lock)
                        {
                            LS_update_both_X_Y_samples();
                        }
                        else // altrimenti e' gia' stato calcolato
                        {
                            LS_update_Q_sample();
                        }

                        Display_LiveSampler.Show_wave(LS_sound_id);
                    }
                }
            }

            // Display Right wave or VCF
            if (Read_pushbutton(EN_PB_To))
            {
                if (LS_instrument == 0) // Left
                {
                    LS_instrument = 1;            // Right
                    LS_sound_id = SOUNDS_MAX + 1; // Right
                    if (LS_state != REC)
                    {
                        if (!LS_XY_lock)
                        {
                            LS_update_both_X_Y_samples();
                        }
                        else // altrimenti e' gia' stato calcolato
                        {
                            LS_update_Q_sample();
                        }
                        Display_LiveSampler.Show_wave(LS_sound_id);
                    }
                }
            }
        }

        // Switch Mode
        if (Switches_manager.Get_change(SwitchModes))
        {
            switch (Switches_manager.Get_value(SwitchModes))
            {
            case SwModesSampler:
            {
                Switch_from_LIVE_SAMPLING_to_DIRECT_SAMPLING();
            }
            break;

            case SwModesLiveSampler:
                break;

            case SwModesPerformance:
            {
                Switch_from_LIVE_SAMPLING_to_PERFORMANCE();
            }
            break;

            case SwModesMidiLoop:
            {
                Switch_from_LIVE_SAMPLING_to_MIDI_LOOP();
            }
            break;
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
                Lilla_state_0 = LIVE_SAMPLING;
                Switch_to_MIXER();
            }
            break;

            case SwToolsDelay:
            {
                Switch_from_LIVE_SAMPLING_to_DELAY(); // setta anche: Lilla_state_0 = LIVE_SAMPLING;
            }
            break;

            case SwToolsSetup:
            {
                Lilla_state_0 = LIVE_SAMPLING;
                Golive_SETUP();
            }
            break;

            case SwToolsTest:
            {
                Lilla_state_0 = LIVE_SAMPLING;
                Golive_MIDI_MONITOR();
            }
            break;
            }
        }
    }
    return true;
}

void Golive_with_LIVE_SAMPLING(void)
{
    // Discard notices from the previous page before entering Live Sampler.

    AudioNoInterrupts();
    AudioPlayer::Take_live_unrecorded_notice();
    AudioInterrupts();

    Lilla_state = LIVE_SAMPLING;

    Display_LiveSampler.Page();

    Clear_UI_events();

    // restore LEDs
    Performance_led_set.Restore_all_LED();

    LS_update_menu_elements();
    Display_LiveSampler.Menu();
    Pointer_LiveSampler.Set_pointer_to_first_menu_element();
    LS_local_pointer = Pointer_LiveSampler.Get_pointer();

    if (!LS_XY_lock)
    {
        LS_update_both_X_Y_samples();
    }
    else // altrimenti e' gia' stato calcolato
    {
        LS_update_Q_sample();
    }

    Display_LiveSampler.Show_wave(LS_sound_id);

    Print_Patch(Patch_id);
}

void LS_refresh_LS_page(void)
{
    // Discard notices from the previous page before entering Live Sampler.

    AudioNoInterrupts();
    AudioPlayer::Take_live_unrecorded_notice();
    AudioInterrupts();

    Lilla_state = LIVE_SAMPLING;

    Display_LiveSampler.Page();

    // restore LEDs
    Performance_led_set.Restore_all_LED();

    LS_update_menu_elements();
    Display_LiveSampler.Menu();

    if (!LS_XY_lock)
    {
        LS_update_both_X_Y_samples();
    }
    else // altrimenti e' gia' stato calcolato
    {
        LS_update_Q_sample(); // Usato da LS_wave_color
    }
    Display_LiveSampler.Show_wave(LS_sound_id);

    Clear_UI_events();
}
