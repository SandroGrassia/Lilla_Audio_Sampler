/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#pragma once

#include "DisplayPrimitives.h"

// Startup logo and cover animations.
class DisplayStartup
{
private:
    // Logo
    void Logo(const float light);
    void Cover_text(const float light);
    uint16_t Calc_color(uint16_t color_peak, float light);
    static constexpr int Logo_position_DX = 80; // pixel
    static constexpr int Logo_position_DY = 45; // pixel
    static constexpr int Text_position_DY = 175; // pixel

public:
    DisplayStartup() {}

    void Lilla_cover_slow(void);
    void Lilla_cover_saturate(void);
};
