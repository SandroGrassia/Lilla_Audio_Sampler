/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#include "DisplayMidiLoop.h"

FLASHMEM
void DisplayMidiLoop::Loop_REC_advice(int track, bool on)
{
    if (on)
    {
        tft.setCursor(display_coordinate_x(Loop_LOOPS_X + 7 * track), display_coordinate_y(Loop_HEAD_R));
        tft.setTextColor(ILI9341_WHITE);
        tft.print(track + 1);
        tft.print("-REC");
    }
    else
    {
        Cancel_text_reset_cursor(display_coordinate_x(Loop_LOOPS_X + 7 * track), display_coordinate_y(Loop_HEAD_R), 5);
    }
}

FLASHMEM
void DisplayMidiLoop::Loop_show_Loop_page(void)
{
    tft.fillScreen(ILI9341_BLACK);
    Loop_show_midi_loop_title();
    Loop_menu();
    Display_Manager.P_show_Patch_number(Patch_id);
    Display_Manager.P_Patch_VOLUME(true);
    Display_Manager.ALL_show_effects();

    tft.setTextColor(TEXT_COLOR);
    tft.setCursor(display_coordinate_x(18), display_coordinate_y(0));
    tft.print("LOOP");
    Loop_loop_id();

    tft.setCursor(display_coordinate_x(Loop_HEAD_C - 1), display_coordinate_y(Loop_HEAD_R - 1));
    tft.print("METRO");
    tft.setCursor(display_coordinate_x(Loop_HEAD_C - 1), display_coordinate_y(Loop_HEAD_R));
    tft.print("TRACK");
    tft.setCursor(display_coordinate_x(Loop_HEAD_C - 1), display_coordinate_y(Loop_HEAD_R + 1));
    tft.print("SLIDE");
    tft.setCursor(display_coordinate_x(Loop_HEAD_C - 2), display_coordinate_y(Loop_HEAD_R + 2));
    tft.print("TRANSP");
    tft.setCursor(display_coordinate_x(Loop_HEAD_C - 1), display_coordinate_y(Loop_HEAD_R + 3));
    tft.print("LEVEL");
    tft.setCursor(display_coordinate_x(Loop_HEAD_C - 1), display_coordinate_y(Loop_HEAD_R + 4));
    tft.print("SOUND");

    tft.setTextColor(ILI9341_WHITE);

    for (auto instrument_id = 0; instrument_id < INSTRUMENTS_MAX; ++instrument_id)
    {
        if (Patch[Patch_id].Instrument[instrument_id].used)
        {
            tft.setCursor(display_coordinate_x(Loop_HEAD_C + 3), 151 + instrument_id * 11);
            tft.print(instrument_id + 1);
        }
    }

    for (auto track = 0; track < TRACKS; ++track)
    {
        Loop_track_data(track);
    }

    Loop_time_stretched();
}

FLASHMEM
void DisplayMidiLoop::Loop_loop_id(void)
{
    Cancel_text(display_coordinate_x(22), display_coordinate_y(0), 5);
    tft.setCursor(display_coordinate_x(23), display_coordinate_y(0));
    tft.setTextColor(ILI9341_YELLOW);

    if (LOOP_id < 0)
    {
        tft.print("---");
        return;
    }

    tft.print(LOOP_id);
}

FLASHMEM
void DisplayMidiLoop::Loop_menu(void)
{
    uint8_t position = 0;

    Cancel_text(display_coordinate_x(0), display_coordinate_y(1), 27);
    tft.setTextColor(MENU_COLOR);

    for (auto element = 0; element < 4; ++element)
    {
        if (Menu_Loop[element])
        {
            if (position == 0)
            {
                X_position_Menu_Loop[position] = 0;
            }
            else
            {
                X_position_Menu_Loop[position] = X_position_Menu_Loop[position - 1] + dimension_voice_Menu_Loop[element_Menu_Loop[position - 1]] + 1;
            }

            element_Menu_Loop[position] = element;
            position_Menu_Loop[element] = position;
            tft.setCursor(display_coordinate_x(X_position_Menu_Loop[position]), display_coordinate_y(1));
            tft.print(Menu_Loop_char[element]);
            ++position;
        }
    }
}

