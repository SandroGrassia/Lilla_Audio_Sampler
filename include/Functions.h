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
static inline int16_t Lilla_saturate16(const int32_t &val) __attribute__((always_inline, unused));
static inline int16_t Lilla_saturate16(const int32_t &val)
{
    int16_t out;
    int32_t tmp;
    asm volatile("ssat %0, %1, %2" : "=r"(tmp) : "I"(16), "r"(val));
    out = (int16_t)(tmp);
    return out;
}

static inline void PrintErrorLocation(const __FlashStringHelper *message, const char *file, int line)
{
    Serial.print(message);
    Serial.print(file);
    Serial.print(F(":"));
    Serial.println(line);
}

#define PRINT_ERROR(message) PrintErrorLocation((message), __FILE__, __LINE__)

static inline void PrintControlPoint(
    const char *expression,
    int value,
    const char *file,
    int line)
{
    Serial.print(F("CONTROL POINT - "));
    Serial.print(file);
    Serial.print(F(" - line "));
    Serial.print(line);
    Serial.print(F(" - "));
    Serial.print(expression);
    Serial.print(F(": "));
    Serial.println(value);
}

#define PRINT_CONTROL_POINT(value) \
    PrintControlPoint(#value, (value), __FILE__, __LINE__)
