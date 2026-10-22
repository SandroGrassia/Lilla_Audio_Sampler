/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#include "DisplaySetup.h"
#include "SharedElements.h"

FLASHMEM void DisplaySetup::SETUP_show_SETUP_page(void)
{
    tft.fillScreen(ILI9341_BLACK);

    Backgorund_red(0, 0, 5); // Display.Board(float   col, float row, int chars)
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(0));
    tft.setTextColor(ILI9341_WHITE);
    tft.print("SETUP");

    tft.setTextColor(TEXT_COLOR);
    tft.setCursor(display_coordinate_x(SET_column_row_Convert[0]), display_coordinate_y(SET_column_row_Convert[1]));
    tft.print(F("CONVERT SD AUDIO FILES"));
    tft.setCursor(display_coordinate_x(SET_column_row_Import[0]), display_coordinate_y(SET_column_row_Import[1]));
    tft.print(F("IMPORT AUDIO FILES FROM SD"));
    tft.setCursor(display_coordinate_x(SET_column_label), display_coordinate_y(SET_column_row_KeyStep[1]));
    tft.print(F("KEYBOARD STEP"));
    tft.setCursor(display_coordinate_x(SET_column_label), display_coordinate_y(SET_column_row_FirstOctave[1]));
    tft.print(F("FIRST OCTAVE"));
    tft.setCursor(display_coordinate_x(SET_column_row_ControlChange[0]), display_coordinate_y(SET_column_row_ControlChange[1]));
    tft.print(F("CONTROL CHANGE ASSIGNMENT"));
    tft.setCursor(display_coordinate_x(SET_column_row_Backup[0]), display_coordinate_y(SET_column_row_Backup[1]));
    tft.print(F("CONFIGURATION BACKUP"));
    tft.setCursor(display_coordinate_x(SET_column_row_Restore[0]), display_coordinate_y(SET_column_row_Restore[1]));
    tft.print(F("RESTORE CONFIGURATION FROM BACKUP"));
    tft.setCursor(display_coordinate_x(SET_column_row_FactoryReset[0]), display_coordinate_y(SET_column_row_FactoryReset[1]));
    tft.print(F("FACTORY RESET"));

    SETUP_show_Key_step_value();
    SETUP_show_First_octave_value();
}

FLASHMEM void DisplaySetup::SETUP_show_Key_step_value(void)
{
    Cancel_text_reset_cursor(display_coordinate_x(SET_column_row_KeyStep[0]), display_coordinate_y(SET_column_row_KeyStep[1]), SET_chars_KeyStep);
    tft.setTextColor(ILI9341_YELLOW);

    switch (key_step)
    {
    case 0:
        tft.print("1 semitone");
        break;

    case 1:
        tft.print("1/2 semitone");
        break;

    case 2:
        tft.print("1/4 semitone");
        break;

    case 3:
        tft.print("1/8 semitone");
        break;

    default:
        break;
    }
}

FLASHMEM void DisplaySetup::SETUP_show_First_octave_value(void)
{
    Cancel_text_reset_cursor(display_coordinate_x(SET_column_row_FirstOctave[0]), display_coordinate_y(SET_column_row_FirstOctave[1]), SET_chars_FirstOctave);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(first_octave);
}

