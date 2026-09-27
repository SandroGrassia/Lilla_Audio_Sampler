/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#include "DisplaySound.h"
#include "SharedMixer.h"
#include <math.h>

FLASHMEM
void DisplaySound::Show_pointer_frame(S_field_description_struct description, bool show)
{
    switch (description.field_name)
    {
    case field_S_Menu:
        Frame_by_col_row(S_column_menu_element[description.menu_element], S_row_menu, S_dimension_voice_menu[S_element_menu[description.menu_element]], show);
        break;

    case field_S_Value:
        Frame_by_col_row(S_column_row_value_element[description.value_element][0], S_column_row_value_element[description.value_element][1], S_chars_value_element[description.value_element], show);
        break;
    }
}

FLASHMEM
void DisplaySound::Show_SOUND_page(int patch_id, int instrument_id)
{
    tft.fillScreen(ILI9341_BLACK);

    if (Lilla_state_0 == MIDI_LOOP)
    {
        Display_MidiLoop.Show_MIDI_LOOP();
    }
    else
    {
        Display_Common.P_show_PERFORMANCE_title();
    }

    tft.setCursor(display_coordinate_x(29), display_coordinate_y(0));
    tft.setTextColor(TEXT_COLOR);
    tft.print("PATCH");
    tft.setCursor(display_coordinate_x(S_column_row_Patch[0]), display_coordinate_y(S_column_row_Patch[1]));
    tft.setTextColor(ILI9341_WHITE);
    tft.print(Patch_id);

    tft.setCursor(display_coordinate_x(19), display_coordinate_y(0));// 23 -4
    tft.setTextColor(TEXT_COLOR);
    tft.print("SOUND");
    tft.setCursor(display_coordinate_x(S_column_row_Sound[0]), display_coordinate_y(S_column_row_Sound[1]));
    tft.setTextColor(ILI9341_WHITE);
    tft.print(instrument_id + 1);

    tft.setCursor(display_coordinate_x(39), display_coordinate_y(0));
    tft.setTextColor(TEXT_COLOR);
    tft.print("FILE");
    Show_File_value(instrument_id);

    Display_Common.Show_all_effects();

    tft.setCursor(display_coordinate_x(0), display_coordinate_y(4.9));
    tft.setTextColor(TEXT_COLOR);
    tft.print("GAIN");
    Show_Gain_value(patch_id, instrument_id);

    tft.setCursor(display_coordinate_x(18), display_coordinate_y(4.9));
    tft.setTextColor(TEXT_COLOR);
    tft.print("PITCH");
    Show_Pitch_value(instrument_id);

    tft.setCursor(display_coordinate_x(31), display_coordinate_y(4.9));
    tft.setTextColor(TEXT_COLOR);
    tft.print("PAN");
    Show_Pan_value(instrument_id);

    tft.setCursor(display_coordinate_x(41.5), display_coordinate_y(4.9));
    tft.setTextColor(TEXT_COLOR);
    tft.print("MIDI_CH");
    Show_Midi_channel_value(instrument_id);

    tft.setCursor(display_coordinate_x(0), display_coordinate_y(5.9));
    tft.setTextColor(TEXT_COLOR);
    tft.print("ATT");
    Show_Attack_value(instrument_id);

    tft.setCursor(display_coordinate_x(18), display_coordinate_y(5.9));
    tft.setTextColor(TEXT_COLOR);
    tft.print("DEC");
    Show_Decay_value(instrument_id);

    tft.setCursor(display_coordinate_x(31), display_coordinate_y(5.9));
    tft.setTextColor(TEXT_COLOR);
    tft.print("SUS");
    Show_Sustain_value(instrument_id);

    tft.setCursor(display_coordinate_x(41.5), display_coordinate_y(5.9));
    tft.setTextColor(TEXT_COLOR);
    tft.print("REL");
    Show_Release_value(instrument_id);

    tft.setCursor(display_coordinate_x(0), display_coordinate_y(6.9));
    tft.setTextColor(TEXT_COLOR);
    tft.print("PLAY MODE");
    Show_Play_mode_value(instrument_id);

    tft.setCursor(display_coordinate_x(31), display_coordinate_y(6.9));
    tft.setTextColor(TEXT_COLOR);
    tft.print("NOCLICK");
    Show_Noclick_value(instrument_id, true);

    tft.setCursor(display_coordinate_x(0), display_coordinate_y(15));
    tft.setTextColor(TEXT_COLOR);
    tft.print("TRIM STEP");
    Show_Trim_step_value();

    tft.setCursor(display_coordinate_x(37), display_coordinate_y(15));
    tft.setTextColor(TEXT_COLOR);
    tft.print("MAX PITCH");
    Show_players_Pitch_max_value(instrument_id);
}

