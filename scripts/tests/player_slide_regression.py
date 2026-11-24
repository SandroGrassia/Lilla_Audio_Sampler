"""Exercise sliding source geometry and the production shared-buffer renderer."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile
from production_source import method

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / 'lib/AudioPlayer/AudioPlayer.cpp').read_text(encoding='utf-8')
render = method(source, 'void AudioPlayer::Harvest_slide_samples(int left, int right, float distance)')
fixture = r'''
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include "PlayerLoopSlide.h"
constexpr int LOOP_FWD = 2, LOOP_REV = 5;
struct AudioPlayer
{
    PlayerLoopSlide loop_slide;
    float pitch = 1, initial_index_offset = 0, a_first_sample = 0;
    int A_Flash_sample = 10000, B_Flash_sample = 0, mode_player = LOOP_FWD, delta_Noclick = 0, harvested_samples = 0, slide_step = 0;
    static int16_t samples_basket[6000];
    static int16_t Source(int index) { return static_cast<int16_t>((index * 37) % 60001 - 30000); }
    void Read_samples(int16_t *target, int first, int count)
    {
        assert(first >= 0 && first + count <= 50000);
        for (int i = 0; i < count; ++i) { target[i] = Source(first + i); }
    }
    void Harvest_slide_samples(int left, int right, float distance);
};
int16_t AudioPlayer::samples_basket[6000];
'''
tests = r'''
void Check_direction(bool reverse)
{
    for (int span : {2, 10, 80, 600, 601, 2000, 5000})
    {
        for (int overlap : {0, 2, 4, 32, 300})
        {
            if (overlap > span / 2)
            {
                continue;
            }
            for (int step : {-2000, -17, -1, 0, 1, 17, 2000})
            {
                for (float pitch : {.01f, .25f, 1.0f, 2.5f, 35.0f})
                {
                    AudioPlayer voice;
                    voice.mode_player = reverse ? LOOP_REV : LOOP_FWD;
                    voice.B_Flash_sample = 10000 + span - 1;
                    voice.delta_Noclick = overlap;
                    voice.pitch = pitch;
                    voice.slide_step = step;
                    voice.loop_slide.Reset(10000, 10000 + span - 1, overlap, 0, 0, 49999, reverse);
                    int64_t consumed = 0;
                    float fraction = 0;
                    const int period = span - overlap;
                    for (int block = 0; block < 12; ++block)
                    {
                        const int left = block % 3 == 0 ? 400 : (block % 3 == 1 ? 7 : 0);
                        const int right = left == 7 ? 8 : 1;
                        const int outputs = block % 2 == 0 ? 128 : 32; // Include outgoing note tails.
                        voice.Harvest_slide_samples(left, right, (outputs - 1) * pitch);
                        assert(voice.harvested_samples <= 6000);
                        assert(std::abs(voice.initial_index_offset - left - fraction) < .0001f);
                        for (int i = 0; i < voice.harvested_samples; ++i)
                        {
                            const int64_t position = consumed + i - left;
                            const int64_t cycle = static_cast<int64_t>(std::floor(static_cast<double>(position) / period));
                            const int phase = position - cycle * period;
                            const auto window = [&](int64_t n) { return std::clamp<int64_t>(std::llround(10000.0 + std::max<int64_t>(0, n) * static_cast<double>(step / pitch)), 0, 50000 - span); };
                            const auto sample = [&](int64_t cycle, int offset) { return AudioPlayer::Source(window(cycle) + (reverse ? span - 1 - offset : offset)); };
                            const int raw = span - 2 * overlap;
                            int16_t expected;
                            if (phase < raw)
                            {
                                expected = sample(cycle, overlap + phase);
                            }
                            else
                            {
                                const int weight = phase - raw;
                                const float gain = static_cast<float>(weight) / (overlap - 1);
                                expected = static_cast<int16_t>(sample(cycle, span - overlap + weight) * (1 - gain) + sample(cycle + 1, weight) * gain);
                            }
                            assert(voice.samples_basket[i] == expected);
                        }
                        const float next = fraction + outputs * pitch;
                        consumed += static_cast<int>(std::floor(next));
                        fraction = next - std::floor(next);
                        const int base = reverse ? voice.B_Flash_sample - overlap : 10000 + overlap;
                        assert(voice.a_first_sample == base + voice.loop_slide.Phase());
                        std::fill(voice.samples_basket, voice.samples_basket + 6000, -1234); // Another Player overwrites the entire shared basket.
                    }
                    voice.loop_slide.Reset(10000, 10000 + span - 1, overlap, step, 0, 49999, reverse);
                    assert(voice.loop_slide.Fraction() == 0);
                    assert(voice.loop_slide.Describe(0).source == (reverse ? 10000 + span - 1 - overlap : 10000 + overlap));
                }
            }
        }
    }
    // Bounds of a cached subrange, and source lookahead must never advance the audible cursor.
    PlayerLoopSlide cache;
    cache.Reset(120, 219, 20, -50, 100, 999, reverse);
    const auto first = cache.Describe(0);
    cache.Describe(100000);
    assert(cache.Describe(0).source == first.source);
    cache.Advance(160);
    assert(cache.Describe(0).source == (reverse ? 179 : 120));
}
void Check_pitch_compensation(bool reverse)
{
    // The same output duration must move the window equally at every pitch, including SLIDE +/-1.
    for (int step : {-17, -1, 1, 17})
    {
        for (float pitch : {.25f, .5f, 1.0f, 2.0f, 2.5f, 8.0f, 35.0f})
        {
            AudioPlayer voice;
            voice.mode_player = reverse ? LOOP_REV : LOOP_FWD;
            voice.B_Flash_sample = 10159;
            voice.delta_Noclick = 32;
            voice.slide_step = step;
            voice.pitch = pitch;
            voice.loop_slide.Reset(10000, 10159, 32, 0, 0, 49999, reverse);
            for (int block = 0; block < 400; ++block)
            {
                voice.Harvest_slide_samples(0, 1, 127 * pitch);
            }
            const int expected = 10000 + 400 * step + (reverse ? 127 : 32);
            assert(voice.loop_slide.Describe(0).source == expected);
        }
    }

    // A pitch change must preserve the crossfade already prefetched by the preceding block.
    AudioPlayer changing;
    changing.mode_player = reverse ? LOOP_REV : LOOP_FWD;
    changing.B_Flash_sample = 10099;
    changing.delta_Noclick = 20;
    changing.slide_step = 20;
    changing.pitch = 2;
    changing.loop_slide.Reset(10000, 10099, 20, 0, 0, 49999, reverse);
    changing.loop_slide.Advance(57);
    changing.Harvest_slide_samples(0, 4, 0);
    changing.pitch = 4;
    changing.Harvest_slide_samples(0, 1, 0);
    assert(changing.loop_slide.Phase() == 63);
    assert(changing.loop_slide.Describe(-3).head == 10010 + (reverse ? 99 : 0));
    assert(changing.loop_slide.Describe(77).head == 10015 + (reverse ? 99 : 0));

    // Fractional motion survives large file offsets and repeated changes of the effective step.
    PlayerLoopSlide precise;
    precise.Reset(1000000000, 1000000099, 20, 0, 0, 1500000000, reverse);
    double position = 1000000000.0;
    for (int cycle = 0; cycle < 800; ++cycle)
    {
        const float step = cycle % 2 == 0 ? .25f : .125f;
        precise.Queue_step(step);
        precise.Advance(80);
        position += step;
        assert(precise.Describe(0).source == std::llround(position) + (reverse ? 79 : 20));
    }
    precise.Queue_step(0);
    precise.Advance(8000);
    assert(precise.Describe(0).source == 1000000150 + (reverse ? 79 : 20));

    // Very slow pitch can exceed the window length; clamp at the source edge without accumulated overshoot.
    for (int sign : {-1, 1})
    {
        precise.Reset(400, 499, 20, sign * 200000.0f, 0, 999, reverse);
        precise.Advance(1000000000.0f);
        assert(precise.Describe(0).source == (sign > 0 ? 900 : 0) + (reverse ? 79 : 20));
        precise.Queue_step(-sign * .25f);
        precise.Advance(80 * 4);
        assert(precise.Describe(0).source == (sign > 0 ? 899 : 1) + (reverse ? 79 : 20));
    }
}

int main()
{
    Check_direction(false);
    Check_direction(true);
    Check_pitch_compensation(false);
    Check_pitch_compensation(true);
    puts("FWD/REV sliding loops: pitch-independent motion, fractional steps, pitch changes, rendering, bounds and Note On reset passed");
}
'''

compiler = shutil.which('g++') or 'C:/msys64/ucrt64/bin/g++.exe'
with tempfile.TemporaryDirectory() as folder:
    cpp = Path(folder) / 'slide.cpp'
    exe = Path(folder) / 'slide.exe'
    cpp.write_text(fixture + render + tests, encoding='utf-8')
    environment = os.environ.copy()
    environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
    subprocess.run([compiler, '-std=c++20', '-O2', '-Wall', '-Wextra', '-I' + str(ROOT / 'lib/AudioPlayer'), str(cpp), str(ROOT / 'lib/AudioPlayer/PlayerLoopSlide.cpp'), '-o', str(exe)], check=True, env=environment)
    subprocess.run([str(exe)], check=True, env=environment)
