/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#include "DisplayLiveSampler.h"

void DisplayLiveSampler::Led_LIVE_SAMPLING(const bool on)
{
    if (on)
    {
        tft.drawBitmap(display_coordinate_x(39.5), display_coordinate_y(0), led_pic, 6, 8, ((MX_mute[0] && MX_mute[1]) ? RED_ON : GREEN_ON));
    }
    else
    {
        tft.drawBitmap(display_coordinate_x(39.5), display_coordinate_y(0), led_pic, 8, 8, ((MX_mute[0] && MX_mute[1]) ? RED_OFF : GREEN_OFF));
    }
    return;
}

FLASHMEM
void DisplayLiveSampler::Confirm_EXIT_from_LS(void)
{
    const int L_POPUP = 106;                 // Larghezza
    const int H_POPUP = 47;                  // Altezza
    const int X_POPUP = (320 - L_POPUP) / 2; // X posizione su display
    const int Y_POPUP = (240 - H_POPUP) / 2; // Y posizione su display
    const int Y_POPUP_TXT = 10;              // prima riga testo
    const int Y_POPUP_OPT = 30;              // riga opzioni

    tft.fillRoundRect(X_POPUP, Y_POPUP, L_POPUP, H_POPUP, 4, ILI9341_RED); // does NOT delete frame
    tft.setCursor(X_POPUP + display_coordinate_x(1), Y_POPUP + Y_POPUP_TXT);
    tft.setTextColor(ILI9341_WHITE);
    tft.print("STOP RECORDING?");
    tft.setTextColor(ILI9341_YELLOW);
    tft.setCursor(X_POPUP + display_coordinate_x(5.5), Y_POPUP + Y_POPUP_OPT);
    tft.print("NO");
    tft.setCursor(X_POPUP + display_coordinate_x(9.5), Y_POPUP + Y_POPUP_OPT);
    tft.print("YES");
}

FLASHMEM
void DisplayLiveSampler::Page(void)
{
    //("012345678901234567890"); // Size 1: 21 chars
    tft.fillScreen(ILI9341_BLACK);
    Page_title();

    tft.setTextColor(TEXT_COLOR);
    tft.setCursor(display_coordinate_x(LS_column_row_VOLUME[0]), display_coordinate_y(LS_column_row_VOLUME[1]));
    tft.print("VOLUME");
    Volume();

    tft.setTextColor(TEXT_COLOR);
    tft.setCursor(display_coordinate_x(LS_column_row_BUFFER[0]), display_coordinate_y(LS_column_row_BUFFER[1]));
    tft.print("BUFFER");
    Buffer();

    tft.setTextColor(TEXT_COLOR);
    tft.setCursor(display_coordinate_x(LS_column_row_GAIN[0]), display_coordinate_y(LS_column_row_GAIN[1]));
    tft.print("LINE IN GAIN");
    Gain();

    tft.setTextColor(TEXT_COLOR);
    tft.setCursor(display_coordinate_x(LS_column_row_PLAY_MODE[0]), display_coordinate_y(LS_column_row_PLAY_MODE[1]));
    tft.print("PLAY MODE");
    Play_mode();

    tft.setTextColor(TEXT_COLOR);
    tft.setCursor(display_coordinate_x(LS_column_row_FEEDBACK[0]), display_coordinate_y(LS_column_row_FEEDBACK[1]));
    tft.print("FEEDBACK");
    Feedback();

    tft.setTextColor(TEXT_COLOR);
    tft.setCursor(display_coordinate_x(LS_column_row_WINDOW[0]), display_coordinate_y(LS_column_row_WINDOW[1]));
    tft.print("WINDOW");
    Window();

    tft.setTextColor(TEXT_COLOR);
    tft.setCursor(display_coordinate_x(LS_column_row_START_POINT[0]), display_coordinate_y(LS_column_row_START_POINT[1]));
    tft.print("START POINT");
    Start_point();

    tft.setTextColor(TEXT_COLOR);
    tft.setCursor(display_coordinate_x(LS_column_row_LOOP[0]), display_coordinate_y(LS_column_row_LOOP[1]));
    tft.print("LOOP");
    Loop_time();

    tft.setTextColor(TEXT_COLOR);
    tft.setCursor(display_coordinate_x(LS_column_row_STEP[0]), display_coordinate_y(LS_column_row_STEP[1]));
    tft.print("STEP");
    Step();

}