FLASHMEM
void DisplaySound::Show_SOUND_menu(void)
{
        auto position = 0;

        Delete_text_row(S_row_menu);

        for (auto element = 0; element < S_menu_elements; ++element) // menu element
        {
            if (S_Menu[element])
            {
                if (position == 0)
                {
                    S_column_menu_element[position] = 0;
                }

                else
                {
                    S_column_menu_element[position] = S_column_menu_element[position - 1] + S_dimension_voice_menu[S_element_menu[position - 1]] + 1;
                }

                S_element_menu[position] = static_cast<S_menu_elements_name>(element);
                S_position_menu[element] = position;
                tft.setCursor(display_coordinate_x(S_column_menu_element[position]), display_coordinate_y(S_row_menu));
                tft.setTextColor(MENU_COLOR);
                tft.print(S_menu_char[element]);
                ++position;
            }
        }
}

FLASHMEM
void DisplaySound::Delete_all_menu_frame(void)
{
    int position;

    for (auto element = 0; element < 3; ++element)
    {
        if (S_Menu[element])
        {
            position = S_position_menu[element];
            Frame_by_col_row(S_column_menu_element[position], 1, S_dimension_voice_menu[element], false);
        }
    }
}

FLASHMEM
void DisplaySound::Show_Attack_value(int instrument_id)
{
    Cancel_text_reset_cursor(display_coordinate_x(S_column_row_value_element[value_S_Attack][0]), display_coordinate_y(S_column_row_value_element[value_S_Attack][1]), S_chars_Attack);
    tft.setTextColor(ILI9341_YELLOW);

    switch (Preset[instrument_id].attack_type)
    {
    case 0:
        tft.print("SLOW "); // Slow
        break;
    case 1:
        tft.print("FAST "); // Fast
        break;
    default:
        break;
    }

    tft.print(Preset[instrument_id].attack, 2);
    Show_measure_unit("sec", 3);
}

FLASHMEM
void DisplaySound::Show_Decay_value(int instrument_id)
{
    Cancel_text_reset_cursor(display_coordinate_x(S_column_row_value_element[value_S_Decay][0]), display_coordinate_y(S_column_row_value_element[value_S_Decay][1]), S_chars_Decay);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(Preset[instrument_id].decay, 2);
    Show_measure_unit("sec", 3);
}

FLASHMEM
void DisplaySound::Show_Sustain_value(int instrument_id)
{
    Cancel_text_reset_cursor(display_coordinate_x(S_column_row_value_element[value_S_Sustain][0]), display_coordinate_y(S_column_row_value_element[value_S_Sustain][1]), S_chars_Sustain);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(Preset[instrument_id].sustain * 100, 0);
    tft.print("%");
}

FLASHMEM
void DisplaySound::Show_Release_value(int instrument_id)
{
    Cancel_text_reset_cursor(display_coordinate_x(S_column_row_value_element[value_S_Release][0]), display_coordinate_y(S_column_row_value_element[value_S_Release][1]), S_chars_Release);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(Preset[instrument_id].release, 1);
    Show_measure_unit("sec", 3);
}

FLASHMEM
void DisplaySound::Show_File_value(int instrument_id)
{
    char audio_filename[NAME_FILE_SIZE];
    Cancel_text_reset_cursor(display_coordinate_x(S_column_row_value_element[value_S_File][0]), display_coordinate_y(S_column_row_value_element[value_S_File][1]), S_chars_File);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(Get_file_name(Preset[instrument_id].file, audio_filename));
}

