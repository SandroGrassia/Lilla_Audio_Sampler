#include <cstdio>
#include <cmath>
#include "PlayerReadBudget.h"
#include "PlaybackProfile.h"

int main()
{
    std::puts("PSRAM, long FW-REV loop, neutral tuning, antialias disabled. All costs in us per 128-sample block.");
    std::puts("Read estimate excludes the 10% budget margin. Reservation includes the 10% margin and bend/vibrato x1.515, capped at pitch 35.");
    std::puts("Neither column is the complete Player CPU cost. Remaining Player processing is not isolated by available measurements.");
    std::puts("Semitones | Pitch | Read estimate | Read reservation");
    for (int semitones = 0; semitones <= 61; ++semitones)
    {
        const float base = std::exp2(semitones / 12.0f);
        PlayerReadBudget::Plan plan;
        plan.source = PlayerReadSource::Psram;
        plan.span = 1000000;
        plan.loop = true;
        plan.pingpong = true;
        plan.pitch = base;
        const float read = PlayerReadBudget::Estimate(plan, 128) / PlayerReadBudget::Margin;
        plan.pitch = std::fmin(base * PlayerReadBudget::Maximum_modulation, MAX_PITCH_PSRAM);
        const float reserved = PlayerReadBudget::Estimate(plan, 128);
        std::printf("+%d | %.3f | %.1f | %.1f\n", semitones, base, read, reserved);
    }
    std::puts("VCF+LFO: 15 us per enabled voice, provisional conservative allowance.");
    std::puts("Stereo delay lines: approximately 52.3 us globally; feedback filters: 5.4 us; stereo compressor: 62.3 us; total: approximately 120 us.");
    std::puts("Delay figures are sums of archived median polled timings, not a measured simultaneous maximum. Source: measurements/reverb-cost/polyphony.json.");
    std::puts("Do not add read estimate and reservation together. The 1800 us read budget already leaves a separate allowance for VCF and other processing.");
}