FLASHMEM
void DisplayLiveSampler::Page_title(void)
{
    //("012345678901234567890"); // Size 1: 21 chars
    Backgorund_red(0, 0, 12); // Display.Backgorund_red(float & col, float row, int chars)
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(0));
    tft.print("LIVE SAMPLER");
}

FLASHMEM
void DisplayLiveSampler::Volume(void)
{
    Cancel_text_reset_cursor(display_coordinate_x(LS_column_row_volume[0]), display_coordinate_y(LS_column_row_volume[1]), 4);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(volume_patch);
}

FLASHMEM
void DisplayLiveSampler::Gain(void)
{
    Cancel_text_reset_cursor(display_coordinate_x(LS_column_row_gain[0]), display_coordinate_y(LS_column_row_gain[1]), 2);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(Line_in_gain + 1);
}

FLASHMEM
void DisplayLiveSampler::Buffer(void) //
{
    Cancel_text_reset_cursor(display_coordinate_x(LS_column_row_buffer[0]), display_coordinate_y(LS_column_row_buffer[1]), 12);
    tft.setTextColor(ILI9341_WHITE);
    tft.print(LS_buffer_dim / AUDIO_SAMPLE_RATE, 1);
    Show_measure_unit("sec", 3);
}

FLASHMEM
void DisplayLiveSampler::Play_mode(void)
{
    Cancel_text_reset_cursor(display_coordinate_x(LS_column_row_play_mode[0]), display_coordinate_y(LS_column_row_play_mode[1]), LS_chars_play_mode);
    tft.setTextColor(ILI9341_YELLOW);
    if (LS_mode > 1)
    {
        tft.print(loop_mode[LS_mode]);
        tft.print(" ");
    }
    tft.print(name_mode[LS_mode]);

    LS_chars_play_mode = (tft.getCursorX() - display_coordinate_x(LS_column_row_play_mode[0])) / 6;
}

FLASHMEM
void DisplayLiveSampler::Feedback(void)
{
    Cancel_text_reset_cursor(display_coordinate_x(LS_column_row_feedback[0]), display_coordinate_y(LS_column_row_feedback[1]), LS_chars_feedback);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(100 * LS_fbk_table[LS_feedback], 2);
    tft.setTextColor(ILI9341_ORANGE);
    tft.print("%");

    LS_chars_feedback = (tft.getCursorX() - display_coordinate_x(LS_column_row_feedback[0])) / 6;
}

FLASHMEM
void DisplayLiveSampler::Step(void)
{
    Cancel_text_reset_cursor(display_coordinate_x(LS_column_row_step[0]), display_coordinate_y(LS_column_row_step[1]), LS_chars_step);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(LS_X_step);
    tft.setTextColor(ILI9341_ORANGE);
    tft.print("samples");
}

FLASHMEM
void DisplayLiveSampler::Window(void)
{
    Cancel_text_reset_cursor(display_coordinate_x(LS_column_row_window[0]), display_coordinate_y(LS_column_row_window[1]), LS_chars_window);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(LS_window_width / AUDIO_SAMPLE_RATE, 1);
    tft.setTextColor(ILI9341_ORANGE);
    tft.print("sec");

    LS_chars_window = (tft.getCursorX() - display_coordinate_x(LS_column_row_window[0])) / 6;
}

FLASHMEM
void DisplayLiveSampler::Loop_time(void)
{
    Cancel_text_reset_cursor(display_coordinate_x(LS_column_row_loop_time[0]), display_coordinate_y(LS_column_row_loop_time[1]), LS_chars_loop_time);
    if (LS_mode > 1)
    {
        tft.setTextColor(ILI9341_YELLOW);
        tft.print(LS_XY_delta / AUDIO_SAMPLE_RATE, 2);
        Show_measure_unit("sec", 3);
    }
    else
    {
        tft.print("--");
    }
}

