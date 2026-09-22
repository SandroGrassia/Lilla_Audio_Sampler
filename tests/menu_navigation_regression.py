"""Exercise production circular pointers and background MIDI-loop dispatch in Setup."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[1]
def read(p): return (ROOT / p).read_text(encoding='utf-8')
def strip(s): return '\n'.join(line for line in s.splitlines() if not line.startswith(('#include', '#pragma')))
def method(s, signature): return re.search(r'^' + re.escape(signature) + r'[^\n]*\n\{.*?^\}', s, re.M | re.S).group(0)
prefix = r"""
#include <cassert>
#include <cstdint>
#include <iostream>
#define FLASHMEM
#define F(x) x
struct SerialStub { template<class T> void print(T) {} template<class T> void println(T) {} void println() {} } Serial;
struct DisplayStub
{
    template<class... T> void VCF_show_pointer_frame(T...) {}
    template<class... T> void LS_show_pointer_frame(T...) {}
    template<class... T> void Loop_show_pointerTrack(T...) {}
    template<class... T> void Loop_show_pointerMenu(T...) {}
    void SETUP_show_frame(int) {}
} Display_VCF, Display_LiveSampler, Display_MidiLoop, Display_Manager;
constexpr int TRACKS = 2, INSTRUMENTS = 1;
int LOOP_events[TRACKS] = {};
int LS_menu_max = 1, position_Menu_LS[4] = {0,0,1,0}, element_Menu_LS[4] = {0,2};
int LOOP_menu_max = 1, position_Menu_LOOP[4] = {0,0,0,1}, element_Menu_LOOP[4] = {0,3};
enum LS_field_name { field_LS_Menu, field_LS_Value };
enum LS_menu_element_name { value_LS_Recording, value_LS_Stop, value_LS_MonoStereo, value_LS_Erase };
constexpr int LS_value_names = 4;
enum LS_value_name { value_LS_Play_mode, value_LS_Feedback, value_LS_Window, value_LS_Gain };
struct LS_pointer_struct { LS_field_name field_name; LS_menu_element_name menu_element; LS_value_name value_element; };
enum LOOP_field_name { field_LOOP_Menu, field_LOOP_TrackValues };
enum LOOP_menu_element_name : int { value_LOOP_Menu_none = -1, value_LOOP_New, value_LOOP_Save, value_LOOP_SaveAsNew, value_LOOP_Delete };
constexpr int LOOP_track_values = 3;
enum LOOP_track_value_name : int { value_LOOP_Track_none = -1, value_LOOP_shift, value_LOOP_pitch, value_LOOP_level };
struct LOOP_field_description_struct { LOOP_field_name field_name; LOOP_menu_element_name menu_element; LOOP_track_value_name track_value_element; };
enum State { PERFORMANCE, MIDI_LOOP, DELAY_SETTINGS, SOUND_EDIT, INSTRUMENT_VCF, MIXER, SETUP, CC_SETTINGS };
State Lilla_state = MIDI_LOOP, Lilla_state_0 = PERFORMANCE;
int stop_calls = 0;
void LOOP_stop_all_midi_tracks() { ++stop_calls; }
void Golive_SETUP() { Lilla_state = SETUP; }
int result = 0, SET_menu = 0, rotation = 0;
constexpr int EN_PB_Select = 0;
int Read_encoder_simple(int) { return rotation; }
void Clear_UI_events() {}
unsigned long now = 100;
unsigned long LOOP_Clock() { return now; }
unsigned long LOOP_Clock_time_from_virtual_time(int time) { return now + time + 1; }
bool LOOP_metronomo_run = false, LOOP_metronomo_flag_IN[2] = {}, LOOP_track_run[TRACKS] = {true, false};
struct Metro { unsigned long metro_time = 0; void Update(bool) {} int Read_metro_delta_ms() { return 100; } } LOOP_metronomo;
unsigned long LOOP_play_time[TRACKS] = {};
uint32_t LOOP_play_event[TRACKS] = {};
int LOOP_pitch_int[TRACKS] = {};
struct LoopEvent { int midi_channel = 0, note_number = 60, velocity = 100, time = 0; bool note_on = true; } LOOP_element[TRACKS][2];
int constrain(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
bool tuning_tone_flag = false, TT_playing = false, TT_led_flag = false;
int tuning_tone_last_note = 0, tuning_tone_volume = 0, Patch_id = 0;
float pitch_from_note[128] = {}, Volume_float[41] = {};
struct Tone { void Frequency(float) {} void Amplitude(float) {} void Start() {} void Stop() {} } tone;
struct Instrument { bool used = true; };
struct PatchData { struct Instrument Instrument[INSTRUMENTS]; } Patch[1];
uint8_t map_instrument_for_note[16][128] = {};
bool bitRead(uint8_t x, int bit) { return (x >> bit) & 1; }
struct Manager
{
    int starts = 0, stops = 0;
    void Multicast_stop_players_for_loop_track(int) {}
    void Play_note(int, int, float, int) { ++starts; }
    void Multicast_stop_players_for_NoteOff(int, int, int) { ++stops; }
} manager;
struct MidiReader
{
    static constexpr uint8_t Loop_events_per_track = 2;
    Manager *Players_Manager = &manager;
    Tone *Tone_generator = &tone;
    void Update_loops();
};
"""
prefix += strip(read('lib/SharedVCF/SharedVCF.h')) + '\n'
production = ''
for cls in ['PointerVCF', 'PointerLiveSampler', 'PointerMidiLoop']:
    prefix += strip(read(f'lib/{cls}/{cls}.h')) + '\n'
    production += strip(read(f'lib/{cls}/{cls}.cpp')) + '\n'
main = read('src/main.cpp')
production += method(main, 'void Switch_from_MIDI_LOOP_to_SETUP') + '\n'
a = main.index('        result = Read_encoder_simple(EN_PB_Select);', main.index('    if (Lilla_state == SETUP)'))
b = main.index('        // Choose menu item', a)
production += 'void Step_setup()\n{\n' + main[a:b] + '}\n'
production += method(read('lib/MidiReader/MidiReader.cpp'), 'void MidiReader::Update_loops') + '\n'
checks = r"""
int main()
{
    PointerVCF vcf;
    vcf.Set_pointer_to_FilterType();
    for (int i = 0; i <= value_VCF_FilterType; ++i) { vcf.Move_pointer(-1); }
    assert(vcf.Get_VCF_value_name() == VCF_value_names - 1);
    vcf.Move_pointer(1);
    assert(vcf.Get_VCF_value_name() == value_VCF_Menu);
    for (int i = 0; i < VCF_value_names; ++i) { vcf.Move_pointer(1); }
    assert(vcf.Get_VCF_value_name() == value_VCF_Menu);
    PointerLiveSampler live;
    live.Set_pointer_to_first_menu_element();
    live.Move_pointer(-1);
    assert(live.Get_pointer().field_name == field_LS_Value && live.Get_pointer().value_element == value_LS_Gain);
    live.Move_pointer(1);
    assert(live.Get_pointer().field_name == field_LS_Menu && live.Get_pointer().menu_element == value_LS_Recording);
    for (int i = 0; i < LS_menu_max + 1 + LS_value_names; ++i) { live.Move_pointer(1); }
    assert(live.Get_pointer().field_name == field_LS_Menu && live.Get_pointer().menu_element == value_LS_Recording);
    for (int i = 0; i < LS_menu_max + 1 + LS_value_names; ++i) { live.Move_pointer(-1); }
    assert(live.Get_pointer().field_name == field_LS_Menu && live.Get_pointer().menu_element == value_LS_Recording);
    PointerMidiLoop loop;
    loop.Set_pointer_to_first_menu_element();
    loop.Move_pointer(-1);
    assert(loop.Get_pointer().track_value_element == value_LOOP_pitch);
    loop.Move_pointer(1);
    assert(loop.Get_pointer().menu_element == value_LOOP_New);
    for (int i = 0; i < 5; ++i) { loop.Move_pointer(1); }
    assert(loop.Get_pointer().menu_element == value_LOOP_New);
    for (int i = 0; i < 5; ++i) { loop.Move_pointer(-1); }
    assert(loop.Get_pointer().menu_element == value_LOOP_New);
    LOOP_menu_max = -1; loop.Set_pointer_to_first_menu_element(); loop.Move_pointer(1);
    assert(loop.Get_pointer().menu_element == value_LOOP_Menu_none);
    SET_menu = 0; rotation = -1; Step_setup(); assert(SET_menu == 6);
    rotation = 1; Step_setup(); assert(SET_menu == 0);
    for (int i = 0; i < 7; ++i) { Step_setup(); }
    assert(SET_menu == 0);
    Switch_from_MIDI_LOOP_to_SETUP();
    assert(stop_calls == 0 && Lilla_state_0 == MIDI_LOOP && Lilla_state == SETUP && LOOP_track_run[0]);
    MidiReader reader;
    LOOP_events[0] = 2;
    LOOP_element[0][1].note_on = false;
    map_instrument_for_note[0][60] = 1;
    for (State page : {SETUP, CC_SETTINGS, MIDI_LOOP})
    {
        Lilla_state = page; LOOP_play_event[0] = 0; LOOP_play_time[0] = now;
        const int starts = manager.starts, stops = manager.stops;
        reader.Update_loops();
        assert(manager.starts == starts + 1);
        ++now; reader.Update_loops();
        assert(manager.stops == stops + 1);
    }
    Lilla_state = SETUP; Lilla_state_0 = PERFORMANCE;
    const int starts = manager.starts;
    now += 100; reader.Update_loops();
    assert(manager.starts == starts);
    std::cout << "PASS: circular navigation, filtered menus, empty loop menu and uninterrupted Setup/CC loop NoteOn/NoteOff\n";
}
"""
compiler = shutil.which('g++') or 'C:/msys64/ucrt64/bin/g++.exe'
env = os.environ.copy()
env['PATH'] = str(Path(compiler).parent) + os.pathsep + env.get('PATH', '')
with tempfile.TemporaryDirectory(prefix='menu-navigation-') as folder:
    cpp = Path(folder) / 'test.cpp'
    exe = Path(folder) / 'test.exe'
    cpp.write_text(prefix + production + checks, encoding='utf-8')
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True, env=env)
    subprocess.run([str(exe)], check=True, env=env)
