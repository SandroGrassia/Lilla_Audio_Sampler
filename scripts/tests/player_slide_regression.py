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
struct AudioPlayer
{
    PlayerLoopSlide loop_slide;
    float pitch = 1, initial_index_offset = 0, a_first_sample = 0;
    int A_Flash_sample = 10000, delta_Noclick = 0, harvested_samples = 0;
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
int main()
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
                    voice.delta_Noclick = overlap;
                    voice.pitch = pitch;
                    voice.loop_slide.Reset(10000, 10000 + span - 1, overlap, step, 0, 49999);
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
                            const auto window = [&](int64_t n) { return std::clamp<int64_t>(10000 + std::max<int64_t>(0, n) * step, 0, 50000 - span); };
                            const int raw = span - 2 * overlap;
                            int16_t expected;
                            if (phase < raw)
                            {
                                expected = AudioPlayer::Source(window(cycle) + overlap + phase);
                            }
                            else
                            {
                                const int weight = phase - raw;
                                const float gain = static_cast<float>(weight) / (overlap - 1);
                                expected = static_cast<int16_t>(AudioPlayer::Source(window(cycle) + span - overlap + weight) * (1 - gain) + AudioPlayer::Source(window(cycle + 1) + weight) * gain);
                            }
                            assert(voice.samples_basket[i] == expected);
                        }
                        const float next = fraction + outputs * pitch;
                        consumed += static_cast<int>(std::floor(next));
                        fraction = next - std::floor(next);
                        std::fill(voice.samples_basket, voice.samples_basket + 6000, -1234); // Another Player overwrites the entire shared basket.
                    }
                    voice.loop_slide.Reset(10000, 10000 + span - 1, overlap, step, 0, 49999);
                    assert(voice.loop_slide.Fraction() == 0);
                    assert(voice.loop_slide.Describe(0).source == 10000 + overlap);
                }
            }
        }
    }
    // Bounds of a cached subrange, and source lookahead must never advance the audible cursor.
    PlayerLoopSlide cache;
    cache.Reset(120, 219, 20, -50, 100, 999);
    const auto first = cache.Describe(0);
    cache.Describe(100000);
    assert(cache.Describe(0).source == first.source);
    cache.Advance(160);
    assert(cache.Describe(0).source == 120);
    puts("Sliding loops: shared-buffer rendering, pitch/history, bounds and Note On reset passed");
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
