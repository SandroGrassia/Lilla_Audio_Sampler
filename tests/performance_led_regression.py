"""Exercise production LED state transitions without Teensy hardware."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
header = (ROOT / 'lib/PerformanceLedSet/PerformanceLedSet.h').read_text(encoding='utf-8')
source = (ROOT / 'lib/PerformanceLedSet/PerformanceLedSet.cpp').read_text(encoding='utf-8')
header = '\n'.join(line for line in header.splitlines() if not line.startswith(('#include', '#pragma')))
source = '\n'.join(line for line in source.splitlines() if not line.startswith('#include'))
loop_header = (ROOT / 'lib/LoopLedSet/LoopLedSet.h').read_text(encoding='utf-8')
loop_source = (ROOT / 'lib/LoopLedSet/LoopLedSet.cpp').read_text(encoding='utf-8')
header += '\n' + '\n'.join(line for line in loop_header.splitlines() if not line.startswith(('#include', '#pragma')))
source += '\n' + '\n'.join(line for line in loop_source.splitlines() if not line.startswith('#include'))
player = (ROOT / 'lib/AudioPlayer/AudioPlayer.cpp').read_text(encoding='utf-8')
led_method = re.search(r'void AudioPlayer::My_LED\(bool on\)\n\{.*?^\}', player, re.M | re.S).group(0)
start = player[player.index('void AudioPlayer::Start_playing'):player.index('void AudioPlayer::Enforce_cycle_deadline')]
assert '    My_LED(false);\n\n    instrument_id = instrument_id_wait;' in start
source += r'''
constexpr int MIDI_LOOP = 1;
int Lilla_state = 0;
struct Statistics
{
    int performance[INSTRUMENTS] = {};
    int loop[TRACKS][INSTRUMENTS] = {};
    void Inc_total_Players_per_instrument(int i) { ++performance[i]; }
    void Dec_total_Players_per_instrument(int i) { --performance[i]; }
    void Inc_total_Players_per_track_instrument(int t, int i) { ++loop[t][i]; }
    void Dec_total_Players_per_track_instrument(int t, int i) { --loop[t][i]; }
};
struct AudioPlayer
{
    bool myLED = false;
    int instrument_id = 0, track = -1, led_instrument_id = 0, led_track = -1;
    Statistics *Players_statistics_ptr;
    void My_LED(bool on);
};
''' + led_method
checks = r'''
int main()
{
    Statistics stats;
    AudioPlayer player;
    player.Players_statistics_ptr = &stats;
    player.My_LED(true);
    player.My_LED(true);
    assert(stats.performance[0] == 1);
    player.My_LED(false); // Unconditional detach used by every Start_playing path.
    player.instrument_id = 1;
    player.My_LED(true);
    assert(stats.performance[0] == 0 && stats.performance[1] == 1);
    Lilla_state = MIDI_LOOP;
    player.track = 2;
    player.My_LED(false); // A mode change must not change the counter being released.
    assert(stats.performance[1] == 0);
    player.My_LED(true);
    Lilla_state = 0;
    player.instrument_id = 3;
    player.My_LED(false);
    player.My_LED(false);
    assert(stats.loop[2][1] == 0);
    PerformanceLedSet leds;
    for (int i = 0; i < INSTRUMENTS; ++i)
    {
        assert(leds.Read_LED_activity(i) == -1);
        leds.Request_LED_switch(i, true);
        assert(leds.Read_LED_activity(i) == 2);
        assert(leds.Consume_LED_activity(i) == 2);
        assert(leds.Read_LED_activity(i) == 1);
        leds.Request_LED_switch(i, false);
        assert(leds.Read_LED_activity(i) == -2);
        assert(leds.Consume_LED_activity(i) == -2);
    }
    // A new IRQ request during drawing survives the already acknowledged snapshot.
    leds.Request_LED_switch(0, true);
    assert(leds.Consume_LED_activity(0) == 2);
    leds.Request_LED_switch(0, false);
    assert(leds.Consume_LED_activity(0) == -2);
    irq_mask = 1;
    leds.Consume_LED_activity(0);
    leds.Restore_all_LED();
    assert(irq_mask == 1);
    irq_mask = 0;
    LoopLedSet loop;
    loop.Request_LED_switch(2, 3, true);
    assert(loop.Consume_LED_activity(2, 3) == 2);
    loop.Request_LED_switch(2, 3, false);
    assert(loop.Consume_LED_activity(2, 3) == -2);
    loop.Request_LED_switch(2, 3, true);
    assert(loop.Consume_LED_activity(2, 3) == 2);
    leds.Restore_all_LED();
    for (int i = 0; i < INSTRUMENTS; ++i)
    {
        assert(leds.Read_LED_activity(i) == -2);
        leds.Request_LED_switch(i, true);
        assert(leds.Read_LED_activity(i) == 2);
    }
}
'''
compiler = shutil.which('g++') or 'C:/msys64/ucrt64/bin/g++.exe'
env = os.environ.copy()
env['PATH'] = str(Path(compiler).parent) + os.pathsep + env.get('PATH', '')
with tempfile.TemporaryDirectory() as folder:
    cpp = Path(folder) / 'led_test.cpp'
    exe = Path(folder) / 'led_test.exe'
    cpp.write_text('#include <cstdint>\n#include <cstdlib>\n#include <cassert>\nconstexpr int INSTRUMENTS = 8;\nconstexpr int TRACKS = 4;\nuint32_t irq_mask = 0;\nuint32_t __get_primask() { return irq_mask; }\nvoid __disable_irq() { irq_mask = 1; }\nvoid __enable_irq() { irq_mask = 0; }\n' + header + '\n' + source + '\n' + checks, encoding='utf-8')
    subprocess.run([compiler, '-std=c++17', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True, env=env)
    subprocess.run([str(exe)], check=True, env=env)
print('PASS: LED ownership, reassignment, mode changes, IRQ request consumption and redraw')
