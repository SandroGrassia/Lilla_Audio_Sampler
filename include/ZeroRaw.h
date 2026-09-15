#pragma once

#include <stdint.h>

// First second of the bundled mono PCM16 recording, stored only in firmware Flash.
extern const int16_t zeroraw[44100];

// Preserve a valid 0.raw, or create it from firmware. Call with players stopped.
bool ZeroRaw_ensure_file(void);