FLASHMEM
void DisplayLiveSampler::Start_point(void) // X_sample_delta
{
    float local_value = 0;
    Cancel_text_reset_cursor(display_coordinate_x(LS_column_row_start_point[0]), display_coordinate_y(LS_column_row_start_point[1]), LS_chars_start_point);
    tft.setTextColor(ILI9341_YELLOW);

    if (LS_XY_lock)
    {
        tft.print("FIXED ");
        tft.print(LS_X_sample / AUDIO_SAMPLE_RATE, 2);
        Show_measure_unit("sec", 3);
    }
    else
    {
        if (LS_X_delta == 0)
        {
            tft.print("SYNC");
        }

        else if (LS_X_delta > LS_buffer_dim / 2)
        {
            local_value = (LS_buffer_dim - LS_X_delta) / AUDIO_SAMPLE_RATE;
            if (local_value >= 0)
            {
                tft.print("BEHIND ");
                tft.print(local_value, 2);
            }
            else
            {
                tft.print("AHEAD ");
                tft.print(-local_value, 2);
            }
        }
        else
        {
            local_value = -LS_X_delta / AUDIO_SAMPLE_RATE;
            if (local_value >= 0)
            {
                tft.print("BEHIND ");
                tft.print(local_value, 2);
            }
            else
            {
                tft.print("AHEAD ");
                tft.print(-local_value, 2);
            }
        }

        Show_measure_unit("sec", 3);
    }
}

FLASHMEM
void DisplayLiveSampler::Menu(void)
{
    auto position = 0; // position on display
    Delete_text_row(1);
    tft.setTextColor(MENU_COLOR);

    for (auto element = 0; element < LS_menu_elements; ++element) // menu element
    {
        if (Menu_LS[element])
        {
            if (position == 0)
            {
                X_position_Menu_LS[position] = 0;
            }

            else
            {
                X_position_Menu_LS[position] = X_position_Menu_LS[position - 1] + dimension_voice_Menu_LS[element_Menu_LS[position - 1]] + 1;
            }

            element_Menu_LS[position] = element;
            position_Menu_LS[element] = position;
            tft.setCursor(display_coordinate_x(X_position_Menu_LS[position]), display_coordinate_y(1));
            tft.setTextColor(MENU_COLOR);
            tft.print(Menu_LS_char[element]);
            ++position;
        }
    }
}

FLASHMEM
void DisplayLiveSampler::Menu_frame(const int position)
{
    Delete_menu_frames();
    Frame_by_col_row(X_position_Menu_LS[position], 1, dimension_voice_Menu_LS[element_Menu_LS[position]], true);
}

FLASHMEM
void DisplayLiveSampler::Delete_menu_frames(void)
{
    int position;

    for (auto element = 0; element < LS_menu_elements; ++element)
    {
        if (Menu_LS[element])
        {
            position = position_Menu_LS[element];
            Frame_by_col_row(X_position_Menu_LS[position], 1, dimension_voice_Menu_LS[element], false);
        }
    }
}

