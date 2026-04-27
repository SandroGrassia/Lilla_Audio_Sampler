/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#include "DisplayMixer.h"

void DisplayMixer::MX_show_pointer_frame(const MX_pointer_struct pointer, const bool show)
{
    switch (pointer.field_name)
    {
    case field_MX_Source:
    {
    Frame_by_col_row(MX_column_row_Source[pointer.source][0], MX_column_row_Source[pointer.source][1], MX_frame_wide_high_Source[0], MX_frame_wide_high_Source[1], show); 
    }
    break;

    case field_MX_Elements:
    {
        switch (pointer.element)
        {
        case value_MX_Mute_Gain:
            Frame_by_col_row(MX_column_row_Mute_Gain[pointer.source][0], MX_column_row_Mute_Gain[pointer.source][1], MX_frame_wide_high_Mute_Gain[0], MX_frame_wide_high_Mute_Gain[1], show);
            break;

        case value_MX_Pan:
            Frame_by_col_row(MX_column_row_Pan[pointer.source][0], MX_column_row_Pan[pointer.source][1], MX_frame_wide_high_Pan[0], MX_frame_wide_high_Pan[1], show);
            break;

        case value_MX_Lineout:
            Frame_by_col_row(MX_column_row_Lineout[pointer.source][0], MX_column_row_Lineout[pointer.source][1], MX_frame_wide_high_Lineout[0], MX_frame_wide_high_Lineout[1], show);
            break;

        case value_MX_Monitor:
            Frame_by_col_row(MX_column_row_Monitor[pointer.source][0], MX_column_row_Monitor[pointer.source][1], MX_frame_wide_high_Monitor[0], MX_frame_wide_high_Monitor[1], show);
            break;

        default:
            break;
        }
    }
    break;

    default:
        break;
    }
}

FLASHMEM
void DisplayMixer::MX_page(void)
{
    tft.fillScreen(ILI9341_BLACK);

    Backgorund_red(0, 0, 5); // Display.Board(float   col, float row, int chars)
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(0));
    tft.setTextColor(ILI9341_WHITE);
    tft.print("MIXER");

    tft.setCursor(display_coordinate_x(0), display_coordinate_y(MX_row_Sound));
    tft.setTextColor(TEXT_COLOR);
    tft.print("SOURCE");

    tft.setCursor(display_coordinate_x(0), display_coordinate_y(MX_row_Sound + 1));
    tft.setTextColor(TEXT_COLOR);
    tft.print("MUTE");

    tft.setCursor(display_coordinate_x(0), display_coordinate_y(MX_row_Sound + 2));
    tft.setTextColor(TEXT_COLOR);
    tft.print("GAIN");

    tft.setCursor(display_coordinate_x(0), display_coordinate_y(MX_row_Sound + 3));
    tft.setTextColor(TEXT_COLOR);
    tft.print("PAN");

    tft.setCursor(display_coordinate_x(0), display_coordinate_y(MX_row_Sound + 4));
    tft.setTextColor(TEXT_COLOR);
    tft.print("LINEOUT");

    tft.setCursor(display_coordinate_x(0), display_coordinate_y(MX_row_Sound + 5));
    tft.setTextColor(TEXT_COLOR);
    tft.print("MONITOR");
}

FLASHMEM
void DisplayMixer::MX_source_values(const int source,const bool bright)
{
    if (source == LINE_IN_source || Patch[Patch_id].Instrument[source].used)
    {
        MX_source_values_write(source, bright);
    }
}

FLASHMEM
void DisplayMixer::MX_source_values_jump(const int old_source, const int source)
{
    MX_source_values_write(old_source, false); // 0
    MX_source_values_write(source, true);  // 6
}

