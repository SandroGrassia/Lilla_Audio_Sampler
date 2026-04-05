/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#include "DisplayVCF.h"

DisplayVCF Display_VCF;

FLASHMEM
void DisplayVCF::VCF_show_pointer_frame(int pointer, bool show)
{
        Frame_by_col_row(VCF_column_row_value_element[pointer][0], VCF_column_row_value_element[pointer][1], VCF_chars_value_element[pointer], show);

}

FLASHMEM
void DisplayVCF::VCF_show_VCF_page(int patch_id, int instrument_id)
{
    auto sound_id = Patch[patch_id].Instrument[instrument_id].sound_id;

    tft.fillScreen(ILI9341_BLACK);
    if (Lilla_state_0 != LIVE_SAMPLING)
    {
        if (Lilla_state_0 != MIDI_LOOP)
        {
            Display_Manager.P_show_PERFORMANCE_title();
        }
        else
        {
            Display_Manager.Loop_show_midi_loop_title();
        }

        Display_Manager.P_show_Patch_number(false);

        tft.setCursor(display_coordinate_x(23), display_coordinate_y(0));
        tft.setTextColor(TEXT_COLOR);
        tft.print("SOUND");
        tft.setCursor(display_coordinate_x(28.5), display_coordinate_y(0));
        tft.setTextColor(ILI9341_WHITE);
        tft.print(instrument_id + 1);

        tft.setCursor(display_coordinate_x(43), display_coordinate_y(0));
        tft.setTextColor(TEXT_COLOR);
        tft.print("GAIN");
        VCF_show_sound_gain_value(sound_id);
    }
    else
    {
        //("012345678901234567890"); // Size 1: 21 chars
        Show_Board(0, 0, 12); // Display.Board(float & col, float row, int chars)
        tft.setTextColor(ILI9341_WHITE);
        tft.setCursor(display_coordinate_x(0), display_coordinate_y(0));
        tft.print("LIVE SAMPLER");

        tft.setCursor(display_coordinate_x(41), display_coordinate_y(0));
        tft.setTextColor(TEXT_COLOR);
        tft.print("VOLUME");
        Display_Manager.P_Patch_volume_value(true); // true: YELLOW
    }

    if (Lilla_state_0 != MIDI_LOOP)
    {
        tft.setCursor(display_coordinate_x(0), display_coordinate_y(1));
        tft.setTextColor(MENU_COLOR);
        tft.print("RETURN");
        Frame_by_col_row(0, 1, 6, true);
    }

    Display_Manager.ALL_show_effects();
    VCF_show_solo_value();

    Show_Board(0, 6.8, 9); // Display.Board(float   col, float row, int chars)
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(6.8));
    tft.setTextColor(ILI9341_WHITE);
    tft.print("VCF + LFO");

    tft.setCursor(display_coordinate_x(0), display_coordinate_y(8));
    tft.setTextColor(TEXT_COLOR);
    tft.print("FILTER TYPE");
    VCF_show_filter_type_value(instrument_id);

    tft.setCursor(display_coordinate_x(0), display_coordinate_y(9));
    tft.setTextColor(TEXT_COLOR);
    tft.print("CUTOFF (PITCH 1.0)");
    VCF_show_cutoff_value(instrument_id);

    tft.setCursor(display_coordinate_x(0), display_coordinate_y(10));
    tft.setTextColor(TEXT_COLOR);
    tft.print("RESONANCE");
    VCF_show_resonance_value(instrument_id);

    tft.setCursor(display_coordinate_x(30), display_coordinate_y(8));
    tft.setTextColor(TEXT_COLOR);
    tft.print("MODULATION_SOURCE");
    VCF_show_lfo_Modulation(instrument_id);

    tft.setCursor(display_coordinate_x(30), display_coordinate_y(9));
    tft.setTextColor(TEXT_COLOR);
    tft.print("MOD FREQ/TIME");
    VCF_show_lfo_freq_time(instrument_id);

    tft.setCursor(display_coordinate_x(30), display_coordinate_y(10));
    tft.setTextColor(TEXT_COLOR);
    tft.print("MOD DEPTH");
    VCF_show_lfo_modulation_depth(instrument_id);
}

