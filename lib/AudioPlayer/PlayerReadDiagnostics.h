#pragma once

#include <stdint.h>

// Source-transfer counters for one completed audio block, not a future voice reservation.
enum class PlayerReadSource : uint8_t { Flash, Psram, Ram };

struct PlayerReadUsage
{
    uint32_t operations = 0;
    uint32_t samples = 0;
    uint32_t model_samples = 0; // Sum of max(10, samples) for each operation, preserving the model's minimum.
    uint32_t uncovered_operations = 0;

    void Add(int count)
    {
        if (count <= 0)
        {
            return;
        }
        ++operations;
        samples += static_cast<uint32_t>(count);
        model_samples += count < 10 ? 10u : static_cast<uint32_t>(count);
        if (count > 4500)
        {
            ++uncovered_operations;
        }
    }
};

struct PlayerReadDiagnostics
{
    enum Flags : uint16_t { RamLoopProxy = 1, LiveCopyProxy = 2, PaddedRead = 4, PacketOpen = 8, Transition = 16, RestartExecuted = 32 };
    PlayerReadUsage sources[3];
    uint32_t cycle = 0;
    uint32_t harvest_cycles = 0; // Whole harvest, including assembly/reversal/counter overhead, not just memory transfers.
    uint16_t harvests = 0;
    uint16_t flags = 0;
    float maximum_pitch = 0.0f; // Maximum pitch actually used in this block, NOT the possible maximum bend.
};
