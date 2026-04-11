/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#include "DisplayManager.h"

void DisplayManager::Lilla_cover_slow(void)
{
    tft.fillScreen(ILI9341_BLACK);

    // fade-in
    for (auto i = 0; i <= 10; ++i)
    {
        Logo(static_cast<float>(i) / 10.0f);
        delay(50);
    }
    for (auto i = 0; i <= 20; ++i)
    {
        Cover_text(static_cast<float>(i) / 20.0f);
        delay(50);
    }

    // fade-out
    delay(4000);
    for (auto i = 50; i >= 0; --i)
    {
        Cover_text(i / 50.0f);
        delay(30);
    }
    for (auto i = 20; i >= 0; --i)
    {
        Logo(static_cast<float>(i) / 20.0f);
        delay(30);
    }

    // all black
    tft.fillScreen(ILI9341_BLACK);
    delay(200);
}

void DisplayManager::Lilla_cover_saturate(void)
{
    for (auto i = 0; i <= 20; ++i)
    {
        tft.fillScreen(Calc_color(ILI9341_WHITE, static_cast<float>(i) / 20.0f));
        Cover_text(1);
        Logo(1);
        delay(50);
    }

    // all black
    tft.fillScreen(ILI9341_BLACK);
    delay(200);
}

FLASHMEM
void DisplayManager::P_show_Patch_number(bool change_patch)
{
    tft.setCursor(display_coordinate_x(P_column_PATCH), display_coordinate_y(0));
    tft.setTextColor(TEXT_COLOR);
    tft.print("PATCH");
    tft.setCursor(display_coordinate_x(P_column_Patch_id), display_coordinate_y(0));
    tft.setTextColor(change_patch ? ILI9341_YELLOW : ILI9341_WHITE);
    tft.print(Patch_id);
}

FLASHMEM
void DisplayManager::P_Patch_VOLUME(bool change_vol)
{
    tft.setCursor(display_coordinate_x(P_column_VOLUME), display_coordinate_y(0));
    tft.setTextColor(TEXT_COLOR);
    tft.print("VOLUME");
    P_Patch_volume_value(change_vol);
}

FLASHMEM
void DisplayManager::P_Patch_volume_value(bool change_vol)
{
    Cancel_text_reset_cursor(display_coordinate_x(P_column_Volume_value), display_coordinate_y(0), 4);
    tft.setTextColor(ILI9341_YELLOW); // tft.setTextColor(change_vol ? ILI9341_YELLOW : ILI9341_WHITE);
    tft.print(volume_patch / 20.0f, 2);
}

FLASHMEM
void DisplayManager::Loop_REC_advice(int track, bool on)
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

void DisplayManager::P_show_delete_Instrument_frame(float line, bool show)
{
    Frame_by_pixels(display_coordinate_x(P_column_Instrument_frame), P_Instrument_pixels_y(line) - 4, P_chars_width_Instrument_frame, show);
}

void DisplayManager::Led_PERFORMANCE_instrument(int instrument_id, bool on)
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

void DisplayManager::Led_SOUND_EDIT_instrument(int instrument_id, bool on)
{
    if (on)
    {
        tft.drawBitmap(display_coordinate_x(22) - 4, display_coordinate_y(0), led_pic, 6, 8, (MX_mute[instrument_id] ? RED_ON : GREEN_ON));
    }
    else
    {
        tft.drawBitmap(display_coordinate_x(22) - 4, display_coordinate_y(0), led_pic, 8, 8, (MX_mute[instrument_id] ? RED_OFF : GREEN_OFF));
    }
    return;
}

void DisplayManager::Led_INSTRUMENT_VCF_instrument(int instrument_id, bool on)
{
    if (Lilla_state_0 == PERFORMANCE)
    {
        if (on)
        {
            tft.drawBitmap(display_coordinate_x(22) - 4, display_coordinate_y(0), led_pic, 6, 8, (MX_mute[instrument_id] ? RED_ON : GREEN_ON));
        }
        else
        {
            tft.drawBitmap(display_coordinate_x(22) - 4, display_coordinate_y(0), led_pic, 8, 8, (MX_mute[instrument_id] ? RED_OFF : GREEN_OFF));
        }
    }
    else if (Lilla_state_0 == LIVE_SAMPLING)
    {
        if (on)
        {
            tft.drawBitmap(display_coordinate_x(40) - 4, display_coordinate_y(0), led_pic, 6, 8, (MX_mute[instrument_id] ? RED_ON : GREEN_ON));
        }
        else
        {
            tft.drawBitmap(display_coordinate_x(40) - 4, display_coordinate_y(0), led_pic, 8, 8, (MX_mute[instrument_id] ? RED_OFF : GREEN_OFF));
        }
    }
}

void DisplayManager::Led_DIRECT_SAMPLING(bool on)
{
    // PATCHES_MAX ha 2 instrument
    if (on)
    {
        tft.drawBitmap(display_coordinate_x(0), display_coordinate_y(8), led_pic, 6, 8, ((MX_mute[0] && MX_mute[1]) ? RED_ON : GREEN_ON));
    }
    else
    {
        tft.drawBitmap(display_coordinate_x(0), display_coordinate_y(8), led_pic, 8, 8, ((MX_mute[0] && MX_mute[1]) ? RED_OFF : GREEN_OFF));
    }
    return;
}

void DisplayManager::Led_tuning_tone(int patch_id)
{
    if (Lilla_state == PERFORMANCE)
    {
        tft.drawBitmap(P_pixel_x_LED, P_Instrument_pixels_y(Patch[patch_id].instruments), led_pic, 8, 8, (TT_playing ? ILI9341_RED : RED_OFF));
    }
}

