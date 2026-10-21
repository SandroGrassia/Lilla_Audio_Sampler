#pragma once

#include <Arduino.h>
#include <stdint.h>

// Middle-C sine with zero endpoints, mono PCM16 at 44100 Hz, stored in firmware Flash.
PROGMEM extern const int16_t zeroraw[43996];

// Used directly by LillaSerialFlashFile only when external 0.raw is absent.
