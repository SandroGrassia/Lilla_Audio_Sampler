"""Exercise the production IRQ wrapper with deterministic cycle counts and a fake vector table."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
arduino = r"""
#pragma once
#include <cstdint>
#include <cstdio>
#define F(value) value
inline uint32_t ARM_DWT_CYCCNT = 0, ARM_DEMCR = 0, ARM_DWT_CTRL = 0;
constexpr uint32_t ARM_DEMCR_TRCENA = 1, ARM_DWT_CTRL_CYCCNTENA = 1, F_CPU_ACTUAL = 600000000;
constexpr int IRQ_SOFTWARE = 1;
inline void (* volatile _VectorsRam[32])() = {};
inline void attachInterruptVector(int irq, void (*handler)()) { _VectorsRam[irq + 16] = handler; }
inline uint32_t mask = 0;
inline uint32_t __get_primask() { return mask; }
inline void __disable_irq() { mask = 1; }
inline void __enable_irq() { mask = 0; }
struct SerialStub
{
    template<class... T> void printf(const char *, T...) {}
    void println(const char *) {}
};
inline SerialStub Serial;
"""
checks = r"""
#include <cassert>
#include <iostream>
#include "AudioTimingMonitor.h"
#include "SharedElements.h"
using namespace AudioTimingMonitor;
int scenario = 0, calls = 0;
void Fake_irq()
{
    ++calls;
    ++audio_update_cycle;
    ARM_DWT_CYCCNT += 100;
    {
        Scope controls(Controls);
        ARM_DWT_CYCCNT += 30;
        if (scenario == 3)
        {
            Event_count(BudgetRetirement);
        }
    }
    for (int i = 0; i < PLAYERS; ++i)
    {
        Scope player(Players);
        Event_count(Visited);
        Event_count(ActivePlayer);
        if (i == 0 && scenario == 1)
        {
            Event_count(DeadlineStop);
            continue;
        }
        if (i == 0 && scenario == 2)
        {
            Event_count(AllocationFailure);
            continue;
        }
        if (i == 0 && scenario == 4)
        {
            Event_count(Tail);
            continue;
        }
        {
            Scope zero(Zero);
            ARM_DWT_CYCCNT += 10;
        }
        {
            Scope copy(Psram);
            ARM_DWT_CYCCNT += 20;
        }
        {
            Scope flash(Flash);
            ARM_DWT_CYCCNT += 5;
        }
        ARM_DWT_CYCCNT += 40;
        Event_count(Rendered);
    }
    ARM_DWT_CYCCNT += scenario == 5 ? 2000000 : 200;
}
void Tick() { _VectorsRam[IRQ_SOFTWARE + 16](); }
int main()
{
    assert(!Enable(true)); // No installed handler must be reported, not replaced.
    assert(mask == 0);
    _VectorsRam[IRQ_SOFTWARE + 16] = Fake_irq;
    assert(Enable(true));
    const auto wrapper = _VectorsRam[IRQ_SOFTWARE + 16];
    Tick();
    Report r;
    Copy(r);
    assert(r.blocks == 1 && r.full_blocks == 1 && r.incomplete_blocks == 0);
    assert(r.last.irq_cycles == 1530 && r.last.residual_cycles == 970);
    assert(r.last.cycles[Players] == 1200 && r.last.cycles[Controls] == 30);
    assert(r.last.cycles[Zero] == 160 && r.last.cycles[Psram] == 320 && r.last.cycles[Flash] == 80);
    assert(r.last.cycle == audio_update_cycle && r.last.events[Rendered] == 16);
    for (scenario = 1; scenario <= 4; ++scenario)
    {
        Tick();
    }
    Copy(r);
    assert(r.blocks == 5 && r.full_blocks == 1 && r.incomplete_blocks == 4);
    assert(r.event_totals[DeadlineStop] == 1 && r.event_totals[AllocationFailure] == 1 && r.event_totals[BudgetRetirement] == 1 && r.event_totals[Tail] == 1);
    scenario = 5;
    Tick();
    Copy(r);
    assert(r.overruns == 1 && r.full_blocks == 2);
    assert(r.peak_irq.cycle == audio_update_cycle && r.peak_full_residual.cycle == audio_update_cycle);
    assert(r.peak_irq.irq_cycles - r.peak_irq.residual_cycles == 560);
    const uint32_t blocks = r.blocks;
    assert(Enable(false));
    Tick();
    Copy(r);
    assert(r.blocks == blocks && !active); // Frozen capture, audio still runs.
    mask = 1;
    Copy(r);
    assert(mask == 1);
    assert(Enable(true) && mask == 1);
    assert(_VectorsRam[IRQ_SOFTWARE + 16] == wrapper); // Re-enable must never chain the wrapper to itself.
    mask = 0;
    scenario = 0;
    ARM_DWT_CYCCNT = UINT32_MAX - 99;
    Tick();
    Copy(r);
    assert(r.blocks == 1 && r.last.irq_cycles == 1530 && r.last.residual_cycles == 970);
    Print();
    assert(calls == 8);
    std::cout << "PASS: full IRQ accounting, same-cycle subtraction, incomplete-block flags, freeze/reset and cycle-counter wrap\n";
}
"""
compiler = shutil.which('g++') or 'C:/msys64/ucrt64/bin/g++.exe'
env = os.environ.copy()
env['PATH'] = str(Path(compiler).parent) + os.pathsep + env.get('PATH', '')
with tempfile.TemporaryDirectory(prefix='audio-timing-') as folder:
    folder = Path(folder)
    (folder / 'Arduino.h').write_text(arduino)
    (folder / 'AudioStream.h').write_text('#pragma once\nconstexpr int AUDIO_BLOCK_SAMPLES = 128;\nconstexpr double AUDIO_SAMPLE_RATE_EXACT = 44100;\n')
    (folder / 'SharedElements.h').write_text('#pragma once\n#include <cstdint>\nconstexpr int PLAYERS = 16;\ninline uint32_t audio_update_cycle = 0;\n')
    (folder / 'util').mkdir()
    (folder / 'util/atomic.h').write_text('#include <Arduino.h>\n')
    (folder / 'test.cpp').write_text(checks)
    exe = folder / 'test.exe'
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', '-I' + str(folder), '-I' + str(ROOT / 'lib/AudioTimingMonitor'), str(folder / 'test.cpp'), str(ROOT / 'lib/AudioTimingMonitor/AudioTimingMonitor.cpp'), '-o', str(exe)], check=True, env=env)
    subprocess.run([str(exe)], check=True, env=env)
