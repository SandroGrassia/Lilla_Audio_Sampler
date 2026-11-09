"""Verify that production retrigger mixing overlaps the outgoing waveform and new attack."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "lib/AudioPlayer/AudioPlayer.cpp").read_text(encoding="utf-8")
start = source.index("        if (warmup_for_play_again_flag)")
end = source.index("        if (!live_restart_pending)", start)
restart = source[start:end]
start = source.index("            if (!shoot_flag && mix_flag && sample < mix_samples)")
end = source.index("            if (restart_flag)", start)
mix = source[start:end]
fixture = r"""
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
constexpr int AUDIO_BLOCK_SAMPLES = 128;
int16_t Lilla_saturate16(float value) { return static_cast<int16_t>(value); }
struct AudioPlayer
{
    bool warmup_for_play_again_flag = true, restart_flag = false, main_settings_editing_flag = false;
    bool mix_flag = false, shoot_flag = false;
    uint8_t mix_samples = 32;
    float a_sample = 0, a_first_sample = 17, b_sample = 0, initial_index_offset = 0, pitch = 1;
    float volume_gain = 1, velocity_gain = 1, ADSR_gain = 0.5f, pitch_based_gain_correction = 1;
    float raw_first_value_cache[AUDIO_BLOCK_SAMPLES] = {};
    int16_t block[AUDIO_BLOCK_SAMPLES] = {};
    int starts = 0, harvested = 0;
    void Harvest_samples(int count) { harvested += count; }
    float Read_rendered_sample(int sample, float) { return starts == 0 ? 8000 + sample * 10 : 4000; }
    void Start_playing() { ++starts; ADSR_gain = 0; }
    void Prepare()
    {
        float F_basket_sample = 0;
        RESTART
    }
    void Render()
    {
        for (int sample = 0; sample < AUDIO_BLOCK_SAMPLES; ++sample)
        {
            ADSR_gain = sample / 512.0f;
            float F_basket_sample = sample, value_float = 0, cache = 0;
            MIX
        }
    }
};
int main()
{
    for (int length : {16, 24, 32, 48, 64, 128})
    {
        AudioPlayer player;
        player.mix_samples = length;
        player.Prepare();
        assert(player.starts == 1 && player.harvested == length);
        assert(player.mix_flag && player.shoot_flag && !player.restart_flag && !player.warmup_for_play_again_flag);
        player.Render();
        for (int sample = 0; sample < AUDIO_BLOCK_SAMPLES; ++sample)
        {
            const float attack = 4000 * (sample / 512.0f);
            const float tail = sample < length ? (length - 1 - sample) / static_cast<float>(length - 1) * (4000 + sample * 5) : 0;
            assert(std::abs(player.block[sample] - static_cast<int16_t>(attack + tail)) <= 1);
        }
        assert(player.block[0] == 4000); // Attack starts at zero while the old waveform is still audible.
        assert(player.block[length - 1] > 0); // Attack is already running when the tail reaches zero.
    }
    std::cout << "PASS: production retrigger overlaps outgoing waveform with attack at all six mix lengths, including a full 128-sample block\n";
}
"""
fixture = fixture.replace("RESTART", restart).replace("MIX", mix)
compiler = shutil.which("g++") or "C:/msys64/ucrt64/bin/g++.exe"
env = os.environ.copy()
env["PATH"] = str(Path(compiler).parent) + os.pathsep + env.get("PATH", "")
with tempfile.TemporaryDirectory(prefix="lilla-retrigger-") as directory:
    cpp = Path(directory) / "restart.cpp"
    exe = Path(directory) / "restart.exe"
    cpp.write_text(fixture, encoding="utf-8", newline="\r\n")
    subprocess.run([compiler, "-std=c++20", "-Wall", "-Wextra", "-Werror", str(cpp), "-o", str(exe)], check=True, env=env)
    subprocess.run([str(exe)], check=True, env=env)
