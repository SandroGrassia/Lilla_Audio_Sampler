/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#include "DisplayStorage.h"
#include "SharedElements.h"
#include "SharedSampler.h"
#include "SharedVFS.h"

FLASHMEM
void DisplayStorage::VFS_show_packets(void)
{
    Cancel_text_reset_cursor(display_coordinate_x(21), display_coordinate_y(7), 40);
    Frame_by_col_row(21, 7, 21, true);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print((VFS_packets * PACKET_DIM) / 1048576.0f, 2);
    Show_measure_unit("MB", 2);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(" (");
    tft.print(VFS_packets * 0.743, 0);
    Show_measure_unit("sec", 3);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(" MONO)");

    Cancel_text_reset_cursor(display_coordinate_x(27), display_coordinate_y(8), 40);
    tft.setTextColor(ILI9341_WHITE);
    tft.print(((VFS_packets_max - VFS_packets) * PACKET_DIM) / 1048576.0f, 2);
    Show_measure_unit("MB", 2);
    tft.setTextColor(ILI9341_WHITE);
    tft.print(" (");
    tft.print((VFS_packets_max - VFS_packets) * 0.743, 0);
    Show_measure_unit("sec", 3);
    tft.setTextColor(ILI9341_WHITE);
    tft.print(")");
}

FLASHMEM
void DisplayStorage::Import_raw_files_frame(uint8_t value)
{
    const int left = Centered_element_left(8 * 6);
    const int top = Centered_element_top(display_coordinate_y(Import_popup_lines)) + 5 + 15 * (Import_popup_lines - 1);
    Confirm_frame_on_RED(left, top, 2, value == 0);
    Confirm_frame_on_RED(left + 5 * 6, top, 3, value == 1);
}

FLASHMEM
void DisplayStorage::Confirm_config_import_popup(void)
{
    const char *lines[Restore_popup_lines] = {"PLEASE COPY THE CHOSEN BACKUP'S CONTENTS", "INTO SD /LILLABACKUP FIRST", "", "- WARNING -", "RESTORE CONFIGURATION DELETES PATCHES AND", "AUDIO RECORDING. AUDIO FILES IN FLASH ARE KEPT", "", "DO YOU WANT TO PROCEED?", "CANCEL    GO"};
    Show_popup_text(lines, Restore_popup_lines, ILI9341_WHITE, ILI9341_RED);
}

FLASHMEM void DisplayStorage::Confirm_config_restore_frame(uint8_t value)
{
    const int left = Centered_element_left(12 * 6);
    const int top = Centered_element_top(display_coordinate_y(Restore_popup_lines)) + 5 + 15 * (Restore_popup_lines - 1);
    Confirm_frame_on_RED(left, top, 6, value == 0);
    Confirm_frame_on_RED(left + 10 * 6, top, 2, value == 1);
}

FLASHMEM
void DisplayStorage::Factory_reset_wait_popup(void)
{
    L_POPUP = display_coordinate_x(38);
    H_POPUP = display_coordinate_y(3); // 64 pixel
    Y_POPUP = Centered_element_top(H_POPUP);
    X_POPUP = Centered_element_left(L_POPUP);
    Y_POPUP_TXT = 10; // Prima riga testo

    tft.fillRoundRect(X_POPUP, Y_POPUP, L_POPUP, H_POPUP, 4, ILI9341_YELLOW);
    tft.setCursor(X_POPUP + display_coordinate_x(0), Y_POPUP + Y_POPUP_TXT);
    tft.setTextColor(ILI9341_BLACK);

    //       ("01234567890123456789012345678901234567");
    tft.print(F("            FACTORY RESET"));
    tft.setCursor(X_POPUP + display_coordinate_x(0), Y_POPUP + Y_POPUP_TXT + display_coordinate_y(1));
    tft.setTextColor(ILI9341_BLACK);

    //       ("01234567890123456789012345678901234567");
    tft.print(F("    PLEASE WAIT - DO NOT SWITCH OFF"));
}