FLASHMEM
void DisplayManager::P_show_pointer_frame(P_field_description_struct value, bool show)
{
    switch (value.field_name)
    {
    case field_P_Menu:
        Frame_by_col_row(P_column_menu_element[value.element], P_row_menu_element[value.element], P_dimension_voice_menu[P_element_menu[value.element]], show);
        break;

    case field_P_Patch:
        Frame_by_col_row(P_column_Patch_id, 0, 3, show);
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
void DisplayManager::P_show_PERFORMANCE_page(bool change_patch, bool change_vol)
{
    P_Patch_header(change_patch, change_vol);
    P_show_Performance_menu(); // displays the menu and updates P_menu_max
    P_show_Instruments_header();
    P_show_all_instruments(Patch_id);
}

FLASHMEM
void DisplayManager::P_show_PERFORMANCE_title(void)
{
    Backgorund_red(0, 0, 11); // Display.Backgorund_red(float col, float row, int chars)
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(0));
    tft.setTextColor(ILI9341_WHITE);
    tft.print("PERFORMANCE");
}

FLASHMEM
void DisplayManager::ALL_show_effects()
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
void DisplayManager::Resolution(void)
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
void DisplayManager::Downsampling(void)
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
void DisplayManager::Lowpass_filter(void)
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
void DisplayManager::Loop_show_Loop_page(void)
{
    tft.fillScreen(ILI9341_BLACK);
    Loop_show_midi_loop_title(); // title
    Loop_menu();
    P_show_Patch_number(Patch_id);
    P_Patch_VOLUME(true);
    ALL_show_effects();

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

    // show LOOP_time
    Loop_time_stretched();
}

FLASHMEM
void DisplayManager::Loop_loop_id(void)
{
    Cancel_text(display_coordinate_x(22), display_coordinate_y(0), 5); // Cancel_text(int X, int Y, int N)
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
void DisplayManager::Loop_menu(void)
{
    uint8_t position = 0; // position on display

    Cancel_text(display_coordinate_x(0), display_coordinate_y(1), 27); // Cancel_text(int X, int Y, int N)
    tft.setTextColor(MENU_COLOR);

    for (auto element = 0; element < 4; ++element) // menu element
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
void DisplayManager::Loop_show_frame_menu(int position, bool fresh) // after Loop_menu() use fresh = true
{
    if (!fresh && Loop_menu_position_0 >= 0)
    {
        Frame_by_col_row(Loop_X_position_menu_0, 1, Loop_dimension_voice_menu_0, false); // Frame_by_col_row(X_position_Menu_Loop[position_0], 1, dimension_voice_Menu_Loop[element_Menu_Loop[position_0]], false);
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

void DisplayManager::Loop_Delete_all_frame_menu(void)
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
void DisplayManager::Loop_show_midi_loop_title(void)
{
    Backgorund_red(0, 0, 9); // Backgorund_red(float col, float row, int chars)
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(0));
    tft.setTextColor(ILI9341_WHITE);
    tft.print("MIDI LOOP");
}

FLASHMEM
void DisplayManager::Loop_track_data(int track)
{
    // Numero
    Cancel_text_reset_cursor(display_coordinate_x(Loop_LOOPS_X + 7 * track), display_coordinate_y(Loop_HEAD_R), 5);
    if (LOOP_events[track] > 0)
    {
        tft.setTextColor(ILI9341_WHITE);
        tft.print(track + 1); // Comincia da 1
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
void DisplayManager::Loop_time_stretched(void)
{
    // LOOP_time
    Cancel_text_reset_cursor(display_coordinate_x(Loop_LOOP_TIME), display_coordinate_y(Loop_HEAD_R - 1), 7);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print((float)(LOOP_time * LOOP_stretch) / 1000.0);
    tft.setTextColor(ILI9341_ORANGE);
    tft.print("s");
}

void DisplayManager::Logo(float light)
{
    tft.drawBitmap(0 + Logo_position_DX, 42 + Logo_position_DY, LOGO_0, 168, 49, Calc_color(ILI9341_YELLOW, light));
    tft.drawBitmap(0 + Logo_position_DX, 14 + Logo_position_DY, LOGO_1, 8, 26, Calc_color(ILI9341_RED, light));
    tft.drawBitmap(32 + Logo_position_DX, 9 + Logo_position_DY, LOGO_2, 16, 31, Calc_color(ILI9341_RED, light));
    tft.drawBitmap(48 + Logo_position_DX, 20 + Logo_position_DY, LOGO_3, 16, 20, Calc_color(ILI9341_MAGENTA, light));
    tft.drawBitmap(88 + Logo_position_DX, 0 + Logo_position_DY, LOGO_4, 8, 40, Calc_color(ILI9341_MAGENTA, light));
    tft.drawBitmap(-1 + Logo_position_DX, 100 + Logo_position_DY, audio_sampler, 168, 18, Calc_color(ILI9341_WHITE, light));
}

void DisplayManager::Cover_text(float light)
{
    tft.setTextColor(Calc_color(TEXT_COLOR, light)); // 16-bit ('565') color settings
    tft.setCursor(display_coordinate_x(13), display_coordinate_y(12));
    tft.print("UPDATE ");
    tft.setTextColor(Calc_color(ILI9341_WHITE, light)); // 16-bit ('565') color settings
    tft.print(FIRMWARE_VERSION);

    tft.setTextColor(Calc_color(TEXT_COLOR, light)); // 16-bit ('565') color settings
    tft.setCursor(display_coordinate_x(13), display_coordinate_y(13) - 4);
    tft.print("AUDIO MEMORY ");
    tft.setTextColor(Calc_color(ILI9341_WHITE, light)); // 16-bit ('565') color settings
    tft.print(flash_dimension_MB);
    tft.print("MB");

    tft.setTextColor(Calc_color(TEXT_COLOR, light)); // 16-bit ('565') color settings
    tft.setCursor(display_coordinate_x(13), display_coordinate_y(14) - 8);
    tft.print("LIVE SAMPLER MEMORY ");
    tft.setTextColor(Calc_color(ILI9341_WHITE, light)); // 16-bit ('565') color settings
    tft.print("16MB");
}

uint16_t DisplayManager::Calc_color(uint16_t color_peak, float light) // 16-bit ('565') color settings
{
    // extracts components of peak (regime) value
    uint16_t red = color_peak >> 11;
    uint16_t green = (color_peak & 0b11111100000) >> 5;
    uint16_t blue = color_peak & 0b11111;

    // modulate each components
    red = static_cast<float>(red) * light;
    green = static_cast<float>(green) * light;
    blue = static_cast<float>(blue) * light;

    uint16_t value = (red << 11) + (green << 5) + blue;
    return value;
}

int DisplayManager::P_Instrument_pixels_y(int position)
{
    return 4 + 15.0 * (6 + position) + 7;
}

FLASHMEM
void DisplayManager::DS_confirm_EXIT_from_DS(void)
{
    L_POPUP = 106;
    H_POPUP = 47;
    X_POPUP = (320 - L_POPUP) / 2;
    Y_POPUP = (240 - H_POPUP) / 2;
    Y_POPUP_TXT = 10;
    Y_POPUP_OPT = 30;

    tft.fillRoundRect(X_POPUP, Y_POPUP, L_POPUP, H_POPUP, 4, ILI9341_RED); // does NOT delete frame
    tft.setCursor(X_POPUP + display_coordinate_x(1.5), Y_POPUP + Y_POPUP_TXT);
    tft.setTextColor(ILI9341_WHITE);
    tft.print("STOP SAMPLING?");
    tft.setTextColor(ILI9341_YELLOW);
    tft.setCursor(X_POPUP + display_coordinate_x(5.5), Y_POPUP + Y_POPUP_OPT);
    tft.print("NO");
    tft.setCursor(X_POPUP + display_coordinate_x(9.5), Y_POPUP + Y_POPUP_OPT);
    tft.print("YES");
}

FLASHMEM
void DisplayManager::DS_page(int recording)
{
    tft.fillScreen(ILI9341_BLACK);

    Backgorund_red(0, 0, 7); // Display.Backgorund_red(float   col, float row, int chars)
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(0));
    tft.print("SAMPLER");

    tft.setCursor(display_coordinate_x(41), display_coordinate_y(0));
    tft.setTextColor(TEXT_COLOR);
    tft.print("VOLUME");
    DS_update_volume();

    tft.setCursor(display_coordinate_x(0), display_coordinate_y(4));
    tft.setTextColor(TEXT_COLOR);
    tft.print("AUDIO MEMORY ");
    tft.setTextColor(ILI9341_WHITE);
    tft.print(flash_dimension_MB);
    Show_measure_unit("MB", 2);
    tft.setTextColor(ILI9341_WHITE);
    tft.print(" (");
    tft.print(Get_flash_size() / 88100.0f);
    Show_measure_unit("sec", 3);
    tft.setTextColor(ILI9341_WHITE);
    tft.print(")");

    tft.setCursor(display_coordinate_x(0), display_coordinate_y(5));
    tft.setTextColor(TEXT_COLOR);
    tft.print("- FREE FOR RECORDINGS");
    DS_available_memory();

    tft.setCursor(display_coordinate_x(0), display_coordinate_y(6));
    tft.setTextColor(TEXT_COLOR);
    tft.print("- FREE FOR RAW FILES ");
    DS_raw_available_memory();

    DS_Recording_description(recording, true);
    DS_sampler_IO();
}

FLASHMEM
void DisplayManager::DS_sampler_IO(void)
{
    tft.setCursor(DS_VUMETER_BAR_X + 1, DS_VUMETER_BAR_Y + 4);
    tft.setTextColor(TEXT_COLOR);
    tft.print("L");
    tft.setCursor(DS_VUMETER_BAR_X + DS_VUMETER_BAR_DX, DS_VUMETER_BAR_Y + 4);
    tft.print("R");

    // tft.drawRect(DS_VUMETER_BAR_X - 8, DS_VUMETER_BAR_Y - BAR_ELEMENTS - 6, 36, BAR_ELEMENTS + 36, ILI9341_GREEN);
    tft.drawRect(DS_VUMETER_BAR_X - 1, DS_VUMETER_BAR_Y - BAR_ELEMENTS - 1, DS_VUMETER_BAR_DISTANCE + 2, BAR_ELEMENTS + 2, 0x03E0);
    tft.drawRect(DS_VUMETER_BAR_X - 1 + DS_VUMETER_BAR_DX, DS_VUMETER_BAR_Y - BAR_ELEMENTS - 1, DS_VUMETER_BAR_DISTANCE + 2, BAR_ELEMENTS + 2, 0x03E0);
    DS_bar(0, 0); // 0 <= value <= BAR_ELEMENTS
    DS_bar(1, 0); // 0 <= value <= BAR_ELEMENTS

    // Schema IN OUT
    tft.drawRect(DS_START_X, DS_START_Y, 31, 15, ILI9341_GREEN);
    tft.drawBitmap(DS_START_X - 19, DS_START_Y + 4, DS_freccia, 19, 7, ILI9341_GREEN);
    tft.setCursor(DS_START_X - 63, DS_START_Y + 4);
    tft.setTextColor(TEXT_COLOR);
    tft.print("LINE-IN");

    tft.setCursor(DS_START_X + 4, DS_START_Y + 17);
    tft.setTextColor(TEXT_COLOR);
    tft.print("GAIN");

    // Sampler
    DS_sampler_frame(true);
    DS_sampler_txt(false);
    DS_show_gain();
}

void DisplayManager::DS_bar(int channel, int value) // 0 <= value <= BAR_ELEMENTS
{
    int X0 = (channel == 0 ? DS_VUMETER_BAR_X : DS_VUMETER_BAR_X + DS_VUMETER_BAR_DX);
    float value_float;
    const float BAR_ELEMENTS_float = BAR_ELEMENTS;

    if (value < 0)
    {
        value = 0;
    }

    if (value > DS_VU_meter_value_old[channel])
    {
        for (auto i = DS_VU_meter_value_old[channel] + 1; i <= value; ++i)
        {
            value_float = i / BAR_ELEMENTS_float;
            // builds bricks
            tft.drawFastHLine(X0, DS_VUMETER_BAR_Y - i, DS_VUMETER_BAR_DISTANCE, DS_calc_bar_color(value_float));
        }
    }
    else if (value < DS_VU_meter_value_old[channel])
    {
        for (auto i = value + 1; i <= DS_VU_meter_value_old[channel]; ++i)
        {
            // remove bricks
            tft.drawFastHLine(X0, DS_VUMETER_BAR_Y - i, DS_VUMETER_BAR_DISTANCE, ILI9341_BLACK);
        }
    }
    DS_VU_meter_value_old[channel] = value;
}

uint16_t DisplayManager::DS_calc_bar_color(float value) // 16-bit ('565') color settings
{
    const float soglia = 0.5;
    uint16_t red = (value >= soglia ? 31 : 31.0f * (value / soglia));                   // (value >= 0.5f ? 31: 31.0f * 2.0f * value)
    uint16_t green = (value <= soglia ? 63 : 63.0f * (1.2f - value) / (1.2f - soglia)); // (value <= 0.5f ? 63: 63.0f * 2.0f * (1.0f - value))
    uint16_t blue = 0;                                                                  // 33 * 0

    if (false)
    {
        Serial.print("value ");
        Serial.print(value);
        Serial.print("  red ");
        Serial.print(red);
        Serial.print("  green ");
        Serial.print(green);
        Serial.print("  hex ");
        Serial.println((red << 11) + (green << 5) + blue, HEX);
        Serial.println();
    }

    return (red << 11) + (green << 5) + blue;
}

FLASHMEM
void DisplayManager::DS_line_out(bool visible)
{
    tft.drawBitmap(DS_START_X + 37, DS_START_Y + 7, DS_freccia_gomito, 13, 23, (visible ? ILI9341_GREEN : GREEN_OFF));
    tft.setCursor(DS_START_X + 55, DS_START_Y + 23);
    tft.setTextColor((visible ? TEXT_COLOR : TEXT_OFF_COLOR));
    tft.print("LINE-OUT");
}

FLASHMEM
void DisplayManager::DS_sampler_frame(bool visible)
{
    tft.drawBitmap(DS_START_X + 31, DS_START_Y + 4, DS_freccia, 19, 7, (visible ? ILI9341_GREEN : ILI9341_BLACK));
    tft.drawRoundRect(DS_START_X + 50, DS_START_Y, 51, 15, 3, (visible ? ILI9341_GREEN : ILI9341_BLACK));
}

FLASHMEM
void DisplayManager::DS_sampler_txt(bool color)
{
    tft.setCursor(DS_START_X + 58, DS_START_Y + 4);
    tft.setTextColor(color ? ILI9341_RED : RED_OFF);
    tft.print("RECORD");
}

FLASHMEM
void DisplayManager::DS_available_memory(void)
{
    col = 22;
    row = 5;
    Cancel_text_reset_cursor(display_coordinate_x(col), display_coordinate_y(row), 10);
    tft.setTextColor(ILI9341_WHITE);
    tft.print(VFS_Get_packets_free() * 0.743, 1);
    Show_measure_unit("sec", 3);
}

FLASHMEM
void DisplayManager::DS_raw_available_memory()
{
    col = 21;
    row = 6;
    Cancel_text_reset_cursor(display_coordinate_x(col), display_coordinate_y(row), 6);
    tft.setTextColor(ILI9341_WHITE);
    tft.print((Get_flash_size() - Get_flash_occupation() - FLASH_FREE_SPACE) / 88200.0f);
    Show_measure_unit("sec", 3);
}

FLASHMEM
void DisplayManager::DS_hide_recording()
{
    Cancel_text(display_coordinate_x(0), display_coordinate_y(8), 18);
    Cancel_text(display_coordinate_x(0), display_coordinate_y(9), 29);
    Cancel_text(display_coordinate_x(0), display_coordinate_y(10), 18);
    Cancel_text(display_coordinate_x(0), display_coordinate_y(11), 18);
    Cancel_text(display_coordinate_x(0), display_coordinate_y(12), 18);
}

FLASHMEM
void DisplayManager::DS_advice_delete(bool value)
{
    tft.setTextColor((value ? ILI9341_YELLOW : ILI9341_BLACK));
    tft.setCursor(display_coordinate_x(3.5), display_coordinate_y(8));
    tft.print("PLEASE WAIT");
}

FLASHMEM
void DisplayManager::DS_advice_no_conversion(int DS_export, bool value)
{
    tft.setTextColor((value ? TEXT_COLOR : ILI9341_BLACK));
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(8));
    tft.print("UNABLE TO CREATE RAW FILE");
    tft.setTextColor((value ? ILI9341_YELLOW : ILI9341_BLACK));
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(9));

    if (DS_export == 0)
    {
        tft.print("AUDIO MEMORY INSUFFICIENT");
    }
    else
    {
        tft.print(".RAW NAMESPACE IS FULL");
    }
}

FLASHMEM
void DisplayManager::DS_conversion_options(int file_L_RAW, int file_R_RAW, int DS_export)
{
    tft.fillRect(0, 120, 320, 120, ILI9341_BLACK); // cancella Recording e grafica
    tft.setTextColor(TEXT_COLOR);
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(8));
    tft.print("OPTIONS:");

    tft.setCursor(display_coordinate_x(0), display_coordinate_y(9));
    if (!Recording[recording].stereo)
    {
        tft.print("- MONO ");
    }
    else
    {
        tft.print("- LEFT ");
    }

    tft.setTextColor(ILI9341_WHITE);
    tft.print(name_file[recording + FIRST_RECORDING_FILE]);
    tft.print(" --> ");
    tft.print(name_file[file_L_RAW]);
    tft.print(" (");
    tft.print(Recording[recording].bytes >> 10);
    tft.print("kB)");

    if (Recording[recording].stereo && DS_export == 2)
    {
        tft.setTextColor(TEXT_COLOR);
        tft.setCursor(display_coordinate_x(0), display_coordinate_y(10));
        tft.print("- RIGHT ");

        tft.setTextColor(ILI9341_WHITE);
        tft.print(name_file[recording + FIRST_RECORDING_FILE + 1]);
        tft.print(" --> ");
        tft.print(name_file[file_R_RAW]);
        tft.print(" (");
        tft.print(Recording[recording].bytes >> 10);
        tft.print("kB)");
    }
}

FLASHMEM
void DisplayManager::DS_export_options(int file_L_RAW, int file_R_RAW, int DS_export)
{

    tft.fillRect(0, 120, 320, 120, ILI9341_BLACK); // cancella Recording e grafica
    tft.setTextColor(TEXT_COLOR);
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(8));
    tft.print("SD EXPORT OPTIONS:");

    tft.setCursor(display_coordinate_x(0), display_coordinate_y(9));

    if (!Recording[recording].stereo)
    {
        tft.print("- MONO ");
    }
    else
    {
        tft.print("- LEFT ");
    }

    tft.setTextColor(ILI9341_WHITE);
    tft.print(name_file[recording + FIRST_RECORDING_FILE]);
    tft.print(" --> ");
    // DISPLAY_DS_export_filename(0);
    tft.print(" (");
    tft.print(Recording[recording].bytes >> 10);
    tft.print("kB)");

    if (Recording[recording].stereo && DS_export == 2)
    {
        tft.setTextColor(TEXT_COLOR);
        tft.setCursor(display_coordinate_x(0), display_coordinate_y(10));
        tft.print("- RIGHT ");

        tft.setTextColor(ILI9341_WHITE);
        tft.print(name_file[recording + FIRST_RECORDING_FILE + 1]);
        tft.print(" --> ");
        // DISPLAY_DS_export_filename(1);
        tft.print(" (");
        tft.print(Recording[recording].bytes >> 10);
        tft.print("kB)");
    }
}

FLASHMEM
void DisplayManager::DS_Recording_description(int recording, bool led)
{
    // RECORDING
    tft.setCursor((led ? display_coordinate_x(1.5) : display_coordinate_x(0)), display_coordinate_y(8));
    tft.setTextColor(TEXT_COLOR);
    tft.print("RECORDING ");

    if (recording >= 0)
    {
        tft.setTextColor(ILI9341_YELLOW);
        tft.print(recording);
    }
    else
    {
        tft.setTextColor(ILI9341_WHITE);
        tft.print("NONE");
    }

    // FILE LEFT o MONO
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(9));
    tft.setTextColor(TEXT_COLOR);

    if (recording < 0)
    {
        tft.print("TEST_FILE ");
        tft.setTextColor(ILI9341_WHITE);
        tft.print("0.RAW");
    }
    else
    {
        if (Recording[recording].stereo)
        {
            tft.print("LEFT_FILE ");
        }
        else
        {
            tft.print("MONO_FILE ");
        }
        tft.setTextColor(ILI9341_WHITE);
        tft.print(name_file[2 * recording + FIRST_RECORDING_FILE]);
    }

    // FILE RIGHT
    if (recording >= 0 && Recording[recording].stereo)
    {
        tft.setCursor(display_coordinate_x(0), display_coordinate_y(10));
        tft.setTextColor(TEXT_COLOR);
        tft.print("RIGHT_FILE ");
        tft.setTextColor(ILI9341_WHITE);
        tft.print(name_file[2 * recording + FIRST_RECORDING_FILE + 1]);
    }
    DS_recording_seconds();
    if (led)
    {
        DS_update_volume();
    }
}

