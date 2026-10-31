#include "PlayerReadBudget.h"

float PlayerReadBudget::Transfer_us(PlayerReadSource source, uint32_t samples)
{
    return source == PlayerReadSource::Flash ? 2.2526f + 0.385575f * samples : (source == PlayerReadSource::Psram ? 0.4655f + 0.059608f * samples : 0.1165f + 0.006258f * samples);
}

float PlayerReadBudget::Estimate(const Plan &plan, uint32_t output_samples)
{
    if (!std::isfinite(plan.pitch) || plan.pitch <= 0.0f || plan.span <= 0)
    {
        return INFINITY;
    }
    float extent = std::ceil((output_samples - 1u) * plan.pitch) + 2.0f; // Fractional starting positions can require both interpolation neighbours.
    const float filtered_extent = plan.antialias && plan.pitch > 1.0f ? extent + PlayerAntialias::History(plan.pitch) : std::ceil((output_samples - 1u) * std::fmin(plan.pitch, 1.0f)) + PlayerInterpolator::Taps;
    extent = std::fmax(extent, filtered_extent); // Include IIR source history or the low-pitch FIR neighbours.
    if (extent > PlayerAntialias::Basket_capacity)
    {
        return INFINITY;
    }
    const uint32_t samples = static_cast<uint32_t>(extent); // Upper bound for source samples in this harvest.
    uint32_t operations = samples > 4500u ? 2u : 1u; // Long windows split into individually calibrated transfers.
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
    }
    if (plan.packets)
    {
        cost += 40.0f * operations; // Initial packet-open allowance; not included in the seek/read calibration.
    }
    if (plan.antialias && plan.pitch > 1.0f)
    {
        cost += 0.10f * samples + 8.0f; // Initial IIR CPU allowance (60 cycles/source sample at 600 MHz), including context; validate on hardware.
    }
    return cost * Margin;
}
