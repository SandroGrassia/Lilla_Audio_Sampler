/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.com
*/

#pragma once

#include <stdint.h>

class PlayerLoopSlide
{
public:
    struct Segment
    {
        int source;
        int head;
        int weight;
        int count;
    };

    void Reset(int first, int last, int crossfade, int step, int source_first, int source_last);
    Segment Describe(int offset) const; // Pure lookup, including interpolation history and lookahead across cycles.
    void Advance(float distance);
    void Queue_step(int step); // Apply at the first crossfade not already rendered, preserving its incoming window.
    void Commit_lookahead(int offset); // Protect endpoint samples already used by interpolation.
    int History(int requested) const; // Limit reconstructed context to retained step changes.
    float Fraction() const;
    float Phase() const;

private:
    struct Step
    {
        int64_t cycle;
        int first;
        int stride;
    };
    static constexpr int STEP_HISTORY = 8; // Geometry only: no per-voice audio buffers.
    Step steps[STEP_HISTORY] = {};
    int step_count = 0;
    int64_t rendered_until = -1;
    int span = 0;
    int overlap = 0;
    int minimum = 0;
    int maximum = 0;
    int64_t cursor = 0;
    float fraction = 0.0f;
    int Window(int64_t cycle) const;
};
