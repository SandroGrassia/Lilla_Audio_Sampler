/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 */

#include <Audio.h>
#include "SetupPage.h"
#include "main.h"
#include "LiveSamplerPage.h"
#include "DelayPage.h"
#include "MixerPage.h"
#include "MidiMonitorPage.h"
#include "CCSettingsPage.h"
#include "UserInterface.h"
#include "Functions.h"
#include "SharedElements.h"
#include "PlayersManager.h"
#include "MidiReader.h"
#include "LillaClock.h"
#include "LillaSerialFlash.h"
#include "ArchivingManager.h"
#include "Switches.h"
#include "ShiftRegisters.h"
#include "PointerSampler.h"
#include "GlobalDisplaySetup.h"
#include "GlobalDisplayStorage.h"
#include "GlobalDisplaySampler.h"

static int8_t SET_menu;                                // Current Setup menu selection.

static void SET_Ask_if_IMPORT_EXPORT_setup(void);
static void SET_Ask_if_FACTORY_RESET(void);

void Handle_Setup(void)
{
    if (Lilla_state == SETUP)
    {
        // Change Patch VOLUME
        if (Read_encoder(EN_PB_LineOutVol, volume_patch, PATCH_VOLUME_MAX, 0, 1))
        {
            AudioNoInterrupts();
            Players_Manager.Update_all_Preset_volume(Patch_id, Patch_volume_gain(volume_patch));
            Players_Manager.Broadcast_volume();
            AudioInterrupts();
        }

        // Set Key Step
        if (SET_menu == 0 && Read_encoder_inverse(EN_PB_Value, key_step, 3, 0, 1))
        {
            Display_Setup.SETUP_show_Key_step_value();
            Calc_pitch_from_note(key_step);
            Require_FRAM(Archive.Save_key_step(static_cast<uint8_t>(key_step)));
        }

        // Set Prima ottava
        if (SET_menu == 1 && Read_encoder(EN_PB_Value, first_octave, 0, -2, 1))
        {
            Display_Setup.SETUP_show_First_octave_value();
        }

        // Change menu item  -  uint8_t SET_menu;
        result = Read_encoder_simple(EN_PB_Select);
        if (result != 0)
        {
            SET_menu = (SET_menu + result + 7) % 7;
            Display_Setup.SETUP_show_frame(SET_menu);

            Clear_UI_events();
        }

        // Choose menu item
        if (Read_pushbutton(EN_PB_Select))
        {
            switch (SET_menu)
            {
            case 2: // switch to CC Settings
                Golive_CC_SETTINGS();
                break;

                // case 2: // USB access to SD card - funzionalita' MTP
                // break;

            case 3: // import RAW files from SD
            {
                const bool resume_controls = Trigger.Is_running();
                if (!P_Quiesce_audio_players())
                {
                    break;
                }

                bool flash_changed = false;
                if (SET_Copy_audio_files_from_SD_to_Flash(flash_changed))
                {
                    VFS_Make_VFS();
                    DS_seed_all_Recordings();
                    File_scanner.Read_all_file_data();

                    // switch off Tools LED
                    TOOLS_pushbutton = false;
                    Shifters_manager.Switch_led(LED_Tools, false);

                    Reload_system_state();
                }
                else
                {
                    // Cancellation can resume the old inventory; a failed destructive import cannot.
                    if (!flash_changed && resume_controls)
                    {
                        AudioNoInterrupts();
                        const bool ready = S_Fill_all_tables();
                        if (ready)
                        {
                            Midi_reader.Start();
                            Trigger.Start();
                        }
                        AudioInterrupts();
                    }
                    if (flash_changed)
                    {
                        Serial.println(F("RAW import failed; audio remains stopped. Retry the import."));
                    }
                    Display_Setup.SETUP_show_SETUP_page();
                    Display_Setup.SETUP_show_frame(SET_menu);
                }
                break;
            }

            case 4: // Restore configuration and Recording audio from the backup root.
                Display_Storage.Confirm_config_import_popup();
                Display_Storage.Confirm_config_import_frame(0);
                SET_Ask_if_IMPORT_EXPORT_setup();
                if (result == 0)
                {
                    Display_Setup.SETUP_show_SETUP_page();
                    Display_Setup.SETUP_show_frame(SET_menu);
                    break;
                }

                // check SD presence
                if (!SD.begin(BUILTIN_SDCARD))
                {
                    Display_Setup.SETUP_show_SETUP_page();
                    Display_Setup.SETUP_show_frame(SET_menu);
                    break;
                }
                if (!SD.exists("/LILLABACKUP/LILLA_CONFIG.fram"))
                {
                    Display_Setup.SETUP_show_SETUP_page();
                    Display_Setup.SETUP_show_frame(SET_menu);
                    break;
                }
                else
                {
                    const bool resume_controls = Trigger.Is_running();
                    if (!P_Quiesce_audio_players())
                    {
                        break;
                    }
                    bool config_error = false;
                    if (!BACKUP_Restore(&config_error))
                    {
                        if (config_error)
                        {
                            // Validation failed before changing storage, but quiescing discarded the runtime tables.
                            if (resume_controls)
                            {
                                const bool audio_enabled = NVIC_IS_ENABLED(IRQ_SOFTWARE) != 0;
                                AudioNoInterrupts();
                                const bool ready = S_Fill_all_tables();
                                if (ready)
                                {
                                    Midi_reader.Start();
                                    Trigger.Start();
                                }
                                if (audio_enabled)
                                {
                                    AudioInterrupts();
                                }
                            }
                            Display_Setup.SETUP_show_SETUP_page();
                            Display_Setup.SETUP_show_frame(SET_menu);
                            break;
                        }
                        Serial.println(F("Full restore failed: check configuration, audio CRCs and packet capacity. Retry from /LILLABACKUP."));
                        Display_Storage.Config_import_FILE_error_popup();
                        delay(5000);
                        Reload_system_state();
                        break;
                    }
                    Serial.println(F("Configuration and Recording audio restored and verified."));

                    // switch off Tools LED
                    TOOLS_pushbutton = false;
                    Shifters_manager.Switch_led(LED_Tools, false);

                    Reload_system_state();
                }
                break;

            case 5: // Create a new numbered backup with Recording audio.
                Display_Storage.Confirm_config_export_popup();
                Display_Storage.Confirm_config_import_frame(0);
                SET_Ask_if_IMPORT_EXPORT_setup();
                if (result == 0)
                {
                    Display_Setup.SETUP_show_SETUP_page();
                    Display_Setup.SETUP_show_frame(SET_menu);
                    break;
                }

                // check SD presence
                if (SD.begin(BUILTIN_SDCARD))
                {
                    if (BACKUP_Export())
                    {
                        Display_Storage.Config_export_save_popup();
                    }
                    else
                    {
                        Display_Storage.Config_export_SD_error_popup();
                    }
                    delay(5000);
                    Display_Setup.SETUP_show_SETUP_page();
                    Display_Setup.SETUP_show_frame(SET_menu);
                }

                else
                {
                    Display_Storage.SD_missing(ILI9341_BLACK);
                    delay(5000);
                    Display_Setup.SETUP_show_SETUP_page();
                    Display_Setup.SETUP_show_frame(SET_menu);
                    break;
                }

                break;

            case 6: // Factory reset
                Display_Storage.Confirm_factory_reset_popup();
                Display_Storage.Confirm_config_import_frame(0);

                SET_Ask_if_FACTORY_RESET();
                if (result == 0)
                {
                    Display_Setup.SETUP_show_SETUP_page();
                    Display_Setup.SETUP_show_frame(SET_menu);
                    break;
                }
                Display_Storage.Factory_reset_wait_popup();

                delay(3000); // per ripensamenti last minute!

                // switch off Tools LED
                TOOLS_pushbutton = false;
                Shifters_manager.Switch_led(LED_Tools, false);

                if (!P_Quiesce_audio_players())
                {
                    break;
                }
                Factory_setup_FRAM();
                Reload_system_state();
                break;

            default:
                PRINT_ERROR(F("Switch MISSING! "));
                break;
            }
        }

        // Switch verso un TOOL
        if (Switches_manager.Get_change(SwitchTools))
        {
            switch (Switches_manager.Get_value(SwitchTools))
            {
            case SwToolsMixer:
            {
                if (first_octave != first_octave_cache)
                {
                    Require_FRAM(Archive.Save_first_octave(first_octave));
                }
                Switch_to_MIXER();
                break;
            }
            break;

            case SwToolsDelay:
            {
                if (first_octave != first_octave_cache)
                {
                    Require_FRAM(Archive.Save_first_octave(first_octave));
                }

                Golive_DELAY_SETTINGS();
                break;
            }
            break;

            case SwToolsSetup:
                break;

            case SwToolsTest:
            {
                if (first_octave != first_octave_cache)
                {
                    Require_FRAM(Archive.Save_first_octave(first_octave));
                }

                Golive_MIDI_MONITOR();
                break;
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
                if (first_octave != first_octave_cache)
                {
                    Require_FRAM(Archive.Save_first_octave(first_octave));
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
                    Switch_from_MIDI_LOOP_to_DIRECT_SAMPLING();
                    break;

                default:
                    PRINT_ERROR(F("Switch MISSING! "));
                    break;
                }
                break;
            }
            break;

            case SwModesLiveSampler:
            {
                if (first_octave != first_octave_cache)
                {
                    Require_FRAM(Archive.Save_first_octave(first_octave));
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
                    Switch_from_MIDI_LOOP_to_LIVE_SAMPLING();
                    break;

                default:
                    PRINT_ERROR(F("Switch MISSING! "));
                    break;
                }
                break;
            }
            break;

            case SwModesPerformance:
            {
                if (first_octave != first_octave_cache)
                {
                    Require_FRAM(Archive.Save_first_octave(first_octave));
                }

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
                break;
            }
            break;

            case SwModesMidiLoop:
            {
                if (first_octave != first_octave_cache)
                {
                    Require_FRAM(Archive.Save_first_octave(first_octave));
                }
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
                break;
            }
            break;
            }
        }
    }
}

