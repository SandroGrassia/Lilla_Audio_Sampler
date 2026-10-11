"""Check production timbre filter response, bypass, transitions and voice reset."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = r"""
#include "PlayerToneFilter.h"
#include <cassert>
#include <cmath>
#include <cstdint>
int main()
{
    const float pitch = std::exp2(11.0f / 12.0f);
    assert(std::fabs(PlayerToneFilter::Cutoff(1.0f) - 20000.0f) < 1.0f);
    assert(std::fabs(PlayerToneFilter::Cutoff(pitch) - 4500.0f) < 1.0f);
    assert(std::fabs(PlayerToneFilter::Cutoff(35.0f) - 4500.0f) < 1.0f);
    PlayerToneFilter filter;
    int16_t block[128];
    for (int i = 0; i < 128; ++i)
    {
        block[i] = i * 200 - 12000;
    }
    filter.Process(block, 128, pitch, false, 44100.0f);
    for (int i = 0; i < 128; ++i)
    {
        assert(block[i] == i * 200 - 12000);
    }
    for (const float frequency : {1000.0f, 4500.0f, 10000.0f})
    {
        filter.Reset();
        double input_power = 0.0, output_power = 0.0;
        for (int b = 0; b < 160; ++b)
        {
            for (int i = 0; i < 128; ++i)
            {
                block[i] = static_cast<int16_t>(10000.0 * std::sin(6.28318530718 * frequency * (b * 128 + i) / 44100.0));
                if (b > 20)
                {
                    input_power += double(block[i]) * block[i];
                }
            }
            filter.Process(block, 128, pitch, true, 44100.0f);
            if (b > 20)
            {
                for (int i = 0; i < 128; ++i)
                {
                    output_power += double(block[i]) * block[i];
                }
            }
        }
        const double gain = std::sqrt(output_power / input_power);
        const double ratio = std::tan(3.14159265359 * frequency / 44100.0) / std::tan(3.14159265359 * 4500.0 / 44100.0);
        const double expected = 1.0 / std::sqrt(1.0 + std::pow(ratio, 4.0));
        assert(std::fabs(gain - expected) < 0.003);
    }
    for (int b = 0; b < 300; ++b)
    {
        for (int i = 0; i < 128; ++i)
        {
            block[i] = 1000;
        }
        filter.Process(block, 128, b % 2 ? 1.01f : 35.0f, b % 3 != 0, 44100.0f);
    }
    filter.Reset();
    for (auto &sample : block)
    {
        sample = 0;
    }
    filter.Process(block, 128, pitch, true, 44100.0f);
    for (auto sample : block)
    {
        assert(sample == 0);
    }
}
""".replace('#include <cstdint>', '#include <cstdint>\n#include <initializer_list>')
compiler = shutil.which('g++') or 'C:/msys64/ucrt64/bin/g++.exe'
environment = os.environ.copy()
environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
with tempfile.TemporaryDirectory() as temp:
    folder = Path(temp)
    test = folder / 'test.cpp'
    test.write_text(source)
    exe = folder / 'test.exe'
    subprocess.run([compiler, '-std=c++20', '-O2', '-I', str(root / 'lib/AudioPlayer'), str(test), str(root / 'lib/AudioPlayer/PlayerToneFilter.cpp'), '-o', str(exe)], check=True, env=environment)
    subprocess.run([str(exe)], check=True, env=environment)
print('Tone filter response, bypass, transitions and reset passed.')
