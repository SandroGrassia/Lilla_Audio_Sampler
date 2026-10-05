/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#include "DisplayPerformance.h"
#include "GlobalDisplayCommon.h"
#include "SharedMixer.h"

void DisplayPerformance::P_show_delete_Instrument_frame(float line, bool show)
{
    Frame_by_pixels(display_coordinate_x(P_column_Instrument_frame), P_Instrument_pixels_y(line) - 4, P_chars_width_Instrument_frame, show);
}

void DisplayPerformance::Led_PERFORMANCE_instrument(int instrument_id, bool on)
{
    if (on)
    {
        tft.drawBitmap(P_pixel_x_LED, P_Instrument_pixels_y(P_line_of_instrument[instrument_id]), led_pic, 8, 8, (MX_mute[instrument_id] ? RED_ON : GREEN_ON));
    }
    else
    {
        tft.drawBitmap(P_pixel_x_LED, P_Instrument_pixels_y(P_line_of_instrument[instrument_id]), led_pic, 8, 8, (MX_mute[instrument_id] ? RED_OFF : GREEN_OFF));
    }
}

void DisplayPerformance::Led_tuning_tone(int patch_id)
{
    if (Lilla_state == PERFORMANCE)
    {
        tft.drawBitmap(P_pixel_x_LED, P_Instrument_pixels_y(Patch[patch_id].instruments), led_pic, 8, 8, (TT_playing ? ILI9341_RED : RED_OFF));
    }
}

FLASHMEM
void DisplayPerformance::P_show_pointer_frame(P_field_description_struct value, bool show)
{
    switch (value.field_name)
    {
    case field_P_Menu:
        Frame_by_col_row(P_column_menu_element[value.element], P_row_menu_element[value.element], P_dimension_voice_menu[P_element_menu[value.element]], show);
        break;

    case field_P_Patch:
        Frame_by_col_row(DisplayCommon::P_column_Patch_id, 0, 3, show);
        break;

    case field_P_Instrument:
    {
        Frame_by_pixels(display_coordinate_x(P_column_Instrument_frame), P_Instrument_pixels_y(value.instrument_line), P_chars_width_Instrument_frame, show);
        break;
    }

    case field_P_Instrument_inside:
        Frame_by_pixels(display_coordinate_x(P_column_instrument_element[value.element]), P_Instrument_pixels_y(value.instrument_line), P_chars_instrument_element[value.element], show);
        break;
    }
}

FLASHMEM
void DisplayPerformance::P_show_PERFORMANCE_page(bool change_patch, bool change_vol)
{
    P_Patch_header(change_patch, change_vol);
    P_show_Performance_menu(); // Draw the menu and update its navigation layout.
    P_show_Instruments_header();
    P_show_all_instruments(Patch_id);
}

int DisplayPerformance::P_Instrument_pixels_y(int position)
{
    return 4 + 15.0 * (6 + position) + 7;
}

FLASHMEM
void DisplayPerformance::P_Patch_header(bool change_patch, bool change_vol)
{
    tft.fillScreen(ILI9341_BLACK);
    Display_Common.P_show_PERFORMANCE_title();
    Display_Common.P_show_Patch_number(change_patch);
    Display_Common.P_Patch_VOLUME(change_vol);
    Display_Common.Show_all_effects();
}

FLASHMEM
void DisplayPerformance::P_show_Instruments_header(void)
{
    tft.setTextColor(TEXT_COLOR);

    tft.setCursor(display_coordinate_x(P_column_SOUND_title), display_coordinate_y(P_row_Instrument_title));
    tft.print("SOUND");

    tft.setCursor(display_coordinate_x(P_column_LOCK_title), display_coordinate_y(P_row_Instrument_title)); // (X0i + 2 * TT_Instrument_SPACE_X + 6)
    tft.print("LOCK");

    tft.setCursor(display_coordinate_x(P_column_P_title), display_coordinate_y(P_row_Instrument_title)); // (x_pos(X0i + 5 + TT_Instrument_SPACE_X)
    tft.print("P");

    tft.setCursor(display_coordinate_x(P_column_MIDI_title), display_coordinate_y(P_row_Instrument_title));
    tft.print("MIDI");

    tft.setCursor(display_coordinate_x(P_column_ROOT_K_title), display_coordinate_y(P_row_Instrument_title));
    tft.print("ROOT-K");

    tft.setCursor(display_coordinate_x(P_column_FROM_K_title), display_coordinate_y(P_row_Instrument_title));
    tft.print("FROM-K");

    tft.setCursor(display_coordinate_x(P_column_TO_K_title), display_coordinate_y(P_row_Instrument_title));
    tft.print("TO-K");

    tft.setCursor(display_coordinate_x(P_column_PAN_title), display_coordinate_y(P_row_Instrument_title));
    tft.print("PAN");

    tft.setCursor(display_coordinate_x(P_column_GAIN_title), display_coordinate_y(P_row_Instrument_title));
    tft.print("GAIN");

    tft.drawLine(6, display_coordinate_y(P_row_Instrument_title) + 11, 312, display_coordinate_y(P_row_Instrument_title) + 11, 0x630C);
}

