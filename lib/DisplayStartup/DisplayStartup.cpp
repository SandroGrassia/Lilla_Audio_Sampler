/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#include "DisplayStartup.h"
#include "SharedElements.h"

void DisplayStartup::Lilla_cover_slow(void)
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
    delay(6000);
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

void DisplayStartup::Lilla_cover_saturate(void)
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

void DisplayStartup::Logo(float light)
{
    tft.drawBitmap(0 + Logo_position_DX, 42 + Logo_position_DY, LOGO_0, 168, 49, Calc_color(ILI9341_YELLOW, light));
    tft.drawBitmap(0 + Logo_position_DX, 14 + Logo_position_DY, LOGO_1, 8, 26, Calc_color(ILI9341_RED, light));
    tft.drawBitmap(32 + Logo_position_DX, 9 + Logo_position_DY, LOGO_2, 16, 31, Calc_color(ILI9341_RED, light));
    tft.drawBitmap(48 + Logo_position_DX, 20 + Logo_position_DY, LOGO_3, 16, 20, Calc_color(ILI9341_MAGENTA, light));
    tft.drawBitmap(88 + Logo_position_DX, 0 + Logo_position_DY, LOGO_4, 8, 40, Calc_color(ILI9341_MAGENTA, light));
    tft.drawBitmap(-1 + Logo_position_DX, 100 + Logo_position_DY, audio_sampler, 168, 18, Calc_color(ILI9341_WHITE, light));
}

void DisplayStartup::Cover_text(float light)
{
    tft.setTextColor(Calc_color(TEXT_COLOR, light)); // 16-bit ('565') color settings
    tft.setCursor(display_coordinate_x(13), Text_position_DY);
    tft.print("UPDATE ");
    tft.setTextColor(Calc_color(ILI9341_WHITE, light)); // 16-bit ('565') color settings
    tft.print(FIRMWARE_VERSION);

    tft.setTextColor(Calc_color(TEXT_COLOR, light)); // 16-bit ('565') color settings
    tft.setCursor(display_coordinate_x(13), Text_position_DY + 11);
    tft.print("AUDIO REPOSITORY ");
    tft.setTextColor(Calc_color(ILI9341_WHITE, light)); // 16-bit ('565') color settings
    tft.print(verified_flash_memory_MB);
    tft.print("MB");

    tft.setTextColor(Calc_color(TEXT_COLOR, light)); // 16-bit ('565') color settings
    tft.setCursor(display_coordinate_x(13), Text_position_DY + 22);
    tft.print("AUDIO RAM ");
    tft.setTextColor(Calc_color(ILI9341_WHITE, light)); // 16-bit ('565') color settings
    tft.print(PSRAM_TOTAL_SAMPLES * sizeof(int16_t) / (1UL << 20));
    tft.print("MB");

    tft.setTextColor(Calc_color(TEXT_COLOR, light)); // 16-bit ('565') color settings
    tft.setCursor(display_coordinate_x(13), Text_position_DY + 33);
    tft.print("LIVE SAMPLER CACHE ");
    tft.setTextColor(Calc_color(ILI9341_WHITE, light)); // 16-bit ('565') color settings
    tft.print("16MB");
}

uint16_t DisplayStartup::Calc_color(uint16_t color_peak, float light) // 16-bit ('565') color settings
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