FLASHMEM
void DisplaySound::Show_Midi_channel_value(int instrument_id)
{
    Cancel_text_reset_cursor(display_coordinate_x(S_column_row_value_element[value_S_Midi][0]), display_coordinate_y(S_column_row_value_element[value_S_Midi][1]), S_chars_Midi);
    tft.setTextColor(ILI9341_WHITE);
    tft.print(Preset[instrument_id].midi_channel + 1);
}
FLASHMEM
void DisplaySound::Show_Pitch_value(int instrument_id)
{
    Cancel_text_reset_cursor(display_coordinate_x(S_column_row_value_element[value_S_Pitch][0]), display_coordinate_y(S_column_row_value_element[value_S_Pitch][1]), S_chars_Pitch);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(Preset[instrument_id].pitch, 3);
}

FLASHMEM
void DisplaySound::Show_Gain_value(int patch_id, int instrument_id)
{
    Cancel_text_reset_cursor(display_coordinate_x(S_column_row_value_element[value_S_Gain][0]), display_coordinate_y(S_column_row_value_element[value_S_Gain][1]), S_chars_Gain);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(Sound[Get_sound_id(patch_id, instrument_id)].gain / 20.0);
}

FLASHMEM
void DisplaySound::Show_Pan_value(int instrument_id)
{
    Cancel_text_reset_cursor(display_coordinate_x(S_column_row_value_element[value_S_Pan][0]), display_coordinate_y(S_column_row_value_element[value_S_Pan][1]), S_chars_Pan);
    tft.setTextColor(ILI9341_YELLOW);

    if (Preset[instrument_id].pan < 0)
    {
        tft.print("L");
    }
    else if (Preset[instrument_id].pan > 0)
    {
        tft.print("R");
    }

    tft.print(abs(Preset[instrument_id].pan));
}

FLASHMEM
void DisplaySound::Show_Play_mode_value(int instrument_id)
{
    Cancel_text_reset_cursor(display_coordinate_x(S_column_row_value_element[value_S_PlayMode][0]), display_coordinate_y(S_column_row_value_element[value_S_PlayMode][1]), S_chars_PlayMode);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(loop_mode[Preset[instrument_id].mode]);
    tft.setCursor(display_coordinate_x(14), display_coordinate_y(6.9));
    tft.print(name_mode[Preset[instrument_id].mode]);
}

FLASHMEM
void DisplaySound::Show_Noclick_value(int instrument_id, bool value)
{
    Cancel_text_reset_cursor(display_coordinate_x(S_column_row_value_element[value_S_Noclick][0]), display_coordinate_y(S_column_row_value_element[value_S_Noclick][1]), S_chars_NoClick);
    tft.setTextColor((value ? ILI9341_YELLOW : ILI9341_WHITE));
    tft.print(Preset[instrument_id].Noclick);
    Show_measure_unit("S", 1);
}

FLASHMEM
void DisplaySound::Show_Trim_step_value(void)
{
    Cancel_text_reset_cursor(display_coordinate_x(9.5), display_coordinate_y(15), 6);
    tft.setTextColor(ILI9341_YELLOW);

    switch (trim_speed)
    {
    case 0:
        tft.print("1");
        break;
    case 1:
        tft.print("10");
        break;
    case 2:
        tft.print("100");
        break;
    case 3:
        tft.print("1K");
        break;
    case 4:
        tft.print("10K");
        break;
    case 5:
        tft.print("TOT/16");
        break;
    default:
        break;
    }
}

FLASHMEM
void DisplaySound::Show_players_Pitch_max_value(int instrument_id) // max pitch related to which media is read
{
    Cancel_text_reset_cursor(display_coordinate_x(46.5), display_coordinate_y(15), 5);
    tft.setTextColor(ILI9341_WHITE);

    const auto &preset = Preset[instrument_id];
    const bool live = preset.file >= FIRST_LIVE_SAMPLING_FILE;
    const float max_pitch = Playback_pitch_limit(preset.use_Wavetable, preset.source.storage == Psram, live);
    const int max_pitch_semitones = static_cast<int>(floorf(12.0f * log2f(max_pitch)));
    if (max_pitch_semitones >= 0)
    {
        tft.print("+");
    }
    tft.print(max_pitch_semitones);
    tft.print("st");
}