FLASHMEM
void DisplayPerformance::P_show_Instrument_description(int patch_id, int instrument_id, bool editing)
{
    // P_delete_instrument_by_position(position);
    P_show_Sound_number(instrument_id, editing);
    P_show_Lock_value(patch_id, instrument_id, editing);
    P_show_Precedence_value(patch_id, instrument_id, editing);
    P_show_Midi_value(patch_id, instrument_id, editing);
    P_show_RootKey_value(patch_id, instrument_id, editing);
    P_show_FromKey_value(patch_id, instrument_id, editing);
    P_show_ToKey_value(patch_id, instrument_id, editing);
    P_show_Pan_value(patch_id, instrument_id, editing);
    P_show_Gain_value(patch_id, instrument_id, editing);
}

FLASHMEM
void DisplayPerformance::P_show_Sound_number(int instrument_id, bool editing)
{
    auto position = P_line_of_instrument[instrument_id];
    auto y_display = P_Instrument_pixels_y(position);

    auto x_display = display_coordinate_x(P_column_Sound);
    tft.setTextColor((editing ? ILI9341_YELLOW : ILI9341_WHITE));

    Cancel_text_reset_cursor(x_display, y_display, 1);
    tft.print(instrument_id + 1);
}

FLASHMEM
void DisplayPerformance::P_show_Lock_value(int patch_id, int instrument_id, bool editing)
{
    auto position = P_line_of_instrument[instrument_id];
    auto y_display = P_Instrument_pixels_y(position);

    auto x_display = display_coordinate_x(P_column_instrument_element[0]);
    tft.setTextColor((editing ? ILI9341_YELLOW : ILI9341_WHITE));

    Cancel_text_reset_cursor(x_display, y_display, 1);
    if (Patch[patch_id].Instrument[instrument_id].lock)
    {
        tft.print("Y");
    }
    else
    {
        tft.print("N");
    }
}

FLASHMEM
void DisplayPerformance::P_show_Precedence_value(int patch_id, int instrument_id, bool editing)
{
    auto position = P_line_of_instrument[instrument_id];
    auto y_display = P_Instrument_pixels_y(position);

    auto x_display = display_coordinate_x(P_column_instrument_element[1]);
    tft.setTextColor((editing ? ILI9341_YELLOW : ILI9341_WHITE));

    Cancel_text_reset_cursor(x_display, y_display, 1);
    if (Patch[patch_id].Instrument[instrument_id].precedence)
    {
        tft.print("X");
    }
    else
    {
        tft.print("N");
    }
}

FLASHMEM
void DisplayPerformance::P_show_Midi_value(int patch_id, int instrument_id, bool editing)
{
    auto position = P_line_of_instrument[instrument_id];
    auto y_display = P_Instrument_pixels_y(position);

    auto x_display = display_coordinate_x(P_column_instrument_element[2]);
    tft.setTextColor((editing ? ILI9341_YELLOW : ILI9341_WHITE));

    Cancel_text_reset_cursor(x_display, y_display, 2);
    tft.print(Get_midi_channel(patch_id, instrument_id) + 1);
}

FLASHMEM
void DisplayPerformance::P_show_RootKey_value(int patch_id, int instrument_id, bool editing)
{
    auto position = P_line_of_instrument[instrument_id];
    auto y_display = P_Instrument_pixels_y(position);

    auto x_display = display_coordinate_x(P_column_instrument_element[3]);
    tft.setTextColor((editing ? ILI9341_YELLOW : ILI9341_WHITE));

    Cancel_text_reset_cursor(x_display, y_display, 4);
    Note(Patch[patch_id].Instrument[instrument_id].root_key);
}

FLASHMEM
void DisplayPerformance::P_show_FromKey_value(int patch_id, int instrument_id, bool editing)
{
    auto position = P_line_of_instrument[instrument_id];
    auto y_display = P_Instrument_pixels_y(position);

    auto x_display = display_coordinate_x(P_column_instrument_element[4]);
    tft.setTextColor((editing ? ILI9341_YELLOW : ILI9341_WHITE));

    Cancel_text_reset_cursor(x_display, y_display, 4);
    Note(Patch[patch_id].Instrument[instrument_id].from_note);
}

