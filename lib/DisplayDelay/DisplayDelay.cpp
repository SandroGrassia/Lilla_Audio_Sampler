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
    switch (Lilla_state_0)
    {
    case PERFORMANCE:
    {
        Backgorund_red(0, 0, 17); // DISPLAY_board(float col, float row, int chars)
        tft.setCursor(display_coordinate_x(0), display_coordinate_y(0));
        tft.setTextColor(ILI9341_WHITE);
        tft.print("PERFORMANCE ");
        tft.setTextColor(ILI9341_WHITE);
        tft.print("DELAY");
    }
    break;

    case DIRECT_SAMPLING:
    {
        Backgorund_red(0, 0, 20);
        tft.setCursor(display_coordinate_x(0), display_coordinate_y(0));
        tft.setTextColor(ILI9341_WHITE);
        tft.print("DIRECT SAMPLER DELAY");
    }
    break;

    case LIVE_SAMPLING:
    {
        Backgorund_red(0, 0, 18); // DISPLAY_board(float col, float row, int chars)
        tft.setCursor(display_coordinate_x(0), display_coordinate_y(0));
        tft.setTextColor(ILI9341_WHITE);
        tft.print("LIVE SAMPLER ");
        tft.setTextColor(ILI9341_WHITE);
        tft.print("DELAY");
    }
    break;

    case MIDI_LOOP:
    {
        Backgorund_red(0, 0, 18); // DISPLAY_board(float col, float row, int chars)
        tft.setCursor(display_coordinate_x(0), display_coordinate_y(0));
        tft.setTextColor(ILI9341_WHITE);
        tft.print("MIDI LOOP ");
        tft.setTextColor(ILI9341_WHITE);
        tft.print("DELAY");
    }
    break;

    default:
    {
        Backgorund_red(0, 0, 5); // DISPLAY_board(float col, float row, int chars)
        tft.setCursor(display_coordinate_x(0), display_coordinate_y(0));
        tft.setTextColor(ILI9341_WHITE);
        tft.print("DELAY");
    }
    break;
    }

    tft.setCursor(display_coordinate_x(DELAY_column_row_VOLUME[0]), display_coordinate_y(DELAY_column_row_VOLUME[1]));
    tft.setTextColor(TEXT_COLOR);
    tft.print("VOLUME");
    Display_Common.P_Patch_volume_value(true);

    Display_Common.Show_all_effects();

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
    if (Lilla_state_0 == DIRECT_SAMPLING)
    {
        D_disabled();
    }
}

