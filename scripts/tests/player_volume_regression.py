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
header = (ROOT / 'lib/AudioPlayer/AudioPlayer.h').read_text(encoding='utf-8')
step = re.search(r'    static constexpr float VOLUME_GAIN_STEP = .*?;', header).group(0)
code = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <algorithm>
#include <cmath>
struct AudioPlayer
{
    bool idle = false, volume_flag = false, volume_gain_warmup_flag = false;
    STEP_DECLARATION
    float volume_gain = 0, volume_gain_wait = 0;
    uint32_t samples_counter = 0;
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
        int sample = 0;
        do
        {
            const float previous = volume_gain;
            Tick();
            assert(volume_gain >= std::min(start, target) && volume_gain <= std::max(start, target));
            assert(target >= start ? volume_gain >= previous : volume_gain <= previous);
            const float expected_step = std::min(VOLUME_GAIN_STEP, std::fabs(target - previous));
            assert(std::fabs(std::fabs(volume_gain - previous) - expected_step) < 0.000001f);
            assert(++sample < static_cast<int>(std::ceil(std::fabs(target - start) / VOLUME_GAIN_STEP * 1.02f)) + 2);
        } while (volume_flag || volume_gain_warmup_flag);
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
    for (int i = 0; i < static_cast<int>(0.9f / AudioPlayer::VOLUME_GAIN_STEP); ++i)
    {
        player.Tick();
        assert(player.volume_flag);
    }
    player.Ramp(1.0f); // Floating-point rounding may add a final partial step.
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
    std::puts("PASS: volume bounds, monotonic ramps, fixed gain steps and exact completion, mute, reversal, retarget and rollover");
}
'''.replace('STEP_DECLARATION', step).replace('WARMUP', warmup).replace('METHODS', methods)
compiler = shutil.which('g++') or 'C:/msys64/ucrt64/bin/g++.exe'
with tempfile.TemporaryDirectory(prefix='lilla-volume-') as directory:
    cpp, exe = Path(directory) / 'volume.cpp', Path(directory) / 'volume.exe'
    cpp.write_text(code, encoding='utf-8')
    subprocess.run([compiler, '-std=c++17', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    environment = os.environ.copy()
    environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
    subprocess.run([str(exe)], check=True, env=environment)