FLASHMEM
void DisplayMidiLoop::Loop_show_frame_menu(int position, bool fresh)
{
    if (!fresh && Loop_menu_position_0 >= 0)
    {
        Frame_by_col_row(Loop_X_position_menu_0, 1, Loop_dimension_voice_menu_0, false);
    }

    if (!Menu_Loop[0] && !Menu_Loop[1] && !Menu_Loop[2])
    {
        Loop_menu_position_0 = -1;
        return;
    }

    Frame_by_col_row(X_position_Menu_Loop[position], 1, dimension_voice_Menu_Loop[element_Menu_Loop[position]], true);
    choice_loop_menu = element_Menu_Loop[position];

    Loop_X_position_menu_0 = X_position_Menu_Loop[position];
    Loop_dimension_voice_menu_0 = dimension_voice_Menu_Loop[element_Menu_Loop[position]];
    Loop_menu_position_0 = position;
}

void DisplayMidiLoop::Loop_Delete_all_frame_menu(void)
{
    for (auto element = 0; element < 4; ++element)
    {
        if (Menu_Loop[element])
        {
            Frame_by_col_row(X_position_Menu_Loop[position_Menu_Loop[element]], 1, dimension_voice_Menu_Loop[element], false);
        }
    }
}

FLASHMEM
void DisplayMidiLoop::Loop_show_midi_loop_title(void)
{
    Backgorund_red(0, 0, 9);
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(0));
    tft.setTextColor(ILI9341_WHITE);
    tft.print("MIDI LOOP");
}

FLASHMEM
void DisplayMidiLoop::Loop_track_data(int track)
{
    // Numero
    Cancel_text_reset_cursor(display_coordinate_x(Loop_LOOPS_X + 7 * track), display_coordinate_y(Loop_HEAD_R), 5);
    if (LOOP_events[track] > 0)
    {
        tft.setTextColor(ILI9341_WHITE);
        tft.print(track + 1);
    }

    // Slide
    Cancel_text_reset_cursor(display_coordinate_x(Loop_LOOPS_X + 7 * track), display_coordinate_y(Loop_HEAD_R + 1), 6);
    if (LOOP_events[track] > 0)
    {
        tft.setTextColor(ILI9341_YELLOW);
        tft.print((float)(LOOP_slide[track]) / 1000.0f, 2);
        tft.setTextColor(ILI9341_ORANGE);
        tft.print("s");
    }

    // Pitch
    Cancel_text_reset_cursor(display_coordinate_x(Loop_LOOPS_X + 7 * track), display_coordinate_y(Loop_HEAD_R + 2), 7);
    if (LOOP_events[track] > 0)
    {
        tft.setTextColor(ILI9341_YELLOW);
        tft.print(LOOP_pitch_int[track]);
        tft.setTextColor(ILI9341_ORANGE);
        tft.print("key");
    }

    // Volume
    Cancel_text_reset_cursor(display_coordinate_x(Loop_LOOPS_X + 7 * track), display_coordinate_y(Loop_HEAD_R + 3), 6);
    if (LOOP_events[track] > 0)
    {
        tft.setTextColor(ILI9341_YELLOW);
        tft.print(LOOP_volume[track], 1);
    }
}

FLASHMEM
void DisplayMidiLoop::Loop_time_stretched(void)
{
    Cancel_text_reset_cursor(display_coordinate_x(Loop_LOOP_TIME), display_coordinate_y(Loop_HEAD_R - 1), 7);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print((float)(LOOP_time * LOOP_stretch) / 1000.0);
    tft.setTextColor(ILI9341_ORANGE);
    tft.print("s");
}

void DisplayMidiLoop::Loop_led(int track, int instrument_id, bool on)
{
    tft.drawBitmap(Loop_LED_X + track * 42, Loop_LED_Y + instrument_id * Loop_LED_DY, led_pic, 8, 8, on ? GREEN_ON : GREEN_OFF);
}

void DisplayMidiLoop::Loop_led_metronomo(int Xled, int Yled, bool ONled)
{
    tft.drawBitmap(Xled, Yled, led_pic, 8, 8, ONled ? RED_ON : RED_OFF);
}
