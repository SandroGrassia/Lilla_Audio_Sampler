"""Check bounded background reads, IRQ restoration and SPI ownership across MIDI activity."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / 'lib/LillaSerialFlash/LillaSerialFlash.cpp').read_text(encoding='utf-8')
method = re.search(r'^bool LillaSerialFlashFile::Read_audio_samples_background\(.*?^}', source, re.M | re.S).group(0)
prefix = r'''
#include <cassert>
#include <cstdint>
#include <algorithm>
bool enabled = true;
int owners = 0, calls = 0, fail_at = -1, serviced = 0;
constexpr int IRQ_SOFTWARE = 0;
bool NVIC_IS_ENABLED(int) { return enabled; }
void NVIC_DISABLE_IRQ(int) { enabled = false; }
void NVIC_ENABLE_IRQ(int) { enabled = true; }
void AudioStartUsingSPI() { assert(!enabled); ++owners; }
void AudioStopUsingSPI() { assert(!enabled && owners > 0); --owners; }
struct LillaSerialFlashFile
{
    static bool Read_audio_samples_background(int, int16_t *, int, int);
    static bool Read_audio_samples(int, int16_t *out, int first, int count)
    {
        assert(owners > 0 && count > 0 && count <= 128);
        if (enabled)
        {
            // A pending audio callback can run between transfers, start a Flash voice and stop it again.
            enabled = false;
            AudioStartUsingSPI();
            AudioStopUsingSPI();
            enabled = true;
            ++serviced;
        }
        assert(owners > 0);
        if (calls++ == fail_at)
        {
            return false;
        }
        for (int i = 0; i < count; ++i)
        {
            out[i] = first + i;
        }
        return true;
    }
};
'''
tests = r'''
int main()
{
    for (bool initial_irq : {false, true})
    {
        for (int initial_owners : {0, 2})
        {
            for (int failure : {-1, 0, 2})
            {
                enabled = initial_irq;
                owners = initial_owners;
                calls = serviced = 0;
                fail_at = failure;
                int16_t samples[502];
                std::fill_n(samples, 502, -1);
                const bool success = LillaSerialFlashFile::Read_audio_samples_background(1, samples + 1, 37, 500);
                assert(success == (failure < 0));
                assert(enabled == initial_irq && owners == initial_owners);
                assert(calls == (failure < 0 ? 4 : failure + 1));
                assert(serviced == (initial_irq ? calls : 0));
                assert(samples[0] == -1 && samples[501] == -1);
                const int copied = failure < 0 ? 500 : failure * 128;
                for (int i = 0; i < copied; ++i)
                {
                    assert(samples[i + 1] == 37 + i);
                }
            }
        }
    }
}
'''
compiler = shutil.which('g++') or r'C:\msys64\ucrt64\bin\g++.exe'
with tempfile.TemporaryDirectory(prefix='lilla-background-flash-') as directory:
    cpp = Path(directory) / 'test.cpp'
    exe = Path(directory) / 'test.exe'
    cpp.write_text(prefix + method + tests, encoding='utf-8', newline='\r\n')
    subprocess.run([compiler, '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    environment = os.environ.copy()
    environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
    subprocess.run([str(exe)], check=True, env=environment)
print('PASS: bounded reads, audio service between chunks, IRQ/SPI restoration and read failures')
