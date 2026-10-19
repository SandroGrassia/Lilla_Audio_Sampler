"""Exercise production VCF menu handlers and legacy/shared-table cutoff decoding."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
main = (root / 'src/main.cpp').read_text(encoding='utf-8')
shared = (root / 'lib/SharedElements/SharedElements.h').read_text(encoding='utf-8')
header = (root / 'lib/SharedVCF/SharedVCF.h').read_text(encoding='utf-8')
implementation = (root / 'lib/SharedVCF/SharedVCF.cpp').read_text(encoding='utf-8')
table = re.search(r'constexpr int lowpass_value\[LPF_MAX \+ 1\] = .*?;', shared).group(0)
handlers = main[main.index('        case value_VCF_FilterType:'):main.index('        case value_VCF_Resonance:')]
toggle = re.search(r'^void Macro_VCF_filter_on_none\(void\)\n\{.*?^\}', main, re.M | re.S).group(0)
code = r'''
#include <cassert>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <iostream>
constexpr int LPF_MAX = 39;
''' + table + '\n' + re.sub(r'^#(?:include|pragma).*$', '', header + '\n' + implementation, flags=re.M) + r'''
struct FilterData { int use = 0, type = 0; uint8_t pivot = 10; };
struct Instrument { FilterData Filter; };
struct PatchData { Instrument Instrument[2]; } Patch[1];
struct PresetData { struct { int use = 0; float pivot = 0; } Filter; } Preset[2];
constexpr int LIVE_SAMPLING = 1, EN_PB_Value = 0, EN_PB_Select = 1;
int Patch_id = 0, Instrument_id = 0, Lilla_state_0 = 0, rotation = 0;
bool pressed = false;
void AudioNoInterrupts() {}
void AudioInterrupts() {}
bool Read_pushbutton(int) { bool result = pressed; pressed = false; return result; }
bool Read_encoder(int, int &value, int high, int low, int increment)
{
    int next = std::clamp(value + rotation * increment, low, high);
    rotation = 0;
    bool changed = next != value;
    value = next;
    return changed;
}
struct Manager
{
    int type_updates[2] = {}, cutoff_updates[2] = {};
    void Update_Preset_IF(int, int i) { Preset[i].Filter.use = Patch[0].Instrument[i].Filter.use; Preset[i].Filter.pivot = Calc_vcf_cutoff(Patch[0].Instrument[i].Filter.pivot); }
    void Multicast_IF_update_filter_type(int i) { ++type_updates[i]; }
    void Multicast_IF_pivot(int i) { ++cutoff_updates[i]; }
} Players_Manager;
struct Display { void VCF_show_filter_type_value(int) {} void VCF_show_cutoff_value(int) {} } Display_VCF;
''' + toggle + '\nvoid Edit(int item)\n{\n    switch (item)\n    {\n' + handlers + r'''
    }
}
int main()
{
    for (int legacy = 0; legacy <= 100; ++legacy)
    {
        assert(Calc_vcf_cutoff(legacy) == 20.0f * powf(2.0f, legacy / 10.0f));
    }
    for (int index = 0; index <= LPF_MAX; ++index)
    {
        assert(Calc_vcf_cutoff(VCF_CUTOFF_TABLE_BASE + index) == lowpass_value[index]);
        assert(VCF_cutoff_index(VCF_CUTOFF_TABLE_BASE + index) == index);
    }
    for (int context : {0, LIVE_SAMPLING})
    {
        Lilla_state_0 = context;
        Patch[0] = {};
        for (int selection = 1; selection <= 4; ++selection)
        {
            rotation = 1;
            Edit(value_VCF_FilterType);
            assert(Preset[0].Filter.use && Patch[0].Instrument[0].Filter.type == selection - 1);
        }
        pressed = true;
        Edit(value_VCF_FilterType);
        assert(!Preset[0].Filter.use);
        pressed = true;
        Edit(value_VCF_FilterType);
        assert(Preset[0].Filter.use && Patch[0].Instrument[0].Filter.type == 3);
        for (int selection = 3; selection >= 0; --selection)
        {
            rotation = -1;
            Edit(value_VCF_FilterType);
            assert(Preset[0].Filter.use == (selection != 0));
            if (context == LIVE_SAMPLING)
            {
                assert(Preset[1].Filter.use == Preset[0].Filter.use);
                assert(Patch[0].Instrument[1].Filter.type == Patch[0].Instrument[0].Filter.type);
            }
        }
        rotation = 1;
        Edit(value_VCF_FilterType);
        assert(Preset[0].Filter.use && Patch[0].Instrument[0].Filter.type == 0);
        Patch[0].Instrument[0].Filter.pivot = VCF_CUTOFF_TABLE_BASE;
        for (int index = 1; index <= LPF_MAX; ++index)
        {
            rotation = 1;
            Edit(value_VCF_Cutoff);
            assert(Preset[0].Filter.pivot == lowpass_value[index]);
            if (context == LIVE_SAMPLING)
            {
                assert(Preset[1].Filter.pivot == Preset[0].Filter.pivot);
            }
        }
        rotation = 1;
        Edit(value_VCF_Cutoff);
        assert(Preset[0].Filter.pivot == 20000);
        for (int index = LPF_MAX - 1; index >= 0; --index)
        {
            rotation = -1;
            Edit(value_VCF_Cutoff);
            assert(Preset[0].Filter.pivot == lowpass_value[index]);
        }
        rotation = -1;
        Edit(value_VCF_Cutoff);
        assert(Preset[0].Filter.pivot == 40);
    }
    assert(Players_Manager.type_updates[0] && Players_Manager.type_updates[1]);
    assert(Players_Manager.cutoff_updates[0] && Players_Manager.cutoff_updates[1]);
    std::cout << "PASS: NONE selection, pushbutton bypass, stereo controls, all LPF steps, bounds and legacy frequencies\n";
}
'''
compiler = shutil.which('g++') or 'C:/msys64/ucrt64/bin/g++.exe'
env = os.environ.copy()
env['PATH'] = str(Path(compiler).parent) + os.pathsep + env.get('PATH', '')
with tempfile.TemporaryDirectory(prefix='vcf-controls-') as folder:
    cpp, exe = Path(folder) / 'test.cpp', Path(folder) / 'test.exe'
    cpp.write_text(code, encoding='utf-8')
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True, env=env)
    subprocess.run([str(exe)], check=True, env=env)
