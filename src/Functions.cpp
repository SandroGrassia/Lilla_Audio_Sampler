#include "Functions.h"

int16_t Lilla_saturate16(const int32_t &val)
{
    int16_t out;
    int32_t tmp;
    asm volatile("ssat %0, %1, %2" : "=r"(tmp) : "I"(16), "r"(val));
    out = (int16_t)(tmp);
    return out;
}

void PrintErrorLocation(const __FlashStringHelper *message, const char *file, int line)
{
    Serial.print(message);
    Serial.print(file);
    Serial.print(F(":"));
    Serial.println(line);
}

void PrintControlPoint(const char *expression, int value, const char *file, int line)
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
