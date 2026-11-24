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

    struct Position
    {
        int64_t first; // Fixed-point window origin, including accumulated fractional motion.
        int source;
        float fraction;
    };

    void Reset(int first, int last, int crossfade, float step, int source_first, int source_last, bool reverse = false);
    Position Get_position() const;
    void Reframe(const Position &position, int first_delta, int length, int crossfade, float step); // Resize/shift the reached window while retaining the read position where possible.
    Segment Describe(int offset) const; // Pure lookup, including interpolation history and lookahead across cycles.
    void Advance(float distance);
    void Queue_step(float step); // Pitch-compensated samples per cycle; apply at the first crossfade not already rendered.
    void Commit_lookahead(int offset); // Protect endpoint samples already used by interpolation.
    int History(int requested) const; // Limit reconstructed context to retained step changes.
    float Fraction() const;
    float Phase() const;

private:
    struct Step
    {
        int64_t cycle;
        int64_t first;
        int64_t stride;
        int64_t cycles_to_edge;
    };
    static constexpr int64_t POSITION_SCALE = 1LL << 24; // Retain fractional motion even at large file offsets and across pitch changes.
    static constexpr int STEP_HISTORY = 8; // Geometry only: no per-voice audio buffers.
    Step steps[STEP_HISTORY] = {};
    int step_count = 0;
    int64_t rendered_until = -1;
    int span = 0;
    int overlap = 0;
    bool reverse = false; // Playback direction; signed steps always follow source coordinates.
    int minimum = 0;
    int maximum = 0;
    int64_t cursor = 0;
    float fraction = 0.0f;
    Step Make_step(int64_t cycle, int64_t first, int64_t stride) const;
    int64_t Window_position(int64_t cycle) const;
    int Window(int64_t cycle) const;
};
