"""Exercise production NEW navigation and recording resume control blocks."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

source = (Path(__file__).resolve().parents[2] / 'src/main.cpp').read_text(encoding='utf-8')
start = source.index('            const int navigation_id =')
navigation = source[start:source.index('            PRINT_CONTROL_POINT(new_loop_id);', start)]
start = source.index('            if (Read_pushbutton(PB_Rec[track])', source.index('        // Recording'))
condition = source[start:source.index('\n', start)].strip()[4:-1]
code = r"""
#include <cassert>
#include <cstdio>
#include <initializer_list>
constexpr int NEW_LOOP = -1, TRACKS = 4, MASTER_TRACK = 0;
bool saved[10] = {};
int LOOP_Get_next_loop_id_in_SD(int id)
{
    for (int i = id + 1; i < 10; ++i)
    {
        if (saved[i])
        {
            return i;
        }
    }
    return id;
}
int LOOP_Get_previous_loop_id_in_SD(int id)
{
    for (int i = id - 1; i >= 0; --i)
    {
        if (saved[i])
        {
            return i;
        }
    }
    return id;
}
int Navigate(int LOOP_id, int LOOP_suggested_id, int result)
{
    NAVIGATION
    return new_loop_id;
}
bool LOOP_run_button_state;
int LOOP_events[TRACKS] = {};
int PB_Rec[TRACKS] = {};
bool pressed = true;
bool Read_pushbutton(int) { return pressed; }
bool Can_record(int track)
{
    return CONDITION;
}
int main()
{
    for (int i = 0; i < 5; ++i)
    {
        saved[i] = true;
    }
    assert(Navigate(NEW_LOOP, 5, -1) == 4);
    assert(Navigate(NEW_LOOP, 5, 1) == NEW_LOOP);
    saved[7] = true;
    assert(Navigate(NEW_LOOP, 5, 1) == 7);
    assert(Navigate(4, 5, -1) == 3);
    assert(Navigate(0, 5, -1) == 0);
    assert(Navigate(NEW_LOOP, 0, -1) == NEW_LOOP);
    for (bool running : {false, true})
    {
        LOOP_run_button_state = running;
        for (int master_events : {0, 10})
        {
            LOOP_events[MASTER_TRACK] = master_events;
            for (int track : {0, 1, 2, 3})
            {
                pressed = true;
                assert(Can_record(track) == (track == MASTER_TRACK || (running && master_events > 0)));
                pressed = false;
                assert(!Can_record(track));
            }
        }
    }
    std::puts("PASS: NEW neighbours, navigation boundaries, REC from STOP restricted to master, running REC eligibility");
}
""".replace('NAVIGATION', navigation).replace('CONDITION', condition)
compiler = shutil.which('g++') or 'C:/msys64/ucrt64/bin/g++.exe'
with tempfile.TemporaryDirectory(prefix='lilla-controls-') as directory:
    cpp, exe = Path(directory) / 'controls.cpp', Path(directory) / 'controls.exe'
    cpp.write_text(code, encoding='utf-8')
    subprocess.run([compiler, '-std=c++17', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    environment = os.environ.copy()
    environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
    subprocess.run([str(exe)], check=True, env=environment)
