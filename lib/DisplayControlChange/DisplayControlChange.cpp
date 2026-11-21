/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#include "DisplayControlChange.h"
#include "SharedElements.h"

FLASHMEM void DisplayControlChange::CC_show_ControlChange_page(void)
{
    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(display_coordinate_x(CC_column_row_Title[0]), display_coordinate_y(CC_column_row_Title[1]));
    tft.setTextColor(TEXT_COLOR);
    tft.print("CONTROL CHANGE ASSIGNMENT");

    tft.setCursor(display_coordinate_x(CC_column_row_Return[0]), display_coordinate_y(CC_column_row_Return[1]));
    tft.setTextColor(MENU_COLOR);
    tft.print("RETURN");

    tft.setTextColor(TEXT_COLOR);
    for (int instrument = 0; instrument < INSTRUMENTS; ++instrument)
    {
        tft.setCursor(display_coordinate_x(CC_column_label), display_coordinate_y(CC_column_row_FirstSoundGain[1] + instrument * CC_sound_row_step));
        tft.print("GAIN SOUND ");
        tft.print(instrument + 1);
    }
    tft.setCursor(display_coordinate_x(CC_column_label), display_coordinate_y(CC_column_row_LowpassFilter[1]));
    tft.print("LPF CUTOFF");
}

FLASHMEM void DisplayControlChange::CC_show_all_sound_gains(void)
{
    for (int instrument = 0; instrument < INSTRUMENTS; ++instrument)
    {
        CC_show_sound_gain(instrument);
    }
}

FLASHMEM void DisplayControlChange::CC_show_sound_gain(int instrument)
{
    Cancel_text_reset_cursor(display_coordinate_x(CC_column_row_FirstSoundGain[0]), display_coordinate_y(CC_column_row_FirstSoundGain[1] + instrument * CC_sound_row_step), CC_chars_Value);
    tft.setTextColor(ILI9341_YELLOW);
    if (CC_Sound_gain[instrument] > 0)
    {
        tft.print(CC_Sound_gain[instrument]);
    }
    else
    {
        tft.print(" -");
    }
}

FLASHMEM void DisplayControlChange::CC_show_lowpass_filter_value(void)
{
    Cancel_text_reset_cursor(display_coordinate_x(CC_column_row_LowpassFilter[0]), display_coordinate_y(CC_column_row_LowpassFilter[1]), CC_chars_Value);
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

FLASHMEM void DisplayControlChange::Show_pointer_frame(CC_field_description_struct description, bool show)
{
    switch (description.field_name)
    {
    case field_CC_Return:
        Frame_by_col_row(CC_column_row_Return[0], CC_column_row_Return[1], CC_chars_Return, show);
        break;
    case field_CC_SoundGain:
        Frame_by_col_row(CC_column_row_FirstSoundGain[0], CC_column_row_FirstSoundGain[1] + description.instrument * CC_sound_row_step, CC_chars_Value, show);
        break;
    case field_CC_LowpassFilter:
        Frame_by_col_row(CC_column_row_LowpassFilter[0], CC_column_row_LowpassFilter[1], CC_chars_Value, show);
        break;
    }
}
