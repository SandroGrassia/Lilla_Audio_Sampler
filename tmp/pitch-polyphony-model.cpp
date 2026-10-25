#include <cstdio>
#include <cmath>
#include <algorithm>
#include "PlayerReadBudget.h"
#include "PlaybackProfile.h"

int main()
{
    std::puts("semitones,pitch,reserved_pitch,no_aa,aa,fw_rev_no_aa,fw_rev_aa");
    for (int semitones = 0; semitones <= 61; ++semitones)
    {
        const float base = std::exp2(semitones / 12.0f);
        const float reserved = std::fmin(base * PlayerReadBudget::Maximum_modulation, MAX_PITCH_PSRAM);
        std::printf("%d,%.5f,%.5f", semitones, base, reserved);
        for (int mode = 0; mode < 4; ++mode)
        {
            PlayerReadBudget::Plan plan;
            plan.source = PlayerReadSource::Psram;
            plan.pitch = reserved;
            plan.span = 1000000;
            plan.loop = mode >= 2;
            plan.pingpong = mode >= 2;
            plan.antialias = mode % 2 != 0 && PlayerAntialias::Enabled(0x80 | 30, base);
            const float cost = PlayerReadBudget::Estimate(plan, 128);
            std::printf(",%d", std::min(16, static_cast<int>(std::floor(PlayerReadBudget::Limit_us / cost))));
        }
        std::puts("");
    }
}