FLASHMEM
void DisplayManager::DS_recording_seconds(void)
{
    if (recording < 0)
    {
        return;
    }

    row = 11;
    if (!Recording[recording].stereo)
    {
        row -= 1;
    }

    tft.setCursor(display_coordinate_x(0), display_coordinate_y(row));
    tft.setTextColor(TEXT_COLOR);
    tft.print("LENGTH ");

    tft.setTextColor(ILI9341_WHITE);
    tft.print(Recording[recording].seconds, 1);
    Show_measure_unit("sec", 3);
}

FLASHMEM
void DisplayManager::DS_update_recording_seconds(float value)
{
    row = 11;
    if (Recording[recording].stereo)
    {
        Cancel_text_reset_cursor(display_coordinate_x(7), display_coordinate_y(row), 7);
    }
    else
    {
        Cancel_text_reset_cursor(display_coordinate_x(7), display_coordinate_y(row - 1), 7);
    }

    tft.setTextColor(ILI9341_WHITE);
    tft.print(value / 1000.0f, 1);
    Show_measure_unit("sec", 3);
}

FLASHMEM
void DisplayManager::DS_volume(void)
{
    row = 12;
    if (!Recording[recording].stereo)
    {
        row = 11;
    }
    if (recording < 0)
    {
        row = 10;
    }
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(row));
    tft.setTextColor(TEXT_COLOR);
    tft.print("VOLUME ");

    tft.setTextColor(ILI9341_YELLOW);
    tft.print(volume_patch / 20.0f, 2);
}

FLASHMEM
void DisplayManager::DS_update_volume(bool adj)
{
    Cancel_text_reset_cursor(display_coordinate_x(48), display_coordinate_y(0), 4);
    tft.setTextColor(adj ? ILI9341_YELLOW : ILI9341_WHITE);
    tft.print(volume_patch / 20.0f, 2);
}

