/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

// https://github.com/PaulStoffregen/cores/blob/master/teensy4/extmem.c

#pragma once

#include <Arduino.h>
#include "config.h"

class PsramManager
{
public:
PsramManager (void) {}

int16_t* New_samples_array(uint32_t dimension);
bool Remove_samples_array(int16_t* start_point);
};