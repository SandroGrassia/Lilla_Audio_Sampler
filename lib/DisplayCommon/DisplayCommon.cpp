/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#include "DisplayCommon.h"
#include "SharedElements.h"
#include <AudioStream.h>

FLASHMEM
void DisplayCommon::P_show_Patch_number(bool change_patch)
{
    tft.setCursor(display_coordinate_x(P_column_PATCH), display_coordinate_y(0));
    tft.setTextColor(TEXT_COLOR);
    tft.print("PATCH");
    tft.setCursor(display_coordinate_x(P_column_Patch_id), display_coordinate_y(0));
    tft.setTextColor(change_patch ? ILI9341_YELLOW : ILI9341_WHITE);
    tft.print(Patch_id);
}

FLASHMEM
void DisplayCommon::P_Patch_VOLUME(bool change_vol)
{
    tft.setCursor(display_coordinate_x(P_column_VOLUME), display_coordinate_y(0));
    tft.setTextColor(TEXT_COLOR);
    tft.print("VOLUME");
    P_Patch_volume_value(change_vol);
}

FLASHMEM
void DisplayCommon::P_Patch_volume_value(bool change_vol)
{
    Cancel_text_reset_cursor(display_coordinate_x(P_column_Volume_value), display_coordinate_y(0), 4);
    tft.setTextColor(ILI9341_YELLOW); // tft.setTextColor(change_vol ? ILI9341_YELLOW : ILI9341_WHITE);
    tft.print(volume_patch);
}

FLASHMEM
void DisplayCommon::P_show_PERFORMANCE_title(void)
{
    Backgorund_red(0, 0, 11); // Display.Backgorund_red(float col, float row, int chars)
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(0));
    tft.setTextColor(ILI9341_WHITE);
    tft.print("PERFORMANCE");
}

FLASHMEM
void DisplayCommon::Show_all_effects()
{
    float Y_EFF;

    if (Lilla_state == MIDI_LOOP)
    {
        Y_EFF = 1.5;
        tft.setTextColor(TEXT_COLOR);

        tft.setCursor(display_coordinate_x(30), display_coordinate_y(Y_EFF));
        tft.print("RESOLUTION");

        tft.setCursor(display_coordinate_x(30), display_coordinate_y(Y_EFF + 1));
        tft.print("DOWNSAMPLING");

        tft.setCursor(display_coordinate_x(0), display_coordinate_y(Y_EFF + 1));
        tft.print("LPF CUTOFF");
    }

    else
    {
        Y_EFF = 2.5;
        Delete_text_row(Y_EFF);
        tft.setTextColor(TEXT_COLOR);

        tft.setCursor(display_coordinate_x(0), display_coordinate_y(Y_EFF));
        tft.print("RESOLUTION");

        tft.setCursor(display_coordinate_x(30), display_coordinate_y(Y_EFF));
        tft.print("DOWNSAMPLING");

        tft.setCursor(display_coordinate_x(0), display_coordinate_y(Y_EFF + 1));
        tft.print("LPF CUTOFF");
    }

    Resolution();
    Downsampling();
    Lowpass_filter();
}

FLASHMEM
void DisplayCommon::Resolution(void)
{
    float Y_EFF;

    if (Lilla_state == MIDI_LOOP)
    {
        Y_EFF = 1.5;
        Cancel_text_reset_cursor(display_coordinate_x(43), display_coordinate_y(Y_EFF), 9);
        tft.setTextColor(ILI9341_YELLOW);
        tft.print(resolution_value[resolution], 1);
        tft.setTextColor(ILI9341_ORANGE);
        tft.print("bits");
        return;
    }

    Y_EFF = 2.5;
    Cancel_text_reset_cursor(display_coordinate_x(11), display_coordinate_y(Y_EFF), 8);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(resolution_value[resolution], 1);
    tft.setTextColor(ILI9341_ORANGE);
    tft.print("bits");
}

FLASHMEM
void DisplayCommon::Downsampling(void)
{
    const float Y_EFF = 2.5;

    if (Lilla_state == MIDI_LOOP)
    {
        Cancel_text_reset_cursor(display_coordinate_x(43), display_coordinate_y(Y_EFF), 9);
        tft.setTextColor(ILI9341_YELLOW);
        float down = AUDIO_SAMPLE_RATE / downsampling;

        if (down < 1000)
        {
            tft.print(down, 0);
            tft.setTextColor(ILI9341_ORANGE);
            tft.print("Hz");
        }
        else
        {
            tft.print(down / 1000.0f, 3);
            tft.setTextColor(ILI9341_ORANGE);
            tft.print("kHz");
        }

        return;
    }

    Cancel_text_reset_cursor(display_coordinate_x(43), display_coordinate_y(Y_EFF), 9);
    tft.setTextColor(ILI9341_YELLOW);
    float down = AUDIO_SAMPLE_RATE / downsampling;

    if (down < 1000)
    {
        tft.print(down, 0);
        tft.setTextColor(ILI9341_ORANGE);
        tft.print("Hz");
    }
    else
    {
        tft.print(down / 1000.0f, 3);
        tft.setTextColor(ILI9341_ORANGE);
        tft.print("kHz");
    }
}

FLASHMEM
void DisplayCommon::Lowpass_filter(void)
{
    float Y_EFF = (Lilla_state == MIDI_LOOP ? 1.5 : 2.5);
    Cancel_text_reset_cursor(display_coordinate_x(11), display_coordinate_y(Y_EFF + 1), 7);

    float F = lowpass_value[lowpass_target];
    tft.setTextColor(ILI9341_YELLOW);

    if (F > 9999)
    {
        tft.print(F / 1000, 0);
        tft.setTextColor(ILI9341_ORANGE);
        tft.print("kHz");
    }
    else if (F > 999)
    {
        tft.print(F / 1000.0f, 2);
        tft.setTextColor(ILI9341_ORANGE);
        tft.print("kHz");
    }
    else
    {
        tft.print(F, 0);
        tft.setTextColor(ILI9341_ORANGE);
        tft.print("Hz");
    }
}

FLASHMEM
void DisplayCommon::Patch_volume_color(bool change_patch, bool change_vol)
{
    P_show_Patch_number(change_patch);
    P_Patch_VOLUME(change_vol);
    P_Patch_volume_value(change_vol);
}

FLASHMEM
void DisplayCommon::Confirm_no_yes_popup_frame(int value)
{
    const int popup_x = Centered_element_left(106);
    const int options_y = Centered_element_top(47) + 30;
    Confirm_frame_on_RED(popup_x + display_coordinate_x(5.5), options_y, 2, value == 0);
    Confirm_frame_on_RED(popup_x + display_coordinate_x(9.5), options_y, 3, value == 1);
}
