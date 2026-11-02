"""Exercise production layer recording UI, scheduling and shared track edits."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def read(name):
    return (ROOT / name).read_text(encoding='utf-8')


def strip(text):
    return re.sub(r'^\s*#(?:include|pragma)[^\n]*', '', text, flags=re.M)


def block(text, start):
    opening = text.index('{', start)
    depth, end = 1, opening + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]


main = read('src/main.cpp')
reader = read('lib/MidiReader/MidiReader.cpp')
prefix = r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>
#define DMAMEM
#define FLASHMEM
using elapsedMillis = uint32_t;
using std::min;
using std::max;
constexpr int TRACKS = 4, INSTRUMENTS = 1;
'''
production = strip(read('lib/SharedLoop/SharedLoop.h')) + strip(read('lib/SharedLoop/SharedLoop.cpp'))
fixture = r'''
enum State { PERFORMANCE, MIDI_LOOP, DELAY_SETTINGS, SOUND_EDIT, INSTRUMENT_VCF, MIXER, SETUP, CC_SETTINGS };
State Lilla_state = MIDI_LOOP, Lilla_state_0 = PERFORMANCE;
uint32_t physical_clock = 0;
unsigned long LOOP_Clock() { return physical_clock / LOOP_stretch; }
void LOOP_restart_clock() { physical_clock = 0; }
bool irq = true;
void AudioNoInterrupts() { assert(irq); irq = false; }
void AudioInterrupts() { assert(!irq); irq = true; }
bool LOOP_run_button_state = false, LOOP_original = false;
bool LOOP_track_run_memo[TRACKS] = {};
int LOOP_stretch_int = 100;
int LOOP_Get_first_loop_id_free() { return 0; }
void Clear_UI_events() {}
void LOOP_select_menu_elements() {}
int LOOP_local_pointer = 0;
struct Pointer
{
    void Show_pointer(bool) {}
    void Set_pointer_to_first_menu_element() {}
    int Get_pointer() { return 0; }
} Pointer_MidiLoop;
struct Display
{
    int full = 0;
    void Show_layers_full() { ++full; }
    void Show_loop_id() {}
    void Loop_total_time() {}
    void Loop_REC_advice(int, bool) {}
    bool Update_save_failed() { return false; }
    void Show_track_all_data(int) {}
    void Show_menu() {}
} Display_MidiLoop;
struct Metro
{
    uint32_t metro_time = 0, delta = 250;
    void Leds_off() {}
    void Led_ON(int) {}
    void Setup(int duration) { delta = duration / 4; }
    void Update(bool = true) {}
    uint32_t Read_metro_delta_ms() { return delta; }
} LOOP_metronomo;
constexpr int PB_Rec[TRACKS] = {0, 1, 2, 3}, LED_Rec[TRACKS] = {0, 1, 2, 3};
bool rec_pressed = false;
bool Read_pushbutton(int) { const bool pressed = rec_pressed; rec_pressed = false; return pressed; }
struct Step { uint32_t elapsed; int action; }; // 0 wait, 1 NoteOn, 2 NoteOff, 3 REC.
std::vector<Step> steps;
size_t step_index = 0;
void Check_timeout();
void Learn_on(uint8_t, uint8_t, uint8_t);
void Learn_off(uint8_t, uint8_t, uint8_t);
struct Shifters
{
    bool leds[TRACKS] = {};
    void Switch_led(int id, bool on) { leds[id] = on; }
    void Update()
    {
        assert(step_index < steps.size()); // A stuck recording fails instead of hanging the test.
        const Step step = steps[step_index++];
        physical_clock += step.elapsed;
        LOOP_learn_clock += step.elapsed;
        Check_timeout();
        if (step.action == 1)
        {
            Learn_on(0, 60, 100);
        }
        else if (step.action == 2)
        {
            Learn_off(0, 60, 0);
        }
        rec_pressed = step.action == 3;
    }
} Shifters_manager;
void Update_instruments_leds() {}
bool tuning_tone_flag = false, TT_playing = false, TT_led_flag = false;
int tuning_tone_last_note = 0, tuning_tone_volume = 0, Patch_id = 0;
float pitch_from_note[128] = {}, Volume_float[41] = {};
struct Tone { void Frequency(float) {} void Amplitude(float) {} void Start() {} void Stop() {} } tone;
struct Instrument { bool used = true; };
struct PatchData { struct Instrument Instrument[INSTRUMENTS]; } Patch[1];
uint8_t map_instrument_for_note[16][128] = {};
bool bitRead(uint8_t value, int bit) { return (value >> bit) & 1; }
int constrain(int value, int low, int high) { return std::clamp(value, low, high); }
struct Message { int track, layer, note; bool on; };
struct Manager
{
    std::vector<Message> messages;
    void Multicast_stop_players_for_loop_layer(int, int) {}
    void Play_note(int, int note, float, int track, int layer) { messages.push_back({track, layer, note, true}); }
    void Multicast_stop_players_for_NoteOff(int, int note, int track, int layer) { messages.push_back({track, layer, note, false}); }
} manager;
struct MidiReader
{
    static constexpr uint8_t Loop_events_per_layer = 2;
    Manager *Players_Manager = &manager;
    Tone *Tone_generator = &tone;
    void Update_loops();
};
'''
timeout = block(reader, reader.index('    if (LOOP_learn_flag &&'))
production += fixture + '\nvoid Check_timeout()\n{\n' + timeout + '\n}\n'
for name, condition in [('Learn_on', 'if ((Lilla_state == MIDI_LOOP) && LOOP_learn_flag)'), ('Learn_off', 'if ((Lilla_state == MIDI_LOOP) && LOOP_learn_flag && LOOP_elements > 0)')]:
    production += f'void {name}(uint8_t midi_channel, uint8_t note_number, uint8_t velocity)\n{{\n' + block(reader, reader.index(condition)) + '\n}\n'
recording = block(main, main.index('            if (Read_pushbutton(PB_Rec[track])'))
production += 'void Record(int requested)\n{\n    for (int track = requested; track <= requested; ++track)\n    {\n' + recording + '\n    }\n}\n'
production += block(reader, reader.index('void MidiReader::Update_loops(void)'))
checks = r'''
void Run(int track, std::vector<Step> actions)
{
    steps = actions;
    step_index = 0;
    rec_pressed = true;
    Record(track);
    assert(step_index == steps.size());
    assert(!Shifters_manager.leds[track] && !LOOP_learn_flag && irq);
}
int main()
{
    // The master first layer alone defines the duration and is committed by REC.
    Run(0, {{100, 1}, {250, 2}, {750, 3}});
    assert(LOOP_time == 1000 && LOOP_layer_events[0][0] == 2 && LOOP_events[0] == 2);
    assert(LOOP_element[0][0][0].time == 0 && LOOP_element[0][0][1].time == 250);
    const auto original = LOOP_element[0][0][0];
    LOOP_play_event[0][0] = 1;
    LOOP_play_time[0][0] = 8250;
    // Multiple master boundaries without notes leave the layer armed; first NoteOn starts it.
    physical_clock = 100;
    Run(0, {{5000, 0}, {200, 1}, {100, 2}, {600, 0}});
    assert(LOOP_time == 1000 && LOOP_events[0] == 4 && LOOP_layer_events[0][1] == 2);
    assert(LOOP_element[0][1][0].time == 300 && LOOP_element[0][1][1].time == 400);
    assert(LOOP_play_event[0][0] == 1 && LOOP_play_time[0][0] == 8250);
    assert(std::memcmp(&original, &LOOP_element[0][0][0], sizeof(original)) == 0);
    // Cancel a partially recorded or merely armed layer without touching previous layers.
    Run(0, {{100, 1}, {100, 3}});
    assert(LOOP_First_free_layer(0) == 2 && LOOP_events[0] == 4);
    Run(0, {{100, 3}});
    assert(LOOP_First_free_layer(0) == 2 && LOOP_events[0] == 4);
    Run(0, {{20000, 0}, {1, 0}});
    assert(LOOP_First_free_layer(0) == 2 && LOOP_events[0] == 4);
    // Empty non-master tracks follow the same cancellation and timeout rules.
    Run(1, {{100, 1}, {100, 3}});
    assert(LOOP_events[1] == 0 && LOOP_layer_events[1][0] == 0);
    Run(1, {{20001, 0}});
    assert(LOOP_events[1] == 0);
    // Recording on a muted track resumes all existing layers from the current master phase.
    physical_clock = 0;
    Run(1, {{300, 1}, {100, 2}, {600, 0}});
    LOOP_track_run[1] = false;
    LOOP_play_time[1][0] = 1;
    physical_clock = 0;
    Run(1, {{300, 1}, {100, 2}, {600, 0}});
    assert(LOOP_track_run[1] && LOOP_play_time[1][0] == 1300 && LOOP_play_time[1][1] == 1300);
    // All remaining slots can be filled; the fifth REC preserves every layer.
    for (int track = 0; track < TRACKS; ++track)
    {
        while (LOOP_First_free_layer(track) >= 0)
        {
            const int layer = LOOP_First_free_layer(track);
            physical_clock = 0;
            Run(track, {{300, 1}, {100, 2}, {600, 0}});
            assert(LOOP_layer_events[track][layer] == 2);
        }
        const uint32_t total = LOOP_events[track];
        const int notices = Display_MidiLoop.full;
        Run(track, {});
        assert(Display_MidiLoop.full == notices + 1 && LOOP_events[track] == total);
    }
    // Four independent layers dispatch equal-pitch notes and retain common transposition.
    MidiReader midi;
    map_instrument_for_note[0][72] = 1;
    LOOP_pitch_int[1] = 12;
    for (int track = 0; track < TRACKS; ++track)
    {
        LOOP_track_run[track] = track == 1;
    }
    physical_clock = 0;
    LOOP_restart_procedure(1);
    physical_clock = 300;
    midi.Update_loops();
    assert(manager.messages.size() == 4);
    for (int layer = 0; layer < LOOP_LAYERS; ++layer)
    {
        assert(manager.messages[layer].on && manager.messages[layer].layer == layer && manager.messages[layer].note == 72);
    }
    physical_clock = 400;
    midi.Update_loops();
    assert(manager.messages.size() == 8);
    for (int layer = 0; layer < LOOP_LAYERS; ++layer)
    {
        assert(!manager.messages[4 + layer].on && LOOP_play_time[1][layer] == 1300);
    }
    // Shift affects every layer, and restart selects the earliest event after wrapping.
    LOOP_Shift_track(1, 650);
    physical_clock = 25;
    LOOP_restart_procedure(1);
    for (int layer = 0; layer < LOOP_LAYERS; ++layer)
    {
        assert(LOOP_element[1][layer][0].time == 950 && LOOP_element[1][layer][1].time == 50);
        assert(LOOP_play_event[1][layer] == 1 && LOOP_play_time[1][layer] == 50);
    }
    physical_clock = 975;
    LOOP_restart_procedure(1);
    assert(LOOP_play_event[1][0] == 1 && LOOP_play_time[1][0] == 1050);
    // Time stretch follows the master virtual clock, never the physical learn duration.
    LOOP_layer_events[2][3] = 0;
    LOOP_events[2] -= 2;
    LOOP_stretch = 2.0f;
    physical_clock = 0;
    Run(2, {{600, 1}, {200, 2}, {1199, 0}, {1, 0}});
    assert(LOOP_element[2][3][0].time == 300 && LOOP_layer_events[2][3] == 2);
    // Every layer has its own 1000-event capacity, including a master overdub.
    LOOP_layer_events[0][3] = 0;
    LOOP_events[0] -= 2;
    LOOP_stretch = 1.0f;
    physical_clock = 0;
    std::vector<Step> capacity(LOOP_EVENTS, {0, 1});
    Run(0, capacity);
    assert(LOOP_layer_events[0][3] == LOOP_EVENTS && LOOP_First_free_layer(0) == -1);
    assert(LOOP_layer_events[0][0] == 2 && LOOP_time == 1000);
    std::cout << "PASS: layer recording UI, LEDs, arming, 20s timeout, cancellation, four-layer limit, capacity, stretch, scheduling and shared shift/transpose\n";
}
'''
compiler = shutil.which('g++') or 'C:/msys64/ucrt64/bin/g++.exe'
environment = os.environ.copy()
environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
with tempfile.TemporaryDirectory(prefix='lilla-layers-') as directory:
    cpp, exe = Path(directory) / 'layers.cpp', Path(directory) / 'layers.exe'
    cpp.write_text(prefix + production + checks, encoding='utf-8')
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True, env=environment)
    subprocess.run([str(exe)], check=True, env=environment)
