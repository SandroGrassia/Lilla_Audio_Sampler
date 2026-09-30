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
    Frame_by_col_row(0, 1, 4, false); // DISPLAY_confirm_frame(uint8_t col, uint8_t row, uint8_t chars, bool   print)
    Frame_by_col_row(5, 1, 6, false);

    switch (value)
    {
    case 0: // EXIT
        Frame_by_col_row(0, 1, 4, true);
        break;
    case 1: // IMPORT
        Frame_by_col_row(5, 1, 6, true);
        break;
    default:
        break;
    }
}

FLASHMEM
void DisplayStorage::Confirm_config_import_popup(void)
{
    L_POPUP = display_coordinate_x(46);
    H_POPUP = display_coordinate_y(5.2);
    Y_POPUP = Centered_element_top(H_POPUP);
    X_POPUP = Centered_element_left(L_POPUP);
    tft.fillRoundRect(X_POPUP, Y_POPUP, L_POPUP, H_POPUP, 4, ILI9341_RED);

    Y_POPUP_TXT = 10; // Prima riga testo
    Y_POPUP_OPT = Y_POPUP_TXT + 50;
    X_POPUP_OPT = display_coordinate_x(19); // Colonna prima opzione, generalmente NO

    tft.setCursor(X_POPUP + display_coordinate_x(1), Y_POPUP + Y_POPUP_TXT);
    tft.setTextColor(ILI9341_WHITE);

    //       ("01234567890123456789012345678901234567891098765"); // 45 char
    tft.print(F("    RESTORE CONFIGURATION + RECORDING AUDIO"));
    tft.setCursor(X_POPUP + display_coordinate_x(1), Y_POPUP + Y_POPUP_TXT + 15);
    tft.print(F(" WILL DELETE PATCHES, SOUNDS AND RECORDINGS!")); // 43
    tft.setCursor(X_POPUP + display_coordinate_x(1), Y_POPUP + Y_POPUP_TXT + 30);

    //       ("01234567890123456789012345678901234567891098765"); // 45 char
    tft.print(F("        DO YOU REALLY WANT TO PROCEED?"));
    tft.setTextColor(ILI9341_YELLOW);
    tft.setCursor(X_POPUP + X_POPUP_OPT, Y_POPUP + Y_POPUP_OPT);
    tft.print("NO");
    tft.setCursor(X_POPUP + X_POPUP_OPT + display_coordinate_x(4), Y_POPUP + Y_POPUP_OPT);
    tft.print("YES");
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

FLASHMEM
void DisplayStorage::FRAM_io_error_popup(void)
{
    tft.fillScreen(ILI9341_BLACK);
    tft.setTextSize(1);
    tft.setTextColor(ILI9341_YELLOW);
    tft.setCursor(10, 70);
    tft.println(F("FRAM ACCESS FAILED - OPERATION STOPPED"));
    tft.setCursor(10, 100);
    tft.println(F("Data may not have been saved."));
    tft.setCursor(10, 130);
    tft.println(F("Check FRAM connections and restart."));
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
void DisplayStorage::Config_import_REBOOT_popup(void)
{
    L_POPUP = display_coordinate_x(38);
    H_POPUP = display_coordinate_y(3);
    Y_POPUP = Centered_element_top(H_POPUP);
    X_POPUP = Centered_element_left(L_POPUP);
    Y_POPUP_TXT = 20; // Prima riga testo

    tft.fillRoundRect(X_POPUP, Y_POPUP, L_POPUP, H_POPUP, 4, ILI9341_BLACK);
    tft.setCursor(X_POPUP + display_coordinate_x(0), Y_POPUP + Y_POPUP_TXT);
    tft.setTextColor(ILI9341_WHITE);

    //           ("01234567890123456789012345678901234567");
    tft.print(F(" RESTORING CONFIGURATION AND AUDIO"));
}

FLASHMEM
void DisplayStorage::Confirm_config_import_frame(uint8_t value)
{
    Confirm_frame_on_RED(X_POPUP + X_POPUP_OPT, Y_POPUP + Y_POPUP_OPT, 2, false); // DISPLAY_confirm_frame(uint8_t col, uint8_t row, uint8_t chars, bool   print)
    Confirm_frame_on_RED(X_POPUP + X_POPUP_OPT + display_coordinate_x(4), Y_POPUP + Y_POPUP_OPT, 3, false);

    switch (value)
    {
    case 0: // NO
        Confirm_frame_on_RED(X_POPUP + X_POPUP_OPT, Y_POPUP + Y_POPUP_OPT, 2, true);
        break;
    case 1: // YES
        Confirm_frame_on_RED(X_POPUP + X_POPUP_OPT + display_coordinate_x(4), Y_POPUP + Y_POPUP_OPT, 3, true);
        break;
    default:
        break;
    }
}

FLASHMEM
void DisplayStorage::Confirm_config_export_popup(void)
{
    L_POPUP = display_coordinate_x(51);
    H_POPUP = display_coordinate_y(5.2);
    Y_POPUP = Centered_element_top(H_POPUP);
    X_POPUP = Centered_element_left(L_POPUP);
    tft.fillRoundRect(X_POPUP, Y_POPUP, L_POPUP, H_POPUP, 4, ILI9341_RED);

    Y_POPUP_TXT = 10; // Prima riga testo
    Y_POPUP_OPT = Y_POPUP_TXT + 50;
    X_POPUP_OPT = display_coordinate_x(19);

    tft.setCursor(X_POPUP + display_coordinate_x(1), Y_POPUP + Y_POPUP_TXT);
    tft.setTextColor(ILI9341_WHITE);

    //         ("012345678901234567890123456789012345678901234567890");
    tft.print(F("       SAVE CONFIGURATION + RECORDING AUDIO"));
    tft.setCursor(X_POPUP, Y_POPUP + Y_POPUP_TXT + 15);
    tft.print(F("       PREVIOUS BACKUPS WILL NOT BE DELETED"));
    tft.setCursor(X_POPUP, Y_POPUP + Y_POPUP_TXT + 30);
    tft.print(F("        DO YOU REALLY WANT TO PROCEED?"));
    tft.setTextColor(ILI9341_YELLOW);
    tft.setCursor(X_POPUP + X_POPUP_OPT, Y_POPUP + Y_POPUP_OPT);
    tft.print("NO");
    tft.setCursor(X_POPUP + X_POPUP_OPT + display_coordinate_x(4), Y_POPUP + Y_POPUP_OPT);
    tft.print("YES");
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
    L_POPUP = display_coordinate_x(50);
    H_POPUP = display_coordinate_y(3);
    Y_POPUP = Centered_element_top(H_POPUP);
    X_POPUP = Centered_element_left(L_POPUP);
    Y_POPUP_TXT = 20; // Prima riga testo

    tft.fillRoundRect(X_POPUP, Y_POPUP, L_POPUP, H_POPUP, 4, ILI9341_BLACK);
    tft.setCursor(X_POPUP, Y_POPUP + Y_POPUP_TXT);
    tft.setTextColor(ILI9341_WHITE);

    //       ("01234567890123456789012345678901234567890123456789");
    tft.print(F(" NUMBERED BACKUP SAVED IN /LILLABACKUP"));
}

FLASHMEM
void DisplayStorage::Confirm_factory_reset_popup(void)
{
    L_POPUP = display_coordinate_x(49);
    H_POPUP = display_coordinate_y(5.2);
    Y_POPUP = Centered_element_top(H_POPUP);
    X_POPUP = Centered_element_left(L_POPUP);
    tft.fillRoundRect(X_POPUP, Y_POPUP, L_POPUP, H_POPUP, 4, ILI9341_RED);

    Y_POPUP_TXT = 15; // Prima riga testo
    Y_POPUP_OPT = Y_POPUP_TXT + 50;
    X_POPUP_OPT = display_coordinate_x(19);

    tft.setCursor(X_POPUP + display_coordinate_x(1), Y_POPUP + Y_POPUP_TXT);
    tft.setTextColor(ILI9341_WHITE);

    //       ("0123456789012345678901234567890123456789012345678");
    tft.print(F("      WARNING: FACTORY RESET WILL DELETE"));
    tft.setCursor(X_POPUP + display_coordinate_x(1), Y_POPUP + Y_POPUP_TXT + 15);
    tft.print(F("      ALL PATCHES, SOUNDS AND RECORDINGS!"));
    tft.setCursor(X_POPUP + display_coordinate_x(1), Y_POPUP + Y_POPUP_TXT + 30);
    tft.print(F("        DO YOU REALLY WANT TO PROCEED?"));
    tft.setTextColor(ILI9341_YELLOW);
    tft.setCursor(X_POPUP + X_POPUP_OPT, Y_POPUP + Y_POPUP_OPT);
    tft.print("NO");
    tft.setCursor(X_POPUP + X_POPUP_OPT + display_coordinate_x(4), Y_POPUP + Y_POPUP_OPT);
    tft.print("YES");
}

FLASHMEM
void DisplayStorage::Config_reset_popup(void)
{
    L_POPUP = display_coordinate_x(49);
    H_POPUP = display_coordinate_y(5.2);
    Y_POPUP = Centered_element_top(H_POPUP);
    X_POPUP = Centered_element_left(L_POPUP);
    tft.fillRoundRect(X_POPUP, Y_POPUP, L_POPUP, H_POPUP, 4, ILI9341_RED);

    Y_POPUP_TXT = 10; // Prima riga testo
    Y_POPUP_OPT = Y_POPUP_TXT + 50;
    X_POPUP_OPT = display_coordinate_x(19);

    tft.setCursor(X_POPUP + display_coordinate_x(1), Y_POPUP + Y_POPUP_TXT + 15);
    tft.setTextColor(ILI9341_WHITE);

    //       ("0123456789012345678901234567890123456789012345678");
    tft.print(F("   PLEASE WAIT. LILLA WILL RESTART AFTER RESET")); // 43
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
void DisplayStorage::Copy_raw_files_SD_to_Flash_chip_lilla_audio_missing(void)
{
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(3));
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(F("NO FILES TO IMPORT: MISSING /LILLA_AUDIO DIRECTORY!"));
}

FLASHMEM
void DisplayStorage::Copy_raw_files_SD_to_Flash_chip_files_report(unsigned long SD_raw_volume, int SD_raw_files, int raw_files_volume, int flash_raw_files)
{
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(3));
    tft.setTextColor(TEXT_COLOR);
    tft.print(F("SOURCE: SD CARD /LILLA_AUDIO"));
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
void DisplayStorage::Copy_audio_files_SD_to_Flash_chip_last_warning(float erasing_time_ms)
{
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(10));
    tft.setTextColor(TEXT_COLOR);
    tft.println(F("- DELETING TIME"));
    tft.setCursor(display_coordinate_x(16), display_coordinate_y(10));
    tft.setTextColor(ILI9341_WHITE);
    tft.print(erasing_time_ms / 60000, 1);
    Show_measure_unit("min", 3);

    // display SD->Flash menu
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(1));
    tft.setTextColor(MENU_COLOR);
    tft.print("EXIT IMPORT");

    Show_popup_text("IMPORT DELETES ALL AUDIO FILES AND RECORDINGS!", "AUDIO AFTER 33 SECONDS IS NOT PLAYED", ILI9341_WHITE, ILI9341_RED, 75);
}

FLASHMEM
void DisplayStorage::Copy_raw_files_SD_to_Flash_chip_job_start(void)
{
    // Start erasing flash chip
    tft.fillRect(0, display_coordinate_y(11), 320, 240, ILI9341_BLACK);
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(12) - 5);
    tft.setTextColor(ILI9341_YELLOW);

    //        "012345678901234567890 234 X 432 98765432109876543210"); // max 52 char
    tft.print(F("PLEASE WAIT: FLASH MEMORY ERASE IS RUNNING."));
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(13) - 5);
    tft.print(F("THEN AUDIO FILES WILL BE COPIED FROM SD/LILLA_AUDIO TO"));
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(14) - 5);
    tft.print("LILLA FLASH MEMORY");
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
void DisplayStorage::Copy_raw_files_SD_to_Flash_chip_invalid_wav(int row, const char *filename)
{
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(row));
    tft.setTextColor(ILI9341_RED);
    tft.print(filename);
    tft.print(" INVALID WAV - NOT IMPORTED");
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
