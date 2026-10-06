"""Exercise production Sampler capacity menus, notices and export cleanup decisions."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/main.cpp').read_text(encoding='utf-8')


def method(signature):
    return re.search(r'^' + re.escape(signature) + r'\n\{.*?^\}', source, re.M | re.S).group(0)


menu = method('void DS_define_menu(void) // {"Exit"}, {"Delete"}, {"Pause+Rec"}, {"Mono Rec"}, {"Stereo Rec"}, {"Stop"}')
find_free = method('int DS_find_Recording_free(void)')
notice_reset = re.search(r'    static bool recording_limit_notified = false;.*?\n    \}', source, re.S).group(0)
notice = re.search(r'        if \(DS_state == DS_waiting_state && DS_recording_slots_full && !recording_limit_notified\).*?\n        \}', source, re.S).group(0)
export = source.split('                case 11: // EXPORT AS WAV TO SD\n', 1)[1].split('                break;', 1)[0]

prefix = r'''
#include <cassert>
#include <iostream>
#include <string>
constexpr int RECORDINGS = 30, DS_menu_elements = 12;
constexpr int DS_waiting_state = 0, DS_pause_state = 1, DS_recording_state = 2, DS_convert_state = 3, DS_export_SD_state = 4;
constexpr int DIRECT_SAMPLING = 1, ILI9341_WHITE = 1, ILI9341_RED = 2, ILI9341_BLACK = 3, ILI9341_GREEN = 4;
struct Entry { int packets = 0, bytes = 256; bool consistent = true, stereo = false; };
Entry Recording[RECORDINGS];
bool Menu_DS[DS_menu_elements], DS_recording_slots_full = false;
int recording = 0, recordings = 0, DS_state = DS_waiting_state, DS_export = 0, DS_menu_max = 0, DS_local_pointer = 0;
int Lilla_state = DIRECT_SAMPLING, free_packets = 100, notices = 0, cleanup_calls = 0, invalidations = 0, stopped = 0;
bool export_result = false, rebuild_result = true;
struct SerialStub { template<class T> void print(T) {} template<class T> void println(T) {} } Serial;
struct DisplayStub {
    void DS_set_recording_controls(bool) {}
    void DS_page_upper() {}
    void DS_page_lower(int) {}
    void DS_menu() {}
} Display_Sampler;
struct PointerStub {
    void Show_pointer(bool) {}
    void Set_pointer_to_first_menu_element() {}
    int Get_pointer() { return 0; }
} Pointer_Sampler;
struct PeakStub { void reset() {} } PeakTracking_L, PeakTracking_R;
struct PlayersStub { void Stop_all_players() { ++stopped; } } Players_Manager;
int VFS_Get_packets_free() { return free_packets; }
int Get_flash_size() { return 64000000; }
int Get_flash_occupation() { return 0; }
void delay(int) {}
void Clear_UI_events() {}
void AudioNoInterrupts() {}
void AudioInterrupts() {}
void Show_popup_text(const char*, int, int, int = 0) {}
void Show_popup_text(const char* first, const char*, int, int) { assert(std::string(first) == "RECORDING LIMIT REACHED"); ++notices; }
bool DS_export_wav_to_SD() { return export_result; }
void P_Invalidate_recording_cache(int id) { assert(id == recording); ++invalidations; }
void Require_VFS(bool ok) { assert(ok); }
bool VFS_Clean_up_VFS() { assert(export_result); ++cleanup_calls; Recording[recording].packets = 0; return true; }
bool VFS_Defragment() { return true; }
void DS_update_recordings()
{
    recordings = 0;
    for (const auto& entry : Recording)
    {
        if (entry.packets > 0 && entry.consistent)
        {
            ++recordings;
        }
    }
}
int DS_get_next_Recording(int)
{
    for (int id = 0; id < RECORDINGS; ++id)
    {
        if (Recording[id].packets > 0)
        {
            return id;
        }
    }
    return -1;
}
bool DS_back_to_first_DS_Recording() { return rebuild_result; }
'''

checks = r'''
int main()
{
    for (auto& entry : Recording)
    {
        entry.packets = 1;
    }
    DS_update_recordings();
    DS_define_menu();
    assert(DS_find_Recording_free() == -1 && !Menu_DS[1]);
    update_notice();
    update_notice();
    assert(notices == 1);
    Lilla_state = 0;
    update_notice();
    Lilla_state = DIRECT_SAMPLING;
    update_notice();
    assert(notices == 2);
    Recording[17].packets = 0;
    DS_define_menu();
    assert(DS_find_Recording_free() == 17 && Menu_DS[1]);
    update_notice();
    free_packets = 3;
    DS_define_menu();
    assert(!Menu_DS[1] && !DS_recording_slots_full);
    free_packets = 100;
    Recording[17].packets = 1;
    DS_state = DS_recording_state;
    DS_define_menu();
    update_notice();
    assert(notices == 2);
    DS_state = DS_waiting_state;
    DS_define_menu();
    update_notice();
    assert(notices == 3);
    export_result = false;
    run_export();
    assert(Recording[0].packets == 1 && Recording[0].consistent && cleanup_calls == 0 && invalidations == 0 && stopped == 0);
    assert(!Menu_DS[1]);
    export_result = true;
    run_export();
    assert(Recording[0].packets == 0 && cleanup_calls == 1 && invalidations == 1 && stopped == 1);
    assert(recording == 1 && recordings == 29 && Menu_DS[1]);
    for (auto& entry : Recording)
    {
        entry.packets = 0;
    }
    Recording[1].packets = 1;
    Recording[1].stereo = true;
    run_export();
    assert(recording == -1 && recordings == 0 && Menu_DS[1] && !Menu_DS[11]);
    std::cout << "PASS: full/free slot menus, Flash capacity, one notice per visit, last-slot notice, failed/successful mono/stereo export cleanup and empty inventory\n";
}
'''

production = find_free + '\n' + menu + '\nvoid run_export()\n' + export
production += '\nvoid update_notice()\n{\n' + notice_reset + '\nif (Lilla_state == DIRECT_SAMPLING)\n{\n' + notice + '\n}\n}\n'
compiler = shutil.which('g++') or r'C:\msys64\ucrt64\bin\g++.exe'
environment = os.environ.copy()
environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
with tempfile.TemporaryDirectory(prefix='lilla-sampler-capacity-') as directory:
    cpp = Path(directory) / 'test.cpp'
    exe = Path(directory) / 'test.exe'
    cpp.write_text(prefix + production + checks, encoding='utf-8', newline='\r\n')
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True, env=environment)
    subprocess.run([str(exe)], check=True, env=environment)