FLASHMEM
void DisplayMixer::MX_source_values_write(const int source, const bool bright)
{
    int local_sound_id;
    if (source == LINE_IN_source)
    {
        tft.setTextColor((bright ? TEXT_COLOR : 0x6300));
        tft.setCursor(display_coordinate_x(MX_column_Sound + source * 5 - 1), display_coordinate_y(MX_row_Sound));
        tft.print("L_IN");

        tft.drawBitmap(display_coordinate_x(MX_column_Sound + 0.5 + source * 5), display_coordinate_y(MX_row_Sound + 1), led_pic, 8, 8, (MX_mute[source] ? ILI9341_RED : RED_OFF)); // Mute
        tft.setTextColor((bright ? ILI9341_YELLOW : 0x6300));
        tft.setCursor(display_coordinate_x(MX_column_Sound - 1 + source * 5), display_coordinate_y(MX_row_Sound + 2)); // Gain
        tft.print(DS_gain / 20.0f);

        tft.setTextColor((bright ? ILI9341_WHITE : 0x6300));
        tft.setCursor(display_coordinate_x(MX_column_Sound + 0.5 + source * 5), display_coordinate_y(MX_row_Sound + 3)); // Pan
        tft.print("C");
    }

    else
    {
        local_sound_id = Patch[Patch_id].Instrument[source].sound_id;

        tft.setTextColor((bright ? TEXT_COLOR : 0x6300));
        tft.setCursor(display_coordinate_x(MX_column_Sound + source * 5), display_coordinate_y(MX_row_Sound));
        tft.print("S");
        tft.print(source + 1);

        tft.drawBitmap(display_coordinate_x(MX_column_Sound + 0.5 + source * 5), display_coordinate_y(MX_row_Sound + 1), led_pic, 8, 8, (MX_mute[source] ? ILI9341_RED : RED_OFF)); // Mute
        tft.setTextColor((bright ? ILI9341_YELLOW : 0x6300));

        Cancel_text(display_coordinate_x(MX_column_Sound - 1 + source * 5), display_coordinate_y(MX_row_Sound + 2), 4);
        tft.setCursor(display_coordinate_x(MX_column_Sound - 1 + source * 5), display_coordinate_y(MX_row_Sound + 2)); // Gain
        tft.print(Sound[local_sound_id].gain / 20.0f);

        if (Sound[local_sound_id].pan == 0)
        {
            Cancel_text(display_coordinate_x(MX_column_Sound - 0.5 + source * 5), display_coordinate_y(MX_row_Sound + 3), 3);
            tft.setCursor(display_coordinate_x(MX_column_Sound + 0.5 + source * 5), display_coordinate_y(MX_row_Sound + 3)); // Pan
            tft.print("C");
        }
        else
        {
            if (abs(Sound[local_sound_id].pan) < 10) // Pan
            {
                tft.setCursor(display_coordinate_x(MX_column_Sound + source * 5), display_coordinate_y(MX_row_Sound + 3));
            }
            else
            {
                tft.setCursor(display_coordinate_x(MX_column_Sound - 0.5 + source * 5), display_coordinate_y(MX_row_Sound + 3));
            }

            if (Sound[local_sound_id].pan < 0)
            {
                tft.print("L");
            }
            else if (Sound[local_sound_id].pan > 0)
            {
                tft.print("R");
            }
            tft.print(abs(Sound[local_sound_id].pan));
        }
    }

    if (MX_routing_source[source] > 1) // to Audio-out
    {
        tft.drawBitmap(display_coordinate_x(MX_column_Sound + 0.5 + source * 5), display_coordinate_y(MX_row_Sound + 4), led_pic, 8, 8, GREEN_ON);
    }
    else
    {
        tft.drawBitmap(display_coordinate_x(MX_column_Sound + 0.5 + source * 5), display_coordinate_y(MX_row_Sound + 4), led_pic, 8, 8, GREEN_OFF);
    }

    if (MX_routing_source[source] == 1 || MX_routing_source[source] == 3) // to Monitor
    {
        tft.drawBitmap(display_coordinate_x(MX_column_Sound + 0.5 + source * 5), display_coordinate_y(MX_row_Sound + 5), led_pic, 8, 8, GREEN_ON);
    }
    else
    {
        tft.drawBitmap(display_coordinate_x(MX_column_Sound + 0.5 + source * 5), display_coordinate_y(MX_row_Sound + 5), led_pic, 8, 8, GREEN_OFF);
    }  
}

FLASHMEM
void DisplayMixer::MX_source_values_edit(const int source)
{
    Cancel_text(display_coordinate_x(MX_column_Sound + 0.5 + source * 5), display_coordinate_y(MX_row_Sound + 1), 1); // Mute
    Cancel_text(display_coordinate_x(MX_column_Sound - 1 + source * 5), display_coordinate_y(MX_row_Sound + 2), 4);   // Gain
    Cancel_text(display_coordinate_x(MX_column_Sound - 0.5 + source * 5), display_coordinate_y(MX_row_Sound + 3), 3); // Pan
    Cancel_text(display_coordinate_x(MX_column_Sound + 0.5 + source * 5), display_coordinate_y(MX_row_Sound + 4), 1); // to Audio-out
    Cancel_text(display_coordinate_x(MX_column_Sound + 0.5 + source * 5), display_coordinate_y(MX_row_Sound + 5), 1); // to Monitor

    MX_source_values_write(source, true);
}
