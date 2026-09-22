/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#include "DisplaySampler.h"

FLASHMEM
void DisplaySampler::DS_confirm_EXIT_from_DS(void)
{
    const int L_POPUP    = 106;
    const int H_POPUP    = 47;
    const int X_POPUP    = (320 - L_POPUP) / 2;
    const int Y_POPUP    = (240 - H_POPUP) / 2;
    const int Y_POPUP_TXT = 10;
    const int Y_POPUP_OPT = 30;

    tft.fillRoundRect(X_POPUP, Y_POPUP, L_POPUP, H_POPUP, 4, ILI9341_RED);
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
void DisplaySampler::DS_page_upper(void)
{
    const int bottom = display_coordinate_y(DS_ROW_MEMORY) - 4;
    tft.fillRect(0, 0, 320, bottom, ILI9341_BLACK);

    Backgorund_red(DS_column_row_SAMPLER[0], DS_column_row_SAMPLER[1], 7);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(display_coordinate_x(DS_column_row_SAMPLER[0]), display_coordinate_y(DS_column_row_SAMPLER[1]));
    tft.print("SAMPLER");

    tft.setCursor(display_coordinate_x(DS_column_row_VOLUME_LABEL[0]), display_coordinate_y(DS_column_row_VOLUME_LABEL[1]));
    tft.setTextColor(TEXT_COLOR);
    tft.print("VOLUME");
    DS_update_volume();
}

FLASHMEM
void DisplaySampler::DS_page_lower(int recording)
{
    const int top = display_coordinate_y(DS_ROW_MEMORY) - 4;
    tft.fillRect(0, top, 320, 240 - top, ILI9341_BLACK);

    tft.setCursor(display_coordinate_x(DS_column_row_AUDIO_MEMORY[0]), display_coordinate_y(DS_column_row_AUDIO_MEMORY[1]));
    tft.setTextColor(TEXT_COLOR);
    tft.print("AUDIO MEMORY ");
    tft.setTextColor(ILI9341_WHITE);
    tft.print(verified_flash_memory_MB);
    Show_measure_unit("MB", 2);
    tft.setTextColor(ILI9341_WHITE);
    tft.print(" (");
    tft.print(Get_flash_size() / 88100.0f);
    Show_measure_unit("sec", 3);
    tft.setTextColor(ILI9341_WHITE);
    tft.print(")");

    tft.setCursor(display_coordinate_x(DS_column_row_FREE_RECORDINGS[0]), display_coordinate_y(DS_column_row_FREE_RECORDINGS[1]));
    tft.setTextColor(TEXT_COLOR);
    tft.print("- FREE FOR RECORDINGS");
    DS_available_memory();

    tft.setCursor(display_coordinate_x(DS_column_row_FREE_RAW_FILES[0]), display_coordinate_y(DS_column_row_FREE_RAW_FILES[1]));
    tft.setTextColor(TEXT_COLOR);
    tft.print("- FREE FOR RAW FILES ");
    DS_raw_available_memory();

    if (DS_recording_controls_visible)
    {
        DS_sampler_IO();
    }
    else
    {
        DS_Recording_description(recording, true, false);
    }
}

FLASHMEM
void DisplaySampler::DS_set_recording_controls(bool visible)
{
    if (DS_recording_controls_visible == visible)
    {
        return;
    }
    DS_recording_controls_visible = visible;
    const int top = display_coordinate_y(DS_ROW_RECORDING) - 4;
    tft.fillRect(0, top, 320, 240 - top, ILI9341_BLACK);
    if (visible)
    {
        DS_recording_led_visible = false;
        DS_recording_led_redraw = true;
        DS_sampler_IO();
    }
    else
    {
        DS_Recording_description(recording, true, false);
    }
}

FLASHMEM
void DisplaySampler::DS_sampler_IO(void)
{
    if (!DS_recording_controls_visible)
    {
        return;
    }
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(DS_ROW_RECORDING));
    tft.setTextColor(ILI9341_RED);
    tft.print("PAUSE+REC");
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(DS_ROW_GAIN));
    tft.setTextColor(TEXT_COLOR);
    tft.print("LINE IN GAIN");
    DS_show_gain();
    for (int channel = 0; channel < 2; ++channel)
    {
        const int y = display_coordinate_y(channel == 0 ? DS_ROW_LEVEL_L : DS_ROW_LEVEL_R);
        tft.setCursor(display_coordinate_x(0), y);
        tft.setTextColor(TEXT_COLOR);
        tft.print(channel == 0 ? "LEVEL L" : "LEVEL R");
        tft.fillRect(DS_VUMETER_BAR_X, y, BAR_ELEMENTS * DS_VUMETER_STEP_WIDTH, DS_VUMETER_BAR_HEIGHT, ILI9341_BLACK);
        tft.drawRect(DS_VUMETER_BAR_X - 1, y - 1, BAR_ELEMENTS * DS_VUMETER_STEP_WIDTH + 2, DS_VUMETER_BAR_HEIGHT + 2, 0x03E0);
        DS_VU_meter_value_old[channel] = 0;
    }
}

