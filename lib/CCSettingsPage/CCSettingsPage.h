/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 */

#pragma once

#include <Arduino.h>
#include "config.h"

extern uint8_t CC_Sound_gain_cache[INSTRUMENTS]; // Initialized during hardware startup.

void Handle_CC_settings(void); // Process the CC page after Setup in the main loop.
void Golive_CC_SETTINGS(void); // Enter the CC page and initialize its display and selection.
byte CC_Save_settings(void); // Persist Control Change assignments.
byte CC_Read_all_Sound_gain(void); // Reload Control Change assignments from FRAM.
