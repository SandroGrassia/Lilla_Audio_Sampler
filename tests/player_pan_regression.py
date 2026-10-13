"""Exercise live PAN changes, reversals and fresh-note ramp reset using production code."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'lib/AudioPlayer/AudioPlayer.cpp').read_text(encoding='utf-8')
methods = '\n'.join(re.search(r'^void AudioPlayer::' + name + r'\(.*?^\}', source, re.M | re.S).group(0) for name in ('Set_pan', 'Update_pan', 'Update_pan_gain'))
warmup = re.search(r'        if \(pan_gain_warmup_flag\)\n        \{.*?\n        \}', source, re.S).group(0)
start = source[source.index('    pan_int = pan_int_wait;'):source.index('    Noclick_ptr = Noclick_wait_ptr;')]
code = r"""
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
constexpr int SAMPLES_VOLUME = 5000;
float pan_gain_L_table[33], pan_gain_R_table[33];
struct AudioPlayer
{
    bool idle = false, pan_flag = false, pan_gain_warmup_flag = false;
    int pan_int = 0, pan_int_wait = 0;
    uint32_t samples_counter = 0, JP0 = 0;
    float pan_gain_L = 0, pan_gain_R = 0, pan_gain_L_wait = 0, pan_gain_R_wait = 0, K_pan_gain_L = 0, K_pan_gain_R = 0;
    void Set_pan(int);
    void Update_pan(float);
    void Update_pan_gain();
    void Start() { START }
    void Tick()
    {
        WARMUP
        if (pan_flag)
        {
            Update_pan_gain();
        }
        ++samples_counter;
    }
    void Run(int count = SAMPLES_VOLUME + 2)
    {
        for (int i = 0; i < count; ++i)
        {
            Tick();
        }
    }
    void Check(int target)
    {
        assert(!pan_flag && !pan_gain_warmup_flag);
        assert(pan_gain_L == pan_gain_L_table[target + 16]);
        assert(pan_gain_R == pan_gain_R_table[target + 16]);
    }
};
METHODS
int main()
{
    for (int i = 0; i < 33; ++i)
    {
        pan_gain_L_table[i] = (32 - i) / 32.0f;
        pan_gain_R_table[i] = i / 32.0f;
    }
    AudioPlayer player;
    for (int target : {-16, -7, 9, 16})
    {
        player.Set_pan(0);
        player.Start();
        player.Update_pan(target);
        player.Run();
        player.Check(target);
        const float previous_left = player.pan_gain_L;
        player.Update_pan(0);
        player.Tick();
        assert(player.pan_gain_L != previous_left);
        player.Run();
        player.Check(0);
        player.Update_pan(target);
        player.Run(500);
        player.Update_pan(0);
        player.Run();
        player.Check(0);
        player.Update_pan(target);
        player.Update_pan(0);
        player.Run();
        player.Check(0);
    }
    player.Update_pan(-16);
    player.Run(100);
    player.Set_pan(16);
    player.Start();
    player.Run();
    player.Check(16);
    player.Update_pan(0);
    player.Set_pan(-16);
    player.Start();
    player.Run();
    player.Check(-16);
    player.idle = true;
    player.Update_pan(0);
    assert(!player.pan_gain_warmup_flag);
    player.Start();
    player.idle = false;
    player.Check(0);
    std::cout << "PASS: live PAN reset, ramp reversal, queued edits, exact endpoints and new-note reset\n";
}
""".replace('START', start).replace('WARMUP', warmup).replace('METHODS', methods)
compiler = shutil.which('g++') or 'C:/msys64/ucrt64/bin/g++.exe'
environment = os.environ.copy()
environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
with tempfile.TemporaryDirectory(prefix='player-pan-') as folder:
    cpp, exe = Path(folder) / 'test.cpp', Path(folder) / 'test.exe'
    cpp.write_text(code, encoding='utf-8')
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True, env=environment)
    subprocess.run([str(exe)], check=True, env=environment)
