/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>     // https://learn.adafruit.com/adafruit-gfx-graphics-library/graphics-primitives
#include <Adafruit_ILI9341.h> // 1.5.12 version - Hardware-specific library
#include "GlobalDisplayManager.h"
#include "SharedElements.h"
#include "DisplayPrimitives.h"
#include "SharedDelay.h"

class DisplayDelay
{
private:
static constexpr int Delay_ROW_BASE = 6;
  
public:
    DisplayDelay() {}

    void D_show_page(void);
    void D_sounds(void);
    void D_delay(void);
    void D_read_gain(void); // feedback
    void D_delay_LR(void);
    void D_modulation_type(void);
    void D_modulation_frequency(void);
    void D_modulation_depth(void); // index
    void D_modulation_phase_LR(void);
    void D_disabled(void);

};