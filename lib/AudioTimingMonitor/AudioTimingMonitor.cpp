#include "AudioTimingMonitor.h"
#include <AudioStream.h>
#include <util/atomic.h>
#include "SharedElements.h"

namespace AudioTimingMonitor
{
bool active = false;
Sample current;
namespace
{
bool enabled = false;
void (*original_isr)() = nullptr;
Report report;

void Monitored_irq()
{
    if (!enabled)
    {
        original_isr();
        return;
    }
    const uint32_t start = ARM_DWT_CYCCNT;
    current = {};
    active = true;
    original_isr(); // Includes every AudioStream update and the framework's end-of-chain accounting.
    Barrier();
    const uint32_t end = ARM_DWT_CYCCNT;
    active = false;
    current.cycle = audio_update_cycle;
    current.irq_cycles = static_cast<uint32_t>(end - start);
    const uint32_t transfers = current.cycles[Flash] + current.cycles[Psram] + current.cycles[Zero];
    current.residual_cycles = current.irq_cycles >= transfers ? current.irq_cycles - transfers : 0;
    const bool complete = current.events[AllocationFailure] == 0 && current.events[DeadlineStop] == 0 && current.events[BudgetRetirement] == 0 && current.events[Tail] == 0 && current.events[Rendered] == current.events[ActivePlayer];
    for (uint8_t i = 0; i < Events; ++i)
    {
        report.event_totals[i] += current.events[i];
    }
    ++report.blocks;
    report.irq_sum += current.irq_cycles;
    report.residual_sum += current.residual_cycles;
    if (!complete)
    {
        ++report.incomplete_blocks;
    }
    if (current.irq_cycles > static_cast<double>(F_CPU_ACTUAL) * AUDIO_BLOCK_SAMPLES / AUDIO_SAMPLE_RATE_EXACT)
    {
        ++report.overruns;
    }
    if (report.blocks == 1 || current.irq_cycles > report.peak_irq.irq_cycles)
    {
        report.peak_irq = current;
    }
    if (report.blocks == 1 || current.residual_cycles > report.peak_residual.residual_cycles)
    {
        report.peak_residual = current;
    }
    if (complete && current.events[Visited] == PLAYERS && current.events[Rendered] == PLAYERS)
    {
        if (report.full_blocks == 0 || current.residual_cycles > report.peak_full_residual.residual_cycles)
        {
            report.peak_full_residual = current;
        }
        ++report.full_blocks;
    }
    report.last = current;
    const uint32_t tail = static_cast<uint32_t>(ARM_DWT_CYCCNT - end);
    if (tail > report.monitor_tail_max_cycles)
    {
        report.monitor_tail_max_cycles = tail;
    }
}
}

bool Enable(bool value)
{
    const uint32_t mask = __get_primask();
    __disable_irq();
    if (value)
    {
        if (_VectorsRam[IRQ_SOFTWARE + 16] != Monitored_irq)
        {
            original_isr = _VectorsRam[IRQ_SOFTWARE + 16];
            if (original_isr == nullptr)
            {
                if (mask == 0)
                {
                    __enable_irq();
                }
                return false;
            }
            attachInterruptVector(IRQ_SOFTWARE, Monitored_irq);
        }
        ARM_DEMCR |= ARM_DEMCR_TRCENA;
        ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
        report = {};
    }
    enabled = value;
    if (mask == 0)
    {
        __enable_irq();
    }
    return true;
}

void Copy(Report &destination)
{
    const uint32_t mask = __get_primask();
    __disable_irq();
    destination = report;
    if (mask == 0)
    {
        __enable_irq();
    }
}

void Print()
{
    Report snapshot;
    Copy(snapshot);
    const double scale = static_cast<double>(F_CPU_ACTUAL) / 1000000.0;
    Serial.printf("AUDIO_TIMING: blocks=%lu,full_16_blocks=%lu,incomplete_blocks=%lu,irq_body_overruns=%lu,period_us=%.3f,monitor_tail_max_us=%.3f\n", static_cast<unsigned long>(snapshot.blocks), static_cast<unsigned long>(snapshot.full_blocks), static_cast<unsigned long>(snapshot.incomplete_blocks), static_cast<unsigned long>(snapshot.overruns), AUDIO_BLOCK_SAMPLES * 1000000.0 / AUDIO_SAMPLE_RATE_EXACT, snapshot.monitor_tail_max_cycles / scale);
    Serial.printf("AUDIO_TIMING: allocation_failures=%lu,deadline_stops=%lu,budget_retirements=%lu,tail_blocks=%lu\n", static_cast<unsigned long>(snapshot.event_totals[AllocationFailure]), static_cast<unsigned long>(snapshot.event_totals[DeadlineStop]), static_cast<unsigned long>(snapshot.event_totals[BudgetRetirement]), static_cast<unsigned long>(snapshot.event_totals[Tail]));
    Serial.println(F("AUDIO_TIMING: IRQ body includes all AudioStream objects and higher-priority interruptions; exception entry/exit and report aggregation tail are excluded; tail overhead is reported separately"));
    Serial.println(F("AUDIO_TIMING: residual = IRQ - measured Flash seek/read - PSRAM copies - PSRAM zero-fill; packet opens, reversal and scalar RAM/wavetable assembly remain in residual"));
    Serial.println(F("AUDIO_TIMING: scope overhead remains in IRQ; interruptions inside transfers are subtracted with them. Full means 16 rendered voices without tails, allocation failures, deadline stops or budget retirements"));
    if (snapshot.blocks == 0)
    {
        Serial.println(F("AUDIO_TIMING: send t, play the stress patch, T to freeze, then q to report"));
        return;
    }
    Serial.printf("AUDIO_TIMING: mean_irq_us=%.3f,mean_residual_us=%.3f\n", snapshot.irq_sum / scale / snapshot.blocks, snapshot.residual_sum / scale / snapshot.blocks);
    Serial.println(F("snapshot,cycle,irq_us,flash_us,psram_copy_us,zero_us,residual_us,controls_us,players_us,outside_controls_players_us,visited,active,rendered,tails,allocation_failures,deadline_stops,budget_retirements"));
    const Sample *samples[] = {&snapshot.last, &snapshot.peak_irq, &snapshot.peak_residual, &snapshot.peak_full_residual};
    const char *names[] = {"last", "peak_irq", "peak_residual", "peak_full_residual"};
    for (uint8_t i = 0; i < 4; ++i)
    {
        if (i == 3 && snapshot.full_blocks == 0)
        {
            continue;
        }
        const Sample &s = *samples[i];
        const uint32_t accounted = s.cycles[Controls] + s.cycles[Players];
        const uint32_t outside = s.irq_cycles >= accounted ? s.irq_cycles - accounted : 0;
        Serial.printf("%s,%lu,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%u,%u,%u,%u,%u,%u,%u\n", names[i], static_cast<unsigned long>(s.cycle), s.irq_cycles / scale, s.cycles[Flash] / scale, s.cycles[Psram] / scale, s.cycles[Zero] / scale, s.residual_cycles / scale, s.cycles[Controls] / scale, s.cycles[Players] / scale, outside / scale, s.events[Visited], s.events[ActivePlayer], s.events[Rendered], s.events[Tail], s.events[AllocationFailure], s.events[DeadlineStop], s.events[BudgetRetirement]);
    }
}
}
