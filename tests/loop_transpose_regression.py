"""Check that a transpose edit releases only its track before publishing the new pitch."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
main = (ROOT / 'src/main.cpp').read_text(encoding='utf-8')
start = main.index('                    case value_LOOP_pitch:')
body = main[main.index('{', start):main.index('                    break;', start)]
manager = (ROOT / 'lib/PlayersManager/PlayersManager.cpp').read_text(encoding='utf-8')
release = re.search(r'^void PlayersManager::Multicast_stop_players_for_loop_track\(int track\).*?^}', manager, re.M | re.S).group(0)
fixture = r'''
#include <cassert>
#include <cstdio>
#include <initializer_list>
constexpr int PLAYERS = 4;
int LOOP_pitch_int[3] = {}, EN_PB_Track[3] = {0, 1, 2};
bool LOOP_original = true, irq = true, edited = false;
int requested = 0, previous = 0;
void AudioNoInterrupts() { assert(irq); irq = false; }
void AudioInterrupts() { assert(!irq); irq = true; }
bool Read_encoder(int track, int &value, int maximum, int minimum, int step)
{
    assert(irq && maximum == 24 && minimum == -24 && step == 1);
    if (!edited)
    {
        return false;
    }
    value = requested;
    assert(LOOP_pitch_int[track] == previous); // The IRQ must not see the new pitch before notes are released.
    return true;
}
struct Voice
{
    bool active = true, released = false;
    int track = 0;
    bool isPoweredOn() const { return active; }
    int Assigned_track() const { return track; }
};
class PlayersManager
{
public:
    Voice Player_ptr[PLAYERS];
    void Release_Player_noteOff(int player, int track)
    {
        assert(!irq && LOOP_pitch_int[track] == previous);
        Player_ptr[player].released = true;
    }
    void Multicast_stop_players_for_loop_track(int track);
} Players_Manager;
struct Display { void Show_track_all_data(int track) { assert(irq && LOOP_pitch_int[track] == requested); } } Display_MidiLoop;
struct Logger { template<class T> void print(T) {} template<class T> void println(T) {} } Serial;
'''
checks = r'''
int main()
{
    for (int next : {12, -12, 0})
    {
        previous = LOOP_pitch_int[1];
        requested = next;
        Players_Manager.Player_ptr[0] = {true, false, 1};
        Players_Manager.Player_ptr[1] = {true, false, 2};
        Players_Manager.Player_ptr[2] = {false, false, 1};
        Players_Manager.Player_ptr[3] = {true, false, 1};
        edited = false;
        LOOP_original = true;
        Change_pitch(1);
        assert(LOOP_original && LOOP_pitch_int[1] == previous);
        for (auto &voice : Players_Manager.Player_ptr)
        {
            assert(!voice.released);
        }
        edited = true;
        Change_pitch(1);
        assert(irq && !LOOP_original && LOOP_pitch_int[1] == next);
        assert(Players_Manager.Player_ptr[0].released && Players_Manager.Player_ptr[3].released);
        assert(!Players_Manager.Player_ptr[1].released && !Players_Manager.Player_ptr[2].released);
    }
    std::puts("PASS: transpose releases affected voices atomically, preserves other tracks and ignores unchanged controls");
}
'''
compiler = shutil.which('g++') or r'C:\msys64\ucrt64\bin\g++.exe'
environment = os.environ.copy()
environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
with tempfile.TemporaryDirectory(prefix='lilla-transpose-') as directory:
    cpp = Path(directory) / 'test.cpp'
    exe = Path(directory) / 'test.exe'
    cpp.write_text(fixture + release + '\nvoid Change_pitch(int track)\n' + body + checks, encoding='utf-8', newline='\r\n')
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True, env=environment)
    subprocess.run([str(exe)], check=True, env=environment)
