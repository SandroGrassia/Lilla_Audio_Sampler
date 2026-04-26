/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#include "DisplayVCF.h"

void DisplayVCF::VCF_show_pointer_frame(const int pointer, const bool show)
{
    Frame_by_col_row(VCF_column_row_value_element[pointer][0], VCF_column_row_value_element[pointer][1], VCF_chars_value_element[pointer], show);
}

FLASHMEM
void DisplayVCF::VCF_show_VCF_page(const int patch_id, const int instrument_id)
{
    auto sound_id = Patch[patch_id].Instrument[instrument_id].sound_id;

    tft.fillScreen(ILI9341_BLACK);

    if (Lilla_state_0 == LIVE_SAMPLING)
    {
        //("012345678901234567890"); // Size 1: 21 chars
        Backgorund_red(0, 0, 12); // Backgorund_red(float & col, float row, int chars)
        tft.setTextColor(ILI9341_WHITE);
        tft.setCursor(display_coordinate_x(VCF_column_row_LIVE_SAMPLER[0]), display_coordinate_y(VCF_column_row_LIVE_SAMPLER[1]));
        tft.print("LIVE SAMPLER");

        tft.setCursor(display_coordinate_x(VCF_column_row_VOLUME[0]), display_coordinate_y(VCF_column_row_VOLUME[1]));
        tft.setTextColor(TEXT_COLOR);
        tft.print("VOLUME");
        Display_Manager.P_Patch_volume_value(true); // true: YELLOW
    }

    else
    {
        if (Lilla_state_0 == MIDI_LOOP)
        {
            Display_MidiLoop.Show_MIDI_LOOP();
        }
        else
        {
            Display_Manager.P_show_PERFORMANCE_title();
        }

        Display_Manager.P_show_Patch_number(false);

    tft.setCursor(display_coordinate_x(VCF_column_row_SOUND[0]), display_coordinate_y(VCF_column_row_SOUND[1]));
        tft.setTextColor(TEXT_COLOR);
        tft.print("SOUND");
    tft.setCursor(display_coordinate_x(VCF_column_row_SOUND_NUMBER[0]), display_coordinate_y(VCF_column_row_SOUND_NUMBER[1]));
        tft.setTextColor(ILI9341_WHITE);
        tft.print(instrument_id + 1);

    tft.setCursor(display_coordinate_x(VCF_column_row_GAIN[0]), display_coordinate_y(VCF_column_row_GAIN[1]));
        tft.setTextColor(TEXT_COLOR);
        tft.print("GAIN");
        VCF_show_sound_gain_value(sound_id);
    }

    if (Lilla_state_0 != MIDI_LOOP)
    {
        tft.setCursor(display_coordinate_x(VCF_column_row_RETURN[0]), display_coordinate_y(VCF_column_row_RETURN[1]));
        tft.setTextColor(MENU_COLOR);
        tft.print("RETURN");
    }

    Display_Manager.Show_all_effects();
    VCF_show_solo_value();

    Backgorund_red(0, 6.8, 9); // Display.Board(float col, float row, int chars)
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(6.8));
    tft.setTextColor(ILI9341_WHITE);
    tft.print("VCF + LFO");

    tft.setCursor(display_coordinate_x(VCF_column_row_FILTER_TYPE[0]), display_coordinate_y(VCF_column_row_FILTER_TYPE[1]));
    tft.setTextColor(TEXT_COLOR);
    tft.print("FILTER TYPE");
    VCF_show_filter_type_value(instrument_id);

    tft.setCursor(display_coordinate_x(VCF_column_row_CUTOFF[0]), display_coordinate_y(VCF_column_row_CUTOFF[1]));
    tft.setTextColor(TEXT_COLOR);
    tft.print("CUTOFF (PITCH 1.0)");
    VCF_show_cutoff_value(instrument_id);

    tft.setCursor(display_coordinate_x(VCF_column_row_RESONANCE[0]), display_coordinate_y(VCF_column_row_RESONANCE[1]));
    tft.setTextColor(TEXT_COLOR);
    tft.print("RESONANCE");
    VCF_show_resonance_value(instrument_id);

    tft.setCursor(display_coordinate_x(VCF_column_row_MODULATION_SOURCE[0]), display_coordinate_y(VCF_column_row_MODULATION_SOURCE[1]));
    tft.setTextColor(TEXT_COLOR);
    tft.print("MODULATION SOURCE");
    VCF_show_LFO_modulation_source(instrument_id);

    tft.setCursor(display_coordinate_x(VCF_column_row_MODULATION_FREQ_TIME[0]), display_coordinate_y(VCF_column_row_MODULATION_FREQ_TIME[1]));
    tft.setTextColor(TEXT_COLOR);
    tft.print("MOD FREQ/TIME");
    VCF_show_LFO_freq_time(instrument_id);

    tft.setCursor(display_coordinate_x(VCF_column_row_MODULATION_DEPTH[0]), display_coordinate_y(VCF_column_row_MODULATION_DEPTH[1]));
    tft.setTextColor(TEXT_COLOR);
    tft.print("MODULATION DEPTH");
    VCF_show_LFO_modulation_depth(instrument_id);
}