FLASHMEM
void DisplayStorage::SD_missing(uint16_t color)
{
    L_POPUP = display_coordinate_x(18);
    H_POPUP = display_coordinate_y(3);
    Y_POPUP = Centered_element_top(H_POPUP);
    X_POPUP = Centered_element_left(L_POPUP);
    Y_POPUP_TXT = 20; // Prima riga testo

    tft.fillRoundRect(X_POPUP, Y_POPUP, L_POPUP, H_POPUP, 4, color);
    tft.setCursor(X_POPUP + display_coordinate_x(0), Y_POPUP + Y_POPUP_TXT);
    tft.setTextColor(ILI9341_WHITE);

    //       ("012345678901234567");
    tft.print(F("  SD NOT PRESENT"));
}

void DisplayStorage::FRAM_recovery_popup(void)
{
    tft.fillScreen(ILI9341_BLACK);
    tft.setTextSize(1);
    tft.setTextColor(ILI9341_YELLOW);
    tft.setCursor(10, 70);
    tft.println(F("ARCHIVE UNAVAILABLE - RESTORE REQUIRED"));
    tft.setCursor(10, 100);
    tft.println(F("Put backup files in /LILLABACKUP root"));
    tft.setCursor(10, 130);
    tft.println(F("LILLA_CONFIG.fram + REC audio files"));
    tft.setCursor(10, 150);
    tft.println(F("SELECT or serial R: retry full restore"));
}

void DisplayStorage::Config_import_FILE_error_popup(void)
{
    L_POPUP = display_coordinate_x(35);
    H_POPUP = display_coordinate_y(3);
    Y_POPUP = Centered_element_top(H_POPUP);
    X_POPUP = Centered_element_left(L_POPUP);
    Y_POPUP_TXT = 20; // Prima riga testo

    tft.fillRoundRect(X_POPUP, Y_POPUP, L_POPUP, H_POPUP, 4, ILI9341_BLACK);
    tft.setCursor(X_POPUP + display_coordinate_x(0), Y_POPUP + Y_POPUP_TXT);
    tft.setTextColor(ILI9341_YELLOW);

    //       ("01234567890123456789012345678901234");
    tft.print(F(" INVALID BACKUP OR NOT ENOUGH FLASH"));
}

FLASHMEM
void DisplayStorage::Confirm_factory_reset_frame(uint8_t value)
{
    const int left = Centered_element_left(8 * 6);
    const int top = Centered_element_top(display_coordinate_y(Factory_reset_popup_lines)) + 5 + 15 * (Factory_reset_popup_lines - 1);
    Confirm_frame_on_RED(left, top, 2, value == 0);
    Confirm_frame_on_RED(left + 5 * 6, top, 3, value == 1);
}

FLASHMEM
void DisplayStorage::Confirm_config_export_popup(void)
{
    const char *lines[Backup_popup_lines] = {"SAVE CONFIGURATION AND RECORDING AUDIO", "TO A NEW NUMBERED FOLDER /LILLABACKUP IN SD", "(PREVIOUS BACKUPS ARE KEPT)", "- NOTICE -", "AUDIO LIBRARIES AND MIDI LOOPS ARE", "NOT INCLUDED IN BACKUP", "CANCEL    GO"};
    Show_popup_text(lines, Backup_popup_lines, ILI9341_BLACK, ILI9341_GREEN);
}

FLASHMEM void DisplayStorage::Confirm_config_export_frame(uint8_t value)
{
    const int left = Centered_element_left(12 * 6);
    const int top = Centered_element_top(display_coordinate_y(Backup_popup_lines)) + 5 + 15 * (Backup_popup_lines - 1);
    Confirm_frame_on_GREEN(left, top, 6, value == 0, ILI9341_BLACK);
    Confirm_frame_on_GREEN(left + 10 * 6, top, 2, value == 1, ILI9341_BLACK);
}

FLASHMEM
void DisplayStorage::Config_export_SD_error_popup(void)
{
    L_POPUP = display_coordinate_x(27);
    H_POPUP = display_coordinate_y(3);
    Y_POPUP = Centered_element_top(H_POPUP);
    X_POPUP = Centered_element_left(L_POPUP);
    Y_POPUP_TXT = 20; // Prima riga testo
    tft.fillRoundRect(X_POPUP, Y_POPUP, L_POPUP, H_POPUP, 4, ILI9341_BLACK);

    tft.setCursor(X_POPUP, Y_POPUP + Y_POPUP_TXT);
    tft.setTextColor(ILI9341_YELLOW);

    //       ("012345678901234567890123456");
    tft.print(F(" BACKUP FAILED - CHECK LOG"));
}