FLASHMEM
void DisplayManager::DS_show_gain(void)
{
    Cancel_text_reset_cursor(DS_START_X + 4, DS_START_Y + 4, 4);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(DS_gain / 20.0f, 2);
}

FLASHMEM
void DisplayManager::DS_menu(void)
{
    auto position = 0; // position on display

    // DS_define_model(); DISTRIBUITO
    Delete_text_row(1);
    Delete_text_row(2);
    tft.setTextColor(MENU_COLOR);

    for (auto element = 0; element < DS_MV; ++element) // menu element
    {
        if (Menu_DS[element])
        {
            if (position == 0)
            {
                X_position_Menu_DS[position] = 0;
            }
            else
            {
                X_position_Menu_DS[position] = X_position_Menu_DS[position - 1] + dimension_voice_Menu_DS[element_Menu_DS[position - 1]] + 1;
            }

            Y_position_Menu_DS[position] = 1;

            element_Menu_DS[position] = element;
            position_Menu_DS[element] = position;
            tft.setCursor(display_coordinate_x(X_position_Menu_DS[position]), display_coordinate_y(Y_position_Menu_DS[position]));
            tft.print(Menu_DS_char[element]);
            ++position;
        }
    }
}

FLASHMEM
void DisplayManager::DS_frame_menu(int position)
{
    Frame_by_col_row(X_position_Menu_DS[DS_frame_menu_position_0], Y_position_Menu_DS[DS_frame_menu_position_0], dimension_voice_Menu_DS[element_Menu_DS[DS_frame_menu_position_0]], false);
    Frame_by_col_row(X_position_Menu_DS[position], Y_position_Menu_DS[position], dimension_voice_Menu_DS[element_Menu_DS[position]], true);
    choice_DS_menu = element_Menu_DS[position];
    DS_frame_menu_position_0 = position;
}

FLASHMEM
void DisplayManager::VFS_show_packets(void)
{
    Cancel_text_reset_cursor(display_coordinate_x(21), display_coordinate_y(7), 40);
    Frame_by_col_row(21, 7, 21, true);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print((VFS_packets * PACKET_DIM) / 1048576.0f, 2);
    Show_measure_unit("MB", 2);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(" (");
    tft.print(VFS_packets * 0.743, 0);
    Show_measure_unit("sec", 3);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(" MONO)");

    Cancel_text_reset_cursor(display_coordinate_x(27), display_coordinate_y(8), 40);
    tft.setTextColor(ILI9341_WHITE);
    tft.print(((VFS_packets_max - VFS_packets) * PACKET_DIM) / 1048576.0f, 2);
    Show_measure_unit("MB", 2);
    tft.setTextColor(ILI9341_WHITE);
    tft.print(" (");
    tft.print((VFS_packets_max - VFS_packets) * 0.743, 0);
    Show_measure_unit("sec", 3);
    tft.setTextColor(ILI9341_WHITE);
    tft.print(")");
}

FLASHMEM
void DisplayManager::Midi_monitor_page(void)
{
    tft.fillScreen(ILI9341_BLACK);

    Backgorund_red(0, 0, 12); // Display.Board(float col, float row, int chars)
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(0));
    tft.setTextColor(ILI9341_WHITE);
    tft.print("MIDI MONITOR");

    tft.setTextColor(TEXT_COLOR);
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(3));
    tft.print("MIDI CHANNEL");
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(4));
    tft.print("MESSAGE");
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(5));
    tft.print("NOTE-NUMBER");
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(6));
    tft.print("VELOCITY");
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(7));
    tft.print("VALUE");
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(8));
    tft.print("NUMBER");
}

FLASHMEM
void DisplayManager::Midi_monitor_frame(void)
{
    Frame_by_col_row(0, 1, 6, true);
}

FLASHMEM
void DisplayManager::Midi_monitor_data(uint8_t incoming_midi_channel, uint8_t incoming_midi_message, int8_t incoming_note_number, int8_t incoming_velocity, int32_t incoming_midi_value, int8_t incoming_number)
{
    const char message_name[12][20] = {{"NoteOn"}, {"NoteOff"}, {"PitchBend"}, {"AfterTouchPoly"}, {"ControlChange"}, {"ProgramChange"}, {"AfterTouchChange"}, {"SystemExclusive"}, {"Unknown"}};

    tft.setTextColor(ILI9341_YELLOW);
    Cancel_text_reset_cursor(display_coordinate_x(13), display_coordinate_y(3), 2);
    tft.print(incoming_midi_channel + 1);

    Cancel_text_reset_cursor(display_coordinate_x(8), display_coordinate_y(4), 16);
    tft.print(message_name[incoming_midi_message]);

    Cancel_text(display_coordinate_x(12), display_coordinate_y(5), 9);
    if (incoming_note_number >= 0)
    {
        tft.setCursor(display_coordinate_x(12), display_coordinate_y(5));
        tft.print(note_name[incoming_note_number % 12]);
        tft.print((int)(incoming_note_number / 12) + first_octave);
    }

    Cancel_text(display_coordinate_x(9), display_coordinate_y(6), 9);
    if (incoming_velocity >= 0)
    {
        tft.setCursor(display_coordinate_x(9), display_coordinate_y(6));
        tft.print(incoming_velocity);
    }

    Cancel_text(display_coordinate_x(6), display_coordinate_y(7), 9);
    if (incoming_midi_value >= 0)
    {
        tft.setCursor(display_coordinate_x(6), display_coordinate_y(7));
        tft.print(incoming_midi_value);
    }

    Cancel_text(display_coordinate_x(7), display_coordinate_y(8), 9);
    if (incoming_number >= 0)
    {
        tft.setCursor(display_coordinate_x(7), display_coordinate_y(8));
        tft.print(incoming_number);
    }
}

FLASHMEM
void DisplayManager::P_Patch_header(bool change_patch, bool change_vol)
{
    tft.fillScreen(ILI9341_BLACK);
    P_show_PERFORMANCE_title();
    P_show_Patch_number(change_patch);
    P_Patch_VOLUME(change_vol);
    ALL_show_effects();
}

FLASHMEM
void DisplayManager::Patch_volume_color(bool change_patch, bool change_vol)
{
    P_show_Patch_number(change_patch);
    P_Patch_VOLUME(change_vol);
    P_Patch_volume_value(change_vol);
}

FLASHMEM
void DisplayManager::P_show_Instruments_header(void)
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
void DisplayManager::P_show_Instrument_description(int patch_id, int instrument_id, bool editing)
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
void DisplayManager::P_show_Sound_number(int instrument_id, bool editing)
{
    auto position = P_line_of_instrument[instrument_id];
    auto y_display = P_Instrument_pixels_y(position);

    auto x_display = display_coordinate_x(P_column_Sound);
    tft.setTextColor((editing ? ILI9341_YELLOW : ILI9341_WHITE));

    Cancel_text_reset_cursor(x_display, y_display, 1);
    tft.print(instrument_id + 1);
}

FLASHMEM
void DisplayManager::P_show_Lock_value(int patch_id, int instrument_id, bool editing)
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
void DisplayManager::P_show_Precedence_value(int patch_id, int instrument_id, bool editing)
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
void DisplayManager::P_show_Midi_value(int patch_id, int instrument_id, bool editing)
{
    auto position = P_line_of_instrument[instrument_id];
    auto y_display = P_Instrument_pixels_y(position);

    auto x_display = display_coordinate_x(P_column_instrument_element[2]);
    tft.setTextColor((editing ? ILI9341_YELLOW : ILI9341_WHITE));

    Cancel_text_reset_cursor(x_display, y_display, 2);
    tft.print(Get_midi_channel(patch_id, instrument_id) + 1);
}

FLASHMEM
void DisplayManager::P_show_RootKey_value(int patch_id, int instrument_id, bool editing)
{
    auto position = P_line_of_instrument[instrument_id];
    auto y_display = P_Instrument_pixels_y(position);

    auto x_display = display_coordinate_x(P_column_instrument_element[3]);
    tft.setTextColor((editing ? ILI9341_YELLOW : ILI9341_WHITE));

    Cancel_text_reset_cursor(x_display, y_display, 4);
    Note(Patch[patch_id].Instrument[instrument_id].root_key);
}

FLASHMEM
void DisplayManager::P_show_FromKey_value(int patch_id, int instrument_id, bool editing)
{
    auto position = P_line_of_instrument[instrument_id];
    auto y_display = P_Instrument_pixels_y(position);

    auto x_display = display_coordinate_x(P_column_instrument_element[4]);
    tft.setTextColor((editing ? ILI9341_YELLOW : ILI9341_WHITE));

    Cancel_text_reset_cursor(x_display, y_display, 4);
    Note(Patch[patch_id].Instrument[instrument_id].from_note);
}

FLASHMEM
void DisplayManager::P_show_ToKey_value(int patch_id, int instrument_id, bool editing)
{
    auto position = P_line_of_instrument[instrument_id];
    auto y_display = P_Instrument_pixels_y(position);

    auto x_display = display_coordinate_x(P_column_instrument_element[5]);
    tft.setTextColor((editing ? ILI9341_YELLOW : ILI9341_WHITE));

    Cancel_text_reset_cursor(x_display, y_display, 4);
    Note(Patch[patch_id].Instrument[instrument_id].to_note);
}

FLASHMEM
void DisplayManager::P_show_Pan_value(int patch_id, int instrument_id, bool editing)
{
    auto position = P_line_of_instrument[instrument_id];
    auto y_display = P_Instrument_pixels_y(position);

    auto x_display = display_coordinate_x(P_column_instrument_element[6]);
    tft.setTextColor((editing ? ILI9341_YELLOW : ILI9341_WHITE));

    Cancel_text_reset_cursor(x_display, y_display, 2);
    if (Sound[Patch[patch_id].Instrument[instrument_id].sound_id].pan < 0)
    {
        tft.print("L");
    }
    else if (Sound[Patch[patch_id].Instrument[instrument_id].sound_id].pan > 0)
    {
        tft.print("R");
    }
    tft.print(abs(Sound[Patch[patch_id].Instrument[instrument_id].sound_id].pan));
}