void DisplayLiveSampler::Show_wave(const int sound_id)
{
    int id_file = Sound[sound_id].file;

    LS_window_A_sample = LS_constrain_position(LS_X_sample - (LS_window_width - 1) / 2);
    LS_window_B_sample = LS_window_A_sample + LS_window_width - 1;

    if (false)
    {
        Serial.print(F("(LS_buffer_dim - 1): "));
        Serial.print(LS_buffer_dim - 1);
        Serial.print(F("    LS_X_sample: "));
        Serial.print(LS_X_sample);
        Serial.print(F("    LS_window_A_sample: "));
        Serial.print(LS_window_A_sample);
        Serial.print(F("    LS_window_B_sample: "));
        Serial.println(LS_window_B_sample);
    }

    int16_t *Wave_array = Info.LS_620_samples_array(id_file, LS_window_A_sample, LS_window_B_sample);

    // localtimer = 0;

    canvas.fillRect(0, 0, WAVEBOARD_WIDTH, WAVEBOARD_HEIGHT, WAVE_BOARD_COLOR); // .fillRect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint16_t color);

    LS_K_wave_color = (LS_window_B_sample - LS_window_A_sample) / static_cast<float>(WAVEBOARD_WIDTH - 1);

    /* canvas coordinates

                                0                                                 WAVEBOARD_WIDTH - 1 (309)
                                ---------------------------------------------------------------
                            0   |                                                             |
                                |                                                             |
                                |                                                             |
                                |                                                             |
           CANVAS_WAVE_0 (48)   |-------------------------------------------------------------|
                                |                                                             |
                                |                                                             |
                                |                                                             |
                                |                                                             |
    WAVEBOARD_HEIGHT - 1 (96)   |_____________________________________________________________|

    */

    int y0 = CANVAS_WAVE_0;
    int wave_pixel_0 = 0;

    for (auto wave_pixel = 0; wave_pixel < WAVEBOARD_WIDTH; ++wave_pixel)
    {
        int yp = CANVAS_WAVE_0 - (*(Wave_array + wave_pixel) >> 10);                   // valore minimo = 48 - 32 = 16
        int yn = CANVAS_WAVE_0 - (*(Wave_array + wave_pixel + WAVEBOARD_WIDTH) >> 10); // valore massimo 48 + 32 = 80

        uint16_t LS_wave_color = Get_wave_color(wave_pixel);

        if (yp == CANVAS_WAVE_0 && yn == CANVAS_WAVE_0)
        {
            canvas.drawPixel(wave_pixel, yp, LS_WAVE_ZERO_COLOR);
            wave_pixel_0 = wave_pixel;
            y0 = yp;
        }

        else if (yp < CANVAS_WAVE_0 && yn == CANVAS_WAVE_0)
        {
            if (wave_pixel > 0 && y0 != CANVAS_WAVE_0)
            {
                canvas.drawLine(wave_pixel_0, y0, wave_pixel, yp, LS_wave_color);
            }
            else
            {
                canvas.drawPixel(wave_pixel, yp, LS_wave_color);
            }

            wave_pixel_0 = wave_pixel;
            y0 = yp;
        }

        else if (yp == CANVAS_WAVE_0 && yn > CANVAS_WAVE_0)
        {
            if (wave_pixel > 0 && y0 != CANVAS_WAVE_0)
            {
                canvas.drawLine(wave_pixel_0, y0, wave_pixel, yn, LS_wave_color);
            }
            else
            {
                canvas.drawPixel(wave_pixel, yn, LS_wave_color);
            }

            wave_pixel_0 = wave_pixel;
            y0 = yn;
        }

        else if (yp < CANVAS_WAVE_0 && yn > CANVAS_WAVE_0)
        {
            if (wave_pixel > 0 && y0 != CANVAS_WAVE_0)
            {
                canvas.drawLine(wave_pixel_0, y0, wave_pixel, yn, LS_wave_color);
                canvas.drawLine(wave_pixel, yp, wave_pixel, yn, LS_wave_color);
            }
            else
            {
                canvas.drawLine(wave_pixel, yp, wave_pixel, yn, LS_wave_color);
            }

            wave_pixel_0 = wave_pixel;
            y0 = yn;
        }
    }

    canvas.setTextColor(ILI9341_WHITE);

    if (!LS_stereo)
    {
        canvas.setCursor((WAVEBOARD_WIDTH / 2) - 12, display_coordinate_y(0));
        canvas.print("MONO");
    }
    else if (sound_id == SOUNDS_MAX)
    {
        canvas.setCursor((WAVEBOARD_WIDTH / 2) - 63, display_coordinate_y(0));
        canvas.print("(STEREO) LEFT CHANNEL");
    }
    else
    {
        canvas.setCursor((WAVEBOARD_WIDTH / 2) - 66, display_coordinate_y(0));
        canvas.print("(STEREO) RIGHT CHANNEL");
    }

    canvas.setTextColor(TEXT_COLOR);
    canvas.setCursor((WAVEBOARD_WIDTH / 2) - 30, Y_FOOTER_TEXT);
    canvas.print("PLAY POINT");

    Draw_XY_lines();
    Update_REC_LED();

    // memo[0] = localtimer; // 530us
    tft.writeRect(LS_CANVAS_X, LS_CANVAS_Y, canvas.width(), canvas.height(), canvas.getBuffer());
    // memo[1] = localtimer; // memo[1] - memo[0] = 44.000us

    // Serial.print("Fill canvas, microseconds:");
    // Serial.print(memo[0]);
    // Serial.print(" Fill display, microseconds:");
    // Serial.println(memo[1] - memo[0]);
}

void DisplayLiveSampler::Update_REC_LED(void)
{
    if (LS_state != REC)
    {
        LS_blink_ON = false;
    }
    else if (LS_blink_timer >= 500)
    {
        LS_blink_ON = !LS_blink_ON;
        LS_blink_timer = 0;
    }

    canvas.drawBitmap(5, display_coordinate_y(0), led_pic, 5, 8, (LS_blink_ON ? ILI9341_RED : RED_OFF));
    canvas.setCursor(display_coordinate_x(2), display_coordinate_y(0));
    canvas.setTextColor((LS_blink_ON ? ILI9341_RED : RED_OFF));
    canvas.print("CAPTURE");
}