FLASHMEM
void DisplayStorage::Config_export_save_popup(void)
{
    Show_popup_text("NUMBERED BACKUP SAVED IN /LILLABACKUP", ILI9341_WHITE, ILI9341_BLACK);
}

FLASHMEM
void DisplayStorage::Confirm_factory_reset_popup(void)
{
    const char *lines[Factory_reset_popup_lines] = {"- WARNING -", "RESTORE FACTORY SETTINGS", "DELETES ALL PATCHES AND AUDIO RECORDINGS", "ONLY IMPORTED AUDIO FILES ARE KEPT", "", "DO YOU WANT TO PROCEED?", "NO   YES"};
    Show_popup_text(lines, Factory_reset_popup_lines, ILI9341_WHITE, ILI9341_RED);
}

FLASHMEM
void DisplayStorage::VFS_Make_presentation(void)
{
    tft.fillScreen(ILI9341_BLACK);

    tft.setCursor(display_coordinate_x(0), display_coordinate_y(0));
    tft.setTextColor(TEXT_COLOR);
    tft.print(F("SAMPLER: RECORDING MEMORY DIMENSION"));

    tft.setCursor(display_coordinate_x(0), display_coordinate_y(2));
    tft.setTextColor(ILI9341_YELLOW);
    //        "012345678901234567890 234 X 432 98765432109876543210"; // max 52 char
    tft.print(F(" PLEASE ASSIGN THE MEMORY SPACE FOR RECORDINGS (AND"));
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(3));
    tft.print(F("     THE CONSEQUENT SPACE FOR RAW FILES EXPORT)"));

    tft.setCursor(display_coordinate_x(0), display_coordinate_y(5));
    tft.setTextColor(TEXT_COLOR);
    tft.print(F("TOTAL FLASH MEMORY SPACE "));
    tft.setTextColor(ILI9341_WHITE);
    tft.print(Get_flash_size() / 1048576.0f);
    Show_measure_unit("MB", 2);

    tft.setCursor(display_coordinate_x(0), display_coordinate_y(6));
    tft.setTextColor(TEXT_COLOR);
    tft.print(F("AUDIO FILES IMPORTED "));
    tft.setTextColor(ILI9341_WHITE);
    tft.print(Get_flash_occupation() / 1048576.0f);
    Show_measure_unit("MB", 2);
}

FLASHMEM
void DisplayStorage::VFS_Make_assignments(void)
{
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(7));
    tft.setTextColor(TEXT_COLOR);

    // tft.print("0123456789012345678901234567890....."); // max 52 char
    tft.print(F("SPACE FOR RECORDINGS"));

    tft.setCursor(display_coordinate_x(0), display_coordinate_y(8));
    tft.setTextColor(TEXT_COLOR);

    // tft.print("0123456789012345678901234567890....."); // max 52 char
    tft.print(F("SPACE FOR RAW FILES EXPORT"));
}

FLASHMEM
void DisplayStorage::VFS_Make_restart(void)
{
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(10));
    tft.setTextColor(ILI9341_GREEN);
    tft.print(ADV_VFS_3);
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(11));
    tft.print(ADV_VFS_4);
}

FLASHMEM
void DisplayStorage::VFS_Make_not_enough_memory_for_sampler(void)
{
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(10));
    tft.setTextColor(ILI9341_MAGENTA);
    tft.print(ADV_VFS_0);
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(11));
    tft.print(ADV_VFS_1);
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(12));
    tft.print(ADV_VFS_2);
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(13));
    tft.print(ADV_VFS_3);
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(14));
    tft.print(ADV_VFS_4);
}

FLASHMEM
void DisplayStorage::Copy_audio_files_SD_to_Flash_chip_titolo(void)
{
    tft.fillScreen(ILI9341_BLACK);
    tft.setTextColor(TEXT_COLOR);
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(0));
    tft.print(F("IMPORT AUDIO FILES FROM SD TO FLASH MEMORY"));
}

