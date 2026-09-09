/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#pragma once

#include <Arduino.h>
#include <SerialFlash.h>
#include "SharedLiveSampler.h"
#include "LillaSerialFlash.h"
#include "config.h"

class WavetableManager
{
public:
    static constexpr int WAVETABLE_DIM = 674;
    static constexpr uint16_t FULL_WAVETABLE_DIM = 2 * WAVETABLE_DIM;

private:
    static int16_t cache[WAVETABLE_DIM];

public:
    WavetableManager(void) = default; // Keep generation state independent of destination storage.

    bool Make(int file_id, int8_t mode, int A_Flash_sample, int B_Flash_sample, uint16_t delta_Noclick, int16_t *p_Noclick, int16_t *destination); // Generate directly into the caller-owned destination; return false if a source read fails.
};