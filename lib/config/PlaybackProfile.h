/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 */
#pragma once

#include <stdint.h>

// A profile shares one voice budget between Flash and cached file playback.
static constexpr uint8_t OPTIMIZATION_OPTIONS = 3;
static constexpr uint8_t DEFAULT_OPTIMIZATION = 1;
static constexpr int OPTIMIZATION_VOICES[OPTIMIZATION_OPTIONS] = {16, 12, 10};
static constexpr float MAX_PITCH_CACHE[OPTIMIZATION_OPTIONS] = {12.0f, 16.0f, 18.0f};
static constexpr float MAX_PITCH_FLASH[OPTIMIZATION_OPTIONS] = {1.65f, 2.8f, 3.2f};
static constexpr float MAX_PITCH_WAVETABLE = 24.0f; // AudioTables wavetable playback.
static constexpr float MAX_PITCH_PSRAM = 12.0f; // Live Sampler circular buffers only.

// Legacy index 3 and corrupt settings fall back to the middle profile.
constexpr uint8_t Normalize_optimization(uint8_t value)
{
    return value < OPTIMIZATION_OPTIONS ? value : DEFAULT_OPTIMIZATION;
}

// Use the actual file reader, independently of whether a cache is being loaded.
constexpr float Sample_pitch_limit(uint8_t profile, bool cached)
{
    return cached ? MAX_PITCH_CACHE[Normalize_optimization(profile)] : MAX_PITCH_FLASH[Normalize_optimization(profile)];
}

// AudioTables and Live Sampler retain their independent limits.
constexpr float Playback_pitch_limit(uint8_t profile, bool wavetable, bool cached, bool live)
{
    if (live) { return MAX_PITCH_PSRAM; }
    if (wavetable) { return MAX_PITCH_WAVETABLE; }
    return Sample_pitch_limit(profile, cached);
}
