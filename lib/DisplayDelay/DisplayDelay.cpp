/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#include "DisplayDelay.h"

FLASHMEM
void DisplayDelay::D_show_page()
{
    tft.fillScreen(ILI9341_BLACK);

    if (Lilla_state_0 == PERFORMANCE)
    {
        Backgorund_red(0, 0, 17); // DISPLAY_board(float col, float row, int chars)
        tft.setCursor(display_coordinate_x(0), display_coordinate_y(0));
        tft.setTextColor(ILI9341_WHITE);
        tft.print("PERFORMANCE ");
        tft.setTextColor(ILI9341_WHITE);
        tft.print("DELAY");
    }

    else if (Lilla_state_0 == LIVE_SAMPLING)
    {
        Backgorund_red(0, 0, 18); // DISPLAY_board(float col, float row, int chars)
        tft.setCursor(display_coordinate_x(0), display_coordinate_y(0));
        tft.setTextColor(ILI9341_WHITE);
        tft.print("LIVE SAMPLER ");
        tft.setTextColor(ILI9341_WHITE);
        tft.print("DELAY");
    }

    tft.setCursor(display_coordinate_x(DELAY_column_row_VOLUME[0]), display_coordinate_y(DELAY_column_row_VOLUME[1]));
    tft.setTextColor(TEXT_COLOR);
    tft.print("VOLUME");
    Display_Manager.P_Patch_volume_value(true);

    Display_Manager.ALL_show_effects();

    tft.setCursor(display_coordinate_x(0), display_coordinate_y(DEL_ROW_SOUND));
    tft.setTextColor(TEXT_COLOR);
    tft.print("ROUTING");
    D_sounds();

    tft.setCursor(display_coordinate_x(DELAY_column_row_FEEDBACK[0]), display_coordinate_y(DELAY_column_row_FEEDBACK[1]));
    tft.setTextColor(TEXT_COLOR);
    tft.print("FEEDBACK");
    D_feedback();

    tft.setCursor(display_coordinate_x(DELAY_column_row_DELAY_TIME[0]), display_coordinate_y(DELAY_column_row_DELAY_TIME[1]));
    tft.setTextColor(TEXT_COLOR);
    tft.print("DELAY TIME");
    D_delay_time();

    tft.setCursor(display_coordinate_x(DELAY_column_row_DELAY_TIME_LR[0]), display_coordinate_y(DELAY_column_row_DELAY_TIME_LR[1]));
    tft.setTextColor(TEXT_COLOR);
    tft.print("DELAY TIME L/R");
    D_delay_time_LR();

    tft.setCursor(display_coordinate_x(DELAY_column_row_MODULATION_SOURCE[0]), display_coordinate_y(DELAY_column_row_MODULATION_SOURCE[1]));
    tft.setTextColor(TEXT_COLOR);
    tft.print("MOD SOURCE");
    D_modulation_source();

    tft.setCursor(display_coordinate_x(DELAY_column_row_MODULATION_FREQUENCY[0]), display_coordinate_y(DELAY_column_row_MODULATION_FREQUENCY[1]));
    tft.setTextColor(TEXT_COLOR);
    tft.print("MOD FREQUENCY");
    D_modulation_frequency();

    tft.setCursor(display_coordinate_x(DELAY_column_row_MODULATION_DEPTH[0]), display_coordinate_y(DELAY_column_row_MODULATION_DEPTH[1]));
    tft.setTextColor(TEXT_COLOR);
    tft.print("MOD DEPTH");
    D_modulation_depth();

    tft.setCursor(display_coordinate_x(DELAY_column_row_MODULATION_PHASE_LR[0]), display_coordinate_y(DELAY_column_row_MODULATION_PHASE_LR[1]));
    tft.setTextColor(TEXT_COLOR);
    tft.print("MOD PHASE L/R");
    D_modulation_phase_LR();
}

FLASHMEM
void DisplayDelay::D_sounds(void)
{
    Cancel_text_reset_cursor(display_coordinate_x(DELAY_column_row_sounds[0]), display_coordinate_y(DELAY_column_row_sounds[1]), DELAY_chars_sounds); // DISPLAY_text(int X, int Y, int N)
    if (Lilla_state_0 == LIVE_SAMPLING)
    {
        tft.setTextColor(ILI9341_YELLOW);

        if (Delay_values.instrument_route[0])
        {
            tft.print("RECORDED AUDIO");
        }
        else
        {
            tft.print("NONE");
        }
        return;
    }

    for (auto instrument_id = 0; instrument_id < INSTRUMENTS_MAX; ++instrument_id)
    {
        if (Delay_values.instrument_route[instrument_id])
        {
            tft.setTextColor(ILI9341_YELLOW);
            tft.print("S");
            tft.print(instrument_id + 1);
            tft.print("  ");
        }
        else
        {
            tft.setTextColor(0x6300);
            tft.print("S");
            tft.print(instrument_id + 1);
            tft.print("  ");
        }
    }
}