FLASHMEM
void DisplayManager::P_show_Gain_value(int patch_id, int instrument_id, bool editing)
{
    auto position = P_line_of_instrument[instrument_id];
    auto y_display = P_Instrument_pixels_y(position);

    auto x_display = display_coordinate_x(P_column_instrument_element[7]);
    tft.setTextColor((editing ? ILI9341_YELLOW : ILI9341_WHITE));

    Cancel_text_reset_cursor(x_display, y_display, 4);
    tft.print(Sound[Patch[patch_id].Instrument[instrument_id].sound_id].gain / 20.0f, 2);
}

FLASHMEM
void DisplayManager::P_show_all_instruments(int patch_id)
{
    tft.fillRect(0, P_Instrument_pixels_y(0) - 4, 320, 240, ILI9341_BLACK);

    for (auto instrument_id = 0; instrument_id < INSTRUMENTS_MAX; ++instrument_id)
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
void DisplayManager::P_show_TuningTone_instrument(int patch_id)
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
void DisplayManager::P_show_gain_TuningTone(int patch_id)
{
    auto position = Patch[patch_id].instruments;

    Cancel_text_reset_cursor(display_coordinate_x(TT_Instrument_INDENT_X0 + 8 * TT_Instrument_SPACE_X + 35), P_Instrument_pixels_y(position), 4);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(tuning_tone_volume / 20.0f, 2);
}

FLASHMEM
void DisplayManager::P_delete_instrument_by_position(int position)
{
    tft.fillRect(5, P_Instrument_pixels_y(position), 307, 7, ILI9341_BLACK);
}

FLASHMEM
void DisplayManager::Note(int note_number)
{
    tft.print(note_name[note_number % 12]);
    tft.print((int)(note_number / 12.0f) + first_octave);
}

FLASHMEM
void DisplayManager::P_show_Performance_menu(void)
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
            P_element_menu[position] = element;
            P_position_Menu[element] = position;
            tft.setCursor(display_coordinate_x(P_column_menu_element[position]), display_coordinate_y(P_row_menu_element[position]));
            tft.print(P_menu_char[element]);
            ++position;
        }
    }
}

FLASHMEM
void DisplayManager::P_show_frame_Performance_menu(int position, bool fresh)
{
    if (!fresh)
    {
        Frame_by_col_row(P_column_menu_element[P_menu_frame_on_element_0], P_row_menu_element[P_menu_frame_on_element_0], P_dimension_voice_menu[P_element_menu[P_menu_frame_on_element_0]], false);
    }

    Frame_by_col_row(P_column_menu_element[position], P_row_menu_element[position], P_dimension_voice_menu[P_element_menu[position]], true);
    P_choice_menu = P_element_menu[position];
    P_menu_frame_on_element_0 = position;
}

void DisplayManager::P_Delete_all_frame_performance_menu(void)
{
    uint8_t position;

    for (auto element = 0; element < 5; ++element)
    {
        if (Menu_P[element])
        {
            position = P_position_Menu[element];
            Frame_by_col_row(P_column_menu_element[position], P_row_menu_element[position], P_dimension_voice_menu[element], false);
        }
    }
}

FLASHMEM
void DisplayManager::P_Confirm_patch_change_popup(void)
{
    L_POPUP = 106;
    H_POPUP = 47;
    X_POPUP = (320 - L_POPUP) / 2;
    Y_POPUP = (240 - H_POPUP) / 2;
    Y_POPUP_TXT = 10;
    Y_POPUP_OPT = 30;

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
void DisplayManager::P_Confirm_patch_change_popup_frame(int value)
{
    P_Confirm_frame(X_POPUP + display_coordinate_x(2), Y_POPUP + Y_POPUP_OPT, 4, false); // DISPLAY_confirm_frame(uint8_t col, uint8_t row, uint8_t chars, bool   print)
    P_Confirm_frame(X_POPUP + display_coordinate_x(8), Y_POPUP + Y_POPUP_OPT, 2, false);
    P_Confirm_frame(X_POPUP + display_coordinate_x(12), Y_POPUP + Y_POPUP_OPT, 3, false);

    switch (value)
    {
    case 0: // exit
        P_Confirm_frame(X_POPUP + display_coordinate_x(2), Y_POPUP + Y_POPUP_OPT, 4, true);
        break;
    case 1: // no
        P_Confirm_frame(X_POPUP + display_coordinate_x(8), Y_POPUP + Y_POPUP_OPT, 2, true);
        break;
    case 2: // yes
        P_Confirm_frame(X_POPUP + display_coordinate_x(12), Y_POPUP + Y_POPUP_OPT, 3, true);
        break;
    default:
        break;
    }
}

void DisplayManager::P_Confirm_frame(int X, int Y, int chars, bool print)
{
    Frame_by_pixels(X, Y, (6 * chars) + 7, print);
}

FLASHMEM
void DisplayManager::P_Confirm_patch_delete_popup(void)
{
    L_POPUP = 106;
    H_POPUP = 47;
    Y_POPUP = (240 - H_POPUP) / 2;
    X_POPUP = (320 - L_POPUP) / 2;
    Y_POPUP_TXT = 10;
    Y_POPUP_OPT = 30;

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
void DisplayManager::P_Confirm_patch_delete_popup_frame(int value)
{
    P_Confirm_frame(X_POPUP + display_coordinate_x(5.5), Y_POPUP + Y_POPUP_OPT, 2, false); // DISPLAY_confirm_frame(uint8_t col, uint8_t row, uint8_t chars, bool   print)
    P_Confirm_frame(X_POPUP + display_coordinate_x(9.5), Y_POPUP + Y_POPUP_OPT, 3, false);

    switch (value)
    {
    case 0: // NO
        P_Confirm_frame(X_POPUP + display_coordinate_x(5.5), Y_POPUP + Y_POPUP_OPT, 2, true);
        break;
    case 1: // YES
        P_Confirm_frame(X_POPUP + display_coordinate_x(9.5), Y_POPUP + Y_POPUP_OPT, 3, true);
        break;
    default:
        break;
    }
}
FLASHMEM
void DisplayManager::SETUP_show_SETUP_page(void)
{
    tft.fillScreen(ILI9341_BLACK);

    Backgorund_red(0, 0, 5); // Display.Board(float   col, float row, int chars)
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(0));
    tft.setTextColor(ILI9341_WHITE);
    tft.print("SETUP");

    tft.setTextColor(TEXT_COLOR);
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(2));
    tft.print("KEY STEP");
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(3));
    tft.print("FIRST OCTAVE");
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(4));
    tft.print("OPTIMIZATION");
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(5));
    tft.print(F("CONTROL CHANGE ASSIGNMENT"));
    // tft.setCursor(x_pos(0), display_coordinate_y(6));
    // tft.print("*FUTURE DEVELOPMENTS*");
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(6));
    tft.print(F("IMPORT RAW FILES FROM /LILLARAW"));
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(7));
    tft.print(F("IMPORT CONFIGURATION FROM /LILLASET/lilla.txt"));
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(8));
    tft.print(F("EXPORT CONFIGURATION TO /LILLASET/lillaold.txt"));
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(9));
    tft.print(F("FACTORY RESET"));

    SETUP_show_Key_step_value();
    SETUP_show_First_octave_value();
    SETUP_show_Optimization_value();
}

FLASHMEM
void DisplayManager::SETUP_show_Key_step_value(void)
{
    Cancel_text_reset_cursor(display_coordinate_x(9), display_coordinate_y(2), 5);
    tft.setTextColor(ILI9341_YELLOW);

    switch (key_step)
    {
    case 0:
        tft.print(" 1st");
        break;

    case 1:
        tft.print("1/2st");
        break;

    case 2:
        tft.print("1/4st");
        break;

    case 3:
        tft.print("1/8st");
        break;

    default:
        tft.print(" 1st");
        break;
    }
}

FLASHMEM
void DisplayManager::SETUP_show_First_octave_value(void)
{
    Cancel_text_reset_cursor(display_coordinate_x(13), display_coordinate_y(3), 2);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(first_octave);
}

FLASHMEM
void DisplayManager::SETUP_show_Optimization_value(void)
{
    Cancel_text_reset_cursor(display_coordinate_x(13), display_coordinate_y(4), 29);
    tft.setTextColor(ILI9341_ORANGE);
    tft.print("POLYPHONY ");
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(POLYPHONY_FLASH[optimization]);
    tft.setTextColor(ILI9341_ORANGE);
    tft.print(",  MAX PITCH ");
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(MAX_PITCH_FLASH[optimization]);
}

FLASHMEM
void DisplayManager::SETUP_show_frame(int8_t value)
{
    Frame_by_col_row(9, 2, 5, false);   // First octave
    Frame_by_col_row(13, 3, 2, false);  // First octave
    Frame_by_col_row(13, 4, 29, false); // SETUP_Optimization
    Frame_by_col_row(0, 5, 25, false);  // Control Change Assignment
    Frame_by_col_row(0, 6, 31, false);  // Import raw files
    Frame_by_col_row(0, 7, 45, false);  // Import configuration from
    Frame_by_col_row(0, 8, 46, false);  // Export configuration to SD
    Frame_by_col_row(0, 9, 13, false);  // Factory Reset

    switch (value)
    {
    case 0:
        Frame_by_col_row(9, 2, 5, true); // First octave
        break;
    case 1:
        Frame_by_col_row(13, 3, 2, true); // First octave
        break;
    case 2:
        Frame_by_col_row(13, 4, 29, true); // SETUP_Optimization
        break;
    case 3:
        Frame_by_col_row(0, 5, 25, true); // Control Change
        break;
    case 4:
        Frame_by_col_row(0, 6, 31, true); // Import RAW files
        break;
    case 5:
        Frame_by_col_row(0, 7, 45, true); // Import configuration to SD
        break;
    case 6:
        Frame_by_col_row(0, 8, 46, true); // Export configuration to SD
        break;
    case 7:
        Frame_by_col_row(0, 9, 13, true); // Factory reset
        break;
    default:
        break;
    }
}

