/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#pragma once

#include "DisplayPrimitives.h"

// Storage, RAW import, backup, restore and reset screens.
class DisplayStorage
{
private:
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

public:
    DisplayStorage() {}

    void VFS_show_packets(void);
    void VFS_Make_presentation(void);
    void VFS_Make_assignments(void);
    void VFS_Make_restart(void);
    void VFS_Make_not_enough_memory_for_sampler(void);
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
};
