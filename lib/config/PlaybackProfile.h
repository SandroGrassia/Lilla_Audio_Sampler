/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 */
#pragma once

// Final pitch ceilings include tuning, MIDI bend and vibrato; the read budget controls polyphony.
static constexpr float MAX_PITCH_FLASH = 5.0f;
static constexpr float MAX_PITCH_PSRAM = 35.0f; // Cached files and Live Sampler circular buffers.
static constexpr float MAX_PITCH_WAVETABLE = 35.0f;

constexpr float Sample_pitch_limit(bool cached)
{
    return cached ? MAX_PITCH_PSRAM : MAX_PITCH_FLASH;
}

constexpr float Playback_pitch_limit(bool wavetable, bool cached, bool live)
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
