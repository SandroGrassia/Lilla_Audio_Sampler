"""Exercise the production stereo compressor with host AudioStream stubs."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def source(name):
    return re.sub(r'^\s*#(?:include|pragma)[^\n]*', '', (ROOT / name).read_text(encoding='utf-8'), flags=re.M)


prefix = r'''
#define FLASHMEM
#define PROGMEM
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <vector>
constexpr unsigned AUDIO_BLOCK_SAMPLES = 128;
constexpr float AUDIO_SAMPLE_RATE_EXACT = 44100.0f;
struct StereoLiveSampler
{
    bool writing = true;
    bool Is_writing() const { return writing; }
} live_recorder;
struct audio_block_t { int16_t data[AUDIO_BLOCK_SAMPLES] = {}; };
class AudioStream
{
public:
    audio_block_t *pending[4] = {};
    std::vector<int16_t> output[2];
    int allocation_failure = -1;
    int allocations = 0;
    static inline int outstanding = 0;
    AudioStream(int channels, audio_block_t **) { assert(channels == 4); }
    virtual void update() = 0;
    audio_block_t *receiveReadOnly(unsigned channel)
    {
        audio_block_t *block = pending[channel];
        pending[channel] = nullptr;
        return block;
    }
    audio_block_t *allocate()
    {
        if (allocations++ == allocation_failure)
        {
            return nullptr;
        }
        ++outstanding;
        return new audio_block_t;
    }
    void release(audio_block_t *block)
    {
        assert(block != nullptr);
        --outstanding;
        delete block;
    }
    void transmit(audio_block_t *block, unsigned channel)
    {
        output[channel].insert(output[channel].end(), block->data, block->data + AUDIO_BLOCK_SAMPLES);
    }
    void Feed(int left, int right, int feedback_left = 0, int feedback_right = 0, unsigned mask = 15)
    {
        const int values[4] = {left, right, feedback_left, feedback_right};
        allocations = 0;
        for (unsigned channel = 0; channel < 4; ++channel)
        {
            if ((mask & (1u << channel)) != 0)
            {
                pending[channel] = new audio_block_t;
                ++outstanding;
                std::fill(std::begin(pending[channel]->data), std::end(pending[channel]->data), values[channel]);
            }
        }
    }
    void Clear_output()
    {
        output[0].clear();
        output[1].clear();
    }
};
'''

checks = r'''

void Run(LillaCompressor &processor, int blocks, int left, int right, int feedback_left = 0, int feedback_right = 0, unsigned mask = 15)
{
    for (int block = 0; block < blocks; ++block)
    {
        processor.Feed(left, right, feedback_left, feedback_right, mask);
        processor.update();
        assert(AudioStream::outstanding == 0);
    }
}
int main()
{
    LillaCompressor p(LillaCompressor::Profile::Delay);
    // Both bypass and enabled paths retain exactly 32 samples of history.
    for (bool enabled : {false, true})
    {
        p.Set_enabled(enabled);
        p.Reset();
        p.Clear_output();
        Run(p, 1, 1000, -500);
        for (unsigned i = 0; i < AUDIO_BLOCK_SAMPLES; ++i)
        {
            assert(p.output[0][i] == (i < 32 ? 0 : 1000));
            assert(p.output[1][i] == (i < 32 ? 0 : -500));
        }
    }
    p.Set_enabled(true);
    p.Reset();
    p.Clear_output();
    p.sample_clock = UINT32_MAX - 1500;
    // The input sum exceeds int16 range; compression must happen before clipping.
    Run(p, 8, 30000, 15000, 30000, 15000);
    for (unsigned i = 32; i < p.output[0].size(); ++i)
    {
        assert(std::abs(p.output[0][i]) <= 29204);
        assert(std::abs(p.output[0][i] - 2 * p.output[1][i]) <= 1);
    }
    assert(p.output[0].back() >= 29202);
    // Random, isolated and negative peaks, across block boundaries and clock rollover.
    std::mt19937 random(100);
    std::uniform_int_distribution<int> values(-32768, 32767);
    for (int block = 0; block < 400; ++block)
    {
        p.Clear_output();
        p.Feed(0, 0);
        for (unsigned sample = 0; sample < AUDIO_BLOCK_SAMPLES; ++sample)
        {
            for (unsigned channel = 0; channel < 4; ++channel)
            {
                p.pending[channel]->data[sample] = values(random);
            }
        }
        p.update();
        assert(AudioStream::outstanding == 0);
        for (unsigned sample = 0; sample < AUDIO_BLOCK_SAMPLES; ++sample)
        {
            assert(std::abs(p.output[0][sample]) <= 29204);
            assert(std::abs(p.output[1][sample]) <= 29204);
        }
    }
    p.Set_enabled(false);
    p.Reset();
    p.Clear_output();
    Run(p, 1, -32768, 32767, -32768, 32767);
    assert(p.output[0].back() == -32768 && p.output[1].back() == 32767);
    for (unsigned mask = 0; mask < 16; ++mask)
    {
        Run(p, 1, 100, -100, 100, -100, mask);
    }
    for (int failed : {0, 1})
    {
        p.allocation_failure = failed;
        p.Clear_output();
        Run(p, 1, 100, -100);
        assert(p.output[0].empty() && p.output[1].empty());
    }
    p.allocation_failure = -1;
    for (int block = 0; block < 20; ++block)
    {
        p.Set_enabled(block % 2 != 0);
        Run(p, 1, 20000, -20000, 20000, -20000);
    }
    p.Set_enabled(true);
    Run(p, 400, 1000, -500);
    assert(p.output[0].back() >= 999 && p.output[0].back() <= 1000);
    std::cout << "PASS: Delay stereo linking, 32-sample latency, unclipped sum, ceiling, bypass, release, rollover, missing inputs and allocation failures\n";
}
'''
production = source('lib/LillaCompressor/LillaCompressor.h').replace('private:', 'public:') + source('lib/LillaCompressor/LillaCompressor.cpp')
compiler = shutil.which('g++') or 'C:/msys64/ucrt64/bin/g++.exe'
environment = os.environ.copy()
environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
with tempfile.TemporaryDirectory(prefix='lilla-delay-compressor-') as folder:
    cpp, exe = Path(folder) / 'test.cpp', Path(folder) / 'test.exe'
    cpp.write_text(prefix + production + checks, encoding='utf-8')
    subprocess.run([compiler, '-std=c++20', '-O2', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True, env=environment)
    subprocess.run([str(exe)], check=True, env=environment)