FLASHMEM
void DisplayVCF::VCF_show_sound_gain_value(const int sound_id)
{
    Cancel_text_reset_cursor(
        display_coordinate_x(VCF_column_row_value_element[value_VCF_Gain_Volume][0]),
        display_coordinate_y(VCF_column_row_value_element[value_VCF_Gain_Volume][1]),
        VCF_chars_Gain);
    tft.setTextColor((Lilla_state_0 == LIVE_SAMPLING ? ILI9341_WHITE : ILI9341_YELLOW));
    tft.print(Sound[sound_id].gain / 20.0f);
}

FLASHMEM
void DisplayVCF::VCF_show_solo_value(void)
{
    Cancel_text_reset_cursor(display_coordinate_x(21.5), display_coordinate_y(4.9), VCF_chars_Solo);
    if (solo_flag)
    {
        tft.setTextColor(ILI9341_YELLOW);
        tft.print("* SOLO *");
    }
}

FLASHMEM
void DisplayVCF::VCF_show_filter_type_value(const int instrument_id)
{
    Cancel_text_reset_cursor(
        display_coordinate_x(VCF_column_row_value_element[value_VCF_FilterType][0]),
        display_coordinate_y(VCF_column_row_value_element[value_VCF_FilterType][1]),
        VCF_chars_FilterType);
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
void DisplayVCF::VCF_show_cutoff_value(const int instrument_id)
{
    Cancel_text_reset_cursor(
        display_coordinate_x(VCF_column_row_value_element[value_VCF_Cutoff][0]),
        display_coordinate_y(VCF_column_row_value_element[value_VCF_Cutoff][1]),
        VCF_chars_Cutoff);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(Preset[instrument_id].Filter.pivot, 0);
    tft.setTextColor(ILI9341_ORANGE);
    tft.print("Hz");
}

FLASHMEM
void DisplayVCF::VCF_show_resonance_value(const int instrument_id)
{
    Cancel_text_reset_cursor(
        display_coordinate_x(VCF_column_row_value_element[value_VCF_Resonance][0]),
        display_coordinate_y(VCF_column_row_value_element[value_VCF_Resonance][1]),
        VCF_chars_Resonance);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(Preset[instrument_id].Filter.resonance, 2);
}

FLASHMEM
void DisplayVCF::VCF_show_LFO_modulation_source(const int instrument_id)
{
    Cancel_text_reset_cursor(
        display_coordinate_x(VCF_column_row_value_element[value_VCF_LfoModulationType][0]),
        display_coordinate_y(VCF_column_row_value_element[value_VCF_LfoModulationType][1]),
        VCF_chars_LfoModulation);
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

    VCF_show_LFO_freq_time(instrument_id); // if source changes, freq/time may need to switch
}

FLASHMEM
void DisplayVCF::VCF_show_LFO_freq_time(const int instrument_id)
{
    Cancel_text_reset_cursor(
        display_coordinate_x(VCF_column_row_value_element[value_VCF_LfoModFreqTime][0]),
        display_coordinate_y(VCF_column_row_value_element[value_VCF_LfoModFreqTime][1]),
        VCF_chars_ModFreqTime);
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
void DisplayVCF::VCF_show_LFO_modulation_depth(const int instrument_id)
{
    Cancel_text_reset_cursor(
        display_coordinate_x(VCF_column_row_value_element[value_VCF_LfoModDepth][0]),
        display_coordinate_y(VCF_column_row_value_element[value_VCF_LfoModDepth][1]),
        VCF_chars_ModDepth);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(100 * Preset[instrument_id].Filter.index, 0);
    tft.print("%");
}