FLASHMEM
void DisplayManager::CC_show_ControlChange_page(void)
{
    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(0));
    tft.setTextColor(TEXT_COLOR);
    tft.print("CONTROL CHANGE ASSIGNMENT");

    tft.setCursor(display_coordinate_x(0), display_coordinate_y(1));
    tft.setTextColor(MENU_COLOR);
    tft.print("RETURN");

    tft.setTextColor(TEXT_COLOR);
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(3));
    tft.print("GAIN SOUND 1");
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(4));
    tft.print("GAIN SOUND 2");
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(5));
    tft.print("GAIN SOUND 3");
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(6));
    tft.print("GAIN SOUND 4");

    tft.setCursor(display_coordinate_x(0), display_coordinate_y(7));
    tft.print("GAIN SOUND 5");
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(8));
    tft.print("GAIN SOUND 6");
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(9));
    tft.print("GAIN SOUND 7");
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(10));
    tft.print("GAIN SOUND 8");
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(11));
    tft.print("LPF CUTOFF");
}

FLASHMEM
void DisplayManager::CC_show_all_sound_gains(void)
{
    for (auto instrument_id = 0; instrument_id < INSTRUMENTS_MAX; ++instrument_id)
    {
        CC_show_sound_gain(instrument_id);
    }
}

FLASHMEM
void DisplayManager::CC_show_sound_gain(int value)
{
    Cancel_text_reset_cursor(display_coordinate_x(Setup_Control_change_X), display_coordinate_y(value + 3), 3);
    tft.setTextColor(ILI9341_YELLOW);

    if (CC_Sound_gain[value] > 0)
    {
        tft.print(CC_Sound_gain[value]);
    }
    else
    {
        tft.print(" -");
    }
}

FLASHMEM
void DisplayManager::CC_show_lowpass_filter_value(void)
{
    Cancel_text_reset_cursor(display_coordinate_x(Setup_Control_change_X), display_coordinate_y(11), 3);
    tft.setTextColor(ILI9341_YELLOW);

    if (CC_lowpass_filter_value > 0)
    {
        tft.print(CC_lowpass_filter_value);
    }
    else
    {
        tft.print(" -");
    }
}

FLASHMEM
void DisplayManager::CC_show_frame_menu(int value)
{
    Frame_by_col_row(0, 1, 6, false); // Return

    for (auto n = 0; n < 9; ++n)
    {
        Frame_by_col_row(Setup_Control_change_X, n + 3, 3, false);
    }

    if (value == 0)
    {
        Frame_by_col_row(0, 1, 6, true);
    }
    else
    {
        Frame_by_col_row(Setup_Control_change_X, value + 2, 3, true);
    }
}

FLASHMEM
void DisplayManager::Import_raw_files_frame(uint8_t value)
{
    Frame_by_col_row(0, 1, 4, false); // DISPLAY_confirm_frame(uint8_t col, uint8_t row, uint8_t chars, bool   print)
    Frame_by_col_row(5, 1, 6, false);

    switch (value)
    {
    case 0: // EXIT
        Frame_by_col_row(0, 1, 4, true);
        break;
    case 1: // IMPORT
        Frame_by_col_row(5, 1, 6, true);
        break;
    default:
        break;
    }
}
FLASHMEM
void DisplayManager::Confirm_config_import_popup(void)
{
    L_POPUP = display_coordinate_x(46);
    H_POPUP = display_coordinate_y(5.2);
    Y_POPUP = (240 - H_POPUP) / 2;
    X_POPUP = (320 - L_POPUP) / 2;
    tft.fillRoundRect(X_POPUP, Y_POPUP, L_POPUP, H_POPUP, 4, ILI9341_RED);

    Y_POPUP_TXT = 10; // Prima riga testo
    Y_POPUP_OPT = Y_POPUP_TXT + 50;
    X_POPUP_OPT = display_coordinate_x(19);

    tft.setCursor(X_POPUP + display_coordinate_x(1), Y_POPUP + Y_POPUP_TXT);
    tft.setTextColor(ILI9341_WHITE);

    //       ("01234567890123456789012345678901234567891098765"); // 45 char
    tft.print(F("    WARNING: IMPORT CONFIGURATION FROM SD"));
    tft.setCursor(X_POPUP + display_coordinate_x(1), Y_POPUP + Y_POPUP_TXT + 15);
    tft.print(F(" WILL DELETE PATCHES, SOUNDS AND RECORDINGS!")); // 43
    tft.setCursor(X_POPUP + display_coordinate_x(1), Y_POPUP + Y_POPUP_TXT + 30);

    //       ("01234567890123456789012345678901234567891098765"); // 45 char
    tft.print(F("        DO YOU REALLY WANT TO PROCEED?"));
    tft.setTextColor(ILI9341_YELLOW);
    tft.setCursor(X_POPUP + X_POPUP_OPT, Y_POPUP + Y_POPUP_OPT);
    tft.print("NO");
    tft.setCursor(X_POPUP + X_POPUP_OPT + display_coordinate_x(4), Y_POPUP + Y_POPUP_OPT);
    tft.print("YES");
}

FLASHMEM
void DisplayManager::Factory_reset_wait_popup(void)
{
    L_POPUP = display_coordinate_x(38);
    H_POPUP = display_coordinate_y(3); // 64 pixel
    Y_POPUP = (240 - H_POPUP) / 2;
    X_POPUP = (320 - L_POPUP) / 2;
    Y_POPUP_TXT = 10; // Prima riga testo

    tft.fillRoundRect(X_POPUP, Y_POPUP, L_POPUP, H_POPUP, 4, ILI9341_YELLOW);
    tft.setCursor(X_POPUP + display_coordinate_x(0), Y_POPUP + Y_POPUP_TXT);
    tft.setTextColor(ILI9341_BLACK);

    //       ("01234567890123456789012345678901234567");
    tft.print(F("            FACTORY RESET"));
    tft.setCursor(X_POPUP + display_coordinate_x(0), Y_POPUP + Y_POPUP_TXT + display_coordinate_y(1));
    tft.setTextColor(ILI9341_BLACK);

    //       ("01234567890123456789012345678901234567");
    tft.print(F("    PLEASE WAIT - DO NOT SWITCH OFF"));
}

FLASHMEM
void DisplayManager::Encoder_pushbutton_test_board(void)
{
    L_POPUP = display_coordinate_x(32);
    H_POPUP = display_coordinate_y(6) - 4; // 64 pixel
    Y_POPUP = (240 - H_POPUP) / 2;
    X_POPUP = (320 - L_POPUP) / 2;

    tft.fillRoundRect(X_POPUP, Y_POPUP, L_POPUP, H_POPUP, 4, ILI9341_WHITE);

    tft.setTextColor(ILI9341_RED);
    tft.setCursor(X_POPUP + display_coordinate_x(1), Y_POPUP + 4);
    tft.print(F("ENCODERS AND PUSHBUTTONS TEST"));
}

void DisplayManager::Encoder_pushbutton_test_result(const int device, const int element, const int value)
{
    Encoder_pushbutton_test_board();

    struct Result
    {
        int device;
        int element;
        int value;
    };
    static Result memo[5] = {};

    memo[4] = memo[3];
    memo[3] = memo[2];
    memo[2] = memo[1];
    memo[1] = memo[0];
    memo[0] = {device, element, value};

    tft.setTextColor(ILI9341_BLACK);

    for (auto i = 0; i < 5; ++i)
    {
        tft.setCursor(X_POPUP + display_coordinate_x(1), Y_POPUP + display_coordinate_y(5 - i));
        if (memo[i].device == 1)
        {
            tft.print("encoder ");
            tft.print(memo[i].element);
            tft.print(" value ");
            tft.print(memo[i].value);
        }
        if (memo[i].device == 2)
        {
            tft.print("pushbutton ");
            tft.print(memo[i].element);
            tft.print(" pressed");
        }
    }
}

FLASHMEM
void DisplayManager::SD_missing(uint16_t color)
{
    L_POPUP = display_coordinate_x(18);
    H_POPUP = display_coordinate_y(3);
    Y_POPUP = (240 - H_POPUP) / 2;
    X_POPUP = (320 - L_POPUP) / 2;
    Y_POPUP_TXT = 20; // Prima riga testo

    tft.fillRoundRect(X_POPUP, Y_POPUP, L_POPUP, H_POPUP, 4, color);
    tft.setCursor(X_POPUP + display_coordinate_x(0), Y_POPUP + Y_POPUP_TXT);
    tft.setTextColor(ILI9341_WHITE);

    //       ("012345678901234567");
    tft.print(F("  SD NOT PRESENT"));
}

FLASHMEM
void DisplayManager::Config_import_FILE_error_popup(void)
{
    L_POPUP = display_coordinate_x(35);
    H_POPUP = display_coordinate_y(3);
    Y_POPUP = (240 - H_POPUP) / 2;
    X_POPUP = (320 - L_POPUP) / 2;
    Y_POPUP_TXT = 20; // Prima riga testo

    tft.fillRoundRect(X_POPUP, Y_POPUP, L_POPUP, H_POPUP, 4, ILI9341_BLACK);
    tft.setCursor(X_POPUP + display_coordinate_x(0), Y_POPUP + Y_POPUP_TXT);
    tft.setTextColor(ILI9341_YELLOW);

    //       ("01234567890123456789012345678901234");
    tft.print(F("  SD/LILLASET/lilla.txt NOT FOUND"));
}