FLASHMEM
void DisplayStorage::Copy_raw_files_SD_to_Flash_chip_waiting_for_SD(void)
{
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(3));
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(F("WAITING 10sec FOR SD CARD"));
}

FLASHMEM
void DisplayStorage::Copy_raw_files_SD_to_Flash_chip_flash_audio_missing(void)
{
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(3));
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(F("NO FILES TO IMPORT: MISSING /FLASH_AUDIO DIRECTORY!"));
}

FLASHMEM
void DisplayStorage::Copy_raw_files_SD_to_Flash_chip_files_report(unsigned long SD_raw_volume, int SD_raw_files, int raw_files_volume, int flash_raw_files)
{
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(3));
    tft.setTextColor(TEXT_COLOR);
    tft.print(F("SOURCE: SD CARD /FLASH_AUDIO"));
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(4));
    tft.print(F("- AUDIO FILES"));
    tft.setCursor(display_coordinate_x(14), display_coordinate_y(4));
    tft.setTextColor(ILI9341_WHITE);
    tft.print(SD_raw_files);
    tft.print(" (");
    tft.setTextColor(ILI9341_WHITE);
    tft.print(SD_raw_volume / 1048576.0f, 2);
    Show_measure_unit("MB", 2);
    tft.setTextColor(ILI9341_WHITE);
    tft.print(")");

    // Flash chip info
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(6));
    tft.setTextColor(TEXT_COLOR);
    tft.println(F("DESTINATION: LILLA FLASH MEMORY"));
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(7));
    tft.println(F("- DIMENSION"));
    tft.setCursor(display_coordinate_x(12), display_coordinate_y(7));
    tft.setTextColor(ILI9341_WHITE);
    tft.print(verified_flash_memory_MB);
    Show_measure_unit("MB", 2);

    tft.setCursor(display_coordinate_x(0), display_coordinate_y(8));
    tft.setTextColor(TEXT_COLOR);
    tft.print("- AUDIO FILES");
    tft.setCursor(display_coordinate_x(14), display_coordinate_y(8));
    tft.setTextColor(ILI9341_WHITE);
    tft.print(flash_raw_files); // Get_raw_files()
    tft.print(" (");
    tft.setTextColor(ILI9341_WHITE);
    tft.print(raw_files_volume / 1048576.0f, 2); // Get_raw_files_volume()
    Show_measure_unit("MB", 2);
    tft.setTextColor(ILI9341_WHITE);
    tft.print(")");

    tft.setCursor(display_coordinate_x(0), display_coordinate_y(9));
    tft.setTextColor(TEXT_COLOR);
    tft.print(F("- RECORDINGS"));
    tft.setCursor(display_coordinate_x(13), display_coordinate_y(9));
    tft.setTextColor(ILI9341_WHITE);
    tft.print(recordings);
}

FLASHMEM
void DisplayStorage::Copy_audio_files_SD_to_Flash_chip_last_warning(void)
{
    const char *lines[Import_popup_lines] = {"IMPORT AUDIO FROM SD /FLASH_AUDIO", "TO LILLA 64MB FLASH MEMORY.", "FILES LONGER THAN 33 SECONDS ARE TRUNCATED", "- WARNING -", "ALL EXISTING FLASH AUDIO AND RECORDINGS ARE DELETED", "NO   YES"};
    Show_popup_text(lines, Import_popup_lines, ILI9341_WHITE, ILI9341_RED);
}

FLASHMEM
void DisplayStorage::Copy_raw_files_SD_to_Flash_chip_job_start(float erasing_time_ms)
{
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(10));
    tft.setTextColor(TEXT_COLOR);
    tft.println(F("- DELETING TIME"));
    tft.setCursor(display_coordinate_x(16), display_coordinate_y(10));
    tft.setTextColor(ILI9341_WHITE);
    tft.print(erasing_time_ms / 60000, 1);
    Show_measure_unit("min", 3);

    // Start erasing flash chip
    tft.fillRect(0, display_coordinate_y(11), 320, 240, ILI9341_BLACK);
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(12) - 5);
    tft.setTextColor(ILI9341_YELLOW);

    //        "012345678901234567890 234 X 432 98765432109876543210"; // max 52 char
    tft.print(F("PLEASE WAIT: FLASH MEMORY ERASE IS RUNNING."));
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(13) - 5);
    tft.print(F("THEN AUDIO FILES WILL BE COPIED FROM SD/FLASH_AUDIO"));
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(14) - 5);
    tft.print("TO LILLA FLASH MEMORY");
}