FLASHMEM
void DisplayPerformance::P_show_ToKey_value(int patch_id, int instrument_id, bool editing)
{
    auto position = P_line_of_instrument[instrument_id];
    auto y_display = P_Instrument_pixels_y(position);

    auto x_display = display_coordinate_x(P_column_instrument_element[5]);
    tft.setTextColor((editing ? ILI9341_YELLOW : ILI9341_WHITE));

    Cancel_text_reset_cursor(x_display, y_display, 4);
    Note(Patch[patch_id].Instrument[instrument_id].to_note);
}

FLASHMEM
void DisplayPerformance::P_show_Pan_value(int patch_id, int instrument_id, bool editing)
{
    auto position = P_line_of_instrument[instrument_id];
    auto y_display = P_Instrument_pixels_y(position);

    auto x_display = display_coordinate_x(P_column_instrument_element[6]);
    tft.setTextColor((editing ? ILI9341_YELLOW : ILI9341_WHITE));

    const uint16_t sound_id = Get_sound_id(patch_id, instrument_id);

    Cancel_text_reset_cursor(x_display, y_display, 2);
    if (Sound[sound_id].pan < 0)
    {
        tft.print("L");
    }
    else if (Sound[sound_id].pan > 0)
    {
        tft.print("R");
    }
    tft.print(abs(Sound[sound_id].pan));
}

FLASHMEM
void DisplayPerformance::P_show_Gain_value(int patch_id, int instrument_id, bool editing)
{
    auto position = P_line_of_instrument[instrument_id];
    auto y_display = P_Instrument_pixels_y(position);

    auto x_display = display_coordinate_x(P_column_instrument_element[7]);
    tft.setTextColor((editing ? ILI9341_YELLOW : ILI9341_WHITE));

    Cancel_text_reset_cursor(x_display, y_display, 4);
    tft.print(Sound[Get_sound_id(patch_id, instrument_id)].gain / 20.0f, 2);
}

