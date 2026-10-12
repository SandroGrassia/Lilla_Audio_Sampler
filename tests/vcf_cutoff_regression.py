"""Check production VCF cutoff ramps with and without simultaneous LFO edits."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'lib/AudioPlayer/AudioPlayer.cpp').read_text(encoding='utf-8')
methods = '\n'.join(re.search(r'^void AudioPlayer::' + name + r'\(.*?^\}', source, re.M | re.S).group(0) for name in ('Update_VCF_pivot', 'Send_LFO_to_VCF'))
code = r"""
#include <cassert>
#include <cmath>
#include <algorithm>
#include <iostream>
constexpr int AUDIO_BLOCK_SAMPLES = 128;
float constrain(float value, float low, float high) { return std::clamp(value, low, high); }
struct Filter
{
    bool LFO_connect = false;
    float first = 0;
    void Set_filter(int, float frequency) { first = frequency; }
};
struct Wave { float block[128] = {}; };
struct AudioPlayer
{
    bool VCF_modulated = false;
    float VCF_frequency_pivot = 1000, VCF_pivot_target = 1000, VCF_pivot_grain = 0;
    float VCF_central_frequency = 1000, VCF_frequency_array[4] = {}, pitch = 2;
    int VCF_pivot_steps = 0, LFO_index_steps = 0;
    float LFO_index = 1, LFO_index_grain = 0;
    Filter filter;
    Wave wave;
    Filter *VCF_ptr = &filter;
    Wave *LFO_ptr = &wave;
    void Update_VCF_pivot(float);
    void Send_LFO_to_VCF();
};
""" + methods + r"""
int main()
{
    // Pitch bend/vibrato alone must not retune the established VCF center.
    AudioPlayer probe;
    probe.VCF_modulated = true;
    probe.pitch = 1;
    probe.Send_LFO_to_VCF();
    assert(probe.filter.first == 1000);
    probe.pitch = 2;
    probe.LFO_index_steps = 1000;
    probe.LFO_index_grain = 0.001f;
    probe.Send_LFO_to_VCF();
    assert(probe.filter.first == 1000);
    probe.Update_VCF_pivot(2000);
    for (int block = 0; block < 10; ++block)
    {
        probe.Send_LFO_to_VCF();
    }
    assert(probe.VCF_central_frequency == 4000);
    probe.pitch = 0.5f;
    probe.Send_LFO_to_VCF();
    assert(probe.filter.first == 4000);
    for (bool modulated : {false, true})
    {
        AudioPlayer player;
        player.VCF_modulated = modulated;
        player.LFO_index_steps = 1000;
        player.LFO_index_grain = 0.001f;
        player.Update_VCF_pivot(4000);
        for (int block = 0; block < 10; ++block)
        {
            player.Send_LFO_to_VCF();
            for (int section = 0; section < 4; ++section)
            {
                const float expected = (1000 + 75 * (block * 4 + section + 1)) * (modulated ? 2 : 1);
                assert(std::fabs(player.VCF_frequency_array[section] - expected) < 0.01f);
            }
            assert(player.filter.first == player.VCF_frequency_array[0]);
        }
        assert(player.VCF_frequency_pivot == 4000 && player.VCF_pivot_steps == 0);
        assert(player.LFO_index_steps == (modulated ? 960 : 1000));
        // Small changes must not be discarded; mid-ramp edits start from the current value.
        player.Update_VCF_pivot(4001);
        player.Send_LFO_to_VCF();
        assert(player.VCF_frequency_pivot > 4000);
        player.Update_VCF_pivot(20);
        float previous = player.VCF_frequency_pivot;
        for (int block = 0; block < 10; ++block)
        {
            player.Send_LFO_to_VCF();
            assert(player.VCF_frequency_pivot < previous);
            previous = player.VCF_frequency_pivot;
        }
        assert(player.VCF_frequency_pivot == 20 && player.VCF_pivot_steps == 0);
        // LFO modulation is retained and constrained to the supported range.
        if (modulated)
        {
            player.wave.block[0] = 1000;
            player.VCF_frequency_pivot = 1000;
            player.VCF_central_frequency = 2000;
            player.Send_LFO_to_VCF();
            assert(std::fabs(player.filter.first - 2000 * std::exp2(player.LFO_index - 0.003f)) < 0.1f);
        }
    }
    std::cout << "PASS: live VCF cutoff, simultaneous LFO edits, small changes and ramp retargeting\n";
}
"""
compiler = shutil.which('g++') or 'C:/msys64/ucrt64/bin/g++.exe'
env = os.environ.copy()
env['PATH'] = str(Path(compiler).parent) + os.pathsep + env.get('PATH', '')
with tempfile.TemporaryDirectory(prefix='vcf-cutoff-') as folder:
    cpp = Path(folder) / 'test.cpp'
    exe = Path(folder) / 'test.exe'
    cpp.write_text(code, encoding='utf-8', newline='\r\n')
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True, env=env)
    subprocess.run([str(exe)], check=True, env=env)

# Exercise the production DSP loop: updated coefficients must affect the same block.
header = (root / 'lib/AudioVCF/AudioVCF.h').read_text(encoding='utf-8')
implementation = (root / 'lib/AudioVCF/AudioVCF.cpp').read_text(encoding='utf-8')
dsp = r"""
#include <cstdint>
#include <cmath>
#include <cassert>
#include <algorithm>
#include <iostream>
constexpr int AUDIO_BLOCK_SAMPLES = 128;
constexpr float AUDIO_SAMPLE_RATE_EXACT = 44100;
int32_t signed_multiply_accumulate_32x16b(int32_t sum, int32_t coefficient, uint32_t samples)
{
    return static_cast<int32_t>(static_cast<int64_t>(sum) + ((static_cast<int64_t>(coefficient) * static_cast<int16_t>(samples)) >> 16));
}
int32_t signed_multiply_accumulate_32x16t(int32_t sum, int32_t coefficient, uint32_t samples)
{
    return static_cast<int32_t>(static_cast<int64_t>(sum) + ((static_cast<int64_t>(coefficient) * static_cast<int16_t>(samples >> 16)) >> 16));
}
int32_t signed_saturate_rshift(int32_t value, int, int shift) { return std::clamp(value >> shift, -32768, 32767); }
uint32_t pack_16b_16b(int32_t high, int32_t low) { return (static_cast<uint32_t>(high) << 16) | (static_cast<uint32_t>(low) & 65535); }
"""
dsp += '\n'.join(line for line in (header + '\n' + implementation).splitlines() if not line.startswith(('#include', '#pragma')))
dsp += r"""
int main()
{
    alignas(4) int16_t fixed[128], changed[128];
    std::fill_n(fixed, 128, 1000);
    std::fill_n(changed, 128, 1000);
    float frequencies[4] = {200, 5000, 3000, 1000};
    AudioVCF reference, ramp;
    reference._block = fixed;
    ramp._block = changed;
    ramp._frequency = frequencies;
    ramp.LFO_connect = true;
    reference.Set_filter(0, 200);
    ramp.Set_filter(0, 200);
    reference.Update();
    ramp.Update();
    for (int sample = 0; sample < 32; ++sample)
    {
        assert(fixed[sample] == changed[sample]);
    }
    bool changed_in_same_block = false;
    for (int sample = 32; sample < 64; ++sample)
    {
        changed_in_same_block |= fixed[sample] != changed[sample];
    }
    assert(changed_in_same_block);
    std::cout << "PASS: VCF coefficients take effect within the current audio block\n";
}
"""
with tempfile.TemporaryDirectory(prefix='vcf-dsp-') as folder:
    cpp = Path(folder) / 'test.cpp'
    exe = Path(folder) / 'test.exe'
    cpp.write_text(dsp, encoding='utf-8', newline='\r\n')
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True, env=env)
    subprocess.run([str(exe)], check=True, env=env)
