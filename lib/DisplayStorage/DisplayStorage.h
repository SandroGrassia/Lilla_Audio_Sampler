/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#pragma once

#include "DisplayPrimitives.h"

// Storage, audio import, backup, restore and reset screens.
class DisplayStorage
{
private:
    static constexpr int Import_popup_lines = 6;
    static constexpr int Backup_popup_lines = 7;
    static constexpr int Restore_popup_lines = 9;
    static constexpr int Factory_reset_popup_lines = 7;

    // Avvisi
    //                         "0123456789012345678901234..7890123456789109876543210";
    const char ADV_VFS_0[50] = "        NOT ENOUGH MEMORY LEFT FOR SAMPLING!";
    const char ADV_VFS_1[52] = "PLEASE IMPORT SHORTER AUDIO FILES TO FREE UP MEMORY";
    const char ADV_VFS_2[50] = "";
    const char ADV_VFS_3[50] = "    AUDIO FILES IMPORT AND MEMORY CONFIGURATION";
    const char ADV_VFS_4[50] = "     COMPLETED - LILLA RESTARTS IN FEW SECONDS";

    // Show_popup_text
    int L_POPUP;     // Larghezza
    int H_POPUP;     // Altezza
    int X_POPUP;     // X posizione su display
    int Y_POPUP;     // Y posizione su display
    int Y_POPUP_TXT; // prima riga testo
    int X_POPUP_OPT; // riga opzioni
    int Y_POPUP_OPT; // riga opzioni

public:
    DisplayStorage() {}

    FLASHMEM void VFS_show_packets(void);
    FLASHMEM void VFS_Make_presentation(void);
    FLASHMEM void VFS_Make_assignments(void);
    FLASHMEM void VFS_Make_restart(void);
    FLASHMEM void VFS_Make_not_enough_memory_for_sampler(void);
    FLASHMEM void Import_raw_files_frame(uint8_t value);
    FLASHMEM void Confirm_config_import_popup(void);
    FLASHMEM void Confirm_config_restore_frame(uint8_t value);
    FLASHMEM void SD_missing(uint16_t color);
    void Config_import_FILE_error_popup(void);
    void FRAM_recovery_popup(void);
    FLASHMEM void Confirm_factory_reset_frame(uint8_t value);
    FLASHMEM void Confirm_config_export_popup(void);
    FLASHMEM void Confirm_config_export_frame(uint8_t value);
    FLASHMEM void Config_export_SD_error_popup(void);
    FLASHMEM void Config_export_save_popup(void);
    FLASHMEM void Confirm_factory_reset_popup(void);
    FLASHMEM void Factory_reset_wait_popup(void);
    FLASHMEM void Copy_audio_files_SD_to_Flash_chip_titolo(void);           // importa i file audio dalla scheda SD
    FLASHMEM void Copy_raw_files_SD_to_Flash_chip_waiting_for_SD(void);   // attesa 10sec scheda SD
    FLASHMEM void Copy_raw_files_SD_to_Flash_chip_flash_audio_missing(void); // manca /FLASH_AUDIO_FILES
    FLASHMEM void Copy_raw_files_SD_to_Flash_chip_files_report(unsigned long SD_raw_volume, int SD_raw_files, int flash_raw_volume, int flash_raw_files);
    FLASHMEM void Copy_audio_files_SD_to_Flash_chip_last_warning(void);
    FLASHMEM void Copy_raw_files_SD_to_Flash_chip_job_start(float erasing_time_ms); // inizia la cancellazione (erasing) della Flash memory
    FLASHMEM void Update_raw_copy_progress(int barcount);
    FLASHMEM void Copy_raw_files_SD_to_Flash_chip_popup_landscape(void); // black panel
    FLASHMEM void Copy_raw_files_SD_to_Flash_chip_list_landscape(void);
    FLASHMEM void Copy_raw_files_SD_to_Flash_chip_files_to_copy(int row, const char *filename, unsigned long length);
    FLASHMEM void Copy_audio_files_SD_to_Flash_chip_invalid_audio(int row, const char *filename);
    FLASHMEM void Copy_raw_files_SD_to_Flash_chip_duplicate(int row, const char *filename);
    FLASHMEM void Copy_raw_files_SD_to_Flash_chip_flash_error(void);      // Flash memory error!
    FLASHMEM void Copy_raw_files_SD_to_Flash_chip_flash_full_error(void); // Flash memory full!
    FLASHMEM void Copy_raw_files_SD_to_Flash_chip_job_done(void);
    FLASHMEM void Copy_raw_files_SD_to_Flash_chip_file_copied(int row, const char *filename, uint32_t filesize);
};
