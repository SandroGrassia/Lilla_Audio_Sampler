/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 */
#pragma once

// Final pitch ceilings include tuning, MIDI bend and vibrato; the read budget controls polyphony.
static constexpr float MAX_PITCH_FLASH = 5.0f;
static constexpr float MAX_PITCH_PSRAM = 35.0f; // Cached sample files.
static constexpr float MAX_PITCH_LIVE = 35.0f; // Final Live Sampler ceiling, including bend and vibrato; preserves room in a 120 ms loop.
static constexpr float MAX_PITCH_WAVETABLE = 35.0f;

float Sample_pitch_limit(bool cached); // Return the sample pitch ceiling for the selected storage.

float Playback_pitch_limit(bool wavetable, bool cached, bool live); // Return the pitch ceiling for wavetable, cached or live playback.