void DisplayLiveSampler::Draw_XY_lines(void)
{
    // draw LS_X_sample
    canvas.drawLine(WAVEBOARD_WIDTH / 2, CANVAS_WAVE_0 - 30, WAVEBOARD_WIDTH / 2, CANVAS_WAVE_0 + 30, LS_X_COLOR);

    // draw LS_Y_sample
    if (LS_mode > 1) // loops
    {
        int LS_Y_sample_local = LS_Y_sample;
        if (LS_Y_sample_local < LS_window_A_sample)
        {
            LS_Y_sample_local += LS_buffer_dim;
        }

        //     Caso 0: Y NON rientra nella window
        //     0                                        PQ  (FIFO_dim - 1)
        //     |..................................................|
        //                       A===================B  Y

        //     Caso 1: Y incrementato NON rientra nella window
        //     0     PQ                                    (FIFO_dim - 1)
        //     |    (Y)          A===================B            |     Y

        //     Caso 2: Y incrementato rientra nella window
        //     0     PQ                                    (FIFO_dim - 1)
        //     |    (Y)                                 A=========|=====Y======B

        if (LS_Y_sample_local >= LS_window_A_sample && LS_Y_sample_local <= LS_window_B_sample)
        {
            float LS_Y = LS_Y_sample_local - LS_window_A_sample;
            float LS_W = LS_window_width;
            int LS_Y_sample_local_x = (LS_Y / LS_W) * (WAVEBOARD_WIDTH - 1); //  window) * 127.0;
            canvas.drawLine(LS_Y_sample_local_x, CANVAS_WAVE_0 - 30, LS_Y_sample_local_x, CANVAS_WAVE_0 + 30, LS_Y_COLOR);
        }
    }
}

uint16_t DisplayLiveSampler::Get_wave_color(const int point)
{
    int position = LS_constrain_position(LS_window_A_sample + point * LS_K_wave_color);

    if (position > LS_Q_sample)
    {
        position -= LS_buffer_dim;
    }

    int distance = LS_Q_sample - position;
    uint16_t green = constrain(63 * ((float)(LS_buffer_dim - distance) / (float)LS_buffer_dim), 0, 63);

    if ((distance - LS_wave_poit_distance_0) > 1000000)
    {
        green = 63;
    }

    LS_wave_poit_distance_0 = distance;
    return (31 << 11) + (green << 5); // (red << 11) + (green << 5) + blue
}

void DisplayLiveSampler::LS_show_pointer_frame(const LS_pointer_struct pointer, const bool show)
{
    if (pointer.field_name == field_LS_Menu)
    {
        Frame_by_col_row(X_position_Menu_LS[position_Menu_LS[pointer.menu_element]], LS_ROW_MENU, dimension_voice_Menu_LS[element_Menu_LS[position_Menu_LS[pointer.menu_element]]], show);
    }
    else
    {
        switch (pointer.value_element)
        {
        case value_LS_Play_mode:
            Frame_by_col_row(LS_column_row_element[pointer.value_element][0], LS_column_row_element[pointer.value_element][1], LS_chars_play_mode, show);
            break;

        case value_LS_Feedback:
            Frame_by_col_row(LS_column_row_element[pointer.value_element][0], LS_column_row_element[pointer.value_element][1], LS_chars_feedback, show);
            break;

        case value_LS_Window:
            Frame_by_col_row(LS_column_row_element[pointer.value_element][0], LS_column_row_element[pointer.value_element][1], LS_chars_window, show);
            break;

        case value_LS_Gain:
            Frame_by_col_row(LS_column_row_element[pointer.value_element][0], LS_column_row_element[pointer.value_element][1], 2, show);
            break;
        }
    }
}

FLASHMEM
void DisplayLiveSampler::LS_Display_Confirm_capture_frame(uint8_t value)
{
    const int popup_width = display_coordinate_x(sizeof("REPLACE CAPTURE?") - 1 + 2);
    const int popup_height = display_coordinate_y(1) + 15;
    const int text_x = (320 - popup_width) / 2 + display_coordinate_x(1);
    const int option_y = (240 - popup_height) / 2 + 20;
    const int no_x = text_x + 4 * 6;
    const int yes_x = text_x + 8 * 6;

    Frame_by_pixels_on_RED(no_x, option_y, 2, false);
    Frame_by_pixels_on_RED(yes_x, option_y, 3, false);
    Frame_by_pixels_on_RED(value == 0 ? no_x : yes_x, option_y, value == 0 ? 2 : 3, true);
}