void DisplaySound::Show_wave(int instrument_id)
{
    auto sound_id_local = Get_sound_id(Patch_id, instrument_id);                                                // Active sound routed to the selected instrument.
    int yp, yn, y0;                                                                                                          // Upper sample, lower sample, and previous Y position on the canvas.
    int NC_A;                                                                                                                // Width of the no-click curtain drawn at both waveform edges.
    int16_t *X = Info.Sound_620_samples_array(Preset[instrument_id].file, Preset[instrument_id].A, Preset[instrument_id].B); // Two 620-sample envelopes used for waveform rendering.
    float volume_float = Volume_float[Sound[sound_id_local].gain];                                                           // Gain scaling applied to the displayed waveform amplitude.

    NC_A = ((WAVEBOARD_WIDTH * (Preset[instrument_id].Noclick < Noclick_max ? Preset[instrument_id].Noclick : Noclick_max)) / (Preset[instrument_id].B - Preset[instrument_id].A));

    if (NC_A == 0 && (Preset[instrument_id].Noclick > 0))
    {
        NC_A = 1;
    }

    int NC_B = WAVEBOARD_WIDTH - NC_A; // Start position of the right no-click curtain.
    int wave_pixel_0 = 0;              // Previous x coordinate used to connect waveform segments.
    y0 = CANVAS_WAVE_0;                // central position

    // tft.fillRect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint16_t color);
    canvas.fillRect(0, 0, WAVEBOARD_WIDTH, WAVEBOARD_HEIGHT, WAVE_BOARD_COLOR);
    canvas.fillRect(0, 0, NC_A, WAVEBOARD_HEIGHT, CURTAIN_NOCLICK_COLOR);
    canvas.fillRect(NC_B, 0, NC_A, WAVEBOARD_HEIGHT, CURTAIN_NOCLICK_COLOR);

    for (auto wave_pixel = 0; wave_pixel < WAVEBOARD_WIDTH; ++wave_pixel)
    {
        // 16bits>>11 = 5bits from -16 to +15
        yp = CANVAS_WAVE_0 - (*(X + wave_pixel) >> 10) * volume_float; // MAXIMUM delta 32*4 = 128  (−32768 <= int16_t <= +32767)
        yn = CANVAS_WAVE_0 - (*(X + wave_pixel + WAVEBOARD_WIDTH) >> 10) * volume_float;

        if (yp == CANVAS_WAVE_0 && yn == CANVAS_WAVE_0)
        {
            if (wave_pixel == 0)
            {
                canvas.drawPixel(0, yp, WAVE_COLOR);
            }
            else
            {
                canvas.drawLine(wave_pixel_0, y0, wave_pixel, CANVAS_WAVE_0, WAVE_COLOR);
                wave_pixel_0 = wave_pixel;
                y0 = yp;
            }
        }

        else if (yp < CANVAS_WAVE_0 && yn == CANVAS_WAVE_0)
        {
            if (wave_pixel > 0)
            {
                canvas.drawLine(wave_pixel_0, y0, wave_pixel, yp, WAVE_COLOR);
            }
            else
            {
                canvas.drawPixel(0, yp, WAVE_COLOR);
            }

            wave_pixel_0 = wave_pixel;
            y0 = yp;
        }

        else if (yp == CANVAS_WAVE_0 && yn > CANVAS_WAVE_0)
        {
            if (wave_pixel > 0)
            {
                canvas.drawLine(wave_pixel_0, y0, wave_pixel, yn, WAVE_COLOR);
            }
            else
            {
                canvas.drawPixel(0, yn, WAVE_COLOR);
            }

            wave_pixel_0 = wave_pixel;
            y0 = yn;
        }
        else if (yp < CANVAS_WAVE_0 && yn > CANVAS_WAVE_0)
        {
            if (wave_pixel > 0)
            {
                canvas.drawLine(wave_pixel_0, y0, wave_pixel, yn, WAVE_COLOR);
                canvas.drawLine(wave_pixel, yp, wave_pixel, yn, WAVE_COLOR);
            }
            else
            {
                canvas.drawLine(0, yp, 0, yn, WAVE_COLOR);
            }

            wave_pixel_0 = wave_pixel;
            y0 = yn;
        }
    }

    if (solo_flag)
    {
        canvas.setCursor(display_coordinate_x(21) + 5, display_coordinate_y(0));
        canvas.setTextColor(ILI9341_YELLOW);
        canvas.print("* SOLO *");
    }

    canvas.setTextColor(TEXT_COLOR);
    canvas.setCursor(display_coordinate_x(0), Y_FOOTER_TEXT);
    canvas.print("FROM");
    canvas.setTextColor((slicing_mode ? ILI9341_WHITE : ILI9341_YELLOW));
    canvas.setCursor(display_coordinate_x(4.5), Y_FOOTER_TEXT);
    canvas.setTextColor(ILI9341_YELLOW);
    canvas.print(Preset[instrument_id].A);
    canvas.setTextColor(ILI9341_ORANGE);
    canvas.print("S");

    canvas.setTextColor(TEXT_COLOR);
    canvas.setCursor(display_coordinate_x(21), Y_FOOTER_TEXT);
    canvas.print("TOT");
    canvas.setTextColor((slicing_mode ? ILI9341_WHITE : ILI9341_YELLOW));
    canvas.setCursor(display_coordinate_x(24.5), Y_FOOTER_TEXT);

    float time = (Preset[instrument_id].B - Preset[instrument_id].A + 1) / 44100.0f; // Duration of the selected slice in seconds (pitch = 1).

    if (time > 1.0)
    {
        canvas.print(time, 2);
        canvas.setTextColor(ILI9341_ORANGE);
        canvas.print("sec");
    }

    else
    {
        canvas.print(time * 1000.0, 2);
        canvas.setTextColor(ILI9341_ORANGE);
        canvas.print("ms");
    }

    if (slicing_mode) // FIRST/LAST
    {
        String Value = String(Preset[instrument_id].B, DEC);
        canvas.setCursor(display_coordinate_x(46 - Value.length() - 1), Y_FOOTER_TEXT);
        canvas.setTextColor(TEXT_COLOR);
        canvas.print("TO");
        canvas.setCursor(display_coordinate_x(48.5 - Value.length() - 1), Y_FOOTER_TEXT);
        canvas.setTextColor(ILI9341_YELLOW);
        canvas.print(Value);
        canvas.setTextColor(ILI9341_ORANGE);
        canvas.print("S");
    }

    else // Window fissa: FIRST/WINDOW
    {
        String Value = String(Preset[instrument_id].B - Preset[instrument_id].A + 1, DEC);
        canvas.setCursor(display_coordinate_x(45 - Value.length() - 1), Y_FOOTER_TEXT);
        canvas.setTextColor(TEXT_COLOR);
        canvas.print("SLICE");
        canvas.setCursor(display_coordinate_x(50.5 - Value.length() - 1), Y_FOOTER_TEXT);
        canvas.setTextColor(ILI9341_YELLOW);
        canvas.print(Value);
        canvas.setTextColor(ILI9341_ORANGE);
        canvas.print("S");
    }

    tft.writeRect(X_WAVEBOARD_LEFT, WAVE_MAX, canvas.width(), canvas.height(), canvas.getBuffer());
}

void DisplaySound::Led_SOUND_EDIT_instrument(int instrument_id, bool on)
{
    if (on)
    {
        tft.drawBitmap(display_coordinate_x(18) - 4, display_coordinate_y(0), led_pic, 8, 8, (MX_mute[instrument_id] ? RED_ON : GREEN_ON));
    }
    else
    {
        tft.drawBitmap(display_coordinate_x(18) - 4, display_coordinate_y(0), led_pic, 8, 8, (MX_mute[instrument_id] ? RED_OFF : GREEN_OFF));
    }
    return;
}
