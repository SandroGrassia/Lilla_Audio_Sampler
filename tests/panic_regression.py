"""Exercise the production Panic sequence and player cancellation with a simulated audio callback."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
main = (ROOT / 'src/main.cpp').read_text(encoding='utf-8')
panic = main.split('    // Panic\n', 1)[1].split('    // Pre-listen volume', 1)[0]
player = (ROOT / 'lib/AudioPlayer/AudioPlayer.cpp').read_text(encoding='utf-8')
methods = '\n'.join(re.search(r'^void AudioPlayer::' + name + r'\(void\).*?^}', player, re.M | re.S).group(0) for name in ('Fast_stop', 'Panic_stop'))
fixture = r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <cstdio>
constexpr int PLAYERS = 16, TRACKS = 4, AUDIO_BLOCK_SAMPLES = 128, DELAY_CACHE_CHANNEL_SAMPLES = 512;
constexpr int IDLE = 0, IDLE_REQUEST = 1, PLAYING = 2, EN_PB_LineOutVol = 0, MIDI_LOOP = 3, DELAY_SETTINGS = 4, DIRECT_SAMPLING = 5;
bool irq = true, LOOP_track_run[TRACKS];
bool feedback_restored = false;
int elapsed = 0, player_duration = 0, gain_duration = 0, Lilla_state = 0, Lilla_state_0 = 0;
int16_t DELAY_fifo_L[DELAY_CACHE_CHANNEL_SAMPLES], DELAY_fifo_R[DELAY_CACHE_CHANNEL_SAMPLES];
void AudioNoInterrupts() { assert(irq); irq = false; }
void AudioInterrupts() { assert(!irq); irq = true; }
bool Read_pushbutton(int) { return true; }
void Check_cleared()
{
    assert(!irq);
    for (int i = 0; i < DELAY_CACHE_CHANNEL_SAMPLES; ++i)
    {
        assert(DELAY_fifo_L[i] == 0 && DELAY_fifo_R[i] == 0);
    }
}
struct Control
{
    bool running = false;
    int starts = 0;
    bool Is_running() const { assert(!irq); return running; }
    void Stop() { assert(!irq); running = false; }
    void Start() { Check_cleared(); assert(feedback_restored); running = true; ++starts; }
} Midi_reader, Trigger;
struct Envelope { void Fast_stop() { assert(!irq); } } envelope;
class AudioPlayer
{
public:
    bool warmup_for_play_again_flag = true, restart_flag = true, pending_note_released = true;
    int state = PLAYING;
    Envelope *ADSR = &envelope;
    bool isPlaying() { assert(!irq); return state != IDLE; }
    void Fast_stop(void);
    void Panic_stop(void);
} Player[PLAYERS];
struct Manager
{
    void Reset_booked_and_restart_player() { assert(!irq); }
    void Reset_players_to_restart() { assert(!irq); }
} Players_Manager;
struct Feedback
{
    bool Is_silent() { assert(!irq); return elapsed >= gain_duration; }
} D_gain_L_feedback, D_gain_R_n;
struct DelayManager
{
    void Silence_feedback() { assert(!irq && !Midi_reader.running && !Trigger.running); feedback_restored = false; }
    void Restore_feedback() { Check_cleared(); assert(!Midi_reader.running && !Trigger.running); feedback_restored = true; }
} Delay_manager;
struct LEDs { void Request_all_LED_switch_off() { assert(irq); } } Loop_led_set;
struct Display { void D_feedback() { assert(irq); } } Display_Delay;
void delay(int milliseconds)
{
    assert(irq && !Midi_reader.running && !Trigger.running);
    for (bool running : LOOP_track_run)
    {
        assert(!running);
    }
    for (int i = 0; i < DELAY_CACHE_CHANNEL_SAMPLES; ++i)
    {
        assert(DELAY_fifo_L[i] == 123 && DELAY_fifo_R[i] == -321);
    }
    if (milliseconds == 10)
    {
        assert(elapsed >= player_duration && elapsed >= gain_duration);
    }
    elapsed += milliseconds;
    for (auto &voice : Player)
    {
        assert(!voice.warmup_for_play_again_flag && !voice.restart_flag && !voice.pending_note_released);
        if (elapsed >= player_duration)
        {
            voice.state = IDLE;
        }
    }
}
'''
checks = r'''
int main()
{
    for (int duration : {7, 35})
    {
        for (int mode : {0, MIDI_LOOP, DELAY_SETTINGS})
        {
            for (int initial = 0; initial < 4; ++initial)
            {
                elapsed = 0;
                player_duration = duration;
                gain_duration = 25;
                Lilla_state = mode;
                Lilla_state_0 = DIRECT_SAMPLING;
                Midi_reader = {bool(initial & 1), 0};
                Trigger = {bool(initial & 2), 0};
                std::fill_n(DELAY_fifo_L, DELAY_CACHE_CHANNEL_SAMPLES, 123);
                std::fill_n(DELAY_fifo_R, DELAY_CACHE_CHANNEL_SAMPLES, -321);
                std::fill_n(LOOP_track_run, TRACKS, true);
                for (auto &voice : Player)
                {
                    voice = AudioPlayer();
                }
                Player[0].state = IDLE; // Cancellation must also cover an idle voice with stale pending flags.
                Panic();
                assert(feedback_restored);
                assert(irq && elapsed >= std::max(duration, gain_duration) + 10);
                assert(Midi_reader.running == bool(initial & 1) && Midi_reader.starts == bool(initial & 1));
                assert(Trigger.running == bool(initial & 2) && Trigger.starts == bool(initial & 2));
                AudioNoInterrupts();
                Check_cleared();
                AudioInterrupts();
            }
        }
    }
    std::puts("PASS: Panic waits for players and feedback, cancels pending notes, clears both buffers, preserves stopped controls");
}
'''
compiler = shutil.which('g++') or r'C:\msys64\ucrt64\bin\g++.exe'
with tempfile.TemporaryDirectory(prefix='lilla-panic-') as directory:
    cpp = Path(directory) / 'panic.cpp'
    exe = Path(directory) / 'panic.exe'
    cpp.write_text(fixture + methods + '\nvoid Panic() {\n' + panic + '\n}\n' + checks, encoding='utf-8', newline='\r\n')
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', str(cpp), '-o', str(exe)], check=True)
    environment = os.environ.copy()
    environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
    subprocess.run([str(exe)], check=True, env=environment)