FLASHMEM
void DisplayDelay::D_feedback(void)
{
    Cancel_text_reset_cursor(display_coordinate_x(DELAY_column_row_feedback[0]), display_coordinate_y(DELAY_column_row_feedback[1]), DELAY_chars_feedback);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(-Delay_feedback(Delay_data.loop_gain) * 100);
    tft.print("%");
}

FLASHMEM
void DisplayDelay::D_delay_time(void)
{
    Cancel_text_reset_cursor(display_coordinate_x(DELAY_column_row_delay_time[0]), display_coordinate_y(DELAY_column_row_delay_time[1]), DELAY_chars_delay_time);
    tft.setTextColor(ILI9341_YELLOW);

    if (Delay_values.samples < 44100)
    {
        tft.print(Delay_values.samples / 44.1f, 1);
        tft.setTextColor(ILI9341_ORANGE);
        tft.print("ms");
    }
    else
    {
        tft.print(Delay_values.samples / 44100.0f, 2);
        tft.setTextColor(ILI9341_ORANGE);
        tft.print("sec");
    }
}

FLASHMEM
void DisplayDelay::D_delay_time_LR(void)
{
    Cancel_text_reset_cursor(display_coordinate_x(DELAY_column_row_delay_time_LR[0]), display_coordinate_y(DELAY_column_row_delay_time_LR[1]), DELAY_chars_delay_time_LR);
    tft.setTextColor(ILI9341_YELLOW);

    if (Delay_values.samples_LR == 0)
    {
        tft.print(0);
        tft.setTextColor(ILI9341_ORANGE);
        tft.print("ms L");
    }
    else if (Delay_values.samples_LR > 0)
    {
        tft.print("+");
        tft.print(Delay_values.samples_LR / 44.1f);
        tft.setTextColor(ILI9341_ORANGE);
        tft.print("ms R");
    }
    else
    {
        tft.print("+");
        tft.print(-Delay_values.samples_LR / 44.1f);
        tft.setTextColor(ILI9341_ORANGE);
        tft.print("ms L");
    }
}

FLASHMEM
void DisplayDelay::D_modulation_source(void)
{
    Cancel_text_reset_cursor(display_coordinate_x(DELAY_column_row_modulation_source[0]), display_coordinate_y(DELAY_column_row_modulation_source[1]), DELAY_chars_modulation_source);
    tft.setTextColor(ILI9341_YELLOW);
    if (Delay_values.modulation_source == 0) // nessuna modulazione
    {
        tft.print("NONE");
    }
    else if (Delay_values.modulation_source == 1) // LFO
    {
        tft.print("LFO");
    }
    else
    {
        tft.print("SOURCE");
    }
}

FLASHMEM
void DisplayDelay::D_modulation_frequency(void)
{
    Cancel_text_reset_cursor(display_coordinate_x(DELAY_column_row_modulation_frequency[0]), display_coordinate_y(DELAY_column_row_modulation_frequency[1]), DELAY_chars_modulation_frequency);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(Delay_values.modulation_frequency);
    tft.setTextColor(ILI9341_ORANGE);
    tft.print("Hz");
}

FLASHMEM
void DisplayDelay::D_modulation_depth(void) // depth
{
    Cancel_text_reset_cursor(display_coordinate_x(DELAY_column_row_modulation_depth[0]), display_coordinate_y(DELAY_column_row_modulation_depth[1]), DELAY_chars_modulation_depth);
    tft.setTextColor(ILI9341_YELLOW);
    if (Delay_values.modulation_depth <= 1.0f)
    {
        tft.print(Delay_values.modulation_depth * 100, 1);
    }
    else
    {
        tft.print(Delay_values.modulation_depth * 100, 0);
    }
    tft.print("%");
}

FLASHMEM
void DisplayDelay::D_modulation_phase_LR(void)
{
    Cancel_text_reset_cursor(display_coordinate_x(DELAY_column_row_modulation_phase_LR[0]), display_coordinate_y(DELAY_column_row_modulation_phase_LR[1]), DELAY_chars_modulation_phase_LR);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(Delay_values.modulation_phase_LR);
    tft.setTextColor(ILI9341_ORANGE);
    tft.print("deg");
}



FLASHMEM
void DisplayDelay::D_disabled(void)
{
    const int L_POPUP = 228; // 106;
    const int H_POPUP = 28;
    const int X_POPUP = (320 - L_POPUP) / 2;
    const int Y_POPUP = (240 - H_POPUP) / 2;
    const int Y_POPUP_TXT = 10;
    const int Y_POPUP_OPT = 30;

    tft.fillRoundRect(X_POPUP, Y_POPUP, L_POPUP, H_POPUP, 4, ILI9341_RED); // does NOT delete frame
    tft.setCursor(X_POPUP + display_coordinate_x(3), Y_POPUP + Y_POPUP_TXT);
    tft.setTextColor(ILI9341_WHITE);

    //"0123456789012345678901234567890123456789109876543210";
    tft.print("DELAY IS DISABLED WHILE SAMPLING");
}

void DisplayDelay::DELAY_show_pointer_frame(const DELAY_element_name pointer, const bool show)
{
Frame_by_col_row(DELAY_column_row_element[pointer][0], DELAY_column_row_element[pointer][1], DELAY_chars_element[pointer], show);
}