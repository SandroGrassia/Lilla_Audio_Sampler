/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#include "DisplaySetup.h"
#include "SharedElements.h"

FLASHMEM
void DisplaySetup::SETUP_show_SETUP_page(void)
{
    tft.fillScreen(ILI9341_BLACK);

    Backgorund_red(0, 0, 5); // Display.Board(float   col, float row, int chars)
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(0));
    tft.setTextColor(ILI9341_WHITE);
    tft.print("SETUP");

    tft.setTextColor(TEXT_COLOR);
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(2));
    tft.print("KEY STEP");
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(3));
    tft.print("FIRST OCTAVE");
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(4));
    tft.print(F("CONTROL CHANGE ASSIGNMENT"));
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(5));
    tft.print(F("CONVERT AUDIO FILES STORED IN SD /EXTERNAL_AUDIO"));
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(6));
    tft.print(F("IMPORT AUDIO FILES FROM SD /FLASH_AUDIO"));
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(7));
    tft.print(F("RESTORE CONFIG + AUDIO FROM /LILLABACKUP ROOT"));
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(8));
    tft.print(F("NEW NUMBERED BACKUP IN /LILLABACKUP"));
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(9));
    tft.print(F("FACTORY RESET"));

    SETUP_show_Key_step_value();
    SETUP_show_First_octave_value();
}

FLASHMEM
void DisplaySetup::SETUP_show_Key_step_value(void)
{
    Cancel_text_reset_cursor(display_coordinate_x(9), display_coordinate_y(2), 5);
    tft.setTextColor(ILI9341_YELLOW);

    switch (key_step)
    {
    case 0:
        tft.print(" 1st");
        break;

    case 1:
        tft.print("1/2st");
        break;

    case 2:
        tft.print("1/4st");
        break;

    case 3:
        tft.print("1/8st");
        break;

    default:
        tft.print(" 1st");
        break;
    }
}

FLASHMEM
void DisplaySetup::SETUP_show_First_octave_value(void)
{
    Cancel_text_reset_cursor(display_coordinate_x(13), display_coordinate_y(3), 2);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(first_octave);
}

FLASHMEM
void DisplaySetup::SETUP_show_frame(int8_t value)
{
    Frame_by_col_row(9, 2, 5, false);
    Frame_by_col_row(13, 3, 2, false);
    Frame_by_col_row(0, 4, 25, false);
    Frame_by_col_row(0, 5, 48, false);
    Frame_by_col_row(0, 6, 39, false);
    Frame_by_col_row(0, 7, 45, false);
    Frame_by_col_row(0, 8, 35, false);
    Frame_by_col_row(0, 9, 13, false);

    switch (value)
    {
    case 0:
        Frame_by_col_row(9, 2, 5, true);
        break;
    case 1:
        Frame_by_col_row(13, 3, 2, true);
        break;
    case 2:
        Frame_by_col_row(0, 4, 25, true);
        break;
    case 3:
        Frame_by_col_row(0, 5, 48, true);
        break;
    case 4:
        Frame_by_col_row(0, 6, 39, true);
        break;
    case 5:
        Frame_by_col_row(0, 7, 45, true);
        break;
    case 6:
        Frame_by_col_row(0, 8, 35, true);
        break;
    case 7:
        Frame_by_col_row(0, 9, 13, true);
        break;
    default:
        break;
    }
}

FLASHMEM
void DisplaySetup::CC_show_ControlChange_page(void)
{
    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(0));
    tft.setTextColor(TEXT_COLOR);
    tft.print("CONTROL CHANGE ASSIGNMENT");

    tft.setCursor(display_coordinate_x(0), display_coordinate_y(1));
    tft.setTextColor(MENU_COLOR);
    tft.print("RETURN");

    tft.setTextColor(TEXT_COLOR);
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(3));
    tft.print("GAIN SOUND 1");
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(4));
    tft.print("GAIN SOUND 2");
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(5));
    tft.print("GAIN SOUND 3");
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(6));
    tft.print("GAIN SOUND 4");

    tft.setCursor(display_coordinate_x(0), display_coordinate_y(7));
    tft.print("GAIN SOUND 5");
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(8));
    tft.print("GAIN SOUND 6");
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(9));
    tft.print("GAIN SOUND 7");
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(10));
    tft.print("GAIN SOUND 8");
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(11));
    tft.print("LPF CUTOFF");
}

FLASHMEM
void DisplaySetup::CC_show_all_sound_gains(void)
{
    for (auto instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
    {
        CC_show_sound_gain(instrument_id);
    }
}

FLASHMEM
void DisplaySetup::CC_show_sound_gain(int value)
{
    Cancel_text_reset_cursor(display_coordinate_x(Setup_Control_change_X), display_coordinate_y(value + 3), 3);
    tft.setTextColor(ILI9341_YELLOW);

    if (CC_Sound_gain[value] > 0)
    {
        tft.print(CC_Sound_gain[value]);
    }
    else
    {
        tft.print(" -");
    }
}

FLASHMEM
void DisplaySetup::CC_show_lowpass_filter_value(void)
{
    Cancel_text_reset_cursor(display_coordinate_x(Setup_Control_change_X), display_coordinate_y(11), 3);
    tft.setTextColor(ILI9341_YELLOW);

    if (CC_lowpass_filter_value > 0)
    {
        tft.print(CC_lowpass_filter_value);
    }
    else
    {
        tft.print(" -");
    }
}

FLASHMEM
void DisplaySetup::CC_show_frame_menu(int value)
{
    Frame_by_col_row(0, 1, 6, false); // Return

    for (auto n = 0; n < 9; ++n)
    {
        Frame_by_col_row(Setup_Control_change_X, n + 3, 3, false);
    }

    if (value == 0)
    {
        Frame_by_col_row(0, 1, 6, true);
    }
    else
    {
        Frame_by_col_row(Setup_Control_change_X, value + 2, 3, true);
    }
}
