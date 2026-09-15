"""Exercise production LED state transitions without Teensy hardware."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
header = (ROOT / 'lib/PerformanceLedSet/PerformanceLedSet.h').read_text(encoding='utf-8')
source = (ROOT / 'lib/PerformanceLedSet/PerformanceLedSet.cpp').read_text(encoding='utf-8')
header = '\n'.join(line for line in header.splitlines() if not line.startswith(('#include', '#pragma')))
source = '\n'.join(line for line in source.splitlines() if not line.startswith('#include'))
checks = r'''
int main()
{
    PerformanceLedSet leds;
    for (int i = 0; i < INSTRUMENTS; ++i)
    {
        assert(leds.Read_LED_activity(i) == -1);
        leds.Request_LED_switch(i, true);
        assert(leds.Read_LED_activity(i) == 2);
        leds.Write_LED_activity(i, true);
        assert(leds.Read_LED_activity(i) == 1);
        leds.Request_LED_switch(i, false);
        assert(leds.Read_LED_activity(i) == -2);
        leds.Write_LED_activity(i, false);
    }
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
    cpp.write_text('#include <cstdint>\n#include <cstdlib>\n#include <cassert>\nconstexpr int INSTRUMENTS = 8;\n' + header + '\n' + source + '\n' + checks, encoding='utf-8')
    subprocess.run([compiler, '-std=c++17', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True, env=env)
    subprocess.run([str(exe)], check=True, env=env)
print('PASS: first NoteOn, NoteOff and redraw requests for every instrument')
