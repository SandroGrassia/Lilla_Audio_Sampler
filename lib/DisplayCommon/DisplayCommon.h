/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>     // https://learn.adafruit.com/adafruit-gfx-graphics-library/graphics-primitives
#include <ILI9341_t3n.h>
#include <AudioStream.h>      // solo per definizione AUDIO_SAMPLE_RATE
#include "DisplayPrimitives.h"

#include "SharedElements.h"
#include "SharedSampler.h"
#include "SharedLiveSampler.h"
#include "SharedLoop.h"
#include "SharedDelay.h"
#include "SharedMixer.h"
#include "GlobalInfoMaster.h"

class DisplayCommon
{
private:

    // Logo
    void Logo(const float light);
    void Cover_text(const float light);
    uint16_t Calc_color(uint16_t color_peak, float light);
    static constexpr int Logo_position_DX = 80; // pixel
    static constexpr int Logo_position_DY = 45; // pixel
    static constexpr int Text_position_DY = 175; // pixel

    // Avvisi
    //                         "0123456789012345678901234..7890123456789109876543210";
    const char ADV_VFS_0[50] = "    NOT ENOUGH MEMORY LEFT FOR DIRECT-SAMPLING!";
    const char ADV_VFS_1[50] = "     IF DIRECT-SAMPLING IS NEEDED PLEASE REPEAT";
    const char ADV_VFS_2[50] = "         IMPORT WITH MAX 63MB OF RAW FILES";
    const char ADV_VFS_3[50] = "     RAW FILES IMPORT AND MEMORY CONFIGURATION";
    const char ADV_VFS_4[50] = "     COMPLETED. LILLA RESTARTS IN FEW SECONDS.";

    // Show_popup_text
    int L_POPUP;     // Larghezza
    int H_POPUP;     // Altezza
    int X_POPUP;     // X posizione su display
    int Y_POPUP;     // Y posizione su display
    int Y_POPUP_TXT; // prima riga testo
    int X_POPUP_OPT; // riga opzioni
    int Y_POPUP_OPT; // riga opzioni

    // Shared patch header
    static constexpr float P_column_PATCH = 30;
    static constexpr float P_column_VOLUME = 41;
    static constexpr float P_column_Volume_value = 47.5;
    // SETUP
    // Control Change
    static constexpr int Setup_Control_change_X = 13; // posizione (in caratteri)

    // DELAY
    static constexpr int Delay_ROW_BASE = 6;

public:
    static constexpr float P_column_Patch_id = 36;

    DisplayCommon() {}

    void Lilla_cover_slow(void);
    void Lilla_cover_saturate(void);

    // Funzioni comuni
    void Confirm_no_yes_popup_frame(int value);
    void Show_all_effects(void);
    void Resolution(void);
    void Downsampling(void);
    void Lowpass_filter(void);

    // Gestione LED
    void Led_SOUND_EDIT_instrument(int instrument_id, bool on);
    void Led_INSTRUMENT_VCF_instrument(int instrument_id, bool on);
    void Led_DIRECT_SAMPLING(bool on);

    // Shared patch header
    void P_show_PERFORMANCE_title(void);
    void P_show_Patch_number(bool change_patch);
    void P_Patch_VOLUME(bool change_vol); // shows VOLUME <value>
    void P_Patch_volume_value(bool change_vol);
    void Patch_volume_color(bool change_patch, bool change_vol);

    // SETUP
    void SETUP_show_SETUP_page(void);
    void SETUP_show_Key_step_value(void);
    void SETUP_show_First_octave_value(void);
    void SETUP_show_frame(int8_t value);

    // CONTROL CHANGE
    void CC_show_ControlChange_page(void);
    void CC_show_all_sound_gains(void);
    void CC_show_sound_gain(int value);
    void CC_show_lowpass_filter_value(void);
    void CC_show_frame_menu(int value);

    // VFS
    void VFS_show_packets(void);
    void VFS_Make_presentation(void);
    void VFS_Make_assignments(void);
    void VFS_Make_restart(void);
    void VFS_Make_not_enough_memory_for_sampler(void);

    // copia RAW files da SD
    void Import_raw_files_frame(uint8_t value);
    void Confirm_config_import_popup(void);
    void SD_missing(uint16_t color);
    void Config_import_FILE_error_popup(void);
    void FRAM_recovery_popup(void);
    void FRAM_io_error_popup(void);
    void Config_import_REBOOT_popup(void);
    void Confirm_config_import_frame(uint8_t value);
    void Confirm_config_export_popup(void);
    void Config_export_SD_error_popup(void);
    void Config_export_save_popup(void);
    void Confirm_factory_reset_popup(void);
    void Factory_reset_wait_popup(void);
    void Encoder_pushbutton_test_board(void);
    void Encoder_pushbutton_test_result(const int device, const int element, const int value);
    void Config_reset_popup(void);
    void Copy_raw_files_SD_to_Flash_chip_titolo(void);           // importa i file RAW dalla scheda SD
    void Copy_raw_files_SD_to_Flash_chip_waiting_for_SD(void);   // attesa 10sec scheda SD
    void Copy_raw_files_SD_to_Flash_chip_lillaraw_missing(void); // manca /LILLARAW
    void Copy_raw_files_SD_to_Flash_chip_files_report(unsigned long SD_raw_volume, int SD_raw_files, int flash_raw_volume, int flash_raw_files);
    void Copy_raw_files_SD_to_Flash_chip_last_warning(float erasing_time_ms);
    void Copy_raw_files_SD_to_Flash_chip_job_start(void); // inizia la cancellazione (erasing) della Flash memory
    void Update_raw_copy_progress(int barcount);
    void Copy_raw_files_SD_to_Flash_chip_popup_landscape(void); // black panel
    void Copy_raw_files_SD_to_Flash_chip_list_landscape(void);
    void Copy_raw_files_SD_to_Flash_chip_files_to_copy(int row, const char *filename, unsigned long length);
    void Copy_raw_files_SD_to_Flash_chip_flash_error(void);      // Flash memory error!
    void Copy_raw_files_SD_to_Flash_chip_flash_full_error(void); // Flash memory full!
    void Copy_raw_files_SD_to_Flash_chip_job_done(void);
    void Copy_raw_files_SD_to_Flash_chip_file_copied(int row, const char *filename, uint32_t filesize);

    // MIDI_MONITOR
    void Midi_monitor_page(void);
    void Midi_monitor_frame(void);
    void Midi_monitor_data(uint8_t incoming_midi_channel, uint8_t incoming_midi_message, int8_t incoming_note_number, int8_t incoming_velocity, int32_t incoming_midi_value, int8_t incoming_number);
};
