#pragma once

#include <cmath>
#include <stdint.h>
#include "PlayerReadDiagnostics.h"

namespace PlayerReadBudget
{
constexpr int Live_noclick_samples = 128; // Forward Live loops blend the current PSRAM endpoints.
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
    bool packets = false; // Uncached recordings can reopen Flash packets during a read.
};

constexpr float Transfer_us(PlayerReadSource source, uint32_t samples) // Affine per-operation model; sample zero deliberately returns the fixed intercept for aggregation.
{
    return source == PlayerReadSource::Flash ? 2.2526f + 0.385575f * samples : (source == PlayerReadSource::Psram ? 0.4655f + 0.059608f * samples : 0.1165f + 0.006258f * samples);
}

inline float Estimate(const Plan &plan, uint32_t output_samples) // Reserve all possible source segments, including small-read minimums and interpolation endpoints.
{
    if (output_samples == 0)
    {
        return 0.0f;
    }
    if (!std::isfinite(plan.pitch) || plan.pitch <= 0.0f || plan.span <= 0)
    {
        return INFINITY;
    }
    const float extent = std::ceil((output_samples - 1u) * plan.pitch) + 2.0f; // Fractional starting positions can require both interpolation neighbours.
    if (extent > 4500.0f)
    {
        return INFINITY;
    }
    const uint32_t samples = static_cast<uint32_t>(extent); // Upper bound for source samples in this harvest.
    uint32_t operations = 1; // One contiguous transfer unless the source geometry can split it.
    if (plan.source != PlayerReadSource::Ram && plan.loop)
    {
        const int period = plan.pingpong ? 2 * (plan.span - 1) : plan.span - plan.crossfade; // Playback period of a file loop.
        if (period <= 0 || plan.crossfade < 0 || plan.crossfade > plan.span / 2)
        {
            return INFINITY;
        }
        operations = (plan.pingpong ? 2u : 3u) * (samples / static_cast<uint32_t>(period) + 2u); // Include a partially covered period at each end.
    }
    if (plan.live)
    {
        operations *= 2u; // Each logical live segment may wrap the circular buffer.
        if (plan.crossfade > 0)
        {
            operations *= 2u; // Dynamic NoClick reads both live endpoints.
        }
    }
    if (plan.packets)
    {
        operations *= 2u; // At most two 64 KiB packets for each bounded logical segment.
    }
    float cost = Transfer_us(plan.source, samples + 9u * operations) + (operations - 1u) * Transfer_us(plan.source, 0); // Nine extra samples per operation bound the model's minimum of ten.
    if (plan.source == PlayerReadSource::Ram)
    {
        cost += 0.01f * samples; // Initial allowance for scalar/modulo wavetable assembly beyond the memcpy calibration.
    }
    else if (plan.crossfade > 0)
    {
        cost += Transfer_us(PlayerReadSource::Ram, samples + 9u * operations) + (operations - 1u) * Transfer_us(PlayerReadSource::Ram, 0); // Charging a full RAM span as well safely covers every possible NoClick split.
        if (plan.live)
        {
            cost += Transfer_us(PlayerReadSource::Psram, samples) + 0.05f * samples; // Bound the second endpoint read and per-sample blend; validate CPU allowance on hardware.
        }
    }
    if (plan.packets)
    {
        cost += 40.0f * operations; // Initial packet-open allowance; not included in the seek/read calibration.
    }
    return cost * Margin;
}
}
