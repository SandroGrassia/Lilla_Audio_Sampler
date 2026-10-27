#include "PlaybackProfile.h"

float Sample_pitch_limit(bool cached)
{
    return cached ? MAX_PITCH_PSRAM : MAX_PITCH_FLASH;
}

float Playback_pitch_limit(bool wavetable, bool cached, bool live)
{
    if (live)
    {
        return MAX_PITCH_PSRAM;
    }
    if (wavetable)
    {
        return MAX_PITCH_WAVETABLE;
    }
    return Sample_pitch_limit(cached);
}
