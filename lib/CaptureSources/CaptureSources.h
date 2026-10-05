#pragma once

#include "SharedElements.h"
#include "SharedDelay.h"

// Captures reserve ordinary RAW identifiers; only their backing storage is temporary.
constexpr int CAPTURE_SOURCES = INSTRUMENTS;
struct CaptureSource
{
    AudioFileSource audio;
    bool written = false;
};

extern CaptureSource Capture_sources[CAPTURE_SOURCES];
extern volatile bool Capture_learn_key;
extern volatile int Capture_learn_note;
extern int Capture_new_patch;
extern Delay_data_struct Capture_patch_delay;
CaptureSource *Capture_find(int file_id);
bool Capture_pending(int file_id);
bool Capture_read(int file_id, int16_t *destination, int first_sample, int samples);
