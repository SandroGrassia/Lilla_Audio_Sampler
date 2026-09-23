#include "CaptureSources.h"
#include <cstring>

CaptureSource Capture_sources[CAPTURE_SOURCES];
volatile bool Capture_learn_key = false;
volatile int Capture_learn_note = -1;
int Capture_new_patch = -1;
Delay_data_struct Capture_patch_delay = {};

CaptureSource *Capture_find(int file_id)
{
    if (file_id < 1 || file_id >= FIRST_RECORDING_FILE)
    {
        return nullptr;
    }
    for (auto &capture : Capture_sources)
    {
        if (capture.audio.file_id == file_id && capture.audio.psram_ptr != nullptr)
        {
            return &capture;
        }
    }
    return nullptr;
}

bool Capture_pending(int file_id)
{
    const auto *capture = Capture_find(file_id);
    return capture != nullptr && !capture->written;
}

bool Capture_read(int file_id, int16_t *destination, int first_sample, int samples)
{
    const auto *capture = Capture_find(file_id);
    if (capture == nullptr || destination == nullptr || first_sample < 0 || samples < 0 || static_cast<uint32_t>(first_sample) > capture->audio.samples || static_cast<uint32_t>(samples) > capture->audio.samples - first_sample)
    {
        return false;
    }
    memcpy(destination, capture->audio.psram_ptr + first_sample, static_cast<size_t>(samples) * sizeof(int16_t));
    return true;
}
