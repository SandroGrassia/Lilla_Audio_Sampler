#pragma once

#include <stdint.h>

// Middle-C sine with zero endpoints, mono PCM16 at 44100 Hz, stored in firmware Flash.
extern const int16_t zeroraw[43996];

// Preserve a valid 0.raw, or create it from firmware. Call with players stopped.
// With require_sine, replace any different content with the bundled sine.
bool ZeroRaw_ensure_file(bool require_sine = false);
