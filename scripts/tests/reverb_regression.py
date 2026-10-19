"""Host tests for the production coupled stereo reverb and caller-owned PSRAM buffers."""
from pathlib import Path
import ast
import os
import re
import shutil
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[2]
fixture = ast.parse((ROOT / 'scripts/tests/delay_compressor_regression.py').read_text())
prefix = next(ast.literal_eval(n.value) for n in fixture.body if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'prefix' for t in n.targets))
prefix = prefix.replace('assert(channels == 4)', 'assert(channels == 2)').replace('unsigned mask = 15', 'unsigned mask = 3')
def source(name):
    return re.sub(r'^\s*#(?:include|pragma)[^\n]*', '', (ROOT / name).read_text(), flags=re.M)
checks = r''' 
int main()
{
    int16_t memory_l[LillaReverb::CHANNEL_SAMPLES + 2] = {};
    int16_t memory_r[LillaReverb::CHANNEL_SAMPLES + 2] = {};
    memory_l[0] = memory_r[0] = 1234;
    memory_l[LillaReverb::CHANNEL_SAMPLES + 1] = memory_r[LillaReverb::CHANNEL_SAMPLES + 1] = 5678;
    LillaReverb p;
    p.Set_parameters(0, 15, 50);
    p.Setup(memory_l + 1, memory_r + 1);
    p.Feed(12345, -23456);
    p.update();
    assert(p.output[0][0] == 12345 && p.output[1][0] == -23456);
    // Zero decay works on first use and after a nonzero setting, with no recirculating tail.
    for (int previous : {0, 40})
    {
        LillaReverb zero;
        zero.Set_parameters(100, previous, 0);
        zero.Set_parameters(100, 0, 0);
        zero.Setup(memory_l + 1, memory_r + 1);
        bool heard = false;
        for (int block = 0; block < 50; ++block)
        {
            zero.Clear_output();
            zero.Feed(0, 0);
            if (block == 0)
            {
                zero.pending[0]->data[0] = 10000;
            }
            zero.update();
            for (int value : zero.output[0])
            {
                heard = heard || value != 0;
                if (block >= 20)
                {
                    assert(value == 0);
                }
            }
        }
        assert(heard && AudioStream::outstanding == 0);
    }
    p.Set_parameters(100, 15, 50);
    p.Reset();
    double early = 0.0;
    double late = 0.0;
    bool crossed = false;
    for (int block = 0; block < 1800; ++block)
    {
        p.Clear_output();
        p.Feed(0, 0);
        if (block == 0)
        {
            p.pending[0]->data[0] = 30000;
        }
        p.update();
        for (unsigned i = 0; i < AUDIO_BLOCK_SAMPLES; ++i)
        {
            const double energy = static_cast<double>(p.output[0][i]) * p.output[0][i] + static_cast<double>(p.output[1][i]) * p.output[1][i];
            if (block < 200)
            {
                early += energy;
            }
            if (block >= 1600)
            {
                late += energy;
            }
            crossed = crossed || p.output[1][i] != 0;
        }
        assert(AudioStream::outstanding == 0);
    }
    assert(crossed && early > 0.0 && late == 0.0);
    // Maximum decay and correlated full-scale inputs remain bounded without flat-topped output.
    p.Set_parameters(100, 50, 0);
    p.Reset();
    for (int block = 0; block < 1200; ++block)
    {
        p.Clear_output();
        p.Feed(32767, 32767);
        p.update();
        for (unsigned channel = 0; channel < 2; ++channel)
        {
            for (int value : p.output[channel])
            {
                assert(value >= 0 && value < 32767);
            }
        }
    }
    p.Set_parameters(0, 2, 100);
    for (int block = 0; block < 250; ++block)
    {
        p.Feed(12345, -23456);
        p.update();
    }
    assert(p.output[0].back() == 12345 && p.output[1].back() == -23456);
    p.Suspend(true);
    const int16_t before = memory_l[1];
    p.Feed(1000, -1000);
    p.update();
    assert(memory_l[1] == before && p.output[0].back() == 1000);
    p.Reset();
    p.Suspend(false);
    p.Set_parameters(100, 15, 50);
    p.Reset();
    for (unsigned mask = 0; mask < 4; ++mask)
    {
        p.Feed(0, 0, 0, 0, mask);
        p.update();
        assert(AudioStream::outstanding == 0);
    }
    for (int failure : {0, 1})
    {
        p.Clear_output();
        p.allocation_failure = failure;
        p.Feed(1000, -1000);
        p.update();
        assert(p.output[0].empty() && p.output[1].empty() && AudioStream::outstanding == 0);
    }
    assert(memory_l[0] == 1234 && memory_r[0] == 1234);
    assert(memory_l[LillaReverb::CHANNEL_SAMPLES + 1] == 5678 && memory_r[LillaReverb::CHANNEL_SAMPLES + 1] == 5678);
    // At a fixed 90% mix, moderate sinusoids must remain linear despite dry/wet cancellation.
    for (int frequency : {100, 440, 1000})
    {
        LillaReverb a;
        LillaReverb b;
        int16_t a_l[LillaReverb::CHANNEL_SAMPLES] = {};
        int16_t a_r[LillaReverb::CHANNEL_SAMPLES] = {};
        int16_t b_l[LillaReverb::CHANNEL_SAMPLES] = {};
        int16_t b_r[LillaReverb::CHANNEL_SAMPLES] = {};
        a.Set_parameters(90, 40, 0);
        b.Set_parameters(90, 40, 0);
        a.Setup(a_l, a_r);
        b.Setup(b_l, b_r);
        for (int block = 0; block < 900; ++block)
        {
            a.Clear_output();
            b.Clear_output();
            a.Feed(0, 0);
            b.Feed(0, 0);
            for (unsigned sample = 0; sample < AUDIO_BLOCK_SAMPLES; ++sample)
            {
                const int value = static_cast<int>(3000.0 * std::sin(6.28318530718 * frequency * (block * AUDIO_BLOCK_SAMPLES + sample) / AUDIO_SAMPLE_RATE_EXACT));
                a.pending[0]->data[sample] = a.pending[1]->data[sample] = value;
                b.pending[0]->data[sample] = b.pending[1]->data[sample] = 2 * value;
            }
            a.update();
            b.update();
            if (block >= 700)
            {
                for (unsigned sample = 0; sample < AUDIO_BLOCK_SAMPLES; ++sample)
                {
                    assert(std::abs(b.output[0][sample] - 2 * a.output[0][sample]) <= 1);
                    assert(std::abs(b.output[1][sample] - 2 * a.output[1][sample]) <= 1);
                }
            }
        }
    }
    // Reported case: four-second decay, no damping, fixed and changing high wet mixes.
    for (int mix : {0, 80, 90, 98, 100})
    {
        p.allocation_failure = -1;
        p.Set_parameters(mix, 40, 0);
        p.Reset();
        std::mt19937 generator(42);
        std::uniform_int_distribution<int> noise(-12000, 12000);
        double input_energy = 0.0;
        double output_energy = 0.0;
        int clipped = 0;
        for (int block = 0; block < 900; ++block)
        {
            p.Clear_output();
            p.Feed(0, 0);
            for (unsigned i = 0; i < AUDIO_BLOCK_SAMPLES; ++i)
            {
                const int value = noise(generator);
                p.pending[0]->data[i] = value;
                p.pending[1]->data[i] = value;
                if (block >= 700)
                {
                    input_energy += static_cast<double>(value) * value;
                }
            }
            p.update();
            if (block >= 700)
            {
                for (int value : p.output[0])
                {
                    output_energy += static_cast<double>(value) * value;
                    clipped += value == -32768 || value == 32767;
                }
            }
        }
        const double rms_db = 10.0 * std::log10(output_energy / input_energy);
        assert(clipped == 0 && rms_db > -10.0);
        std::cout << "Decay 4s, damping 0, level " << mix << "%: RMS " << rms_db << " dB, clipped " << clipped << "\n";
    }
    std::cout << "PASS: exact OFF, stereo crossfeed, decaying tail, full-scale stability, Panic suspension/reset, buffer guards, missing inputs and allocation failures\n";
}
'''
compiler = shutil.which('g++') or 'C:/msys64/ucrt64/bin/g++.exe'
env = os.environ.copy()
env['PATH'] = str(Path(compiler).parent) + os.pathsep + env.get('PATH', '')
with tempfile.TemporaryDirectory(prefix='lilla-reverb-') as folder:
    cpp = Path(folder) / 'test.cpp'
    exe = Path(folder) / 'test.exe'
    cpp.write_text(prefix + source('lib/LillaReverb/LillaReverb.h') + source('lib/LillaReverb/LillaReverb.cpp') + checks)
    subprocess.run([compiler, '-std=c++20', '-O2', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True, env=env)
    subprocess.run([str(exe)], check=True, env=env)
