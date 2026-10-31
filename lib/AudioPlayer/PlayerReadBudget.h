#pragma once

#include <cmath>
#include <stdint.h>
#include "PlayerInterpolator.h"
#include "PlayerAntialias.h"

enum class PlayerReadSource : uint8_t { Flash, Psram, Ram }; // Storage classes used by the source-read budget.

namespace PlayerReadBudget
{
constexpr float Limit_us = 1800.0f; // Initial source-read allowance per block; leaves 1100 us of the 2900 us audio cycle for other work and safety margin.
constexpr float Margin = 1.10f; // Initial transfer-model margin, to be validated with the full audio chain on hardware.
constexpr float Maximum_modulation = 1.5f * 1.01f; // Current MIDI bend tops below 1.5 and full-depth vibrato below 1.01; reserve both even before they are used.

struct Plan // Source geometry and worst reachable pitch for one current, pending, or edited voice.
{
    PlayerReadSource source = PlayerReadSource::Flash; // Actual storage selected by the reader.
    float pitch = 1.0f; // Maximum reachable pitch after the source-specific ceiling.
    int span = 0; // Inclusive A..B source length.
    int crossfade = 0; // NoClick region length, in source samples.
    bool loop = false; // File loops may require several source segments in one block.
    bool pingpong = false; // Ping-pong uses two raw legs rather than a NoClick segment.
    bool live = false; // Live circular-buffer reads can split at wrap and loop boundaries.
    bool antialias = false; // TONE enables source-domain filtering for upward pitch.
    bool packets = false; // Uncached recordings can reopen Flash packets during a read.
};

float Transfer_us(PlayerReadSource source, uint32_t samples); // Estimate one source transfer; zero samples returns the fixed per-operation cost.

float Estimate(const Plan &plan, uint32_t output_samples); // Reserve source segments, minimum read sizes and interpolation context.
}
