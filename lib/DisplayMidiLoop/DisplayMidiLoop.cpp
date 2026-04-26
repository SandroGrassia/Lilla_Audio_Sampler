/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#include "DisplayMidiLoop.h"

FLASHMEM
void DisplayMidiLoop::Loop_REC_advice(const int track, const bool on)
{
    if (on)
    {
        tft.setCursor(display_coordinate_x(Loop_column_row_track[track][0]), display_coordinate_y(Loop_column_row_track[track][1]));
        tft.setTextColor(ILI9341_WHITE);
        tft.print(track + 1);
        tft.print("-REC");
    }
    else
    {
        Cancel_text_reset_cursor(display_coordinate_x(Loop_column_row_track[track][0]), display_coordinate_y(Loop_column_row_track[track][1]), 5);
    }
}

FLASHMEM
void DisplayMidiLoop::Show_loop_id(void)
{
    Cancel_text_reset_cursor(display_coordinate_x(Loop_column_row_loop[0]), display_coordinate_y(Loop_column_row_loop[1]), Loop_chars_loop);
    tft.setTextColor(ILI9341_YELLOW);

    if (LOOP_id < 0)
    {
        tft.print("---");
        return;
    }

    tft.print(LOOP_id);
}

void DisplayMidiLoop::Show_patch_id(void)
{
    static const int Loop_chars_patch = 3;
    Cancel_text_reset_cursor(display_coordinate_x(Loop_column_row_patch[0]), display_coordinate_y(Loop_column_row_patch[1]), Loop_chars_patch);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(Patch_id);
}

void DisplayMidiLoop::Show_volume(void)
{
    Cancel_text_reset_cursor(display_coordinate_x(Loop_column_row_volume[0]), display_coordinate_y(Loop_column_row_volume[1]), Loop_chars_volume);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(volume_patch / 20.0f, 2);
}

FLASHMEM
void DisplayMidiLoop::Show_Loop_page(void)
{
    tft.fillScreen(ILI9341_BLACK);
    Show_MIDI_LOOP();
    Show_menu();

    Display_Manager.Show_all_effects();

    tft.setTextColor(TEXT_COLOR);
    tft.setCursor(display_coordinate_x(Loop_column_row_LOOP[0]), display_coordinate_y(Loop_column_row_LOOP[1]));
    tft.print("LOOP");
    Show_loop_id();

    tft.setCursor(display_coordinate_x(Loop_column_row_PATCH[0]), display_coordinate_y(Loop_column_row_PATCH[1]));
    tft.setTextColor(TEXT_COLOR);
    tft.print("PATCH");
    Show_patch_id();

    tft.setCursor(display_coordinate_x(Loop_column_row_VOLUME[0]), display_coordinate_y(Loop_column_row_VOLUME[1]));
    tft.setTextColor(TEXT_COLOR);
    tft.print("VOLUME");
    Show_volume();

    tft.setCursor(display_coordinate_x(Loop_column_row_METRO[0]), display_coordinate_y(Loop_column_row_METRO[1]));
    tft.print("METRO");

    tft.setCursor(display_coordinate_x(Loop_column_row_TRACK[0]), display_coordinate_y(Loop_column_row_TRACK[1]));
    tft.print("TRACK");

    tft.setCursor(display_coordinate_x(Loop_column_row_SHIFT[0]), display_coordinate_y(Loop_column_row_SHIFT[1]));
    tft.print("SHIFT");

    tft.setCursor(display_coordinate_x(Loop_column_row_TRANSP[0]), display_coordinate_y(Loop_column_row_TRANSP[1]));
    tft.print("TRANSP");

    tft.setCursor(display_coordinate_x(Loop_column_row_LEVEL[0]), display_coordinate_y(Loop_column_row_LEVEL[1]));
    tft.print("LEVEL");

    tft.setCursor(display_coordinate_x(Loop_column_row_SOUND[0]), display_coordinate_y(Loop_column_row_SOUND[1]));
    tft.print("SOUND");

    tft.setTextColor(ILI9341_WHITE);

    for (auto instrument_id = 0; instrument_id < INSTRUMENTS_MAX; ++instrument_id)
    {
        if (Patch[Patch_id].Instrument[instrument_id].used)
        {
            // tft.setCursor(display_coordinate_x(Loop_HEAD_C + 4), 151 + instrument_id * 11);
            tft.setCursor(display_coordinate_x(Loop_column_row_sound_id_0[0]), display_coordinate_y(Loop_column_row_sound_id_0[1] + Loop_coefficient_row_sound_id * instrument_id));
            tft.print(instrument_id + 1);
        }
    }

    for (auto track = 0; track < TRACKS; ++track)
    {
        Show_track_all_data(track);
    }

    Loop_total_time();
}

