/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#pragma once

#include <Arduino.h>
#include <utility/dspinst.h>
#include <stdint.h>
#include "config.h"

// computes limit(val, 2**bits)
int16_t Lilla_saturate16(const int32_t &val); // Clamp a signed sample to the 16-bit audio range.

void PrintErrorLocation(const __FlashStringHelper *message, const char *file, int line); // Print an error message and its source location.

#define PRINT_ERROR(message) PrintErrorLocation((message), __FILE__, __LINE__)

void PrintControlPoint(const char *expression, int value, const char *file, int line); // Print a control value and its source location.

#define PRINT_CONTROL_POINT(value) PrintControlPoint(#value, (value), __FILE__, __LINE__)
