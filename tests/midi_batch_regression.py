"""Exercise the real MIDI parser, production collector and pending-note control methods on the host."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
MIDI_LIB = Path(os.environ.get('USERPROFILE', str(Path.home()))) / '.platformio/packages/framework-arduinoteensy/libraries/MIDI/src'
reader = (ROOT / 'lib/MidiReader/MidiReader.cpp').read_text(encoding='utf-8')
player = (ROOT / 'lib/AudioPlayer/AudioPlayer.cpp').read_text(encoding='utf-8')
manager = (ROOT / 'lib/PlayersManager/PlayersManager.cpp').read_text(encoding='utf-8')
player_header = (ROOT / 'lib/AudioPlayer/AudioPlayer.h').read_text(encoding='utf-8')
clock_source = (ROOT / 'lib/LillaClock/LillaClock.cpp').read_text(encoding='utf-8')
config = (ROOT / 'lib/config/config.h').read_text(encoding='utf-8')


def method(source, signature):
    match = re.search(r'^' + re.escape(signature) + r'[^\n]*\n\{.*?^\}', source, re.M | re.S)
    assert match, signature
    return match.group(0) + '\n'


prefix = r'''
#include <cassert>
#include <cstdint>
#include <deque>
#include <vector>
#include <iostream>
#include "MIDI.h"
#include "MidiInputBatch.h"
uint32_t clock_us = 0, clock_step = 0;
uint32_t audio_update_time_micros = 0;
uint32_t audio_update_cycle = 0, audio_player_emergency_stops = 0;
struct CallbackStub { unsigned calls = 0; void Update() { ++calls; } } callbacks;
struct LillaClock
{
    uint8_t identity = 0;
    bool stop_flag = true;
    CallbackStub *Filter_Biquad_Manager_ptr = &callbacks, *Delay_Manager_ptr = &callbacks, *Midi_reader_ptr = &callbacks;
    void update();
};
struct DebugSerial { template<class T> void print(T) {} template<class T> void println(T) {} void println() {} } Serial;
uint32_t micros() { const auto now = clock_us; clock_us += clock_step; return now; }
uint32_t millis() { return clock_us / 1000; }
struct Transport
{
    static constexpr bool thruActivated = false;
    std::deque<uint8_t> bytes;
    std::vector<uint8_t> inject;
    unsigned reads = 0;
    void begin() {}
    bool beginTransmission(midi::MidiType) { return true; }
    void endTransmission() {}
    void write(uint8_t) {}
    unsigned available() { return bytes.size(); }
    uint8_t read()
    {
        assert(!bytes.empty());
        const auto value = bytes.front();
        bytes.pop_front();
        ++reads;
        for (auto byte : inject) { bytes.push_back(byte); }
        inject.clear();
        return value;
    }
} Serial1;
midi::MidiInterface<Transport> MIDI(Serial1);
constexpr int CC_SETTINGS = 1, MIDI_LOOP = 2;
int Lilla_state = 0, LOOP_learning_track = 0;
bool display_wait = false, LOOP_learn_flag = false;
uint32_t LOOP_elements = 0, LOOP_learn_clock = 0, LOOP_time = 1000;
uint8_t CC_midi_controller = 0;
struct FakeManager
{
    unsigned begin = 0, end = 0;
    void Begin_midi_batch() { ++begin; }
    void End_midi_batch() { ++end; }
};
struct MidiReader
{
    bool midi_stop_flag = false;
    MidiInputBatch batch;
    FakeManager manager;
    FakeManager *Players_Manager = &manager;
    std::vector<MidiInputMessage> applied;
    unsigned loop_calls = 0;
    void Collect_messages();
    void Update();
    void Handle_message(const MidiInputMessage &message) { applied.push_back(message); }
    void Update_loops() { ++loop_calls; }
};
struct Envelope
{
    unsigned releases = 0;
    void Release_note() { ++releases; }
};
struct AudioPlayer
{
    enum { IDLE, RUNNING, FADING };
    int state = IDLE;
    bool idle = true, power_on = false, warmup_for_play_again_flag = false, restart_flag = false, main_settings_editing_flag = false, pending_note_released = false;
    bool vibrato_flag = false;
    bool patch_release_pending = false;
    unsigned closes = 0;
    void Close_source() { ++closes; }
    void Enforce_cycle_deadline();
    float modulation_depth = 0;
    float pitch = 1.0f;
    int update_time = 0, mix_samples = 0;
    bool wavetable = false;
    float Read_pitch() { return pitch; }
    int Read_use_Wavetable() { return wavetable; }
    int Read_update_time() { return update_time; }
    void Set_mix_samples(uint8_t value) { mix_samples = value; }
    float pitch_note_wait = 0, velocity_gain_wait = 0;
    int patch_id_wait = 0, instrument_id_wait = 0, sound_id_wait = 0, note_wait = 0, track_wait = -1;
    int local_patch = 0, instrument_id = 0, note = 0, track = -1, channel = 0;
    uint32_t time_stamp = 0;
    Envelope envelope;
    Envelope *ADSR = &envelope;
    bool Has_pending_note(void) const;
    bool Needs_restart_mix(void) const;
    int Assigned_note(void) const;
    int Assigned_track(void) const;
    int Assigned_instrument(void) const;
    int Assigned_patch(void) const;
    bool isPoweredOn() { return power_on; }
    int Read_midi_channel() { return channel; }
    void Write_time_stamp(uint32_t value) { time_stamp = value; }
    void My_LED(bool) {}
    void Set_vibrato_flag(bool value) { vibrato_flag = value; }
    void Set_modulation(uint8_t value);
    void Get_ready_to_play(float, float, int, uint8_t, uint16_t, uint8_t);
    void Release_note();
    void Start_playing();
};
constexpr int PLAYERS = 16;
struct PlayersManager
{
    AudioPlayer voices[PLAYERS];
    AudioPlayer *Player_ptr = voices;
    bool Player_booked[PLAYERS] = {}, restart_Player[PLAYERS] = {};
    bool midi_batch_active = false;
    uint8_t restart_mix_first_player = 0;
    uint8_t modulation_value[16] = {}, players_to_restart = 0;
    int mix_calls = 0, mixed_players = 0;
    void Reset_players_to_restart() { players_to_restart = 0; }
    void Calculate_and_set_mix_samples() { ++mix_calls; mixed_players = players_to_restart; Calculate_mix(); }
    void Calculate_mix();
    int Get_span_for_all_cross_mix();
    float Get_cross_mix_time(int, int);
    bool Get_restart_player(int);
    void Cancel_restart_player(int);
    void Reset_booked_and_restart_player();
    void Begin_midi_batch();
    void End_midi_batch();
    void Set_modulation(uint8_t, uint8_t);
    void Release_Player_noteOff(uint8_t player, int = -1) { Player_ptr[player].Release_note(); }
    void Multicast_stop_players_for_NoteOff(int, int, int);
    void Multicast_stop_players_for_loop_track(int);
    void Multicast_all_notes_off(int);
};
'''

production = '\n'.join(line for line in config.splitlines() if line.startswith('static constexpr int AUDIO_')) + '\n'
production += method(clock_source, 'void LillaClock::update')
production += method(reader, 'void MidiReader::Collect_messages') + method(reader, 'void MidiReader::Update')
for name in ['Has_pending_note', 'Needs_restart_mix', 'Assigned_note', 'Assigned_track', 'Assigned_instrument', 'Assigned_patch']:
    inline = re.search(r'    (bool|int) ' + name + r'\(void\) const \{[^\n]+\}', player_header)
    assert inline
    production += inline.group(0).strip().replace(name + '(', 'AudioPlayer::' + name + '(') + '\n'
for name in ['Get_ready_to_play', 'Release_note', 'Set_modulation', 'Enforce_cycle_deadline']:
    production += method(player, 'void AudioPlayer::' + name)
for name in ['Reset_booked_and_restart_player', 'Begin_midi_batch', 'End_midi_batch', 'Set_modulation', 'Multicast_stop_players_for_NoteOff', 'Multicast_stop_players_for_loop_track', 'Multicast_all_notes_off']:
    production += method(manager, 'void PlayersManager::' + name)
for signature in ['int PlayersManager::Get_span_for_all_cross_mix', 'float PlayersManager::Get_cross_mix_time', 'bool PlayersManager::Get_restart_player', 'void PlayersManager::Cancel_restart_player']:
    production += method(manager, signature)
production += method(manager, 'void PlayersManager::Calculate_and_set_mix_samples').replace('Calculate_and_set_mix_samples', 'Calculate_mix', 1)
# Test the production pending-release completion block without mocking the complete audio renderer.
start = method(player, 'void AudioPlayer::Start_playing')
tail = start[start.index('    if (pending_note_released)'):]
production += 'void AudioPlayer::Start_playing()\n{\n    note = note_wait; track = track_wait; instrument_id = instrument_id_wait; local_patch = patch_id_wait; idle = false; state = RUNNING;\n' + tail

tests = r'''
void reset()
{
    Serial1.bytes.clear(); Serial1.inject.clear(); Serial1.reads = 0;
    clock_us = 0; clock_step = 0; Lilla_state = 0; display_wait = false;
    LOOP_learn_flag = false; MIDI.begin(MIDI_CHANNEL_OMNI); MIDI.turnThruOff();
}
int main()
{
    reset();
    MidiReader reader;
    reader.Collect_messages();
    assert(Serial1.reads == 0 && reader.batch.count == 0);
    // Real parser: partial NoteOn persists between cycles; zero velocity is normalized to NoteOff.
    Serial1.bytes = {0x90, 60}; reader.Collect_messages();
    assert(reader.batch.count == 0 && Serial1.reads == 2);
    Serial1.bytes = {100, 60, 0}; reader.Collect_messages();
    assert(reader.batch.count == 2 && reader.batch.ordered[0].type == 0x90 && reader.batch.ordered[1].type == 0x80);
    reset();
    for (int value = 0; value < 20; ++value) { Serial1.bytes.insert(Serial1.bytes.end(), {0xB0, 1, static_cast<uint8_t>(value)}); }
    Serial1.bytes.insert(Serial1.bytes.end(), {0x90, 64, 100});
    reader.Collect_messages();
    assert(Serial1.reads == 63 && reader.batch.count == 1 && reader.batch.controls[0][2].data2 == 19);
    reset();
    for (int value = 0; value < 100; ++value) { Serial1.bytes.insert(Serial1.bytes.end(), {0xB0, 1, static_cast<uint8_t>(value)}); }
    reader.Collect_messages();
    assert(Serial1.reads == MidiInputBatch::Max_rx_bytes && reader.batch.count == 0 && Serial1.bytes.size() == 236);
    // Snapshot: bytes injected during a read are left for the following cycle.
    reset(); Serial1.bytes = {0x90}; Serial1.inject = {62, 100}; reader.Collect_messages();
    assert(Serial1.reads == 1 && Serial1.bytes.size() == 2 && reader.batch.count == 0);
    reader.Collect_messages(); assert(reader.batch.count == 1);
    // Capacity: ninth ordered message is untouched, not lost.
    reset();
    for (int note = 0; note < 9; ++note) { Serial1.bytes.insert(Serial1.bytes.end(), {0x90, static_cast<uint8_t>(note), 100}); }
    reader.Collect_messages(); assert(reader.batch.count == 8 && Serial1.bytes.size() == 3);
    reader.Collect_messages(); assert(reader.batch.count == 1 && reader.batch.ordered[0].data1 == 8);
    // Parse time is a maximum, including timer wraparound; there is no waiting loop.
    reset(); Serial1.bytes = {0x90, 60, 100}; clock_step = 50; reader.Collect_messages();
    assert(Serial1.reads == 1 && Serial1.bytes.size() == 2);
    reset(); clock_us = UINT32_MAX - 20; clock_step = 25; Serial1.bytes = {0x90, 60, 100}; reader.Collect_messages();
    assert(reader.batch.count == 1);
    reset();
    Serial1.bytes.insert(Serial1.bytes.end(), 100, 0xF8); reader.Collect_messages();
    assert(Serial1.reads <= MidiInputBatch::Max_rx_bytes && reader.batch.Full());
    reset();
    // Two independent channels and final values, preserving ordered notes and non-coalescible CCs.
    Serial1.bytes = {0xE0, 0, 64, 0xE1, 0, 70, 0xE0, 1, 65, 0xD0, 50, 0xD0, 60, 0x90, 60, 100, 0x80, 60, 0, 0xB0, 123, 0};
    reader.applied.clear(); reader.Update();
    assert(reader.batch.controls[0][0].data2 == 65 && reader.batch.controls[1][0].data2 == 70);
    assert(reader.batch.controls[0][1].data1 == 60 && reader.batch.count == 3);
    assert(reader.applied[3].type == 0x90 && reader.applied[4].type == 0x80 && reader.applied[5].data1 == 123);
    assert(reader.manager.begin == 1 && reader.manager.end == 1 && reader.loop_calls == 1);
    reset(); reader.midi_stop_flag = true; Serial1.bytes = {0x90, 60, 100}; reader.Update();
    assert(Serial1.reads == 0 && reader.loop_calls == 2);
    reader.midi_stop_flag = false; Lilla_state = CC_SETTINGS; Serial1.bytes = {0xB0, 7, 100, 0xB0, 8, 90}; reader.Collect_messages();
    assert(display_wait && CC_midi_controller == 7 && Serial1.bytes.size() == 3 && reader.batch.count == 0);
    reader.Collect_messages(); assert(Serial1.bytes.size() == 3);
    PlayersManager pm;
    auto &voice = pm.voices[0];
    voice.Get_ready_to_play(1, 1, 0, 0, 0, 60);
    voice.track_wait = 2; voice.Get_ready_to_play(1, 1, 0, 0, 0, 62);
    assert(voice.Read_midi_channel() == 0 && voice.Assigned_note() == 62 && voice.Assigned_track() == 2);
    pm.Multicast_stop_players_for_NoteOff(0, 62, -1); assert(voice.power_on);
    pm.Multicast_stop_players_for_NoteOff(0, 60, 2); assert(voice.power_on);
    pm.Multicast_stop_players_for_NoteOff(0, 62, 2); assert(!voice.power_on && voice.pending_note_released);
    voice.Start_playing(); assert(voice.state == AudioPlayer::FADING && voice.envelope.releases == 1);
    voice.Get_ready_to_play(1, 1, 0, 0, 0, 62); assert(voice.power_on && !voice.pending_note_released);
    pm.Begin_midi_batch(); assert(pm.Player_booked[0]);
    pm.End_midi_batch(); assert(pm.mix_calls == 1 && pm.mixed_players == 1);
    assert(voice.mix_samples == 64 && !pm.midi_batch_active); // The 64-sample tier must assign and account for 64 samples.
    audio_update_time_micros = 3000;
    pm.Begin_midi_batch(); pm.End_midi_batch(); assert(voice.mix_samples == 0);
    assert(pm.Get_span_for_all_cross_mix() == AUDIO_PLAYER_DEADLINE_US); // Main-loop edits must not subtract an old IRQ timestamp.
    audio_update_time_micros = 2685;
    pm.Begin_midi_batch(); pm.End_midi_batch();
    assert(pm.Get_cross_mix_time(0, voice.mix_samples) <= 15);
    audio_update_time_micros = 0;
    pm.Multicast_all_notes_off(0); assert(voice.pending_note_released);
    voice.Get_ready_to_play(1, 1, 0, 0, 0, 62);
    pm.Multicast_stop_players_for_loop_track(2); assert(voice.pending_note_released);
    pm.voices[1].channel = 1;
    pm.Set_modulation(0, 127); pm.Set_modulation(1, 32);
    assert(pm.voices[0].modulation_depth == 1.0f && pm.voices[1].modulation_depth == 32 / 127.0f);
    pm.Set_modulation(0, 0); assert(!pm.voices[0].vibrato_flag && pm.voices[1].vibrato_flag);
    // The shared cycle clock is reset even while control callbacks are paused; Trigger 1 never resets it.
    LillaClock clock;
    audio_update_time_micros = 5000;
    clock.update(); assert(audio_update_time_micros == 0 && audio_update_cycle == 1 && callbacks.calls == 0);
    clock.stop_flag = false; clock.update(); assert(audio_update_cycle == 2 && callbacks.calls == 3);
    clock.identity = 1; audio_update_time_micros = 1000; clock.update();
    assert(audio_update_time_micros == 1000 && audio_update_cycle == 2 && callbacks.calls == 3);
    // Emergency protection includes time spent before Player 0 and discards queued restarts safely.
    voice.state = AudioPlayer::RUNNING; voice.idle = false; voice.power_on = true;
    voice.warmup_for_play_again_flag = true; voice.restart_flag = true;
    voice.pending_note_released = true; voice.main_settings_editing_flag = true; voice.patch_release_pending = true;
    audio_update_time_micros = AUDIO_PLAYER_DEADLINE_US - 1;
    voice.Enforce_cycle_deadline(); assert(voice.state == AudioPlayer::RUNNING && voice.closes == 0);
    audio_update_time_micros = AUDIO_PLAYER_DEADLINE_US;
    voice.Enforce_cycle_deadline();
    assert(voice.state == AudioPlayer::IDLE && voice.idle && !voice.power_on && voice.closes == 1);
    assert(!voice.warmup_for_play_again_flag && !voice.restart_flag && !voice.pending_note_released && !voice.main_settings_editing_flag && !voice.patch_release_pending);
    assert(audio_player_emergency_stops == 1 && audio_update_time_micros == AUDIO_PLAYER_DEADLINE_US);
    voice.Enforce_cycle_deadline(); assert(audio_player_emergency_stops == 1 && voice.closes == 1);
    audio_player_emergency_stops = UINT32_MAX; voice.state = AudioPlayer::RUNNING;
    voice.Enforce_cycle_deadline(); assert(audio_player_emergency_stops == UINT32_MAX);
    // One individual crossfade fits: rotate through all voices instead of favoring low indices.
    PlayersManager fair;
    for (auto &candidate : fair.voices)
    {
        candidate.Get_ready_to_play(1, 1, 0, 0, 0, 60);
        candidate.Get_ready_to_play(1, 1, 0, 0, 0, 62);
    }
    audio_update_time_micros = AUDIO_PLAYER_DEADLINE_US - 20;
    for (int first = 0; first < PLAYERS; ++first)
    {
        fair.Begin_midi_batch(); fair.End_midi_batch();
        for (int id = 0; id < PLAYERS; ++id)
        {
            assert(fair.voices[id].mix_samples == (id == first ? 32 : 0));
        }
    }
    assert(fair.restart_mix_first_player == 0);
    audio_update_time_micros = 0;
    fair.Begin_midi_batch(); fair.End_midi_batch();
    assert(fair.restart_mix_first_player == 0); // Uniform allocation does not advance the fallback cursor.
    for (const auto &candidate : fair.voices) { assert(candidate.mix_samples == 64); }
    std::cout << "PASS: real MIDI parsing, bounded non-blocking collection, coalescing, order, backpressure, CC learn, pending NoteOff and batch accounting\n";
}
'''

compiler = shutil.which('g++') or r'C:\msys64\ucrt64\bin\g++.exe'
with tempfile.TemporaryDirectory(prefix='lilla-midi-') as directory:
    cpp = Path(directory) / 'midi.cpp'
    exe = Path(directory) / ('midi.exe' if os.name == 'nt' else 'midi')
    cpp.write_text(prefix + production + tests, encoding='utf-8', newline='\r\n')
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-I' + str(MIDI_LIB), '-I' + str(ROOT / 'lib/MidiReader'), str(cpp), '-o', str(exe)], check=True)
    environment = os.environ.copy()
    environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
    subprocess.run([str(exe)], check=True, env=environment)
