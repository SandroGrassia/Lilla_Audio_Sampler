"""Exercise the production Delay page and controls when opened from Direct Sampler."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
main = (root / 'src/main.cpp').read_text(encoding='utf-8')
display = (root / 'lib/DisplayDelay/DisplayDelay.cpp').read_text(encoding='utf-8')
header = (root / 'lib/DisplayDelay/DisplayDelay.h').read_text(encoding='utf-8')
header = re.sub(r'^#(?:include|pragma).*$', '', header, flags=re.M)

def method(source, signature):
    return re.search(r'^' + re.escape(signature) + r'[^\n]*\n\{.*?^\}', source, re.M | re.S).group(0) + '\n'

start = main.index('    if (Lilla_state == DELAY_SETTINGS', main.index('#pragma region Delay'))
end = main.index('    // Navigation remains available', start)
controls = 'void Step_controls()\n{\n' + main[start:end] + '}\n'
start = main.index('    // Panic\n')
end = main.index('    // Pre-listen volume', start)
panic = 'void Step_panic()\n{\n' + main[start:end] + '}\n'
start = main.index('            case SwToolsDelay:', main.index('#pragma region Direct Sampler')) if '#pragma region Direct Sampler' in main else main.index('            case SwToolsDelay:', main.index('    if (Lilla_state == DIRECT_SAMPLING)'))
end = main.index('            break;', start)
entry = 'void Enter_from_sampler()\n' + main[main.index('            {', start):end] + '\n'
start = main.index('                case DIRECT_SAMPLING:', main.index('    // Navigation remains available'))
end = main.index('                    break;', start)
back = 'void Return_to_sampler()\n{\n' + main[main.index('\n', start) + 1:end] + '}\n'

prefix = r'''
#include <cassert>
#include <cstdint>
#include <set>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#define FLASHMEM
enum State { PERFORMANCE, DIRECT_SAMPLING, LIVE_SAMPLING, MIDI_LOOP, DELAY_SETTINGS };
State Lilla_state = PERFORMANCE, Lilla_state_0 = PERFORMANCE;
enum DELAY_element_name { value_DELAY_Feedback, value_DELAY_Delay_time, value_DELAY_Delay_time_LR, value_DELAY_Modulation_source, value_DELAY_Modulation_frequency, value_DELAY_Modulation_depth, value_DELAY_Modulation_phase_LR, DELAY_element_names };
enum Item { LOOP_GAIN, SAMPLES, SAMPLES_LR, MODULATION_SOURCE, MODULATION_FREQUENCY, MODULATION_DEPTH, MODULATION_PHASE_LR, INSTRUMENT_ROUTE };
enum Control { EN_PB_LineOutVol, EN_PB_Select, EN_PB_Value };
constexpr int INSTRUMENTS = 8, TRACKS = 2, PATCH_VOLUME_MAX = 100;
constexpr uint16_t ILI9341_BLACK = 0, ILI9341_WHITE = 1, ILI9341_RED = 2, TEXT_COLOR = 3;
int PB_Sound[INSTRUMENTS] = {10,11,12,13,14,15,16,17};
bool LOOP_track_run[TRACKS] = {true, true};
int volume_patch = 10, Patch_id = 0, result = 0, recording = 0, DS_local_pointer = 0;
DELAY_element_name DELAY_local_pointer = value_DELAY_Feedback;
int edits = 0, pointer_moves = 0, pointer_sets = 0, clears = 0, feedback_draws = 0, stops = 0, gains = 0;
bool rotate_value = false, rotate_volume = false;
std::set<int> pressed;
struct DelayData { int loop_gain = 7; } Delay_data, Delay_values;
struct Manager
{
    int route = 0;
    int Get_value(int) { return route; }
} Delay_manager;
bool Read_pushbutton(int id) { return pressed.erase(id) != 0; }
bool Read_encoder(int, int &value, int, int, int)
{
    if (!rotate_volume)
    {
        return false;
    }
    ++value;
    return true;
}
int Read_encoder_simple(int) { return rotate_value ? 1 : 0; }
bool D_Read_value(int)
{
    if (rotate_value)
    {
        ++edits;
        return true;
    }
    return false;
}
void D_Set_value(int item, int value)
{
    ++edits;
    if (item == INSTRUMENT_ROUTE)
    {
        Delay_manager.route = value;
    }
}
void AudioNoInterrupts() {}
void AudioInterrupts() {}
int Delay_feedback(int v) { return v; }
int Patch_volume_gain(int v) { return v; }
struct Gain { void Set_gain(int) { ++gains; } } D_gain_L_feedback, D_gain_R_n;
struct Players
{
    void Update_all_Preset_volume(int, int) { ++edits; }
    void Broadcast_volume() {}
    void Stop_all_players() { ++stops; }
} Players_Manager;
struct LEDs { void Request_all_LED_switch_off() {} } Loop_led_set;
struct Pointer
{
    void Move_pointer(int) { ++pointer_moves; }
    void Set_pointer_to_Feedback() { ++pointer_sets; }
    DELAY_element_name Get_element_name() { return DELAY_local_pointer; }
} Pointer_Delay;
void Clear_UI_events() { ++clears; pressed.clear(); rotate_value = rotate_volume = false; }
std::vector<std::string> labels;
std::string popup;
struct TFT
{
    void fillScreen(int) { labels.clear(); popup.clear(); }
    void setCursor(int, int) {}
    void setTextColor(int) {}
    void print(const char *text) { labels.emplace_back(text); }
} tft;
int display_coordinate_x(float v) { return v; }
int display_coordinate_y(float v) { return v; }
void Backgorund_red(int, int, int) {}
void Show_popup_text(const char *text, int, int) { popup = text; }
struct Common
{
    void P_Patch_volume_value(bool) {}
    void Show_all_effects() {}
} Display_Common;
struct Sampler
{
    void DS_page_upper() {}
    void DS_page_lower(int) {}
    void DS_menu() {}
    void DS_bar(int, int) {}
} Display_Sampler;
struct SamplerPointer
{
    void Set_pointer_to_first_menu_element() {}
    int Get_pointer() { return 0; }
} Pointer_Sampler;
void DS_define_menu() {}
'''
stubs = r'''
DisplayDelay Display_Delay;
void DisplayDelay::D_sounds() {}
void DisplayDelay::D_feedback() { ++feedback_draws; }
void DisplayDelay::D_delay_time() {}
void DisplayDelay::D_delay_time_LR() {}
void DisplayDelay::D_modulation_source() {}
void DisplayDelay::D_modulation_frequency() {}
void DisplayDelay::D_modulation_depth() {}
void DisplayDelay::D_modulation_phase_LR() {}
'''
production = method(display, 'void DisplayDelay::D_show_page()') + method(display, 'void DisplayDelay::D_disabled(void)') + method(main, 'void Golive_DELAY_SETTINGS(void)') + entry + controls + panic + back
checks = r'''
int main()
{
    Lilla_state = DIRECT_SAMPLING;
    Enter_from_sampler();
    assert(Lilla_state == DELAY_SETTINGS && Lilla_state_0 == DIRECT_SAMPLING);
    assert(popup == "DELAY IS DISABLED WHILE SAMPLING");
    assert(std::find(labels.begin(), labels.end(), "DIRECT SAMPLER DELAY") != labels.end());
    assert(pointer_sets == 0);
    const int feedback_before = feedback_draws;
    for (int item = 0; item < DELAY_element_names; ++item)
    {
        DELAY_local_pointer = static_cast<DELAY_element_name>(item);
        rotate_value = rotate_volume = true;
        pressed.insert(EN_PB_Value);
        pressed.insert(PB_Sound, PB_Sound + INSTRUMENTS);
        Step_controls();
        assert(edits == 0 && pointer_moves == 0 && volume_patch == 10 && Delay_manager.route == 0);
    }
    pressed.insert(EN_PB_LineOutVol);
    Step_panic();
    assert(stops == 1 && gains == 0 && Delay_data.loop_gain == 7 && Delay_values.loop_gain == 7);
    assert(feedback_draws == feedback_before);
    Return_to_sampler();
    assert(Lilla_state == DIRECT_SAMPLING && pressed.empty() && !rotate_value && !rotate_volume);
    for (State origin : {PERFORMANCE, LIVE_SAMPLING, MIDI_LOOP})
    {
        Lilla_state_0 = origin;
        Golive_DELAY_SETTINGS();
        assert(popup.empty());
        const int previous_edits = edits;
        DELAY_local_pointer = value_DELAY_Feedback;
        rotate_volume = true;
        pressed.insert(PB_Sound[0]);
        Step_controls();
        assert(edits > previous_edits);
        pressed.insert(EN_PB_LineOutVol);
        Step_panic();
        assert(Delay_data.loop_gain == 0);
    }
    assert(pointer_sets == 3);
    std::cout << "PASS: persistent disabled Delay page, locked controls and routing, panic preserves parameters, Sampler return, other modes editable\n";
}
'''
compiler = shutil.which('g++') or r'C:\msys64\ucrt64\bin\g++.exe'
with tempfile.TemporaryDirectory(prefix='lilla-delay-sampling-') as directory:
    cpp = Path(directory) / 'delay_sampling.cpp'
    exe = Path(directory) / ('delay_sampling.exe' if os.name == 'nt' else 'delay_sampling')
    cpp.write_text(prefix + header + stubs + production + checks, encoding='utf-8', newline='\r\n')
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    environment = os.environ.copy()
    environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
    subprocess.run([str(exe)], check=True, env=environment)