FLASHMEM
void DisplayVCF::VCF_show_sound_gain_value(int sound_id)
{
    Cancel_text_reset_cursor(display_coordinate_x(47.5), display_coordinate_y(0), 4);
    tft.setTextColor((Lilla_state_0 == LIVE_SAMPLING ? ILI9341_WHITE : ILI9341_YELLOW));
    tft.print(Sound[sound_id].gain / 20.0f);
}

FLASHMEM
void DisplayVCF::VCF_show_solo_value(void)
{
    Cancel_text_reset_cursor(display_coordinate_x(21.5), display_coordinate_y(4.9), 8);
    if (solo_flag)
    {
        tft.setTextColor(ILI9341_YELLOW);
        tft.print("* SOLO *");
    }
}

FLASHMEM
void DisplayVCF::VCF_show_filter_type_value(int instrument_id)
{
    Cancel_text_reset_cursor(display_coordinate_x(12), display_coordinate_y(8), 13);
    tft.setTextColor(ILI9341_YELLOW);

    if (Preset[instrument_id].Filter.use == 1)
    {
        switch (Preset[instrument_id].Filter.type)
        {
        case 0: // LP
            tft.print("LOWPASS");
            break;
        case 1: // HP
            tft.print("HIGHPASS");
            break;
        case 2: // BP
            tft.print("BANDPASS");
            break;
        case 3: // Notch
            tft.print("NOTCH");
            break;
        default:
            break;
        }
    }
    else
        tft.print("(NONE)");
}

FLASHMEM
void DisplayVCF::VCF_show_lfo_Modulation(int instrument_id)
{
    Cancel_text_reset_cursor(display_coordinate_x(41), display_coordinate_y(8), 9);
    tft.setTextColor(ILI9341_YELLOW);

    switch (Preset[instrument_id].Filter.modulation)
    {
    case 0:
        tft.print("NONE");
        break;
    case 1:
        tft.print("RISING");
        break;
    case 2:
        tft.print("FALLING");
        ;
        break;
    case 3:
        tft.print("LFO");
        break;
    case 4:
        tft.print("LFO+CC7");
        break;
    default:
        break;
    }

    VCF_show_lfo_freq_time(instrument_id);
}

FLASHMEM
void DisplayVCF::VCF_show_cutoff_value(int instrument_id)
{
    Cancel_text_reset_cursor(display_coordinate_x(19), display_coordinate_y(9), 7);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(Preset[instrument_id].Filter.pivot, 0);
    tft.setTextColor(ILI9341_ORANGE);
    tft.print("Hz");
}


FLASHMEM
void DisplayVCF::VCF_show_lfo_freq_time(int instrument_id)
{
    Cancel_text_reset_cursor(display_coordinate_x(44), display_coordinate_y(9), 7);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(Preset[instrument_id].Filter.frequency_time, 2);

    if (Preset[instrument_id].Filter.periodic)
    {
        Show_measure_unit("Hz", 2);
    }
    else
    {
        Show_measure_unit("sec", 3);
    }
}

FLASHMEM
void DisplayVCF::VCF_show_resonance_value(int instrument_id)
{
    Cancel_text_reset_cursor(display_coordinate_x(10), display_coordinate_y(10), 4);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(Preset[instrument_id].Filter.resonance, 2);
}

FLASHMEM
void DisplayVCF::VCF_show_lfo_modulation_depth(int instrument_id)
{
    Cancel_text_reset_cursor(display_coordinate_x(40), display_coordinate_y(10), 4);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(100 * Preset[instrument_id].Filter.index, 0);
    tft.print("%");
}
