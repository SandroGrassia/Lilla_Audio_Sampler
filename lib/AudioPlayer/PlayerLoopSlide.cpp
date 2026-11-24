/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.com
*/

#include "PlayerLoopSlide.h"
#include <algorithm>
#include <cmath>

void PlayerLoopSlide::Reset(int first, int last, int crossfade, float step, int source_first, int source_last, bool reverse_in)
{
    step_count = 1;
    rendered_until = -1;
    span = last - first + 1;
    overlap = crossfade;
    reverse = reverse_in;
    minimum = source_first;
    maximum = source_last - span + 1;
    steps[0] = Make_step(0, static_cast<int64_t>(first) * POSITION_SCALE, static_cast<int64_t>(std::round(step * POSITION_SCALE)));
    cursor = 0;
    fraction = 0.0f;
}

PlayerLoopSlide::Position PlayerLoopSlide::Get_position() const
{
    return {Window_position(cursor / (span - overlap)), Describe(0).source, fraction};
}

void PlayerLoopSlide::Reframe(const Position &position, int first_delta, int length, int crossfade, float step)
{
    maximum += span - length;
    span = length;
    overlap = crossfade;
    const int64_t first = std::max(static_cast<int64_t>(minimum) * POSITION_SCALE, std::min(static_cast<int64_t>(maximum) * POSITION_SCALE, position.first + static_cast<int64_t>(first_delta) * POSITION_SCALE));
    step_count = 1;
    steps[0] = Make_step(0, first, static_cast<int64_t>(std::round(step * POSITION_SCALE)));
    const int base = Window(0) + (reverse ? span - 1 - overlap : overlap);
    const int phase = reverse ? base - position.source : position.source - base;
    cursor = std::max(0, std::min(span - overlap - 1, phase));
    fraction = phase == cursor ? position.fraction : 0.0f;
    rendered_until = -1; // The outgoing crossmix retains the old interpolation tail; this trajectory uses the new geometry.
}

PlayerLoopSlide::Step PlayerLoopSlide::Make_step(int64_t cycle, int64_t first, int64_t stride) const
{
    const int64_t distance = stride > 0 ? static_cast<int64_t>(maximum) * POSITION_SCALE - first : first - static_cast<int64_t>(minimum) * POSITION_SCALE;
    const int64_t magnitude = stride < 0 ? -stride : stride;
    const int64_t cycles = magnitude == 0 ? 0 : (distance + magnitude - 1) / magnitude;
    return {cycle, first, stride, cycles};
}

int64_t PlayerLoopSlide::Window_position(int64_t cycle) const
{
    int index = step_count - 1;
    while (index > 0 && cycle < steps[index].cycle)
    {
        --index;
    }
    const Step &step = steps[index];
    const int64_t cycles = std::min(step.cycles_to_edge, std::max<int64_t>(0, cycle - step.cycle)); // Stop multiplication at the edge, even after very long playback.
    const int64_t position = step.first + cycles * step.stride;
    return std::max(static_cast<int64_t>(minimum) * POSITION_SCALE, std::min(static_cast<int64_t>(maximum) * POSITION_SCALE, position));
}

int PlayerLoopSlide::Window(int64_t cycle) const
{
    return static_cast<int>((Window_position(cycle) + POSITION_SCALE / 2) / POSITION_SCALE);
}

PlayerLoopSlide::Segment PlayerLoopSlide::Describe(int offset) const
{
    const int period = span - overlap;
    const int64_t position = cursor + offset;
    int64_t cycle = position / period;
    int phase = position % period;
    if (phase < 0)
    {
        phase += period;
        --cycle;
    }
    const int first = Window(cycle);
    const int raw = span - 2 * overlap;
    if (phase < raw)
    {
        return {reverse ? first + span - 1 - overlap - phase : first + overlap + phase, -1, 0, raw - phase};
    }
    const int weight = phase - raw;
    return {reverse ? first + overlap - 1 - weight : first + span - overlap + weight, reverse ? Window(cycle + 1) + span - 1 - weight : Window(cycle + 1) + weight, weight, period - phase};
}

void PlayerLoopSlide::Advance(float distance)
{
    const float next = fraction + distance;
    const int whole = static_cast<int>(std::floor(next));
    cursor += whole;
    fraction = next - whole;
}

float PlayerLoopSlide::Fraction() const
{
    return fraction;
}

float PlayerLoopSlide::Phase() const
{
    return static_cast<float>(cursor % (span - overlap)) + fraction;
}

void PlayerLoopSlide::Queue_step(float step)
{
    const int64_t stride = static_cast<int64_t>(std::round(step * POSITION_SCALE));
    if (stride == steps[step_count - 1].stride)
    {
        return;
    }
    const int period = span - overlap;
    const int64_t frontier = std::max(rendered_until, cursor - (fraction == 0.0f ? 1 : 0));
    const int64_t cycle = frontier + overlap < 0 ? 0 : (frontier + overlap) / period + 1;
    const int64_t target_cycle = std::max<int64_t>(1, cycle);
    const int64_t first = std::max(static_cast<int64_t>(minimum) * POSITION_SCALE, std::min(static_cast<int64_t>(maximum) * POSITION_SCALE, Window_position(target_cycle - 1) + stride));
    const Step next = Make_step(target_cycle, first, stride);
    if (steps[step_count - 1].cycle == target_cycle)
    {
        steps[step_count - 1] = next; // Several encoder or pitch changes before the same junction: latest wins.
        return;
    }
    if (step_count == STEP_HISTORY)
    {
        for (int i = 1; i < STEP_HISTORY; ++i)
        {
            steps[i - 1] = steps[i];
        }
        --step_count;
    }
    steps[step_count++] = next;
}

void PlayerLoopSlide::Commit_lookahead(int offset)
{
    rendered_until = std::max(rendered_until, cursor + offset);
}

int PlayerLoopSlide::History(int requested) const
{
    if (steps[0].cycle == 0)
    {
        return requested;
    }
    const int64_t available = std::max<int64_t>(0, cursor - steps[0].cycle * (span - overlap));
    return static_cast<int>(std::min<int64_t>(requested, available)); // A sudden pitch increase must not invent discarded filter history.
}
