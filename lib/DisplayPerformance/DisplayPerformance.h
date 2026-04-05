/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>     // https://learn.adafruit.com/adafruit-gfx-graphics-library/graphics-primitives
#include <Adafruit_ILI9341.h> // 1.5.12 version - Hardware-specific library
#include <AudioStream.h>      // solo per definizione AUDIO_SAMPLE_RATE
#include "DisplayPrimitives.h"

#include "SharedElements.h"
#include "SharedPerformance.h"

class DisplayPerformance
{
private:

public:
    DisplayPerformance() {}

};