"""Exercise the production stereo compressor with host AudioStream stubs."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def source(name):
    return re.sub(r'^\s*#(?:include|pragma)[^\n]*', '', (ROOT / name).read_text(encoding='utf-8'), flags=re.M)


prefix = r'''
#define FLASHMEM
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
void Run(AudioLiveCompressor &processor, int blocks, int left, int right, int feedback_left = 0, int feedback_right = 0, unsigned mask = 15)
{
    processor.Recorder_ptr = &live_recorder;
    for (int block = 0; block < blocks; ++block)
    {
        processor.Feed(left, right, feedback_left, feedback_right, mask);
        processor.update();
        assert(AudioStream::outstanding == 0);
    }
}

int main()
{
    // All eight sound buttons, including S3, capture without changing compression.
    for (int slot = 0; slot < INSTRUMENTS; ++slot)
    {
        pressed = slot;
        captured = -1;
        const bool before = LS_Compressor.Is_enabled();
        Handle_buttons();
        assert(captured == slot && LS_Compressor.Is_enabled() == before);
    }
    LS_Compressor.Set_enabled(true);
    pressed = 2;
    captured = -1;
    Handle_buttons();
    assert(captured == 2 && LS_Compressor.Is_enabled());
    LS_Compressor.Set_enabled(false);

    for (unsigned mask = 0; mask < 4; ++mask)
    {
        audio_block_t *left = nullptr;
        audio_block_t *right = nullptr;
        if ((mask & 1) != 0)
        {
            left = new audio_block_t;
            ++AudioStream::outstanding;
        }
        if ((mask & 2) != 0)
        {
            right = new audio_block_t;
            ++AudioStream::outstanding;
        }
        bool continued = false;
        Recorder_input_guard(left, right, continued);
        assert(continued);
        assert(AudioStream::outstanding == 0);
    }

    pressed = EN_PB_Select;
    Handle_select();
    assert(LS_Compressor.Is_enabled() && Display_LiveSampler.enabled);
    Handle_select();
    assert(!LS_Compressor.Is_enabled() && !Display_LiveSampler.enabled);
    pressed = -1;
    Handle_select();
    assert(!LS_Compressor.Is_enabled());

    // Inactive updates drain all inputs without allocating or transmitting any output blocks.
    AudioLiveCompressor inactive;
    inactive.Set_enabled(true);
    Run(inactive, 8, 20000, -10000);
    inactive.Clear_output();
    live_recorder.writing = false;
    Run(inactive, 100, 25000, -25000);
    assert(inactive.allocations == 0 && inactive.output[0].empty() && inactive.output[1].empty());
    assert(inactive.Is_enabled());
    live_recorder.writing = true;
    Run(inactive, 1, 4000, -2000);
    for (int value : inactive.output[0])
    {
        assert(value == 0);
    }
    inactive.Clear_output();
    Run(inactive, 1, 4000, -2000);
    assert(inactive.output[0].front() == 4000 && inactive.output[1].front() == -2000);
    inactive.Recorder_ptr = nullptr;
    inactive.Clear_output();
    inactive.Feed(30000, 30000);
    inactive.update();
    assert(inactive.allocations == 0 && inactive.output[0].empty() && AudioStream::outstanding == 0);

    // Bypass is exact apart from a constant 128-sample delay, with no channel swap.
    AudioLiveCompressor dry;
    assert(!dry.Is_enabled());
    Run(dry, 1, 12000, -6000);
    assert(std::all_of(dry.output[0].begin(), dry.output[0].end(), [](int value) { return value == 0; }));
    dry.Clear_output();
    Run(dry, 1, -7000, 3000);
    assert(dry.output[0].front() == 12000 && dry.output[1].back() == -6000);

    // Enable below threshold: unchanged audio and unchanged delay.
    dry.Set_enabled(true);
    Run(dry, 10, 12000, -6000);
    assert(dry.output[0].back() == 12000 && dry.output[1].back() == -6000);

    // An isolated full-scale transient is caught, including after prolonged silence.
    AudioLiveCompressor impulse;
    impulse.Set_enabled(true);
    Run(impulse, 20, 0, 0);
    impulse.Clear_output();
    impulse.Feed(0, 0);
    impulse.pending[0]->data[127] = -32768;
    impulse.pending[1]->data[127] = 16384;
    impulse.update();
    Run(impulse, 2, 0, 0);
    assert(std::abs(static_cast<int>(impulse.output[0][255])) <= 29204);
    assert(std::abs(static_cast<int>(impulse.output[0][255])) > 20000);
    assert(std::abs(impulse.output[0][255] + 2 * impulse.output[1][255]) <= 2);

    // Sum before limiting: a 40000 input retains its correct curve value, not a clipped 32767.
    AudioLiveCompressor wide;
    wide.Set_feedback(1.0f);
    wide.Set_enabled(true);
    Run(wide, 1200, 20000, -10000, 20000, -10000);
    assert(wide.output[0].back() > 28500 && wide.output[0].back() <= 29204);
    assert(std::abs(wide.output[0].back() + 2 * wide.output[1].back()) <= 2);
    wide.Clear_output();
    Run(wide, 8, -32768, 16384, -32768, 16384);
    for (unsigned index = 0; index < wide.output[0].size(); ++index)
    {
        assert(std::abs(static_cast<int>(wide.output[0][index])) <= 29204);
        assert(std::abs(wide.output[0][index] + 2 * wide.output[1][index]) <= 2);
    }

    // Steady over-range stereo input: toggles and reversals have bounded sample changes.
    AudioLiveCompressor toggled;
    toggled.Set_feedback(1.0f);
    Run(toggled, 600, 30000, 10000, 30000, 10000);
    toggled.Clear_output();
    int previous[2] = {32767, 20000};
    for (int block = 0; block < 60; ++block)
    {
        if (block % 7 == 0)
        {
            toggled.Set_enabled(!toggled.Is_enabled());
        }
        if (block == 2 || block == 3)
        {
            toggled.Set_enabled(!toggled.Is_enabled());
        }
        Run(toggled, 1, 30000, 10000, 30000, 10000);
        for (unsigned channel = 0; channel < 2; ++channel)
        {
            for (int sample : toggled.output[channel])
            {
                assert(std::abs(sample - previous[channel]) < 100);
                previous[channel] = sample;
            }
        }
        toggled.Clear_output();
    }
    toggled.Set_enabled(false);
    Run(toggled, 8, 30000, 10000, 30000, 10000);
    assert(toggled.output[0].back() == 32767 && toggled.output[1].back() == 20000);

    // Random input, alternating signs and single-channel peaks exercise the sliding peak window.
    std::mt19937 random(1234);
    std::uniform_int_distribution<int> sample(-32768, 32767);
    wide.Clear_output();
    for (int block = 0; block < 1500; ++block)
    {
        wide.Feed(0, 0);
        for (unsigned channel = 0; channel < 4; ++channel)
        {
            for (int16_t &value : wide.pending[channel]->data)
            {
                value = static_cast<int16_t>(sample(random));
            }
        }
        wide.update();
        for (unsigned channel = 0; channel < 2; ++channel)
        {
            for (int value : wide.output[channel])
            {
                assert(std::abs(value) <= 29204);
            }
        }
        wide.Clear_output();
    }

    // Missing input channels are silence; missing outputs release all blocks and retain history.
    AudioLiveCompressor missing;
    Run(missing, 2, 12000, 0, 0, 0, 1);
    assert(missing.output[0].back() == 12000 && missing.output[1].back() == 0);
    for (int failure : {0, 1})
    {
        missing.Clear_output();
        missing.allocation_failure = failure;
        Run(missing, 1, 7000, -5000);
        assert(missing.output[0].empty() && missing.output[1].empty());
        missing.allocation_failure = -1;
        Run(missing, 1, 0, 0, 0, 0, 0);
        assert(missing.output[0].front() == 7000 && missing.output[1].front() == -5000);
    }
    assert(AudioStream::outstanding == 0);
    std::cout << "PASS: stereo link, wide sum, ceiling, lookahead, bypass/toggle continuity, rapid reversal, missing blocks and allocation failures\n";
}
'''

compiler = shutil.which('g++') or 'C:/msys64/ucrt64/bin/g++.exe'
environment = os.environ.copy()
environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
production = source('lib/AudioLiveCompressor/AudioLiveCompressor.h') + source('lib/AudioLiveCompressor/AudioLiveCompressor.cpp')
main_source = (ROOT / 'src/main.cpp').read_text(encoding='utf-8')
handler_start = main_source.index('        for (int Inst_id = 0;', main_source.index('Display_LiveSampler.Update_no_recorded_audio(live_unrecorded_notice);'))
handler_end = main_source.index('\n        /*', handler_start)
handler = r'''
constexpr int INSTRUMENTS = 8;
const int PB_Sound[INSTRUMENTS] = {0, 1, 2, 3, 4, 5, 6, 7};
int pressed = -1;
int captured = -1;
AudioLiveCompressor LS_Compressor;
struct DisplayStub
{
    bool enabled = false;
    void Compressor(bool value) { enabled = value; }
} Display_LiveSampler;
bool Read_pushbutton(int button) { return button == pressed; }
void AudioNoInterrupts() {}
void AudioInterrupts() {}
void LS_Capture_sound(int slot) { captured = slot; }
void Handle_buttons()
{
''' + main_source[handler_start:handler_end] + '\n}\n'
select_start = main_source.index('            case value_LS_Compressor:')
select_end = main_source.index('            case value_LS_Window:', select_start)
select_body = main_source[select_start:select_end].split(':', 1)[1].rsplit('break;', 1)[0]
handler += '\nconstexpr int EN_PB_Select = 99;\nvoid Handle_select()\n{\n' + select_body + '\n}\n'
recorder = (ROOT / 'lib/StereoLiveSampler/StereoLiveSampler.cpp').read_text(encoding='utf-8')
guard_start = recorder.index('    static const int16_t silence')
guard_end = recorder.index('    // microtimer', guard_start)
guard = r'''
void release(audio_block_t *block)
{
    assert(block != nullptr);
    --AudioStream::outstanding;
    delete block;
}
void Recorder_input_guard(audio_block_t *in_block_L, audio_block_t *in_block_R, bool &continued)
{
''' + recorder[guard_start:guard_end] + r'''
    assert(samples_L != nullptr && samples_R != nullptr);
    continued = true;
''' + recorder[recorder.rindex('    if (in_block_L != nullptr)'):recorder.index('    return;', recorder.rindex('    if (in_block_L != nullptr)'))] + r'''
}
'''
with tempfile.TemporaryDirectory(prefix='lilla-live-compressor-') as folder:
    cpp = Path(folder) / 'test.cpp'
    exe = Path(folder) / 'test.exe'
    cpp.write_text(prefix + production + handler + guard + checks, encoding='utf-8', newline='\r\n')
    subprocess.run([compiler, '-std=c++20', '-O2', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True, env=environment)
    subprocess.run([str(exe)], check=True, env=environment)
