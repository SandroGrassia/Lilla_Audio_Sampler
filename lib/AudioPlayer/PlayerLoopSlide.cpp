/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.com
*/

#include "PlayerLoopSlide.h"
#include <algorithm>
#include <cmath>

void PlayerLoopSlide::Reset(int first, int last, int crossfade, int step, int source_first, int source_last)
{
    steps[0] = {0, first, step};
    step_count = 1;
    rendered_until = -1;
    span = last - first + 1;
    overlap = crossfade;
    minimum = source_first;
    maximum = source_last - span + 1;
    cursor = 0;
    fraction = 0.0f;
}

int PlayerLoopSlide::Window(int64_t cycle) const
{
    int index = step_count - 1;
    while (index > 0 && cycle < steps[index].cycle)
    {
        --index;
    }
    const Step &step = steps[index];
    const int64_t position = step.first + std::max<int64_t>(0, cycle - step.cycle) * step.stride;
    return static_cast<int>(std::max<int64_t>(minimum, std::min<int64_t>(maximum, position)));
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
        return {first + overlap + phase, -1, 0, raw - phase};
    }
    const int weight = phase - raw;
    return {first + span - overlap + weight, Window(cycle + 1) + weight, weight, period - phase};
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

void PlayerLoopSlide::Queue_step(int step)
{
    if (step == steps[step_count - 1].stride)
    {
        return;
    }
    const int period = span - overlap;
    const int64_t frontier = std::max(rendered_until, cursor - (fraction == 0.0f ? 1 : 0));
    const int64_t cycle = frontier + overlap < 0 ? 0 : (frontier + overlap) / period + 1;
    const int64_t target_cycle = std::max<int64_t>(1, cycle);
    const int first = static_cast<int>(std::max<int64_t>(minimum, std::min<int64_t>(maximum, static_cast<int64_t>(Window(target_cycle - 1)) + step)));
    if (steps[step_count - 1].cycle == target_cycle)
    {
        steps[step_count - 1] = {target_cycle, first, step}; // Several encoder changes before the same junction: latest wins.
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
    steps[step_count++] = {target_cycle, first, step};
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
