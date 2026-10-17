"""Exercise production LED state transitions without Teensy hardware."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
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
# Exercise the real page-entry path and MIDI Loop LED drawing branch with a fake display.
main = (ROOT / 'src/main.cpp').read_text(encoding='utf-8')
entry = re.search(r'void Golive_with_MIDI_LOOP\(bool restart\)\n\{.*?^\}', main, re.M | re.S).group(0)
update_start = main.index('void Update_instruments_leds()\n{')
update_end = main.index('    else if (Lilla_state == PERFORMANCE)', update_start)
source += r'''
LoopLedSet Loop_led_set;
constexpr int NEW_LOOP = -1, MASTER_TRACK = 0;
bool LOOP_initial_selection_pending = false; // This fixture exercises returning to an already selected loop.
int LOOP_time = 1000;
int LOOP_play_event[TRACKS] = {};
bool LOOP_track_run_memo[TRACKS] = {}, LOOP_track_run[TRACKS] = {};
int LOOP_Get_next_loop_id_in_SD(int) { return 3; }
bool LOOP_Copy_midi_loop_from_SD_to_RAM(int) { return true; }
void LOOP_set_time_order(int) {}
int LOOP_id = 3, LOOP_suggested_id = -1;
int LOOP_events[TRACKS] = {1, 1, 1, 1};
bool LOOP_run_button_state = false;
int Patch_id = 0;
struct InstrumentStub { bool used = true; };
struct PatchStub { InstrumentStub Instrument[INSTRUMENTS]; } Patch[1];
struct DisplayStub
{
    int leds[TRACKS][INSTRUMENTS] = {};
    void Show_Loop_page()
    {
        for (auto &row : leds)
        {
            for (auto &led : row)
            {
                led = -1; // Page clear removes both bright and dim LED bitmaps.
            }
        }
    }
    void Loop_led(int track, int instrument, bool on)
    {
        assert(irq_mask == 0); // SPI drawing must happen after the atomic state update.
        leds[track][instrument] = on;
    }
} Display_MidiLoop;
struct PointerStub
{
    void Set_pointer_to_first_menu_element() {}
    int Get_pointer() { return 0; }
} Pointer_MidiLoop;
int LOOP_local_pointer = 0;
int player_resets = 0, clock_restarts = 0;
struct StatisticsStub
{
    void Reset_total_Players_per_track_instrument() { ++player_resets; }
} Players_statistics;
struct MetronomeStub { void Setup(int) {} void Leds_off() {} void Led_ON(int) {} } LOOP_metronomo;
void Clear_UI_events() {}
void LOOP_select_menu_elements() {}
int LOOP_Get_first_loop_id_free() { return 4; }
void LOOP_restart_clock() { ++clock_restarts; }
'''
source += main[update_start:update_end] + '}\n' + entry + '\n'
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
    player.My_LED(true); // A background loop note starts while a tool page is displayed.
    assert(stats.loop[2][3] == 1 && stats.performance[3] == 0);
    player.My_LED(false);
    assert(stats.loop[2][3] == 0);
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
    // Return from tools with a mix of acknowledged and pending ON/OFF states across all 32 LEDs.
    for (int track = 0; track < TRACKS; ++track)
    {
        for (int instrument = 0; instrument < INSTRUMENTS; ++instrument)
        {
            const int pattern = (track * INSTRUMENTS + instrument) % 4;
            Loop_led_set.Request_LED_switch(track, instrument, (pattern % 2) != 0);
            if (pattern < 2)
            {
                Loop_led_set.Consume_LED_activity(track, instrument);
            }
        }
    }
    for (int visit = 0; visit < 2; ++visit)
    {
        Lilla_state = 0; // Mixer/Delay: playback requests continue while the loop page is hidden.
        Golive_with_MIDI_LOOP(false);
        assert(player_resets == 0 && clock_restarts == 0);
        for (int track = 0; track < TRACKS; ++track)
        {
            for (int instrument = 0; instrument < INSTRUMENTS; ++instrument)
            {
                const bool on = ((track * INSTRUMENTS + instrument) % 2) != 0;
                assert(Display_MidiLoop.leds[track][instrument] == on);
                assert(Loop_led_set.Read_LED_activity(track, instrument) == (on ? 1 : -1));
            }
        }
    }
    // A note transition after restoration must survive until the next drawing pass.
    Loop_led_set.Restore_all_LED();
    Loop_led_set.Request_LED_switch(0, 0, true);
    Loop_led_set.Request_LED_switch(3, 7, false);
    Update_instruments_leds();
    assert(Display_MidiLoop.leds[0][0] == 1 && Display_MidiLoop.leds[3][7] == 0);
    irq_mask = 1;
    Loop_led_set.Restore_all_LED();
    assert(irq_mask == 1);
    irq_mask = 0;
    Golive_with_MIDI_LOOP(true);
    assert(player_resets == 1 && clock_restarts == 1);
    for (auto &row : Display_MidiLoop.leds)
    {
        for (auto led : row)
        {
            assert(led == 0);
        }
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
    cpp.write_text('#include <cstdint>\n#include <cstdlib>\n#include <cassert>\nconstexpr int INSTRUMENTS = 8;\nconstexpr int TRACKS = 4;\nuint32_t irq_mask = 0;\nuint32_t __get_primask() { return irq_mask; }\nvoid __disable_irq() { irq_mask = 1; }\nvoid __enable_irq() { irq_mask = 0; }\n' + header + '\n' + source + '\n' + checks, encoding='utf-8')
    subprocess.run([compiler, '-std=c++17', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True, env=env)
    subprocess.run([str(exe)], check=True, env=env)
print('PASS: LED ownership, reassignment, mode changes, IRQ request consumption, redraw and all 32 MIDI Loop LEDs after tool return')