FLASHMEM
void DisplayManager::Config_import_REBOOT_popup(void)
{
    L_POPUP = display_coordinate_x(38);
    H_POPUP = display_coordinate_y(3);
    Y_POPUP = (240 - H_POPUP) / 2;
    X_POPUP = (320 - L_POPUP) / 2;
    Y_POPUP_TXT = 20; // Prima riga testo

    tft.fillRoundRect(X_POPUP, Y_POPUP, L_POPUP, H_POPUP, 4, ILI9341_BLACK);
    tft.setCursor(X_POPUP + display_coordinate_x(0), Y_POPUP + Y_POPUP_TXT);
    tft.setTextColor(ILI9341_WHITE);

    //           ("01234567890123456789012345678901234567");
    tft.print(F(" LOADING NEW CONFIGURATION AND REBOOT"));
}

FLASHMEM
void DisplayManager::Confirm_config_import_frame(uint8_t value)
{
    P_Confirm_frame(X_POPUP + X_POPUP_OPT, Y_POPUP + Y_POPUP_OPT, 2, false); // DISPLAY_confirm_frame(uint8_t col, uint8_t row, uint8_t chars, bool   print)
    P_Confirm_frame(X_POPUP + X_POPUP_OPT + display_coordinate_x(4), Y_POPUP + Y_POPUP_OPT, 3, false);

    switch (value)
    {
    case 0: // NO
        P_Confirm_frame(X_POPUP + X_POPUP_OPT, Y_POPUP + Y_POPUP_OPT, 2, true);
        break;
    case 1: // YES
        P_Confirm_frame(X_POPUP + X_POPUP_OPT + display_coordinate_x(4), Y_POPUP + Y_POPUP_OPT, 3, true);
        break;
    default:
        break;
    }
}

FLASHMEM
void DisplayManager::Confirm_config_export_popup(void)
{
    L_POPUP = display_coordinate_x(51);
    H_POPUP = display_coordinate_y(5.2);
    Y_POPUP = (240 - H_POPUP) / 2;
    X_POPUP = (320 - L_POPUP) / 2;
    tft.fillRoundRect(X_POPUP, Y_POPUP, L_POPUP, H_POPUP, 4, ILI9341_RED);

    Y_POPUP_TXT = 10; // Prima riga testo
    Y_POPUP_OPT = Y_POPUP_TXT + 50;
    X_POPUP_OPT = display_coordinate_x(19);

    tft.setCursor(X_POPUP + display_coordinate_x(1), Y_POPUP + Y_POPUP_TXT);
    tft.setTextColor(ILI9341_WHITE);

    //         ("012345678901234567890123456789012345678901234567890");
    tft.print(F("        WARNING: EXPORT CONFIGURATION TO SD"));
    tft.setCursor(X_POPUP, Y_POPUP + Y_POPUP_TXT + 15);
    tft.print(F("  WILL DELETE A PREVIOUS CONFIGURATION FILE SAVED")); // 43
    tft.setCursor(X_POPUP, Y_POPUP + Y_POPUP_TXT + 30);
    tft.print(F("        DO YOU REALLY WANT TO PROCEED?"));
    tft.setTextColor(ILI9341_YELLOW);
    tft.setCursor(X_POPUP + X_POPUP_OPT, Y_POPUP + Y_POPUP_OPT);
    tft.print("NO");
    tft.setCursor(X_POPUP + X_POPUP_OPT + display_coordinate_x(4), Y_POPUP + Y_POPUP_OPT);
    tft.print("YES");
}

FLASHMEM
void DisplayManager::Config_export_SD_error_popup(void)
{
    L_POPUP = display_coordinate_x(27);
    H_POPUP = display_coordinate_y(3);
    Y_POPUP = (240 - H_POPUP) / 2;
    X_POPUP = (320 - L_POPUP) / 2;
    Y_POPUP_TXT = 20; // Prima riga testo
    tft.fillRoundRect(X_POPUP, Y_POPUP, L_POPUP, H_POPUP, 4, ILI9341_BLACK);

    tft.setCursor(X_POPUP, Y_POPUP + Y_POPUP_TXT);
    tft.setTextColor(ILI9341_YELLOW);

    //       ("012345678901234567890123456");
    tft.print(F(" INSUFFICIENT SPACE IN SD"));
}

FLASHMEM
void DisplayManager::Config_export_save_popup(void)
{
    L_POPUP = display_coordinate_x(50);
    H_POPUP = display_coordinate_y(3);
    Y_POPUP = (240 - H_POPUP) / 2;
    X_POPUP = (320 - L_POPUP) / 2;
    Y_POPUP_TXT = 20; // Prima riga testo

    tft.fillRoundRect(X_POPUP, Y_POPUP, L_POPUP, H_POPUP, 4, ILI9341_BLACK);
    tft.setCursor(X_POPUP, Y_POPUP + Y_POPUP_TXT);
    tft.setTextColor(ILI9341_WHITE);

    //       ("01234567890123456789012345678901234567890123456789");
    tft.print(F("  CONFIGURATION SAVED: SD/LILLASET/lillaold.txt"));
}

FLASHMEM
void DisplayManager::Confirm_factory_reset_popup(void)
{
    L_POPUP = display_coordinate_x(49);
    H_POPUP = display_coordinate_y(5.2);
    Y_POPUP = (240 - H_POPUP) / 2;
    X_POPUP = (320 - L_POPUP) / 2;
    tft.fillRoundRect(X_POPUP, Y_POPUP, L_POPUP, H_POPUP, 4, ILI9341_RED);

    Y_POPUP_TXT = 15; // Prima riga testo
    Y_POPUP_OPT = Y_POPUP_TXT + 50;
    X_POPUP_OPT = display_coordinate_x(19);

    tft.setCursor(X_POPUP + display_coordinate_x(1), Y_POPUP + Y_POPUP_TXT);
    tft.setTextColor(ILI9341_WHITE);

    //       ("0123456789012345678901234567890123456789012345678");
    tft.print(F("      WARNING: FACTORY RESET WILL DELETE"));
    tft.setCursor(X_POPUP + display_coordinate_x(1), Y_POPUP + Y_POPUP_TXT + 15);
    tft.print(F("      ALL PATCHES, SOUNDS AND RECORDINGS!"));
    tft.setCursor(X_POPUP + display_coordinate_x(1), Y_POPUP + Y_POPUP_TXT + 30);
    tft.print(F("        DO YOU REALLY WANT TO PROCEED?"));
    tft.setTextColor(ILI9341_YELLOW);
    tft.setCursor(X_POPUP + X_POPUP_OPT, Y_POPUP + Y_POPUP_OPT);
    tft.print("NO");
    tft.setCursor(X_POPUP + X_POPUP_OPT + display_coordinate_x(4), Y_POPUP + Y_POPUP_OPT);
    tft.print("YES");
}

FLASHMEM
void DisplayManager::Config_reset_popup(void)
{
    L_POPUP = display_coordinate_x(49);
    H_POPUP = display_coordinate_y(5.2);
    Y_POPUP = (240 - H_POPUP) / 2;
    X_POPUP = (320 - L_POPUP) / 2;
    tft.fillRoundRect(X_POPUP, Y_POPUP, L_POPUP, H_POPUP, 4, ILI9341_RED);

    Y_POPUP_TXT = 10; // Prima riga testo
    Y_POPUP_OPT = Y_POPUP_TXT + 50;
    X_POPUP_OPT = display_coordinate_x(19);

    tft.setCursor(X_POPUP + display_coordinate_x(1), Y_POPUP + Y_POPUP_TXT + 15);
    tft.setTextColor(ILI9341_WHITE);

    //       ("0123456789012345678901234567890123456789012345678");
    tft.print(F("   PLEASE WAIT. LILLA WILL RESTART AFTER RESET")); // 43
}


FLASHMEM
void DisplayManager::VFS_Make_presentation(void)
{
    tft.fillScreen(ILI9341_BLACK);

    tft.setCursor(display_coordinate_x(0), display_coordinate_y(0));
    tft.setTextColor(TEXT_COLOR);
    tft.print(F("SAMPLER: RECORDING MEMORY DIMENSION"));

    tft.setCursor(display_coordinate_x(0), display_coordinate_y(2));
    tft.setTextColor(ILI9341_YELLOW);
    //        "012345678901234567890 234 X 432 98765432109876543210"; // max 52 char
    tft.print(F(" PLEASE ASSIGN THE MEMORY SPACE FOR RECORDINGS (AND"));
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(3));
    tft.print(F("     THE CONSEQUENT SPACE FOR RAW FILES EXPORT)"));

    tft.setCursor(display_coordinate_x(0), display_coordinate_y(5));
    tft.setTextColor(TEXT_COLOR);
    tft.print(F("TOTAL FLASH MEMORY SPACE "));
    tft.setTextColor(ILI9341_WHITE);
    tft.print(Get_flash_size() / 1048576.0f);
    Show_measure_unit("MB", 2);

    tft.setCursor(display_coordinate_x(0), display_coordinate_y(6));
    tft.setTextColor(TEXT_COLOR);
    tft.print(F("RAW FILES IMPORTED "));
    tft.setTextColor(ILI9341_WHITE);
    tft.print(Get_flash_occupation() / 1048576.0f);
    Show_measure_unit("MB", 2);
}

