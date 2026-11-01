"""Exercise Live loop selection, nonblocking warnings and recording resumption."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile
from production_source import method

ROOT = Path(__file__).resolve().parents[2]
main = (ROOT / 'src/main.cpp').read_text(encoding='utf-8')
display = (ROOT / 'lib/DisplayLiveSampler/DisplayLiveSampler.cpp').read_text(encoding='utf-8')
prefix = r"""
#include <cassert>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#define FLASHMEM
constexpr int REC = 1, PLAYONLY = 2, LIVE_SAMPLING = 3, SOUNDS_MAX = 10, PATCHES_MAX = 5, EN_PB_Value = 0;
constexpr int ILI9341_WHITE = 1, ILI9341_RED = 2, ILI9341_BLACK = 0;
enum LillaPlayModes { ONCE_FWD, ONCE_REV, LOOP_FWD, LOOP_FWD_REV };
LillaPlayModes LS_mode = ONCE_REV;
int LS_state = REC, Lilla_state = LIVE_SAMPLING, Patch_id = PATCHES_MAX, LS_sound_id = SOUNDS_MAX;
int LS_buffer_dim = 4096, LS_X_sample = 700, LS_X_delta = 400, rotation = 1;
bool LS_stereo = false, LS_XY_lock = true, LS_stop_reverse_pending = false;
unsigned long now = 0;
unsigned long millis() { return now; }
void AudioNoInterrupts() {}
void AudioInterrupts() {}
int display_coordinate_x(int x) { return x * 6; }
int display_coordinate_y(int y) { return y * 12; }
int Centered_element_top(int h) { return (320-h)/2; }
int Centered_element_left(int w) { return (240-w)/2; }
std::string popup;
int draws = 0, erases = 0;
void Show_popup_text(const char *text, int, int, int) { popup = text; ++draws; }
struct TFT { void fillRect(int, int, int, int, int) { ++erases; } } tft;
struct Recorder
{
    bool writing = false;
    int Q_sample = 1023, starts = 0;
    bool Is_writing() const { return writing; }
    void Start(bool) { writing = true; ++starts; }
} LiveSampler;
struct DisplayLiveSampler
{
    bool no_recorded_audio_visible = false, loop_modes_notice = false;
    uint32_t no_recorded_audio_started_ms = 0;
    int LS_column_row_GAIN[2] = {0, 10};
    void Show_live_notice();
    void Update_no_recorded_audio(bool requested, bool loop_requested = false);
    void Play_mode() {}
    void Loop_time() {}
    void Show_wave(int) {}
} Display_LiveSampler;
struct Pointer { void Show_pointer(bool) {} } Pointer_LiveSampler;
struct SoundValue { LillaPlayModes mode = LOOP_FWD; } Sound[SOUNDS_MAX + 2];
struct Manager
{
    int updates = 0, edits = 0, resumes = 0, restart_position = -1;
    void Update_Preset_mode(int, int) { ++updates; }
    void Multicast_main_settings_editing(int, int) { ++edits; }
    void Resume_live_recording(int p) { ++resumes; restart_position = p; }
} Players_Manager;
bool Read_encoder(int, int &value, int maximum, int minimum, int)
{
    const int previous = value;
    value = std::min(maximum, std::max(minimum, value + rotation));
    return value != previous;
}
void LS_update_both_X_Y_samples() {}
void LS_update_Q_sample() {}
"""
production = method(display, 'FLASHMEM void DisplayLiveSampler::Show_live_notice(void)')
production += method(display, 'FLASHMEM void DisplayLiveSampler::Update_no_recorded_audio(bool requested, bool loop_requested)')
a = main.index('            case value_LS_Play_mode:')
b = main.index('            case value_LS_Feedback:', a)
production += '\nvoid Adjust_mode()\n{\n    switch (0)\n    {\n' + main[a:b].replace('case value_LS_Play_mode:', 'case 0:', 1) + '\n    }\n}\n'
a = main.index('                    LS_stop_reverse_pending = false;', main.index('                case value_LS_Recording:'))
b = main.index('                    LS_update_menu_elements();', a)
production += '\nvoid Start_recording()\n{\n' + main[a:b] + '\n}\n'
checks = r"""
int main()
{
    LS_mode = ONCE_REV;
    LS_state = REC;
    Adjust_mode();
    assert(LS_mode == ONCE_REV && Players_Manager.updates == 0);
    assert(popup == "STOP RECORDING FOR LOOP MODES" && draws == 1);
    now = 1000;
    Display_LiveSampler.Update_no_recorded_audio(true);
    assert(popup == "STOP RECORDING FOR LOOP MODES" && draws == 1);
    now = 3999;
    Display_LiveSampler.Update_no_recorded_audio(false);
    assert(erases == 0);
    now = 4000;
    Display_LiveSampler.Update_no_recorded_audio(false);
    assert(erases == 1 && !Display_LiveSampler.no_recorded_audio_visible);
    LS_state = PLAYONLY;
    LiveSampler.writing = true;
    Adjust_mode(); // The final recording ramp still prevents loop selection.
    assert(LS_mode == ONCE_REV);
    LiveSampler.writing = false;
    Adjust_mode();
    assert(LS_mode == LOOP_FWD);
    Adjust_mode();
    assert(LS_mode == LOOP_FWD_REV);
    for (bool stereo : {false, true})
    {
        LS_stereo = stereo;
        for (bool fixed : {false, true})
        {
            LS_XY_lock = fixed;
            for (auto mode : {ONCE_FWD, ONCE_REV, LOOP_FWD, LOOP_FWD_REV})
            {
                for (int q : {1023, 4095})
                {
                    LS_mode = mode;
                    LS_state = PLAYONLY;
                    LiveSampler.writing = false;
                    LiveSampler.Q_sample = q;
                    LS_X_sample = 700;
                    LS_X_delta = 400;
                    Players_Manager = {};
                    Start_recording();
                    assert(LS_state == REC && LiveSampler.writing && LiveSampler.Q_sample == q);
                    assert(LS_mode == (mode >= LOOP_FWD ? ONCE_FWD : mode));
                    assert(LS_X_sample == 700 && LS_X_delta == 400 && LS_XY_lock == fixed);
                    assert(Players_Manager.resumes == 1 && Players_Manager.restart_position == (q + 1) % 4096);
                    assert(Players_Manager.updates == (mode >= LOOP_FWD ? (stereo ? 2 : 1) : 0));
                }
            }
        }
    }
    std::cout << "PASS: loop selection blocked throughout recording, four-second popup, stopped selection and recording resumption preserving A\n";
}
"""
compiler = shutil.which('g++') or 'C:/msys64/ucrt64/bin/g++.exe'
env = os.environ.copy()
env['PATH'] = str(Path(compiler).parent) + os.pathsep + env.get('PATH', '')
with tempfile.TemporaryDirectory(prefix='lilla-live-loop-availability-') as folder:
    cpp = Path(folder) / 'test.cpp'
    exe = Path(folder) / 'test.exe'
    cpp.write_text(prefix + production + checks, encoding='utf-8', newline='\r\n')
    subprocess.run([compiler, '-std=c++20', '-O2', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True, env=env)
    subprocess.run([str(exe)], check=True, env=env)