void DisplaySampler::DS_bar(int channel, int value)
{
    if (!DS_recording_controls_visible || channel < 0 || channel > 1)
    {
        return;
    }
    value = constrain(value, 0, BAR_ELEMENTS);
    const int y = display_coordinate_y(channel == 0 ? DS_ROW_LEVEL_L : DS_ROW_LEVEL_R);
    if (value > DS_VU_meter_value_old[channel])
    {
        for (int i = DS_VU_meter_value_old[channel] + 1; i <= value; ++i)
        {
            tft.fillRect(DS_VUMETER_BAR_X + (i - 1) * DS_VUMETER_STEP_WIDTH, y, DS_VUMETER_STEP_WIDTH, DS_VUMETER_BAR_HEIGHT, DS_calc_bar_color(static_cast<float>(i) / BAR_ELEMENTS));
        }
    }
    else if (value < DS_VU_meter_value_old[channel])
    {
        tft.fillRect(DS_VUMETER_BAR_X + value * DS_VUMETER_STEP_WIDTH, y, (DS_VU_meter_value_old[channel] - value) * DS_VUMETER_STEP_WIDTH, DS_VUMETER_BAR_HEIGHT, ILI9341_BLACK);
    }
    DS_VU_meter_value_old[channel] = value;
}

uint16_t DisplaySampler::DS_calc_bar_color(float value)
{
    const float soglia = 0.5;
    uint16_t red   = (value >= soglia ? 31 : 31.0f * (value / soglia));
    uint16_t green = (value <= soglia ? 63 : 63.0f * (1.2f - value) / (1.2f - soglia));
    uint16_t blue  = 0;
    return (red << 11) + (green << 5) + blue;
}

FLASHMEM
void DisplaySampler::DS_sampler_txt(bool color)
{
    if (!DS_recording_controls_visible)
    {
        return;
    }
    Cancel_text_reset_cursor(display_coordinate_x(0), display_coordinate_y(DS_ROW_RECORDING), 9);
    tft.setTextColor(color ? ILI9341_RED : RED_OFF);
    tft.print("RECORDING");
}

FLASHMEM
void DisplaySampler::DS_available_memory(void)
{
    Cancel_text_reset_cursor(display_coordinate_x(DS_column_row_available_memory[0]), display_coordinate_y(DS_column_row_available_memory[1]), DS_chars_available_memory);
    tft.setTextColor(ILI9341_WHITE);
    tft.print(VFS_Get_packets_free() * 0.743, 1);
    Show_measure_unit("sec", 3);
}

FLASHMEM
void DisplaySampler::DS_raw_available_memory(void)
{
    Cancel_text_reset_cursor(display_coordinate_x(DS_column_row_raw_available_memory[0]), display_coordinate_y(DS_column_row_raw_available_memory[1]), DS_chars_raw_available_memory);
    tft.setTextColor(ILI9341_WHITE);
    tft.print((Get_flash_size() - Get_flash_occupation() - FLASH_FREE_SPACE) / 88200.0f);
    Show_measure_unit("sec", 3);
}

FLASHMEM
void DisplaySampler::DS_hide_recording(void)
{
    Cancel_text(display_coordinate_x(DS_column_row_POPUP_LINE_0[0]), display_coordinate_y(DS_column_row_POPUP_LINE_0[1]), 18);
    Cancel_text(display_coordinate_x(DS_column_row_POPUP_LINE_1[0]), display_coordinate_y(DS_column_row_POPUP_LINE_1[1]), 29);
    Cancel_text(display_coordinate_x(DS_column_row_POPUP_LINE_2[0]), display_coordinate_y(DS_column_row_POPUP_LINE_2[1]), 18);
    Cancel_text(display_coordinate_x(0), display_coordinate_y(DS_ROW_LENGTH_STEREO),  18);
    Cancel_text(display_coordinate_x(0), display_coordinate_y(DS_ROW_VOLUME_STEREO),  18);
}

