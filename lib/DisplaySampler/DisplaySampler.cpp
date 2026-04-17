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
void DisplaySampler::DS_page(int recording)
{
    tft.fillScreen(ILI9341_BLACK);

    Backgorund_red(DS_column_row_SAMPLER[0], DS_column_row_SAMPLER[1], 7);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(display_coordinate_x(DS_column_row_SAMPLER[0]), display_coordinate_y(DS_column_row_SAMPLER[1]));
    tft.print("SAMPLER");

    tft.setCursor(display_coordinate_x(DS_column_row_VOLUME_LABEL[0]), display_coordinate_y(DS_column_row_VOLUME_LABEL[1]));
    tft.setTextColor(TEXT_COLOR);
    tft.print("VOLUME");
    DS_update_volume();

    tft.setCursor(display_coordinate_x(DS_column_row_AUDIO_MEMORY[0]), display_coordinate_y(DS_column_row_AUDIO_MEMORY[1]));
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

    tft.setCursor(display_coordinate_x(DS_column_row_FREE_RECORDINGS[0]), display_coordinate_y(DS_column_row_FREE_RECORDINGS[1]));
    tft.setTextColor(TEXT_COLOR);
    tft.print("- FREE FOR RECORDINGS");
    DS_available_memory();

    tft.setCursor(display_coordinate_x(DS_column_row_FREE_RAW_FILES[0]), display_coordinate_y(DS_column_row_FREE_RAW_FILES[1]));
    tft.setTextColor(TEXT_COLOR);
    tft.print("- FREE FOR RAW FILES ");
    DS_raw_available_memory();

    DS_Recording_description(recording, true);
    DS_sampler_IO();
}

FLASHMEM
void DisplaySampler::DS_sampler_IO(void)
{
    tft.setCursor(DS_VUMETER_BAR_X + 1, DS_VUMETER_BAR_Y + 4);
    tft.setTextColor(TEXT_COLOR);
    tft.print("L");
    tft.setCursor(DS_VUMETER_BAR_X + DS_VUMETER_BAR_DX, DS_VUMETER_BAR_Y + 4);
    tft.print("R");

    tft.drawRect(DS_VUMETER_BAR_X - 1, DS_VUMETER_BAR_Y - BAR_ELEMENTS - 1, DS_VUMETER_BAR_DISTANCE + 2, BAR_ELEMENTS + 2, 0x03E0);
    tft.drawRect(DS_VUMETER_BAR_X - 1 + DS_VUMETER_BAR_DX, DS_VUMETER_BAR_Y - BAR_ELEMENTS - 1, DS_VUMETER_BAR_DISTANCE + 2, BAR_ELEMENTS + 2, 0x03E0);
    DS_bar(0, 0);
    DS_bar(1, 0);

    tft.drawRect(DS_START_X, DS_START_Y, 31, 15, ILI9341_GREEN);
    tft.drawBitmap(DS_START_X - 19, DS_START_Y + 4, DS_freccia, 19, 7, ILI9341_GREEN);
    tft.setCursor(DS_START_X - 63, DS_START_Y + 4);
    tft.setTextColor(TEXT_COLOR);
    tft.print("LINE-IN");

    tft.setCursor(DS_START_X + 4, DS_START_Y + 17);
    tft.setTextColor(TEXT_COLOR);
    tft.print("GAIN");

    DS_sampler_frame(true);
    DS_sampler_txt(false);
    DS_show_gain();
}

void DisplaySampler::DS_bar(int channel, int value)
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
            tft.drawFastHLine(X0, DS_VUMETER_BAR_Y - i, DS_VUMETER_BAR_DISTANCE, DS_calc_bar_color(value_float));
        }
    }
    else if (value < DS_VU_meter_value_old[channel])
    {
        for (auto i = value + 1; i <= DS_VU_meter_value_old[channel]; ++i)
        {
            tft.drawFastHLine(X0, DS_VUMETER_BAR_Y - i, DS_VUMETER_BAR_DISTANCE, ILI9341_BLACK);
        }
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
void DisplaySampler::DS_line_out(bool visible)
{
    tft.drawBitmap(DS_START_X + 37, DS_START_Y + 7, DS_freccia_gomito, 13, 23, (visible ? ILI9341_GREEN : GREEN_OFF));
    tft.setCursor(DS_START_X + 55, DS_START_Y + 23);
    tft.setTextColor((visible ? TEXT_COLOR : TEXT_OFF_COLOR));
    tft.print("LINE-OUT");
}

FLASHMEM
void DisplaySampler::DS_sampler_frame(bool visible)
{
    tft.drawBitmap(DS_START_X + 31, DS_START_Y + 4, DS_freccia, 19, 7, (visible ? ILI9341_GREEN : ILI9341_BLACK));
    tft.drawRoundRect(DS_START_X + 50, DS_START_Y, 51, 15, 3, (visible ? ILI9341_GREEN : ILI9341_BLACK));
}

FLASHMEM
void DisplaySampler::DS_sampler_txt(bool color)
{
    tft.setCursor(DS_START_X + 58, DS_START_Y + 4);
    tft.setTextColor(color ? ILI9341_RED : RED_OFF);
    tft.print("RECORD");
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
void DisplaySampler::DS_Recording_description(int recording, bool led)
{
    tft.setCursor(led ? display_coordinate_x(DS_column_row_RECORDING_LED[0]) : display_coordinate_x(DS_column_row_RECORDING[0]), display_coordinate_y(DS_column_row_RECORDING[1]));
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

    if (led)
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
    Cancel_text_reset_cursor(DS_START_X + 4, DS_START_Y + 4, 4);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print(DS_gain / 20.0f, 2);
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
    else
    {
        switch (pointer.value_element)
        {
        case value_DS_Recording:
            Frame_by_col_row(DS_column_row_RECORDING[0] + 10, DS_column_row_RECORDING[1], DS_chars_recording, show);
            break;
        }
    }
}