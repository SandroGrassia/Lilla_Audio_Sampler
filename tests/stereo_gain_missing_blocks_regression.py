"""Verify production StereoGain releases every received block when either input is missing."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
production = '\n'.join(re.sub(r'^#(?:include|pragma).*$', '', (ROOT / path).read_text(encoding='utf-8'), flags=re.M) for path in ('lib/StereoGain/StereoGain.h', 'lib/StereoGain/StereoGain.cpp'))
prefix = r'''
#include <cassert>
#include <cstdint>
#include <cmath>
#include <cstdio>
using std::abs;
constexpr int AUDIO_BLOCK_SAMPLES = 128, SAMPLES_VOLUME = 128;
struct audio_block_t { int16_t data[AUDIO_BLOCK_SAMPLES] = {}; };
class AudioStream
{
public:
    audio_block_t blocks[2];
    bool held[2] = {};
    int mask = 0, releases = 0, transmissions = 0;
    AudioStream(int, audio_block_t**) {}
    virtual void update() = 0;
    audio_block_t *receiveWritable(int channel)
    {
        assert(!held[channel]);
        held[channel] = (mask & (1 << channel)) != 0;
        return held[channel] ? &blocks[channel] : nullptr;
    }
    void release(audio_block_t *block)
    {
        assert(block != nullptr);
        const int channel = block == &blocks[0] ? 0 : 1;
        assert(block == &blocks[channel] && held[channel]);
        held[channel] = false;
        ++releases;
    }
    void transmit(audio_block_t *block, int channel)
    {
        assert(block == &blocks[channel] && held[channel]);
        ++transmissions;
    }
};
'''
checks = r'''
int main()
{
    StereoGain gain;
    for (int iteration = 0; iteration < 100; ++iteration)
    {
        for (int mask = 0; mask < 4; ++mask)
        {
            gain.mask = mask;
            const int releases = gain.releases;
            const int transmissions = gain.transmissions;
            gain.update();
            assert(!gain.held[0] && !gain.held[1]);
            assert(gain.releases == releases + ((mask & 1) != 0) + ((mask & 2) != 0));
            assert(gain.transmissions == transmissions + (mask == 3 ? 2 : 0));
        }
    }
    std::puts("PASS: StereoGain missing L/R/both, paired output, no null release or retained block");
}
'''
compiler = shutil.which('g++') or r'C:\msys64\ucrt64\bin\g++.exe'
environment = os.environ.copy()
environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
with tempfile.TemporaryDirectory(prefix='lilla-stereo-gain-') as directory:
    cpp = Path(directory) / 'test.cpp'
    exe = Path(directory) / 'test.exe'
    cpp.write_text(prefix + production + checks, encoding='utf-8', newline='\r\n')
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True, env=environment)
    subprocess.run([str(exe)], check=True, env=environment)