void Golive_SETUP(void)
{
    Lilla_state = SETUP;

    SET_menu = 0;
    Display_Setup.SETUP_show_SETUP_page();

    Clear_UI_events();

    Display_Setup.SETUP_show_frame(SET_menu);
}

void Switch_from_MIDI_LOOP_to_SETUP(void)
{
    Lilla_state_0 = MIDI_LOOP;
    Golive_SETUP();
}

static void SET_Ask_if_IMPORT_EXPORT_setup(void)
{
    confirmation = false;
    result = 0;

    Clear_UI_events();
    while (!confirmation)
    {
        Shifters_manager.Update();

        if (Read_encoder(EN_PB_Select, result, 1, 0, 1))
        {
            Display_Storage.Confirm_config_import_frame(result);
        }
        if (Read_pushbutton(EN_PB_Select))
        {
            confirmation = true;
        }
    }

    Clear_UI_events();
}

static void SET_Ask_if_FACTORY_RESET(void)
{
    confirmation = false;
    result = 0;

    Clear_UI_events();
    while (!confirmation)
    {
        Shifters_manager.Update();

        if (Read_encoder(EN_PB_Select, result, 1, 0, 1))
        {
            Display_Storage.Confirm_config_import_frame(result);
        }
        if (Read_pushbutton(EN_PB_Select))
        {
            confirmation = true;
        }
    }
    Clear_UI_events();
}