FLASHMEM
void DisplayPerformance::P_show_all_instruments(int patch_id)
{
    tft.fillRect(0, P_Instrument_pixels_y(0) - 4, 320, 240, ILI9341_BLACK);

    for (auto instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
    {
        if (Patch[patch_id].Instrument[instrument_id].used)
        {
            P_show_Instrument_description(patch_id, instrument_id, true);
        }
    }

    if (tuning_tone_flag)
    {
        P_show_TuningTone_instrument(patch_id);
    }
}

FLASHMEM
void DisplayPerformance::P_show_TuningTone_instrument(int patch_id)
{
    auto position = Patch[patch_id].instruments;

    if (tuning_tone_flag)
    {
        tft.setTextColor(ILI9341_WHITE);

        tft.setCursor(display_coordinate_x(TT_Instrument_INDENT_X0 + 2), P_Instrument_pixels_y(position));
        tft.print("TUNING-TONE");

        tft.setCursor(display_coordinate_x(TT_Instrument_INDENT_X0 + 3 * TT_Instrument_SPACE_X + 10), P_Instrument_pixels_y(position));
        tft.print("ALL");

        tft.setCursor(display_coordinate_x(TT_Instrument_INDENT_X0 + 4 * TT_Instrument_SPACE_X + 15), P_Instrument_pixels_y(position)); // root key
        Note(60);

        tft.setCursor(display_coordinate_x(2 + TT_Instrument_INDENT_X0 + 5 * TT_Instrument_SPACE_X + 18.5), P_Instrument_pixels_y(position)); // from key
        Note(0);

        tft.setCursor(display_coordinate_x(2 + TT_Instrument_INDENT_X0 + 6 * TT_Instrument_SPACE_X + 25), P_Instrument_pixels_y(position)); // to key
        Note(127);

        tft.setCursor(display_coordinate_x(TT_Instrument_INDENT_X0 + 7 * TT_Instrument_SPACE_X + 32), P_Instrument_pixels_y(position));
        tft.print("0");

        tft.setCursor(display_coordinate_x(TT_Instrument_INDENT_X0 + 8 * TT_Instrument_SPACE_X + 35), P_Instrument_pixels_y(position));
        tft.setTextColor(ILI9341_YELLOW);
        tft.print(tuning_tone_volume / 20.0f, 2);

        Led_tuning_tone(patch_id);
    }
    else
    {
        tft.fillRect(0, P_Instrument_pixels_y(position) - 4, 320, 15, ILI9341_BLACK);
    }
}

FLASHMEM
void DisplayPerformance::P_show_gain_TuningTone(int patch_id)
{
    auto position = Patch[patch_id].instruments;

    Cancel_text_reset_cursor(display_coordinate_x(TT_Instrument_INDENT_X0 + 8 * TT_Instrument_SPACE_X + 35), P_Instrument_pixels_y(position), 4);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(tuning_tone_volume / 20.0f, 2);
}

FLASHMEM
void DisplayPerformance::P_delete_instrument_by_position(int position)
{
    tft.fillRect(5, P_Instrument_pixels_y(position), 307, 7, ILI9341_BLACK);
}

FLASHMEM
void DisplayPerformance::Note(int note_number)
{
    tft.print(note_name[note_number % 12]);
    tft.print((int)(note_number / 12.0f) + first_octave);
}

FLASHMEM
void DisplayPerformance::P_show_Performance_menu(void)
{
    auto position = 0; // position on display

    Delete_text_row(1);
    tft.setTextColor(MENU_COLOR);

    for (auto element = 0; element < 5; ++element) // menu element
    {
        if (Menu_P[element])
        {
            if (position == 0)
            {
                P_column_menu_element[position] = 0;
            }

            else
            {
                P_column_menu_element[position] = P_column_menu_element[position - 1] + P_dimension_voice_menu[P_element_menu[position - 1]] + 1;
            }

            P_row_menu_element[position] = 1;
            P_element_menu[position] = static_cast<P_menu_elements_name>(element);
            P_position_Menu[element] = position;
            tft.setCursor(display_coordinate_x(P_column_menu_element[position]), display_coordinate_y(P_row_menu_element[position]));
            tft.print(P_menu_char[element]);
            ++position;
        }
    }
}

FLASHMEM
void DisplayPerformance::P_Confirm_patch_change_popup(void)
{
    tft.fillRoundRect(X_POPUP, Y_POPUP, L_POPUP, H_POPUP, 4, ILI9341_RED); // does NOT delete frame
    tft.setCursor(X_POPUP + display_coordinate_x(2), Y_POPUP + Y_POPUP_TXT);
    tft.setTextColor(ILI9341_WHITE);
    tft.print("SAVE CHANGES?");
    tft.setTextColor(ILI9341_YELLOW);
    tft.setCursor(X_POPUP + display_coordinate_x(2), Y_POPUP + Y_POPUP_OPT);
    tft.print("EXIT");
    tft.setCursor(X_POPUP + display_coordinate_x(8), Y_POPUP + Y_POPUP_OPT);
    tft.print("NO");
    tft.setCursor(X_POPUP + display_coordinate_x(12), Y_POPUP + Y_POPUP_OPT);
    tft.print("YES");
}

FLASHMEM
void DisplayPerformance::P_Confirm_patch_change_popup_frame(int value)
{
    Confirm_frame_on_RED(X_POPUP + display_coordinate_x(2), Y_POPUP + Y_POPUP_OPT, 4, false); // DISPLAY_confirm_frame(uint8_t col, uint8_t row, uint8_t chars, bool   print)
    Confirm_frame_on_RED(X_POPUP + display_coordinate_x(8), Y_POPUP + Y_POPUP_OPT, 2, false);
    Confirm_frame_on_RED(X_POPUP + display_coordinate_x(12), Y_POPUP + Y_POPUP_OPT, 3, false);

    switch (value)
    {
    case 0: // exit
        Confirm_frame_on_RED(X_POPUP + display_coordinate_x(2), Y_POPUP + Y_POPUP_OPT, 4, true);
        break;
    case 1: // no
        Confirm_frame_on_RED(X_POPUP + display_coordinate_x(8), Y_POPUP + Y_POPUP_OPT, 2, true);
        break;
    case 2: // yes
        Confirm_frame_on_RED(X_POPUP + display_coordinate_x(12), Y_POPUP + Y_POPUP_OPT, 3, true);
        break;
    default:
        break;
    }
}

FLASHMEM
void DisplayPerformance::P_Confirm_patch_delete_popup(void)
{
    tft.fillRoundRect(X_POPUP, Y_POPUP, L_POPUP, H_POPUP, 4, ILI9341_RED); // does NOT delete frame
    tft.setCursor(X_POPUP + display_coordinate_x(1), Y_POPUP + Y_POPUP_TXT);
    tft.setTextColor(ILI9341_WHITE);
    tft.print("CONFIRM DELETE?");
    tft.setTextColor(ILI9341_YELLOW);
    tft.setCursor(X_POPUP + display_coordinate_x(5.5), Y_POPUP + Y_POPUP_OPT);
    tft.print("NO");
    tft.setCursor(X_POPUP + display_coordinate_x(9.5), Y_POPUP + Y_POPUP_OPT);
    tft.print("YES");
}

FLASHMEM
void DisplayPerformance::P_Confirm_patch_delete_popup_frame(int value)
{
    Display_Common.Confirm_no_yes_popup_frame(value);
}
