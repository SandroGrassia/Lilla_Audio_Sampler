"""Check the production MIDI pitch table against the four advertised key intervals."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / 'src/main.cpp').read_text(encoding='utf-8')
method = re.search(r'^void Calc_pitch_from_note\(const int &key_step\)\n\{.*?^\}', source, re.M | re.S).group(0)
prefix = r'''
#include <cassert>
#include <cmath>
#include <iostream>
constexpr int NOTE_NUMBERS = 128;
float pitch_from_note[NOTE_NUMBERS];
'''
checks = r'''
int main()
{
    const double cents_per_key[] = {100.0, 50.0, 25.0, 12.5};
    const int octave_steps[] = {12, 24, 48, 96};
    for (int setting = 0; setting < 4; ++setting)
    {
        Calc_pitch_from_note(setting);
        assert(pitch_from_note[60] == 1.0f);
        for (int note = 0; note < NOTE_NUMBERS; ++note)
        {
            const double expected = std::exp2((note - 60) * cents_per_key[setting] / 1200.0);
            assert(std::abs(pitch_from_note[note] / expected - 1.0) < 0.000001);
            if (note > 0)
            {
                const double cents = 1200.0 * std::log2(static_cast<double>(pitch_from_note[note]) / pitch_from_note[note - 1]);
                assert(pitch_from_note[note] > pitch_from_note[note - 1]);
                assert(std::abs(cents - cents_per_key[setting]) < 0.002);
            }
            if (note + octave_steps[setting] < NOTE_NUMBERS)
            {
                const double ratio = static_cast<double>(pitch_from_note[note + octave_steps[setting]]) / pitch_from_note[note];
                assert(std::abs(ratio - 2.0) < 0.000002);
            }
        }
    }
    std::cout << "PASS: all 128 MIDI notes, root reference, monotonic pitch, 100/50/25/12.5 cents and octave spacing\n";
}
'''
compiler = shutil.which('g++') or 'C:/msys64/ucrt64/bin/g++.exe'
environment = os.environ.copy()
environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
with tempfile.TemporaryDirectory(prefix='lilla-key-step-') as folder:
    cpp = Path(folder) / 'test.cpp'
    exe = Path(folder) / ('test.exe' if os.name == 'nt' else 'test')
    cpp.write_text(prefix + method + checks, encoding='utf-8', newline='\r\n')
    subprocess.run([compiler, '-std=c++20', '-O2', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True, env=environment)
    subprocess.run([str(exe)], check=True, env=environment)