FLASHMEM
void DisplaySampler::DS_advice_delete(bool value)
{
    tft.setTextColor((value ? ILI9341_YELLOW : ILI9341_BLACK));
    tft.setCursor(display_coordinate_x(DS_column_row_PLEASE_WAIT[0]), display_coordinate_y(DS_column_row_PLEASE_WAIT[1]));
    tft.print("PLEASE WAIT");
}

FLASHMEM
void DisplaySampler::DS_advice_no_conversion(int DS_export, bool value)
{
    tft.setTextColor((value ? TEXT_COLOR : ILI9341_BLACK));
    tft.setCursor(display_coordinate_x(DS_column_row_POPUP_LINE_0[0]), display_coordinate_y(DS_column_row_POPUP_LINE_0[1]));
    tft.print("UNABLE TO CREATE RAW FILE");
    tft.setTextColor((value ? ILI9341_YELLOW : ILI9341_BLACK));
    tft.setCursor(display_coordinate_x(DS_column_row_POPUP_LINE_1[0]), display_coordinate_y(DS_column_row_POPUP_LINE_1[1]));

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
void DisplaySampler::DS_conversion_options(int file_L_RAW, int file_R_RAW, int DS_export)
{
    tft.fillRect(0, 120, 320, 120, ILI9341_BLACK);
    tft.setTextColor(TEXT_COLOR);
    tft.setCursor(display_coordinate_x(DS_column_row_POPUP_LINE_0[0]), display_coordinate_y(DS_column_row_POPUP_LINE_0[1]));
    tft.print("OPTIONS:");

    tft.setCursor(display_coordinate_x(DS_column_row_POPUP_LINE_1[0]), display_coordinate_y(DS_column_row_POPUP_LINE_1[1]));
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
        tft.setCursor(display_coordinate_x(DS_column_row_POPUP_LINE_2[0]), display_coordinate_y(DS_column_row_POPUP_LINE_2[1]));
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
void DisplaySampler::DS_export_options(int file_L_RAW, int file_R_RAW, int DS_export)
{
    tft.fillRect(0, 120, 320, 120, ILI9341_BLACK);
    tft.setTextColor(TEXT_COLOR);
    tft.setCursor(display_coordinate_x(DS_column_row_POPUP_LINE_0[0]), display_coordinate_y(DS_column_row_POPUP_LINE_0[1]));
    tft.print("SD EXPORT OPTIONS:");

    tft.setCursor(display_coordinate_x(DS_column_row_POPUP_LINE_1[0]), display_coordinate_y(DS_column_row_POPUP_LINE_1[1]));
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
    tft.print(" (");
    tft.print(Recording[recording].bytes >> 10);
    tft.print("kB)");

    if (Recording[recording].stereo && DS_export == 2)
    {
        tft.setTextColor(TEXT_COLOR);
        tft.setCursor(display_coordinate_x(DS_column_row_POPUP_LINE_2[0]), display_coordinate_y(DS_column_row_POPUP_LINE_2[1]));
        tft.print("- RIGHT ");

        tft.setTextColor(ILI9341_WHITE);
        tft.print(name_file[recording + FIRST_RECORDING_FILE + 1]);
        tft.print(" --> ");
        tft.print(" (");
        tft.print(Recording[recording].bytes >> 10);
        tft.print("kB)");
    }
}

FLASHMEM
void DisplaySampler::DS_Recording_description(int recording, bool led, bool update_header)
{
    if (DS_recording_controls_visible)
    {
        return;
    }
    DS_recording_led_visible = led;
    DS_recording_led_redraw = true;
    tft.setCursor(led ? display_coordinate_x(DS_column_row_RECORDING_LED[0]) : display_coordinate_x(DS_column_row_RECORDING[0]), display_coordinate_y(DS_column_row_RECORDING[1]));
    tft.setTextColor(TEXT_COLOR);
    tft.print("RECORDING ");

    tft.setCursor(display_coordinate_x(DS_column_row_recording[0]), display_coordinate_y(DS_column_row_recording[1])); // Align the value with its pointer frame, independently of the LED.

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

    tft.setCursor(display_coordinate_x(DS_column_row_FILE_MONO[0]), display_coordinate_y(DS_column_row_FILE_MONO[1]));
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

    if (recording >= 0 && Recording[recording].stereo)
    {
        tft.setCursor(display_coordinate_x(DS_column_row_FILE_RIGHT[0]), display_coordinate_y(DS_column_row_FILE_RIGHT[1]));
        tft.setTextColor(TEXT_COLOR);
        tft.print("RIGHT_FILE ");
        tft.setTextColor(ILI9341_WHITE);
        tft.print(name_file[2 * recording + FIRST_RECORDING_FILE + 1]);
    }

    DS_recording_seconds();

    if (led && update_header)
    {
        DS_update_volume();
    }
}

FLASHMEM
void DisplaySampler::DS_recording_seconds(void)
{
    if (recording < 0)
    {
        return;
    }

    float row = Recording[recording].stereo ? DS_ROW_LENGTH_STEREO : DS_ROW_LENGTH_MONO;

    tft.setCursor(display_coordinate_x(DS_column_row_LENGTH[0]), display_coordinate_y(row));
    tft.setTextColor(TEXT_COLOR);
    tft.print("LENGTH ");
    tft.setTextColor(ILI9341_WHITE);
    tft.print(Recording[recording].seconds, 1);
    Show_measure_unit("sec", 3);
}

FLASHMEM
void DisplaySampler::DS_update_recording_seconds(float value)
{
    float row = Recording[recording].stereo ? DS_ROW_LENGTH_STEREO : DS_ROW_LENGTH_MONO;
    Cancel_text_reset_cursor(display_coordinate_x(DS_column_row_length[0]), display_coordinate_y(row), DS_chars_length);
    tft.setTextColor(ILI9341_WHITE);
    tft.print(value / 1000.0f, 1);
    Show_measure_unit("sec", 3);
}

FLASHMEM
void DisplaySampler::DS_volume(void)
{
    float row = (recording < 0) ? DS_ROW_VOLUME_NONE : (Recording[recording].stereo ? DS_ROW_VOLUME_STEREO : DS_ROW_VOLUME_MONO);

    tft.setCursor(display_coordinate_x(0), display_coordinate_y(row));
    tft.setTextColor(TEXT_COLOR);
    tft.print("VOLUME ");
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(volume_patch / 20.0f, 2);
}

FLASHMEM
void DisplaySampler::DS_update_volume(bool adj)
{
    Cancel_text_reset_cursor(display_coordinate_x(DS_column_row_volume[0]), display_coordinate_y(DS_column_row_volume[1]), DS_chars_volume);
    tft.setTextColor(adj ? ILI9341_YELLOW : ILI9341_WHITE);
    tft.print(volume_patch / 20.0f, 2);
}

FLASHMEM
void DisplaySampler::DS_show_gain(void)
{
    if (!DS_recording_controls_visible)
    {
        return;
    }
    Cancel_text_reset_cursor(display_coordinate_x(DS_COLUMN_GAIN), display_coordinate_y(DS_ROW_GAIN), 2);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(Line_in_gain + 1);
}

FLASHMEM
void DisplaySampler::DS_menu(void)
{
    auto position = 0;

    Delete_text_row(1);
    Delete_text_row(2);
    tft.setTextColor(MENU_COLOR);

    for (auto element = 0; element < DS_menu_elements; ++element)
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
            element_Menu_DS[position]    = element;
            position_Menu_DS[element]    = position;
            tft.setCursor(display_coordinate_x(X_position_Menu_DS[position]), display_coordinate_y(Y_position_Menu_DS[position]));
            tft.print(Menu_DS_char[element]);
            ++position;
        }
    }
}