FLASHMEM
void DisplayDelay::D_sounds(void) // Display the requested setting while the audio transition runs independently.
{
    Cancel_text_reset_cursor(display_coordinate_x(DELAY_column_row_sounds[0]), display_coordinate_y(DELAY_column_row_sounds[1]), DELAY_chars_sounds); // DISPLAY_text(int X, int Y, int N)
    if (Lilla_state_0 == LIVE_SAMPLING)
    {
        tft.setTextColor(ILI9341_YELLOW);

        if ((Delay_data.instrument_route & 3) != 0)
        {
            tft.print("RECORDED AUDIO");
        }
        else
        {
            tft.print("NONE");
        }
        return;
    }

    for (auto instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
    {
        if (bitRead(Delay_data.instrument_route, instrument_id))
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
    tft.print(Delay_data.loop_gain);
    tft.print("%");
}

FLASHMEM
void DisplayDelay::D_delay_time(void) // Display the requested setting while the audio transition runs independently.
{
    Cancel_text_reset_cursor(display_coordinate_x(DELAY_column_row_delay_time[0]), display_coordinate_y(DELAY_column_row_delay_time[1]), DELAY_chars_delay_time);
    tft.setTextColor(ILI9341_YELLOW);

    const int time_tenths = Calc_delay_time_tenths(Delay_data.samples);
    if (time_tenths < 10000)
    {
        tft.print(time_tenths / 10.0f, time_tenths < 10 ? 1 : 0);
        tft.setTextColor(ILI9341_ORANGE);
        tft.print("ms");
    }
    else
    {
        tft.print(time_tenths / 10000.0f, 1);
        tft.setTextColor(ILI9341_ORANGE);
        tft.print("sec");
    }
}

FLASHMEM
void DisplayDelay::D_delay_time_LR(void) // Display the requested setting while the audio transition runs independently.
{
    Cancel_text_reset_cursor(display_coordinate_x(DELAY_column_row_delay_time_LR[0]), display_coordinate_y(DELAY_column_row_delay_time_LR[1]), DELAY_chars_delay_time_LR);
    tft.setTextColor(ILI9341_YELLOW);

    if (Delay_data.samples_LR == 0)
    {
        tft.print(0);
        tft.setTextColor(ILI9341_ORANGE);
        tft.print("ms R");
    }
    else if (Delay_data.samples_LR > 0)
    {
        tft.print("+");
        tft.print(Delay_data.samples_LR);
        tft.setTextColor(ILI9341_ORANGE);
        tft.print("ms L");
    }
    else
    {
        tft.print("+");
        tft.print(-Delay_data.samples_LR);
        tft.setTextColor(ILI9341_ORANGE);
        tft.print("ms R");
    }
}

FLASHMEM
void DisplayDelay::D_modulation_source(void) // Display the requested setting while the audio transition runs independently.
{
    Cancel_text_reset_cursor(display_coordinate_x(DELAY_column_row_modulation_source[0]), display_coordinate_y(DELAY_column_row_modulation_source[1]), DELAY_chars_modulation_source);
    tft.setTextColor(ILI9341_YELLOW);
    if (Delay_data.modulation_source == 0) // nessuna modulazione
    {
        tft.print("NONE");
    }
    else if (Delay_data.modulation_source == 1) // LFO
    {
        tft.print("LFO");
    }
    else
    {
        tft.print("SOURCE");
    }
}

FLASHMEM
void DisplayDelay::D_modulation_frequency(void) // Display the requested setting while the audio transition runs independently.
{
    Cancel_text_reset_cursor(display_coordinate_x(DELAY_column_row_modulation_frequency[0]), display_coordinate_y(DELAY_column_row_modulation_frequency[1]), DELAY_chars_modulation_frequency);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(Calc_delay_frequency(Delay_data.modulation_frequency));
    tft.setTextColor(ILI9341_ORANGE);
    tft.print("Hz");
}

FLASHMEM
void DisplayDelay::D_modulation_depth(void) // Display the requested setting while the audio transition runs independently.
{
    Cancel_text_reset_cursor(display_coordinate_x(DELAY_column_row_modulation_depth[0]), display_coordinate_y(DELAY_column_row_modulation_depth[1]), DELAY_chars_modulation_depth);
    tft.setTextColor(ILI9341_YELLOW);
    if (Calc_delay_depth(Delay_data.modulation_depth) <= 1.0f)
    {
        tft.print(Calc_delay_depth(Delay_data.modulation_depth) * 100, 1);
    }
    else
    {
        tft.print(Calc_delay_depth(Delay_data.modulation_depth) * 100, 0);
    }
    tft.print("%");
}

FLASHMEM
void DisplayDelay::D_modulation_phase_LR(void) // Display the requested setting while the audio transition runs independently.
{
    Cancel_text_reset_cursor(display_coordinate_x(DELAY_column_row_modulation_phase_LR[0]), display_coordinate_y(DELAY_column_row_modulation_phase_LR[1]), DELAY_chars_modulation_phase_LR);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(Delay_data.modulation_phase_LR);
    tft.setTextColor(ILI9341_ORANGE);
    tft.print("deg");
}

FLASHMEM
void DisplayDelay::D_disabled(void)
{
    Show_popup_text("DELAY IS DISABLED WHILE SAMPLING", ILI9341_WHITE, ILI9341_RED);
}

void DisplayDelay::DELAY_show_pointer_frame(const DELAY_element_name pointer, const bool show)
{
    Frame_by_col_row(DELAY_column_row_element[pointer][0], DELAY_column_row_element[pointer][1], DELAY_chars_element[pointer], show);
}
