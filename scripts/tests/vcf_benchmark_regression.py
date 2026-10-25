"""Check diagnostic-only voice admission while preserving production protections."""
from pathlib import Path
import os
import runpy
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
base = runpy.run_path(str(Path(__file__).with_name("player_read_budget_regression.py")))
checks = r"""
int main()
{
    PlayersManager manager;
    for (auto &voice : manager.voices)
    {
        voice.playing = true;
        voice.cost = 500.0f;
    }
    assert(manager.Admit_read_budget(0, 0, 4000.0f));
    assert(!manager.Admit_read_budget(0, 0, INFINITY));
    manager.Prepare_read_budget();
    assert(manager.budget_reserved_us == 8000.0f);
    for (const auto &voice : manager.voices)
    {
        assert(voice.playing && voice.retired == 0);
    }
    assert(manager.budget_retired_players == 0);
    std::cout << "PASS: benchmark admits all 16 voices above budget, rejects invalid geometry and never retires for estimated cost\n";
}
"""
compiler = base["compiler"]
with tempfile.TemporaryDirectory(prefix="lilla-vcf-benchmark-") as folder:
    cpp = Path(folder) / "benchmark.cpp"
    exe = Path(folder) / "benchmark.exe"
    cpp.write_text(base["prefix"] + base["production"] + checks, encoding="utf-8")
    subprocess.run([compiler, "-std=c++20", "-DLILLA_VCF_BENCHMARK", "-include", str(Path(__file__).with_name("sd_sound_source_stub.h")), "-I" + str(root / "lib/AudioPlayer"), "-I" + str(root / "lib/config"), str(cpp), "-o", str(exe)], check=True)
    env = os.environ.copy()
    env["PATH"] = str(Path(compiler).parent) + os.pathsep + env.get("PATH", "")
    subprocess.run([str(exe)], check=True, env=env)

# Execute the production timed block with a deterministic wrapping cycle counter.
import re
header = (root / "lib/AudioPlayer/AudioPlayer.h").read_text(encoding="utf-8")
player = (root / "lib/AudioPlayer/AudioPlayer.cpp").read_text(encoding="utf-8")
timing = re.search(r"    struct VcfTiming.*?    VcfTiming vcf_timing;[^\n]*", header, re.S).group(0)
start = player.index("        if (VCF_connect)\n        {\n#if defined(LILLA_VCF_BENCHMARK)")
end = player.index("        if (live_forward_end)", start)
block = player[start:end]
fixture = r"""
#include <algorithm>
#include <cassert>
#include <cstdint>
using std::max;
uint32_t clock_cycles = 0;
#define ARM_DWT_CYCCNT clock_cycles
struct Filter { int filter_type = 2; float q_value = 3.0f; bool LFO_connect = true; void Update() { clock_cycles += 3000; } } filter;
struct Lfo { void Update() { clock_cycles += 1200; } } lfo;
struct Voice
{
    bool VCF_connect = true, VCF_modulated = true;
    int VCF_pivot_steps = 0, LFO_modulation = 3, LFO_periodic = 1;
    float pitch = 2.0f, VCF_frequency_pivot = 5000.0f;
    Filter *VCF_ptr = &filter;
    Lfo *LFO_ptr = &lfo;
    void Send_LFO_to_VCF() { clock_cycles += 7200; }
""" + timing + "\nvoid Run()\n{\n" + block + "\n}\n};\n" + r"""
int main()
{
    Voice voice;
    clock_cycles = UINT32_MAX - 100;
    voice.Run();
    assert(voice.vcf_timing.blocks == 1);
    assert(voice.vcf_timing.sum[0] == 1200 && voice.vcf_timing.sum[1] == 7200 && voice.vcf_timing.sum[2] == 3000);
    assert(voice.vcf_timing.peak[3] == 11400 && voice.vcf_timing.peak_type == 2);
    voice.VCF_modulated = false;
    voice.Run();
    assert(voice.vcf_timing.blocks == 2 && voice.vcf_timing.sum[3] == 14400 && voice.vcf_timing.peak[3] == 11400);
    voice.VCF_pivot_steps = 1;
    voice.Run();
    assert(voice.vcf_timing.sum[3] == 24600);
    voice.VCF_connect = false;
    voice.Run();
    assert(voice.vcf_timing.blocks == 3);
    voice.vcf_timing = {};
    assert(voice.vcf_timing.blocks == 0 && voice.vcf_timing.sum[3] == 0);
}
"""
with tempfile.TemporaryDirectory(prefix="lilla-vcf-timing-") as folder:
    cpp = Path(folder) / "timing.cpp"
    exe = Path(folder) / "timing.exe"
    cpp.write_text(fixture, encoding="utf-8")
    subprocess.run([compiler, "-std=c++20", "-DLILLA_VCF_BENCHMARK", str(cpp), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True, env=env)
print("PASS: production VCF timing stages, totals, peaks, counter wrap, modulation bypass, cutoff ramps and reset")