FLASHMEM
void DisplaySampler::DS_frame_menu(int position)
{
    Frame_by_col_row(X_position_Menu_DS[DS_frame_menu_position_0], Y_position_Menu_DS[DS_frame_menu_position_0], dimension_voice_Menu_DS[element_Menu_DS[DS_frame_menu_position_0]], false);
    Frame_by_col_row(X_position_Menu_DS[position], Y_position_Menu_DS[position], dimension_voice_Menu_DS[element_Menu_DS[position]], true);
    choice_DS_menu = element_Menu_DS[position];
    DS_frame_menu_position_0 = position;
}

void DisplaySampler::DS_show_pointer_frame(const DS_pointer_struct pointer, const bool show)
{
    if (pointer.field_name == field_DS_Menu)
    {
        const int position = position_Menu_DS[pointer.menu_element];
        Frame_by_col_row(X_position_Menu_DS[position], Y_position_Menu_DS[position], dimension_voice_Menu_DS[element_Menu_DS[position]], show);
    }
    else if (pointer.value_element == value_DS_Recording && !DS_recording_controls_visible)
    {
        Frame_by_col_row(DS_column_row_recording[0], DS_column_row_recording[1], DS_chars_recording, show);
    }
    else if (pointer.value_element == value_DS_Gain && DS_recording_controls_visible)
    {
        Frame_by_col_row(DS_COLUMN_GAIN, DS_ROW_GAIN, 2, show);
    }
}
