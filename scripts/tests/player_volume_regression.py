"""Exercise production volume ramps, exact mute, retargeting and counter rollover."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / 'lib/AudioPlayer/AudioPlayer.cpp').read_text(encoding='utf-8')
methods = '\n'.join(re.search(r'^void AudioPlayer::' + name + r'\(.*?^\}', source, re.M | re.S).group(0) for name in ('Set_volume', 'Update_volume', 'Update_volume_gain'))
warmup = re.search(r'        if \(volume_gain_warmup_flag\)\n        \{.*?\n        \}', source, re.S).group(0)
code = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <algorithm>
constexpr int SAMPLES_VOLUME = 5000;
struct AudioPlayer
{
    bool idle = false, volume_flag = false, volume_gain_warmup_flag = false;
    float volume_gain = 0, volume_gain_wait = 0, K_volume_gain = 0;
    uint32_t samples_counter = 0, JV0 = 0;
    void Set_volume(float);
    void Update_volume(float);
    void Update_volume_gain();
    void Tick()
    {
        WARMUP
        if (volume_flag)
        {
            Update_volume_gain();
        }
        ++samples_counter;
    }
    void Ramp(float target)
    {
        const float start = volume_gain;
        Update_volume(target);
        for (int sample = 0; sample < SAMPLES_VOLUME; ++sample)
        {
            const float previous = volume_gain;
            Tick();
            assert(volume_gain >= std::min(start, target) && volume_gain <= std::max(start, target));
            assert(target >= start ? volume_gain >= previous : volume_gain <= previous);
        }
        assert(volume_gain == target && !volume_flag && !volume_gain_warmup_flag);
    }
};
METHODS
int main()
{
    for (float from : {0.0f, 0.00015f, 0.125f, 0.5f, 1.0f, 2.0f, 4.0f, 8.0f})
    {
        for (float to : {0.0f, 0.00015f, 0.125f, 0.5f, 1.0f, 2.0f, 4.0f, 8.0f})
        {
            AudioPlayer player;
            player.volume_gain = from;
            player.samples_counter = UINT32_MAX - 2500;
            player.Ramp(to);
            player.Tick();
            assert(player.volume_gain == to);
        }
    }
    AudioPlayer player;
    player.Update_volume(1.0f);
    for (int i = 0; i < SAMPLES_VOLUME - 1; ++i)
    {
        player.Tick();
        assert(player.volume_flag);
    }
    player.Tick();
    assert(!player.volume_flag && player.volume_gain == 1.0f);
    player.Update_volume(0.0f);
    for (int i = 0; i < 700; ++i)
    {
        player.Tick();
    }
    player.Ramp(2.0f); // Reverse direction while running.
    player.Update_volume(0.0f);
    player.Tick();
    player.Ramp(player.volume_gain); // A request equal to the current value must cancel the old destination.
    player.Ramp(0.0f);
    for (int i = 0; i < 10000; ++i)
    {
        player.Tick();
        assert(player.volume_gain == 0.0f);
    }
    std::puts("PASS: volume bounds, monotonic ramps, exact 5000-sample completion, mute, reversal, retarget and rollover");
}
'''.replace('WARMUP', warmup).replace('METHODS', methods)
compiler = shutil.which('g++') or 'C:/msys64/ucrt64/bin/g++.exe'
with tempfile.TemporaryDirectory(prefix='lilla-volume-') as directory:
    cpp, exe = Path(directory) / 'volume.cpp', Path(directory) / 'volume.exe'
    cpp.write_text(code, encoding='utf-8')
    subprocess.run([compiler, '-std=c++17', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    environment = os.environ.copy()
    environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
    subprocess.run([str(exe)], check=True, env=environment)