FLASHMEM void DisplaySetup::SETUP_show_frame(int8_t value)
{
    Frame_by_col_row(SET_column_row_Convert[0], SET_column_row_Convert[1], SET_chars_Convert, false);
    Frame_by_col_row(SET_column_row_Import[0], SET_column_row_Import[1], SET_chars_Import, false);
    Frame_by_col_row(SET_column_row_KeyStep[0], SET_column_row_KeyStep[1], SET_chars_KeyStep, false);
    Frame_by_col_row(SET_column_row_FirstOctave[0], SET_column_row_FirstOctave[1], SET_chars_FirstOctave, false);
    Frame_by_col_row(SET_column_row_ControlChange[0], SET_column_row_ControlChange[1], SET_chars_ControlChange, false);
    Frame_by_col_row(SET_column_row_Backup[0], SET_column_row_Backup[1], SET_chars_Backup, false);
    Frame_by_col_row(SET_column_row_Restore[0], SET_column_row_Restore[1], SET_chars_Restore, false);
    Frame_by_col_row(SET_column_row_FactoryReset[0], SET_column_row_FactoryReset[1], SET_chars_FactoryReset, false);

    switch (value)
    {
    case value_SET_Convert:
        Frame_by_col_row(SET_column_row_Convert[0], SET_column_row_Convert[1], SET_chars_Convert, true);
        break;
    case value_SET_Import:
        Frame_by_col_row(SET_column_row_Import[0], SET_column_row_Import[1], SET_chars_Import, true);
        break;
    case value_SET_KeyStep:
        Frame_by_col_row(SET_column_row_KeyStep[0], SET_column_row_KeyStep[1], SET_chars_KeyStep, true);
        break;
    case value_SET_FirstOctave:
        Frame_by_col_row(SET_column_row_FirstOctave[0], SET_column_row_FirstOctave[1], SET_chars_FirstOctave, true);
        break;
    case value_SET_ControlChange:
        Frame_by_col_row(SET_column_row_ControlChange[0], SET_column_row_ControlChange[1], SET_chars_ControlChange, true);
        break;
    case value_SET_Backup:
        Frame_by_col_row(SET_column_row_Backup[0], SET_column_row_Backup[1], SET_chars_Backup, true);
        break;
    case value_SET_Restore:
        Frame_by_col_row(SET_column_row_Restore[0], SET_column_row_Restore[1], SET_chars_Restore, true);
        break;
    case value_SET_FactoryReset:
        Frame_by_col_row(SET_column_row_FactoryReset[0], SET_column_row_FactoryReset[1], SET_chars_FactoryReset, true);
        break;
    default:
        break;
    }
}

FLASHMEM void DisplaySetup::SETUP_show_convert_popup(void)
{
    const char *lines[Convert_popup_lines] = {"CONVERT WAV/AIFF/MP3/RAW", "FROM SD /SD_AUDIO_FILES", "TO /LILLA_PRIVATE/AUDIO_FILES", "ORIGINALS KEPT - EXISTING FILES SKIPPED", "CANCEL    GO"};
    Show_popup_text(lines, Convert_popup_lines, ILI9341_BLACK, ILI9341_GREEN);
    SETUP_show_convert_frame(0);
}

FLASHMEM void DisplaySetup::SETUP_show_convert_frame(int value)
{
    const int left = Centered_element_left(12 * 6);
    const int top = Centered_element_top(display_coordinate_y(Convert_popup_lines)) + 5 + 15 * (Convert_popup_lines - 1);
    Confirm_frame_on_GREEN(left, top, 6, value == 0);
    Confirm_frame_on_GREEN(left + 10 * 6, top, 2, value == 1);
}

FLASHMEM void DisplaySetup::SETUP_show_import_info_popup(void)
{
    const char *lines[Import_info_popup_lines] = {"IMPORT WAV/AIFF/MP3/RAW AUDIO FILES", "FROM SD /FLASH_AUDIO_FILES INTO LILLA", "PERMANENT FLASH MEMORY", "CANCEL   GO"};
    Show_popup_text(lines, Import_info_popup_lines, ILI9341_BLACK, ILI9341_GREEN);
    SETUP_show_import_info_frame(0);
}

FLASHMEM void DisplaySetup::SETUP_show_import_info_frame(int value)
{
    const int left = Centered_element_left(11 * 6);
    const int top = Centered_element_top(display_coordinate_y(Import_info_popup_lines)) + 5 + 15 * (Import_info_popup_lines - 1);
    Confirm_frame_on_GREEN(left, top, 6, value == 0);
    Confirm_frame_on_GREEN(left + 9 * 6, top, 2, value == 1);
}
