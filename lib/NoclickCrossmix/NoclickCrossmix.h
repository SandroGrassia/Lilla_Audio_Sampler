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
public:
    NoclickCrossmix(void) = default; // Keep generation state independent of destination storage.

    bool Make(int file_id, int32_t A_Flash_sample, int32_t B_Flash_sample, uint16_t delta_Noclick, int16_t *destination); // Generate directly into the caller-owned destination; return false if a source read fails.
};