FLASHMEM
void DisplayMidiLoop::Show_menu(void)
{
    uint8_t position = 0;

    Cancel_text(display_coordinate_x(0), display_coordinate_y(1), 27);
    tft.setTextColor(MENU_COLOR);

    for (auto element = 0; element < 4; ++element)
    {
        if (Menu_LOOP[element])
        {
            if (position == 0)
            {
                X_position_Menu_LOOP[position] = 0;
            }
            else
            {
                X_position_Menu_LOOP[position] = X_position_Menu_LOOP[position - 1] + dimension_voice_Menu_LOOP[element_Menu_LOOP[position - 1]] + 1;
            }

            element_Menu_LOOP[position] = element;
            position_Menu_LOOP[element] = position;
            tft.setCursor(display_coordinate_x(X_position_Menu_LOOP[position]), display_coordinate_y(1));
            tft.print(Menu_LOOP_char[element]);
            ++position;
        }
    }
}

FLASHMEM
void DisplayMidiLoop::Show_MIDI_LOOP(void)
{
    Backgorund_red(0, 0, 9);
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(0));
    tft.setTextColor(ILI9341_WHITE);
    tft.print("MIDI LOOP");
}

FLASHMEM
void DisplayMidiLoop::Show_track_all_data(const int track)
{
    // Numero
    Cancel_text_reset_cursor(display_coordinate_x(Loop_column_row_track[track][0]), display_coordinate_y(Loop_column_row_track[track][1]), Loop_chars_track_number);
    if (LOOP_events[track] > 0)
    {
        tft.setTextColor(ILI9341_WHITE);
        tft.print(track + 1);
    }

    // Slide
    Cancel_text_reset_cursor(display_coordinate_x(Loop_column_row_slide[track][0]), display_coordinate_y(Loop_column_row_slide[track][1]), Loop_chars_shift);
    if (LOOP_events[track] > 0)
    {
        tft.setTextColor(ILI9341_YELLOW);
        tft.print((float)(LOOP_slide[track]) / 1000.0f, 2);
        tft.setTextColor(ILI9341_ORANGE);
        tft.print("s");
    }

    // Pitch
    Cancel_text_reset_cursor(display_coordinate_x(Loop_column_row_pitch[track][0]), display_coordinate_y(Loop_column_row_pitch[track][1]), Loop_chars_transpose);
    if (LOOP_events[track] > 0)
    {
        tft.setTextColor(ILI9341_YELLOW);
        tft.print(LOOP_pitch_int[track]);
        tft.setTextColor(ILI9341_ORANGE);
        tft.print("key");
    }

    // Level
    Cancel_text_reset_cursor(display_coordinate_x(Loop_column_row_level[track][0]), display_coordinate_y(Loop_column_row_level[track][1]), Loop_chars_level);
    if (LOOP_events[track] > 0)
    {
        tft.setTextColor(ILI9341_YELLOW);
        tft.print(LOOP_volume[track], 1);
    }
}

FLASHMEM
void DisplayMidiLoop::Loop_total_time(void)
{
    Cancel_text_reset_cursor(display_coordinate_x(Loop_column_row_total_time[0]), display_coordinate_y(Loop_column_row_total_time[1]), Loop_chars_total_time);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print((float)(LOOP_time * LOOP_stretch) / 1000.0);
    tft.setTextColor(ILI9341_ORANGE);
    tft.print("s");
}

void DisplayMidiLoop::Loop_led(const int track, const int instrument_id, const bool on)
{
    // tft.drawBitmap(Loop_LED_X + track * 42, Loop_LED_Y + instrument_id * Loop_LED_DY, led_pic, 8, 8, on ? GREEN_ON : GREEN_OFF);
    tft.drawBitmap(display_coordinate_x(Loop_column_row_sound_LED_0[0] + track * Loop_coefficients_column_row_sound_LED[0]), display_coordinate_y(Loop_column_row_sound_LED_0[1] + instrument_id * Loop_coefficients_column_row_sound_LED[1]), led_pic, 8, 8, on ? GREEN_ON : GREEN_OFF);
}

void DisplayMidiLoop::Loop_led_metronomo(int Xled, int Yled, bool ONled)
{
    tft.drawBitmap(Xled, Yled, led_pic, 8, 8, ONled ? RED_ON : RED_OFF);
}

void DisplayMidiLoop::Loop_show_pointerMenu(const LOOP_menu_element_name pointer, const bool show)
{
    const int position = position_Menu_LOOP[pointer];
    Frame_by_col_row(X_position_Menu_LOOP[position], 1, dimension_voice_Menu_LOOP[element_Menu_LOOP[position]], show);

    Serial.print("DisplayMidiLoop::Loop_show_pointerMenu(const LOOP_menu_element_name pointer, const bool show) - pointer: ");
    Serial.print(pointer);
    Serial.print(" show: ");
    Serial.println(show);
}

void DisplayMidiLoop::Loop_show_pointerTrack(const int track, const LOOP_track_value_name pointerTrack, const bool show)
{
    switch (pointerTrack)
    {
    case value_LOOP_slide:
        Frame_by_col_row(Loop_column_row_slide[track][0], Loop_column_row_slide[track][1], Loop_chars_shift, show);
        break;

    case value_LOOP_pitch:
        Frame_by_col_row(Loop_column_row_pitch[track][0], Loop_column_row_pitch[track][1], Loop_chars_transpose, show);
        break;

    case value_LOOP_level:
        Frame_by_col_row(Loop_column_row_level[track][0], Loop_column_row_level[track][1], Loop_chars_level, show);
        break;
    }
}
