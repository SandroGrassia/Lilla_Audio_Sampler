"""Check recorder ownership, mono rounding, fades and circular writes with shared inputs."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def source(path):
    return re.sub(r'^\s*#(?:include|pragma)[^\n]*', '', (ROOT / path).read_text(encoding='utf-8'), flags=re.M)


prefix = r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
constexpr int AUDIO_BLOCK_SAMPLES = 128;
constexpr int AUDIO_BLOCK_BYTES = 256;
int LS_buffer_dim = 4096;
struct audio_block_t
{
    int16_t data[AUDIO_BLOCK_SAMPLES];
    int references = 2;
};
class AudioStream
{
public:
    audio_block_t *pending[2] = {};
    AudioStream(int channels, audio_block_t **) { assert(channels == 2); }
    virtual void update() = 0;
    audio_block_t *receiveReadOnly(unsigned channel)
    {
        auto *block = pending[channel];
        pending[channel] = nullptr;
        return block;
    }
    void release(audio_block_t *block)
    {
        assert(block != nullptr && block->references == 2);
        --block->references;
    }
};
'''

checks = r'''
int main()
{
    for (bool stereo : {false, true})
    {
        StereoLiveSampler recorder;
        int16_t mono[4096] = {};
        int16_t left[4096] = {};
        int16_t right[4096] = {};
        recorder.LS_buffer_mono_ptr = mono;
        recorder.LS_buffer_L_ptr = left;
        recorder.LS_buffer_R_ptr = right;
        recorder.Reset();
        recorder.Start(stereo);
        auto run = [&]()
        {
            audio_block_t input_l;
            audio_block_t input_r;
            std::fill(std::begin(input_l.data), std::end(input_l.data), 12001);
            std::fill(std::begin(input_r.data), std::end(input_r.data), -3001);
            recorder.pending[0] = &input_l;
            recorder.pending[1] = &input_r;
            recorder.update();
            assert(input_l.references == 1 && input_r.references == 1);
            for (int sample = 0; sample < AUDIO_BLOCK_SAMPLES; ++sample)
            {
                assert(input_l.data[sample] == 12001 && input_r.data[sample] == -3001);
            }
        };
        for (int block = 0; block < 4; ++block)
        {
            run();
        }
        assert(recorder.Q_sample == 511 && recorder.Is_writing());
        assert((stereo ? left[0] : mono[0]) == 0);
        for (int sample = 384; sample < 512; ++sample)
        {
            assert(stereo ? left[sample] == 12001 && right[sample] == -3001 : mono[sample] == 4499);
        }
        recorder.Stop();
        run();
        assert(recorder.Is_writing());
        run();
        assert(recorder.Is_writing());
        run();
        assert(!recorder.Is_writing());
        assert((stereo ? left[895] : mono[895]) < (stereo ? left[512] : mono[512]));
        recorder.Start(stereo);
        for (int block = 0; block < 26; ++block)
        {
            run();
        }
        assert(recorder.Q_sample == 127 && !recorder.first_write_flag);
        assert(stereo ? left[0] == 12001 && right[0] == -3001 : mono[0] == 4499);
        assert((stereo ? left[128] : mono[128]) == 0);
        audio_block_t only_left = {};
        recorder.pending[0] = &only_left;
        recorder.update();
        assert(only_left.references == 1 && recorder.Q_sample == 127);
        recorder.update();
        assert(recorder.Q_sample == 127);
    }
    std::cout << "PASS: shared inputs unchanged/released, mono rounding, stereo separation, fades, wrap and missing inputs\n";
}
'''

compiler = shutil.which('g++') or 'C:/msys64/ucrt64/bin/g++.exe'
environment = os.environ.copy()
environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
production = source('lib/StereoLiveSampler/StereoLiveSampler.h') + source('lib/StereoLiveSampler/StereoLiveSampler.cpp')
with tempfile.TemporaryDirectory(prefix='lilla-live-recorder-') as folder:
    cpp = Path(folder) / 'test.cpp'
    exe = Path(folder) / 'test.exe'
    cpp.write_text(prefix + production + checks, encoding='utf-8', newline='\r\n')
    subprocess.run([compiler, '-std=c++20', '-O2', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True, env=environment)
    subprocess.run([str(exe)], check=True, env=environment)
