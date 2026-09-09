/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

// NoclickCrossmix.h crea il vettore di cross-mix per loop Forward o loop Reverse

#pragma once

#include <Arduino.h>
#include <SerialFlash.h>
#include "SharedElements.h"
#include "SharedSampler.h"
#include "LillaSerialFlash.h"
#include "config.h"

class NoclickCrossmix
{
private:
    int16_t Noclick[NOCLICK_DIM];

public:
    NoclickCrossmix(void) {}

    int16_t *get_pointer(void);
    bool Make(int file_id, int32_t A_Flash_sample, int32_t B_Flash_sample, uint16_t delta_Noclick);
    bool Make(int file_id, int32_t A_Flash_sample, int32_t B_Flash_sample, uint16_t delta_Noclick, int16_t *destination);
};