FLASHMEM
void DisplayStorage::Update_raw_copy_progress(int percentage)
{
    const int raw_copy_progress_Y = 225; // display_coordinate_y(15)
    const int chars_to_cancel = (percentage == 101) ? 11 : 3;

    if (percentage > 101)
    {
        return;
    }

    const auto x_coordinate = display_coordinate_x(0);

    // Da 1 in poi cancella la percentuale precedente.
    if (percentage > 0)
    {
        Cancel_text(x_coordinate + percentage - 1 + 10, raw_copy_progress_Y, chars_to_cancel);
    }

    // A 101 cancella il 100%, senza disegnare altro.
    if (percentage > 100)
    {
        return;
    }

    tft.drawLine(x_coordinate + percentage, raw_copy_progress_Y, x_coordinate + percentage, raw_copy_progress_Y + 5, ILI9341_YELLOW);
    tft.setCursor(x_coordinate + percentage + 10, raw_copy_progress_Y);

    tft.setTextColor(ILI9341_YELLOW);
    tft.print(percentage);
    tft.print("%");

    if (percentage == 100)
    {
        tft.print(" *DONE*");
    }
}

FLASHMEM
void DisplayStorage::Copy_raw_files_SD_to_Flash_chip_popup_landscape(void)
{
    // Start copying AUDIO files from SD to Flash chip
    tft.fillRect(0, 12, 320, 240, ILI9341_BLACK);
}

FLASHMEM
void DisplayStorage::Copy_raw_files_SD_to_Flash_chip_list_landscape(void)
{
    tft.fillRect(0, display_coordinate_y(3), 320, 240, ILI9341_BLACK);
}

FLASHMEM
void DisplayStorage::Copy_raw_files_SD_to_Flash_chip_files_to_copy(int row, const char *filename, unsigned long length)
{
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(row));
    tft.setTextColor(TEXT_COLOR);
    tft.print("COPYING FILE ");
    tft.setTextColor(ILI9341_WHITE);
    tft.print(filename);
    tft.print("  ");
    tft.print(length / 1024);
    tft.print("KB");
}

FLASHMEM
void DisplayStorage::Copy_audio_files_SD_to_Flash_chip_invalid_audio(int row, const char *filename)
{
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(row));
    tft.setTextColor(ILI9341_RED);
    tft.print(filename);
    tft.print(" INVALID AUDIO - NOT IMPORTED");
}

FLASHMEM
void DisplayStorage::Copy_raw_files_SD_to_Flash_chip_duplicate(int row, const char *filename)
{
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(row));
    tft.setTextColor(ILI9341_RED);
    tft.print(filename);
    tft.print(" DUPLICATE - NOT IMPORTED");
}

FLASHMEM
void DisplayStorage::Copy_raw_files_SD_to_Flash_chip_flash_error(void)
{
    tft.setTextColor(TEXT_COLOR);
    tft.print("  FLASH MEMORY ERROR");
}

FLASHMEM
void DisplayStorage::Copy_raw_files_SD_to_Flash_chip_flash_full_error(void)
{
    tft.setTextColor(TEXT_COLOR);
    tft.print("  ERROR: FLASH MEMORY FULL!");
}

FLASHMEM
void DisplayStorage::Copy_raw_files_SD_to_Flash_chip_job_done(void)
{
    // Display RAW files list
    tft.fillRect(0, 12, 320, 240, ILI9341_BLACK);
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(2));
    tft.setTextColor(TEXT_COLOR);
    tft.print(F("AUDIO FILES IMPORT COMPLETED. FILE LIST:"));
}

FLASHMEM
void DisplayStorage::Copy_raw_files_SD_to_Flash_chip_file_copied(int row, const char *filename, uint32_t filesize)
{
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(row));
    tft.setTextColor(ILI9341_WHITE);
    tft.print(filename);
    tft.print("  ");
    tft.print(filesize / 1024);
    tft.print("KB");
}