FLASHMEM
void DisplayManager::VFS_Make_assignments(void)
{
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(7));
    tft.setTextColor(TEXT_COLOR);

    // tft.print("0123456789012345678901234567890....."); // max 52 char
    tft.print(F("SPACE FOR RECORDINGS"));

    tft.setCursor(display_coordinate_x(0), display_coordinate_y(8));
    tft.setTextColor(TEXT_COLOR);

    // tft.print("0123456789012345678901234567890....."); // max 52 char
    tft.print(F("SPACE FOR RAW FILES EXPORT"));
}

FLASHMEM
void DisplayManager::VFS_Make_restart(void)
{
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(10));
    tft.setTextColor(ILI9341_GREEN);
    tft.print(ADV_VFS_3);
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(11));
    tft.print(ADV_VFS_4);
}

FLASHMEM
void DisplayManager::VFS_Make_not_enough_memory_for_sampler(void)
{
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(10));
    tft.setTextColor(ILI9341_MAGENTA);
    tft.print(ADV_VFS_0);
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(11));
    tft.print(ADV_VFS_1);
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(12));
    tft.print(ADV_VFS_2);
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(13));
    tft.print(ADV_VFS_3);
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(14));
    tft.print(ADV_VFS_4);
}

FLASHMEM
void DisplayManager::Copy_raw_files_SD_to_Flash_chip_titolo(void)
{
    tft.fillScreen(ILI9341_BLACK);
    tft.setTextColor(TEXT_COLOR);
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(0));
    tft.print(F("IMPORT RAW FILES FROM SD TO FLASH MEMORY"));
}

FLASHMEM
void DisplayManager::Copy_raw_files_SD_to_Flash_chip_waiting_for_SD(void)
{
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(3));
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(F("WAITING 10sec FOR SD CARD"));
}

FLASHMEM
void DisplayManager::Copy_raw_files_SD_to_Flash_chip_lillaraw_missing(void)
{
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(3));
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(F("NO FILES TO IMPORT: MISSING /LILLARAW DIRECTORY!"));
}

FLASHMEM
void DisplayManager::Copy_raw_files_SD_to_Flash_chip_files_report(unsigned long SD_raw_volume, int SD_raw_files, int raw_files_volume, int flash_raw_files)
{
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(3));
    tft.setTextColor(TEXT_COLOR);
    tft.print(F("SOURCE: SD CARD /LILLARAW"));
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(4));
    tft.print(F("- RAW FILES"));
    tft.setCursor(display_coordinate_x(12), display_coordinate_y(4));
    tft.setTextColor(ILI9341_WHITE);
    tft.print(SD_raw_files);
    tft.print(" (");
    tft.setTextColor(ILI9341_WHITE);
    tft.print(SD_raw_volume / 1048576.0f, 2);
    Show_measure_unit("MB", 2);
    tft.setTextColor(ILI9341_WHITE);
    tft.print(")");

    // Flash chip info
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(6));
    tft.setTextColor(TEXT_COLOR);
    tft.println(F("DESTINATION: LILLA FLASH MEMORY"));
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(7));
    tft.println(F("- DIMENSION"));
    tft.setCursor(display_coordinate_x(12), display_coordinate_y(7));
    tft.setTextColor(ILI9341_WHITE);
    tft.print(flash_dimension_MB);
    Show_measure_unit("MB", 2);

    tft.setCursor(display_coordinate_x(0), display_coordinate_y(8));
    tft.setTextColor(TEXT_COLOR);
    tft.print("- RAW FILES");
    tft.setCursor(display_coordinate_x(12), display_coordinate_y(8));
    tft.setTextColor(ILI9341_WHITE);
    tft.print(flash_raw_files); // Get_raw_files()
    tft.print(" (");
    tft.setTextColor(ILI9341_WHITE);
    tft.print(raw_files_volume / 1048576.0f, 2); // Get_raw_files_volume()
    Show_measure_unit("MB", 2);
    tft.setTextColor(ILI9341_WHITE);
    tft.print(")");

    tft.setCursor(display_coordinate_x(0), display_coordinate_y(9));
    tft.setTextColor(TEXT_COLOR);
    tft.print(F("- RECORDINGS"));
    tft.setCursor(display_coordinate_x(13), display_coordinate_y(9));
    tft.setTextColor(ILI9341_WHITE);
    tft.print(recordings);
}

FLASHMEM
void DisplayManager::Copy_raw_files_SD_to_Flash_chip_last_warning(float erasing_time_ms)
{
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(10));
    tft.setTextColor(TEXT_COLOR);
    tft.println(F("- DELETING TIME"));
    tft.setCursor(display_coordinate_x(16), display_coordinate_y(10));
    tft.setTextColor(ILI9341_WHITE);
    tft.print(erasing_time_ms / 60000, 1);
    Show_measure_unit("min", 3);

    // display SD->Flash menu
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(1));
    tft.setTextColor(MENU_COLOR);
    tft.print("EXIT IMPORT");

    tft.setTextColor(ILI9341_MAGENTA);

    //        "012345678901234567890 234 X 432 98765432109876543210"); // max 52 char
    tft.setCursor(display_coordinate_x(22), display_coordinate_y(12) - 5);
    tft.print("IMPORTANT");
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(13) - 5);
    tft.print(F("- RAW FILES IMPORT WILL DELETE ALL AUDIO FILES AND"));
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(14) - 5);
    tft.print(F("  RECORDINGS IN LILLA!"));
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(15) - 5);
    tft.print(F("- LILLA REQUIRES .raw (lowercase!)"));
}

FLASHMEM
void DisplayManager::Copy_raw_files_SD_to_Flash_chip_job_start(void)
{
    // Start erasing flash chip
    tft.fillRect(0, display_coordinate_y(11), 320, 240, ILI9341_BLACK);
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(12) - 5);
    tft.setTextColor(ILI9341_YELLOW);

    //        "012345678901234567890 234 X 432 98765432109876543210"); // max 52 char
    tft.print(F("PLEASE WAIT: FLASH MEMORY ERASE IS RUNNING."));
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(13) - 5);
    tft.print(F("THAN RAW FILES WILL BE COPYED FROM SD/LILLARAW TO "));
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(14) - 5);
    tft.print("LILLA FLASH MEMORY");
}

FLASHMEM
void DisplayManager::Copy_raw_files_SD_to_Flash_chip_initial_percentage(void)
{
    tft.drawLine(display_coordinate_x(0), BAR_POS_Y, display_coordinate_x(0), BAR_POS_Y + 5, ILI9341_YELLOW);
    tft.setCursor(display_coordinate_x(0) + 10, BAR_POS_Y);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print("0%");
}

FLASHMEM
void DisplayManager::Copy_raw_files_SD_to_Flash_chip_progress(unsigned char barcount)
{
    tft.drawLine(display_coordinate_x(0) + barcount, BAR_POS_Y, display_coordinate_x(0) + barcount, BAR_POS_Y + 5, ILI9341_YELLOW);
    tft.setCursor(display_coordinate_x(0) + barcount + 10, BAR_POS_Y);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(barcount);
    tft.print("%");

    if (barcount == 100)
    {
        tft.print(" *DONE*");
    }
}

FLASHMEM
void DisplayManager::Copy_raw_files_SD_to_Flash_chip_popup_landscape(void)
{
    // Start copying RAW files from SD to Flash chip
    tft.fillRect(0, 12, 320, 240, ILI9341_BLACK);
}

FLASHMEM
void DisplayManager::Copy_raw_files_SD_to_Flash_chip_list_landscape(void)
{
    tft.fillRect(0, display_coordinate_y(3), 320, 240, ILI9341_BLACK);
}

FLASHMEM
void DisplayManager::Copy_raw_files_SD_to_Flash_chip_files_to_copy(int row, const char *filename, unsigned long length)
{
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(row));
    tft.setTextColor(TEXT_COLOR);
    tft.print("COPYING FILE ");
    tft.setTextColor(ILI9341_WHITE);
    tft.print(filename);
    tft.print("  ");
    tft.print(length / 1024);
    tft.print("KB");
}

FLASHMEM
void DisplayManager::Copy_raw_files_SD_to_Flash_chip_flash_error(void)
{
    tft.setTextColor(TEXT_COLOR);
    tft.print("  FLASH MEMORY ERROR");
}

FLASHMEM
void DisplayManager::Copy_raw_files_SD_to_Flash_chip_flash_full_error(void)
{
    tft.setTextColor(TEXT_COLOR);
    tft.print("  ERROR: FLASH MEMORY FULL!");
}

FLASHMEM
void DisplayManager::Copy_raw_files_SD_to_Flash_chip_job_done(void)
{
    // Display RAW files list
    tft.fillRect(0, 12, 320, 240, ILI9341_BLACK);
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(2));
    tft.setTextColor(TEXT_COLOR);
    tft.print(F("RAW FILES IMPORT COMPLETED. FILE LIST:"));
}

FLASHMEM
void DisplayManager::Copy_raw_files_SD_to_Flash_chip_file_copied(int row, const char *filename, uint32_t filesize)
{
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(row));
    tft.setTextColor(ILI9341_WHITE);
    tft.print(filename);
    tft.print("  ");
    tft.print(filesize / 1024);
    tft.print("KB");
}



void DisplayManager::Loop_led(int track, int instrument_id, bool on)
{
    tft.drawBitmap(Loop_LED_X + track * 42, Loop_LED_Y + instrument_id * Loop_LED_DY, led_pic, 8, 8, on ? GREEN_ON : GREEN_OFF);
}

void DisplayManager::Loop_led_metronomo(int Xled, int Yled, bool ONled)
{
    tft.drawBitmap(Xled, Yled, led_pic, 8, 8, ONled ? RED_ON : RED_OFF);
}