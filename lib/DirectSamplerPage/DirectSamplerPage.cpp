/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 */

#include <Audio.h>
#include "DirectSamplerPage.h"
#include "main.h"
#include "DelayPage.h"
#include "MixerPage.h"
#include "MidiMonitorPage.h"
#include "SetupPage.h"
#include "UserInterface.h"
#include "Functions.h"
#include "SharedElements.h"
#include "SharedSampler.h"
#include "SharedVFS.h"
#include "CaptureSources.h"
#include "FileNameRegistry.h"
#include "PlayersManager.h"
#include "StereoSampler.h"
#include "AudioPeakDetector.h"
#include "AmpliOutMuteIn.h"
#include "MidiReader.h"
#include "LillaSerialFlash.h"
#include "ArchivingManager.h"
#include "PointerSampler.h"
#include "PerformanceLedSet.h"
#include "Switches.h"
#include "ShiftRegisters.h"
#include "GlobalDisplaySampler.h"
#include "DisplayPrimitives.h"

bool Handle_Direct_sampler(bool &recording_limit_notified)
{
    char audio_filename[NAME_FILE_SIZE];
    if (Lilla_state == DIRECT_SAMPLING)
    {

        /*
        Direct Sampling (SAMPLER) consente la registrazione sia Mono che Stereo. Prevede l'uso della Patch PATCHES_MAX, dei Sound SOUNDS_MAX e (SOUNDS_MAX + 1) e di 2 Instrument:
        - Patch[PATCHES_MAX].Instrument[0].sound_id == SOUNDS_MAX --> associato a ch. Left oppure Mono
        - Patch[PATCHES_MAX].Instrument[1].sound_id == SOUNDS_MAX + 1 --> associato a ch. Right

        Entrambi gli instrument hanno:
        from_note = 0
        to_note = 127
        root_key = 60
        midi_ch = 0 (midi channel 1)

        Se la registrazione ÃƒÆ’Ã‚Â¨ mono, PATCHES_MAX comprende 1 Instrument e il Sound SOUNDS_MAX:

        */

        // Show the capacity notice once per visit, after the completed recording has been published.
        if (DS_state == DS_waiting_state && DS_recording_slots_full && !recording_limit_notified)
        {
            recording_limit_notified = true;
            Show_popup_text("RECORDING LIMIT REACHED", "DELETE OR EXPORT A RECORDING", ILI9341_WHITE, ILI9341_RED);
            delay(2000);
            Display_Sampler.DS_page_upper();
            Display_Sampler.DS_page_lower(recording);
            Display_Sampler.DS_menu();
            Pointer_Sampler.Show_pointer(true);
            Clear_UI_events();
        }

        // Change volume_patch
        if (DS_state == DS_waiting_state && Read_encoder(EN_PB_LineOutVol, volume_patch, PATCH_VOLUME_MAX, 0, 1))
        {
            AudioNoInterrupts();
            Players_Manager.Update_all_Preset_volume(Patch_id, Patch_volume_gain(volume_patch));
            Players_Manager.Broadcast_volume();
            AudioInterrupts();

            Display_Sampler.DS_update_volume();
        }

        // Update VU meter
        if (DS_state == DS_pause_state || DS_state == DS_recording_state)
        {
            float val;
            if (PeakTracking_L.available())
            {
                val = 20 * log10(PeakTracking_L.read());       // 0 <= PeakTracking_L.read() <= 1.0 ; -inf < val < 0
                Display_Sampler.DS_bar(0, BAR_ELEMENTS + val); // Display_Sampler.DS_bar(0, PeakTracking_L.read() * BAR_ELEMENTS);
            }
            if (PeakTracking_R.available())
            {
                val = 20 * log10(PeakTracking_R.read());
                Display_Sampler.DS_bar(1, BAR_ELEMENTS + val); // Display_Sampler.DS_bar(1, PeakTracking_R.read() * BAR_ELEMENTS);
            }
        }

        if (DS_state == DS_recording_state)
        {
            // Update blinking REC
            if (DS_blink_timer >= 500)
            {
                DS_blink_ON = !DS_blink_ON;
                Display_Sampler.DS_sampler_txt(DS_blink_ON);
                DS_blink_timer = 0;
            }

            // Update seconds and free-memory
            if (DS_recording_time_update >= 200)
            {
                DS_recording_time_update = 0;
                Display_Sampler.DS_available_memory();
            }

            // Stop if SteroSampler has stopped
            if (!DirectSampler.Is_recording())
            {
                Require_FRAM(DirectSampler.Storage_error());
                DS_state = DS_waiting_state;

                // switch OFF Audio Input monitor
                MAIN_mixer_out_L.gain(1, 0.0);
                MAIN_mixer_out_R.gain(1, 0.0);

                if (Recording[recording].packets == 0)
                {
                    // Start from first recording existing
                    recording = DS_get_next_Recording(-1);
                }

                else
                {
                    Recording[recording].consistent = true;
                    // consistent Recording must be saved
                    Require_FRAM(Archive.Save_DS_Recording(recording));
                    DS_read_Recording(recording); // only to update .bytes and .seconds
                }

                DS_update_recordings();

                // Switch off blinking REC
                DS_blink_ON = false;

                // Menu
                DS_define_menu();
                Display_Sampler.DS_menu(); // display the menu and updates DS_menu_max

                // Pointer
                Pointer_Sampler.Set_pointer_to_first_menu_element();
                DS_local_pointer = Pointer_Sampler.Get_pointer();

                Clear_UI_events();

                Display_Sampler.DS_available_memory();

                Display_Sampler.DS_sampler_txt(false);

                VFS_Print_FAT();

                if (!DS_Jump_to_DIRECT_SAMPLING_recording(recording))
                {
                    return false;
                }

                // Reporting
                P_Recording(recording);
            }
        }

        // Move pointer
        result = Read_encoder_simple(EN_PB_Select);
        if (result != 0)
        {
            Pointer_Sampler.Move_pointer(result);
            DS_local_pointer = Pointer_Sampler.Get_pointer();

            Clear_UI_events();
        }

        // Change values
        switch (DS_local_pointer.field_name)
        {
        case field_DS_Menu:
        {
            if (Read_pushbutton(EN_PB_Select) || Read_pushbutton(EN_PB_Value))
            {
                Pointer_Sampler.Print_pointer();

                int first_packet_L = 0;
                int packets_per_channel = 0;
                int last_packet_L = 0;
                int first_packet_R = 0;

                switch (DS_local_pointer.menu_element)
                {
                case 0: // Delete
                {
                    AudioNoInterrupts();
                    Players_Manager.Stop_all_players();
                    AudioInterrupts();

                    Display_Sampler.DS_hide_recording();
                    Display_Sampler.DS_advice_delete(true);

                    // Delete recording
                    P_Invalidate_recording_cache(recording);
                    Recording[recording].consistent = false;
                    Require_VFS(VFS_Clean_up_VFS());
                    Require_VFS(VFS_Defragment());
                    DS_update_recordings();
                    VFS_Print_FAT();

                    // restart from first recording (if exist)
                    recording = DS_get_next_Recording(-1);
                    if (!DS_back_to_first_DS_Recording())
                    {
                        return false;
                    }
                    Display_Sampler.DS_available_memory();
                }
                break;

                case 1: // Pause+Rec (pause before recording, listening Audio Input)
                {
                    DS_state = DS_pause_state;

                    AudioNoInterrupts();
                    Midi_reader.Stop();
                    Players_Manager.Stop_all_players();
                    AudioInterrupts();

                    Display_Sampler.DS_update_volume(false); // cambia il colore del volume in bianco (fisso)

                    recording = DS_find_Recording_free();
                    Serial.println(F("*** Pause + Record: listen to Audio Input ***"));
                    Serial.print(F("**** Prossimo recording: "));
                    Serial.println(recording);

                    // Hide last recording data
                    Display_Sampler.DS_hide_recording();

                    // Switch on Line OUT monitor
                    MAIN_mixer_out_L.gain(1, 1.0);
                    MAIN_mixer_out_R.gain(1, 1.0);

                    // Menu
                    DS_define_menu();
                    Display_Sampler.DS_menu(); // display the menu and updates DS_menu_max

                    // Pointer
                    Pointer_Sampler.Set_pointer_to_first_menu_element();
                    DS_local_pointer = Pointer_Sampler.Get_pointer();

                    Clear_UI_events();
                }
                break;

                case 2: // Mono Rec
                {
                    DS_state = DS_recording_state;
                    P_Invalidate_recording_cache(recording);

                    Recording[recording].stereo = false;
                    Recording[recording].consistent = false;

                    // packet:  0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18
                    // free:    * * * * 1 2 3 4 5 6 7  8  9  10 11 12 13 14 15
                    // result:  * * * * L L L L L L L  L  L  L  L  L  L  L  L

                    packets_per_channel = VFS_Get_packets_free();             // 15
                    first_packet_L = VFS_Get_first_packet_free();             // 4
                    last_packet_L = first_packet_L + packets_per_channel - 1; // 4 + 15 - 1 = 18

                    Serial.println(F("*** Start MONO Sampling! *** "));
                    Serial.print(F("Mono recording from packet: "));
                    Serial.print(first_packet_L);
                    Serial.print(F("  up to packet: "));
                    Serial.println(last_packet_L);

                    // Menu
                    DS_define_menu();
                    Display_Sampler.DS_menu(); // display the menu and updates DS_menu_max

                    // Pointer
                    Pointer_Sampler.Set_pointer_to_first_menu_element();
                    DS_local_pointer = Pointer_Sampler.Get_pointer();

                    Clear_UI_events();

                    Display_Sampler.DS_Recording_description(recording, false);
                    Display_Sampler.DS_sampler_txt(true);

                    DS_blink_timer = 0;
                    DS_blink_ON = true;

                    DirectSampler.Start(first_packet_L, last_packet_L, recording, Recording[recording].stereo); // bool start(int from_packet, int last_packet, int recording_id_in, bool stereo_in)
                    DS_recording_time = 0;
                    DS_recording_time_update = 0;
                }
                break;

                case 3: // Stereo Rec
                {
                    DS_state = DS_recording_state;
                    P_Invalidate_recording_cache(recording);

                    Recording[recording].stereo = true;
                    Recording[recording].consistent = false;

                    // packet:  0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18
                    // free:    * * * * 1 2 3 4 5 6 7  8  9  10 11 12 13 14 15
                    // result:  * * * * L R L R L R L  R  L  R  L  R  L  R  _

                    packets_per_channel = VFS_Get_packets_free() / 2;               // (15/2) = 7
                    first_packet_L = VFS_Get_first_packet_free();                   // 4
                    last_packet_L = first_packet_L + 2 * (packets_per_channel - 1); // 16
                    first_packet_R = first_packet_L + 1;

                    Serial.println(F("*** Start STEREO Sampling! *** "));
                    Serial.print(F("Left recording from packet: "));
                    Serial.print(first_packet_L);
                    Serial.print(F("  Right recording from packet: "));
                    Serial.println(first_packet_R);

                    // Menu
                    DS_define_menu();
                    Display_Sampler.DS_menu(); // display the menu and updates DS_menu_max

                    // Pointer
                    Pointer_Sampler.Set_pointer_to_first_menu_element();
                    DS_local_pointer = Pointer_Sampler.Get_pointer();

                    Clear_UI_events();

                    Display_Sampler.DS_Recording_description(recording, false);
                    Display_Sampler.DS_sampler_txt(true);

                    DS_blink_timer = 0;
                    DS_blink_ON = true;

                    DirectSampler.Start(first_packet_L, last_packet_L, recording, Recording[recording].stereo); // bool start(int from_packet, int last_packet, int recording_id_in, bool stereo_in)
                    DS_recording_time = 0;
                    DS_recording_time_update = 0;
                }
                break;

                case 4: // Stop
                {
                    Serial.println(F("*** Pause+Recording or Recording STOPPED! *** "));
                    if (DS_state == DS_recording_state)
                    {
                        DirectSampler.Stop_and_wait();
                        Require_FRAM(DirectSampler.Storage_error());
                    }
                    DS_state = DS_waiting_state;

                    // switch OFF Line OUT monitor
                    MAIN_mixer_out_L.gain(1, 0.0);
                    MAIN_mixer_out_R.gain(1, 0.0);

                    if (Recording[recording].packets == 0)
                    {
                        Serial.print(F("Recording: "));
                        Serial.print(recording);
                        Serial.println(F(" cancelled."));
                        recording = DS_get_next_Recording(-1);
                    }

                    else
                    {
                        Recording[recording].consistent = true;
                        // consistent Recording must be saved
                        Require_FRAM(Archive.Save_DS_Recording(recording));
                        DS_read_Recording(recording); // only to update .bytes and .seconds
                    }

                    DS_update_recordings();
                    // Switch off blinking REC
                    DS_blink_ON = false;

                    // Menu
                    DS_define_menu();
                    Display_Sampler.DS_menu(); // display the menu and updates DS_menu_max

                    // Pointer
                    Pointer_Sampler.Set_pointer_to_first_menu_element();
                    DS_local_pointer = Pointer_Sampler.Get_pointer();

                    Clear_UI_events();

                    Display_Sampler.DS_available_memory();
                    Display_Sampler.DS_sampler_txt(false);

                    // VFS_Print_FAT();
                    P_Recording(recording);

                    if (!DS_Jump_to_DIRECT_SAMPLING_recording(recording))
                    {
                        return false;
                    }
                    Midi_reader.Start();
                }
                break;

                case 5: // CONVERT_REC_TO_RAW
                {
                    int raw_conversion_choice = 0;
                    bool raw_conversion_confirmed = false;
                    Display_Sampler.DS_confirm_raw_conversion_popup();
                    Clear_UI_events();
                    while (!raw_conversion_confirmed)
                    {
                        Shifters_manager.Update();
                        if (Read_encoder(EN_PB_Select, raw_conversion_choice, 1, 0, 1))
                        {
                            Display_Sampler.DS_confirm_raw_conversion_frame(raw_conversion_choice);
                        }
                        if (Read_pushbutton(EN_PB_Select) || Read_pushbutton(EN_PB_Value))
                        {
                            raw_conversion_confirmed = true;
                        }
                    }
                    Clear_UI_events();
                    Display_Sampler.DS_page_lower(recording);
                    if (raw_conversion_choice == 0)
                    {
                        break;
                    }

                    DS_state = DS_convert_state;

                    AudioNoInterrupts();
                    Players_Manager.Stop_all_players();
                    AudioInterrupts();

                    confirmation = false; // no action
                    int file_L_RAW = -1;
                    int file_R_RAW = -1;
                    int blocks_per_file = ceil(Recording[recording].bytes / 256.0f); // quanti block compongono il file

                    if (!Recording[recording].stereo)
                    {
                        if ((Get_flash_size() - Get_flash_occupation()) >= Recording[recording].bytes)
                        {
                            DS_export = -1; // no filename available;
                            for (auto i = 0; i < FIRST_RECORDING_FILE; ++i)
                            {
                                if (FileNameRegistry::Numeric_available(i) && Capture_find(i) == nullptr && !SerialFlash.exists(Get_file_name(i, audio_filename)))
                                {
                                    file_L_RAW = i;
                                    DS_export = 1;
                                    break;
                                }
                            }
                        }
                        else
                        {
                            DS_export = 0; // no space available
                        }
                    }

                    else
                    {
                        if ((Get_flash_size() - Get_flash_occupation()) >= (2 * Recording[recording].bytes))
                        {
                            DS_export = -1; // no filename available;
                            for (auto i = 0; i < FIRST_RECORDING_FILE; ++i)
                            {
                                if (FileNameRegistry::Numeric_available(i) && Capture_find(i) == nullptr && !SerialFlash.exists(Get_file_name(i, audio_filename)))
                                {
                                    file_L_RAW = i;
                                    DS_export = 1;
                                    break;
                                }
                            }
                            if (DS_export == 1)
                            {
                                for (auto i = file_L_RAW + 1; i < FIRST_RECORDING_FILE; ++i)
                                {
                                    if (FileNameRegistry::Numeric_available(i) && Capture_find(i) == nullptr && !SerialFlash.exists(Get_file_name(i, audio_filename)))
                                    {
                                        file_R_RAW = i;
                                        DS_export = 2;
                                        break;
                                    }
                                }
                            }
                        }
                        else if ((Get_flash_size() - Get_flash_occupation()) >= Recording[recording].bytes)
                        {
                            DS_export = -1; // no filename available;
                            for (auto i = 0; i < FIRST_RECORDING_FILE; ++i)
                            {
                                if (FileNameRegistry::Numeric_available(i) && Capture_find(i) == nullptr && !SerialFlash.exists(Get_file_name(i, audio_filename)))
                                {
                                    file_L_RAW = i;
                                    DS_export = 1;
                                    break;
                                }
                            }
                        }
                        else
                        {
                            DS_export = 0; // no space available
                        }
                    }

                    Serial.print("DS_export ");
                    Serial.println(DS_export);
                    Serial.print("file_L_RAW proposto ");
                    Serial.println(file_L_RAW);
                    Serial.print("file_R_RAW proposto ");
                    Serial.println(file_R_RAW);
                    Serial.println();

                    if (DS_export <= 0)
                    {
                        Display_Sampler.DS_hide_recording();
                        Display_Sampler.DS_advice_no_conversion(DS_export, true);
                        delay(7000);

                        Display_Sampler.DS_advice_no_conversion(DS_export, false);

                        // Menu
                        DS_define_menu();
                        Display_Sampler.DS_menu(); // display the menu and updates DS_menu_max

                        // Pointer
                        Pointer_Sampler.Set_pointer_to_first_menu_element();
                        DS_local_pointer = Pointer_Sampler.Get_pointer();

                        Clear_UI_events();

                        Display_Sampler.DS_Recording_description(recording, true);

                        // Restore LED
                        Performance_led_set.Restore_all_LED();

                        break;
                    }

                    // Menu
                    DS_define_menu();
                    Display_Sampler.DS_menu(); // display the menu and updates DS_menu_max

                    // Pointer
                    Pointer_Sampler.Set_pointer_to_first_menu_element();
                    DS_local_pointer = Pointer_Sampler.Get_pointer();

                    Clear_UI_events();

                    Display_Sampler.DS_conversion_options(file_L_RAW, file_R_RAW, DS_export);

                    // Choose what to do
                    while (!confirmation)
                    {
                        Shifters_manager.Update();

                        // Move pointer
                        result = Read_encoder_simple(EN_PB_Select);
                        if (result != 0)
                        {
                            Pointer_Sampler.Move_pointer_within_menu(result);
                            DS_local_pointer = Pointer_Sampler.Get_pointer();

                            Clear_UI_events();
                        }

                        // Choose element
                        if (Read_pushbutton(EN_PB_Select) || Read_pushbutton(EN_PB_Value))
                        {
                            confirmation = true;
                        }
                    }
                    Clear_UI_events();

                    switch (choice_DS_menu)
                    {
                    case 6: // Cancel (don't export)
                        Serial.println(F("Don't convert any file"));
                        break;

                    case 7:                                                   // Convert Mono (file_L)
                        DS_convert_file_L(file_L_RAW, blocks_per_file * 256); // DS_convert_file_L(int file_L_RAW, int bytes)

                        // occorre rifare lo scan di tutti i file per compilare tutti i metadati del nuovo file, dirindex compreso
                        File_scanner.Read_all_file_data();
                        break;

                    case 8:                                                   // Convert file_L
                        DS_convert_file_L(file_L_RAW, blocks_per_file * 256); // DS_convert_file_L(int file_L_RAW, int bytes)

                        // occorre rifare lo scan di tutti i file per compilare tutti i metadati del nuovo file, dirindex compreso
                        File_scanner.Read_all_file_data();
                        break;

                    case 9:                                                   // Convert file_R
                        DS_convert_file_R(file_R_RAW, blocks_per_file * 256); // DS_convert_file_R(int file_R_RAW, int bytes)

                        // occorre rifare lo scan di tutti i file per compilare tutti i metadati del nuovo file, dirindex compreso
                        File_scanner.Read_all_file_data();
                        break;

                    case 10:                                                  // Convert both file_L and file_R
                        DS_convert_file_L(file_L_RAW, blocks_per_file * 256); // DS_convert_file_L(int file_L_RAW, int bytes)
                        DS_convert_file_R(file_R_RAW, blocks_per_file * 256); // DS_convert_file_R(int file_R_RAW, int bytes)

                        // occorre rifare lo scan di tutti i file per compilare tutti i metadati del nuovo file, dirindex compreso
                        File_scanner.Read_all_file_data();
                        break;

                    default:
                        // Reporting
                        Serial.println(F("Don't convert any file"));
                        break;
                    }

                    // Delete recording
                    if (choice_DS_menu > 6)
                    {
                        P_Invalidate_recording_cache(recording);
                        Recording[recording].consistent = false;
                        Require_VFS(VFS_Clean_up_VFS());
                        Require_VFS(VFS_Defragment());
                        DS_update_recordings();
                        VFS_Print_FAT();

                        // Load first recording (if exist)
                        recording = DS_get_next_Recording(-1);
                        if (!DS_back_to_first_DS_Recording())
                        {
                            return false;
                        }
                    }

                    Print_flash_file_list();

                    // Return
                    DS_state = DS_waiting_state;

                    Display_Sampler.DS_page_upper();
                    Display_Sampler.DS_page_lower(recording);

                    // Menu
                    DS_define_menu();
                    Display_Sampler.DS_menu(); // display the menu and updates DS_menu_max

                    // Pointer
                    Pointer_Sampler.Set_pointer_to_first_menu_element();
                    DS_local_pointer = Pointer_Sampler.Get_pointer();

                    Clear_UI_events();

                    // Switch bar_display ON
                    PeakTracking_L.reset();
                    PeakTracking_R.reset();
                    Display_Sampler.DS_bar(0, 0);
                    Display_Sampler.DS_bar(1, 0);
                }
                break;

                case 11: // EXPORT AS WAV TO SD
                {
                    DS_state = DS_export_SD_state;
                    const bool exported = DS_export_wav_to_SD();
                    if (!exported)
                    {
                        Show_popup_text("EXPORT FAILED", ILI9341_WHITE, ILI9341_RED, 0);
                    }
                    else if (!Recording[recording].stereo)
                    {
                        Show_popup_text("MONO WAV EXPORTED TO SD", ILI9341_BLACK, ILI9341_GREEN);
                    }
                    else
                    {
                        Show_popup_text("STEREO WAV EXPORTED TO SD", ILI9341_BLACK, ILI9341_GREEN);
                    }
                    delay(exported ? 2000 : 4000);
                    DS_state = DS_waiting_state;
                    if (exported)
                    {
                        AudioNoInterrupts();
                        Players_Manager.Stop_all_players();
                        AudioInterrupts();

                        // Reclaim the source only after the WAV has been written, synced and closed successfully.
                        P_Invalidate_recording_cache(recording);
                        Recording[recording].consistent = false;
                        Require_VFS(VFS_Clean_up_VFS());
                        Require_VFS(VFS_Defragment());
                        DS_update_recordings();
                        recording = DS_get_next_Recording(-1);
                        if (!DS_back_to_first_DS_Recording())
                        {
                            return false;
                        }
                    }
                    PeakTracking_L.reset();
                    PeakTracking_R.reset();
                    Display_Sampler.DS_page_lower(recording);
                    DS_define_menu();
                    Display_Sampler.DS_menu();
                    Pointer_Sampler.Set_pointer_to_first_menu_element();
                    DS_local_pointer = Pointer_Sampler.Get_pointer();
                    Clear_UI_events();
                }
                break;

                default:
                    PRINT_ERROR(F("Switch MISSING! "));
                    break;
                } // END switch(choice_DS_menu)
            }
        }
        break;

        case field_DS_Value:
        {

            if (DS_local_pointer.value_element == value_DS_Gain)
            {
                // Share the LINE IN gain with Mixer; allow adjustment while monitoring or recording.
                if ((DS_state == DS_pause_state || DS_state == DS_recording_state) && Read_encoder(EN_PB_Value, Line_in_gain, 15, 0, 1))
                {
                    Audio_shield.lineInLevel(Line_in_gain);
                    Display_Sampler.DS_show_gain();
                }
            }
            else if (DS_local_pointer.value_element == value_DS_Recording && DS_state == DS_waiting_state)
            {
                result = Read_encoder_simple(EN_PB_Value);
                if (result != 0)
                {
                    DS_recording_change = recording;
                    if (result == +1)
                    {
                        DS_recording_change = DS_get_next_Recording(recording);
                    }
                    else
                    {
                        DS_recording_change = DS_get_previous_Recording(recording);
                    }

                    if (DS_recording_change != recording)
                    {
                        AudioNoInterrupts();
                        Players_Manager.Stop_all_players();
                        AudioInterrupts();

                        recording = DS_recording_change;
                        if (!DS_Jump_to_DIRECT_SAMPLING_recording(recording))
                        {
                            return false;
                        }

                        // Recording
                        P_Recording(recording);
                    }
                }
            }
        }
        break;
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
                Lilla_state_0 = DIRECT_SAMPLING;
                Switch_to_MIXER();
            }
            break;

            case SwToolsDelay:
            {
                Lilla_state_0 = DIRECT_SAMPLING;
                Golive_DELAY_SETTINGS();
            }
            break;

            case SwToolsSetup:
            {
                Lilla_state_0 = DIRECT_SAMPLING;
                Golive_SETUP();
            }
            break;

            case SwToolsTest:
            {
                Lilla_state_0 = DIRECT_SAMPLING;
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
                break;

            case SwModesLiveSampler:
            {
                Switch_from_DIRECT_SAMPLING_to_LIVE_SAMPLING();
            }
            break;

            case SwModesPerformance:
            {
                Switch_from_DIRECT_SAMPLING_to_PERFORMANCE();
            }
            break;

            case SwModesMidiLoop:
            {
                Switch_from_DIRECT_SAMPLING_to_MIDI_LOOP();
            }
            break;
            }
        }

    } // END if(Lilla_state == DIRECT_SAMPLING)
    return true;
}

void Golive_DIRECT_SAMPLING(void)
{
    Lilla_state = DIRECT_SAMPLING;

    Clear_UI_events();

    DS_state = DS_waiting_state;

    // Switch ON the VU meter
    PeakTracking_L.reset();
    PeakTracking_R.reset();

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

    // Reporting
    Print_Patch(Patch_id);
    Serial.println(F("*** DIRECT_SAMPLING ***  Sounds are:"));
    Print_Sound(SOUNDS_MAX);
    Print_Sound(SOUNDS_MAX + 1);
}
