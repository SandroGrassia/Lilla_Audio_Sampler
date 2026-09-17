#pragma once
#include <Arduino.h>

namespace AudioTimingMonitor
{
enum Category : uint8_t { Flash, Psram, Zero, Players, Controls, Categories };
enum Event : uint8_t { Visited, ActivePlayer, Rendered, Tail, AllocationFailure, DeadlineStop, BudgetRetirement, Events };
struct Sample
{
    uint32_t cycle = 0;
    uint32_t irq_cycles = 0;
    uint32_t residual_cycles = 0;
    uint32_t cycles[Categories] = {};
    uint16_t events[Events] = {};
};
struct Report
{
    uint32_t blocks = 0, full_blocks = 0, overruns = 0, incomplete_blocks = 0;
    uint32_t monitor_tail_max_cycles = 0;
    uint64_t irq_sum = 0, residual_sum = 0;
    uint32_t event_totals[Events] = {};
    Sample last, peak_irq, peak_residual, peak_full_residual;
};
extern bool active;
extern Sample current;

inline void Event_count(Event event)
{
    if (active)
    {
        ++current.events[event];
    }
}
inline void Barrier()
{
#if defined(__arm__)
    asm volatile("dsb" ::: "memory");
#else
    asm volatile("" ::: "memory");
#endif
}
class Scope
{
    bool measuring;
    Category category;
    uint32_t start;
public:
    explicit Scope(Category value) : measuring(active), category(value), start(0)
    {
        if (measuring)
        {
            Barrier();
            start = ARM_DWT_CYCCNT;
        }
    }
    ~Scope()
    {
        if (measuring)
        {
            Barrier();
            current.cycles[category] += static_cast<uint32_t>(ARM_DWT_CYCCNT - start);
        }
    }
    Scope(const Scope &) = delete;
    Scope &operator=(const Scope &) = delete;
};
// Main-loop API. Enable resets the capture and wraps the currently installed audio ISR.
bool Enable(bool enabled);
void Copy(Report &destination);
void Print();
}
