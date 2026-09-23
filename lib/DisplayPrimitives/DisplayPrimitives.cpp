/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#include "DisplayPrimitives.h"
#include <string.h>

void Cancel_text_reset_cursor(const int X, const int Y, const int N)
{
    Cancel_text(X, Y, N);
    tft.setCursor(X, Y);
}

void Cancel_text(const int X, const int Y, const int N)
{
    tft.fillRect(X, Y, (N * 6), 8, ILI9341_BLACK); // fillRect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint16_t color);
}

int display_coordinate_y(const float row)
{
    return 4 + (15.0 * row);
}

int display_coordinate_x(const float col)
{
    return 4 + (6.0 * col);
}

void Frame_by_col_row(const float col, const float row, const int chars, const int high, const bool show)
{
    Frame_by_pixels(display_coordinate_x(col), display_coordinate_y(row), chars, high, show);
}

void Frame_by_pixels(const int X, const int Y, const int chars, const int high, const bool show)
{
    tft.drawRect(X - 4, Y - 4, (6 * chars) + 7, 4 + (15.0 * high), (show ? FRAME_COLOR : ILI9341_BLACK)); // drawRect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint16_t color)
}


void Frame_by_col_row(const float col, const float row, const int chars, const bool show)
{
    Frame_by_pixels(display_coordinate_x(col), display_coordinate_y(row), chars, show);
}

void Frame_by_pixels(const int X, const int Y, const int chars, const bool show)
{
    tft.drawRect(X - 4, Y - 4, (6 * chars) + 7, Frame_heigh, (show ? FRAME_COLOR : ILI9341_BLACK)); // drawRect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint16_t color)
}

void Frame_by_pixels_on_RED(const int X, const int Y, const int chars, const bool show)
{
    tft.drawRect(X - 4, Y - 4, (6 * chars) + 7, Frame_heigh, (show ? ILI9341_WHITE : ILI9341_RED)); // drawRect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint16_t color)
}

void Show_popup_text(const char *text, uint16_t text_color, uint16_t filler_color, int y_offset)
{
    Show_popup_text(text, "", text_color, filler_color, y_offset);
}

void Show_popup_text(const char *first_line, const char *second_line, uint16_t text_color, uint16_t filler_color, int y_offset)
{
    const size_t first_length = strlen(first_line);
    const size_t second_length = strlen(second_line);
    const bool two_lines = second_length > 0;
    const size_t longest_line = first_length > second_length ? first_length : second_length;

    const int L_POPUP = display_coordinate_x(longest_line + 2);
    const int H_POPUP = display_coordinate_y(1) + (two_lines ? 15 : 0);

    const int X_POPUP = (320 - L_POPUP) / 2;
    const int Y_POPUP = (240 - H_POPUP) / 2 + y_offset;

    tft.fillRoundRect(X_POPUP, Y_POPUP, L_POPUP, H_POPUP, 4, filler_color);
    tft.setTextColor(text_color);
    tft.setCursor(X_POPUP + display_coordinate_x(1), Y_POPUP + 5);
    tft.print(first_line);

    if (two_lines)
    {
        tft.setCursor(X_POPUP + display_coordinate_x(1), Y_POPUP + 20);
        tft.print(second_line);
    }
}

void Backgorund_red(const float col, const float row, const int chars)
{
    tft.fillRect(display_coordinate_x(col) - 4, display_coordinate_y(row) - 2, (6 * chars) + 7, 11, 0x9000); // fillRect(uint16_t x0, uint16_t y0, uint16_t width, uint16_t heigh, uint16_t color);
}

void Show_measure_unit(const char *what, const int lenght)
{
    tft.setTextColor(ILI9341_ORANGE);
    for (auto i = 0; i < lenght; ++i)
    {
        tft.print(*(what + i));
    }
}

void Delete_text_row(const float row)
{
    tft.fillRect(0, display_coordinate_y(row) - 4, 320, 15, ILI9341_BLACK); // fillRect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint16_t color);
}

void Confirm_frame_on_RED(int X, int Y, int chars, bool print)
{
    Frame_by_pixels_on_RED(X, Y, chars, print); // Frame_by_pixels(X, Y, (6 * chars) + 7, print);
}

void Confirm_frame(int X, int Y, int chars, bool print)
{
    Frame_by_pixels(X, Y, chars, print); // Frame_by_pixels(X, Y, (6 * chars) + 7, print);
}

