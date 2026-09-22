"""Compare production voice selection against its frozen pre-refactor policy."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / 'lib/PlayersManager/PlayersManager.cpp').read_text(encoding='utf-8')
header = (ROOT / 'lib/PlayersManager/PlayersManager.h').read_text(encoding='utf-8')
legacy = (ROOT / 'tests/fixtures/player_selection_legacy.inc').read_text(encoding='utf-8')


def method(signature):
    match = re.search(r'^' + re.escape(signature) + r'[^\n]*\n\{.*?^\}', source, re.M | re.S)
    assert match, signature
    return match.group(0) + '\n'


prefix = r'''
#include <cassert>
#include <cstdint>
#include <iostream>
#include <random>
constexpr int PLAYERS = 16, INSTRUMENTS = 8, FIRST_LIVE_SAMPLING_FILE = 100;
bool Is_playback_file(int id) { return id >= 323 && id < 331; }
constexpr int OPTIMIZATION_VOICES[] = {16, 12, 10};
int optimization = 0, Patch_id = 3;
struct PresetData { bool use_Wavetable = false, precedence = false; int file = 0; } Preset[INSTRUMENTS];
struct Voice
{
    bool playing = false, powered = false, sample = false, protected_voice = false, pending = false;
    int patch = 3, patch_wait = 3, instrument = 0, note = 60, track = -1;
    int pending_patch = 3, pending_instrument = 0, pending_note = 60, pending_track = -1;
    unsigned long timestamp = 0;
    bool isPlaying() const { return playing; }
    bool isPoweredOn() const { return powered; }
    bool Uses_sample_voice() const { return playing && sample; }
    bool Has_pending_note() const { return pending; }
    bool Read_precedence() const { return protected_voice; }
    int State() const { return playing ? (powered ? 1 : 2) : 0; }
    int Read_patch_wait() const { return patch_wait; }
    int Read_instrument() const { return instrument; }
    unsigned long Read_time_stamp() const { return timestamp; }
    int Assigned_patch() const { return pending ? pending_patch : patch; }
    int Assigned_instrument() const { return pending ? pending_instrument : instrument; }
    int Assigned_note() const { return pending ? pending_note : note; }
    int Assigned_track() const { return pending ? pending_track : track; }
};
struct PlayersManager
{
    Voice voices[PLAYERS];
    Voice *Player_ptr = voices;
    bool Player_booked[PLAYERS] = {};
    int players_playing = 0;
    void Update_players_stistics()
    {
        players_playing = 0;
        for (const auto &voice : voices) { players_playing += voice.playing; }
    }
    int Count_sample_voices() const;
    int Select_player_for_note(uint8_t, uint8_t, int);
    int Smartfind_oldest_player(uint8_t, bool, bool);
    int Simplefind_oldest_player(bool);
    int Simplefind_oldest_sample_player(bool, bool);
    int Legacy_select(uint8_t, uint8_t, int);
    int Legacy_Smartfind_oldest_player(uint8_t, bool, bool);
    int Legacy_Simplefind_oldest_player(bool);
    int Legacy_Simplefind_oldest_sample_player(bool, bool);
'''

production = method('int PlayersManager::Count_sample_voices')
refactored = 'int PlayersManager::Select_player_for_note' in source
if refactored:
    filters = re.search(r'    enum class Selection_order.*?    };', header, re.S)
    assert filters
    prefix += filters.group(0) + '\n'
    prefix += '    bool Can_select_player(int, const Player_filter &) const;\n    int Find_player(const Player_filter &, Selection_order);\n    int Find_free_player();\n    int Find_other_patch_player(bool, bool);\n'
    for signature in ['int PlayersManager::Select_player_for_note', 'bool PlayersManager::Can_select_player', 'int PlayersManager::Find_player', 'int PlayersManager::Find_free_player', 'int PlayersManager::Find_other_patch_player']:
        production += method(signature)
else:
    start = source.index("    // Se e' una nota")
    end = source.index('    if (finished)\n    {\n        Player_booked', start)
    selection = source[start:end].replace('        return;', '        return -1;')
    production += 'int PlayersManager::Select_player_for_note(uint8_t instrument_id, uint8_t note_number, int track)\n{\n' + selection + '    return finished ? id_player : -1;\n}\n'
for signature in ['int PlayersManager::Smartfind_oldest_player', 'int PlayersManager::Simplefind_oldest_player', 'int PlayersManager::Simplefind_oldest_sample_player']:
    production += method(signature)
prefix += '};\n'

tests = r'''
int main()
{
    PlayersManager pm;
    assert(pm.Select_player_for_note(0, 62, -1) == 0);
    pm.Player_booked[0] = true;
    assert(pm.Select_player_for_note(0, 62, -1) == 1);
    // Exact same-note retrigger intentionally overrides the booking, including a pending replacement.
    pm.voices[3].playing = true; pm.voices[3].pending = true; pm.voices[3].pending_note = 62;
    pm.Player_booked[3] = true;
    assert(pm.Select_player_for_note(0, 62, -1) == 3);
    assert(pm.Select_player_for_note(0, 62, 0) != 3);
    // Preserve the unusual legacy tie-break: last foreign-patch voice for wavetable/live, first for files.
    for (auto &voice : pm.voices) { voice.playing = true; voice.powered = true; voice.pending = false; voice.patch_wait = 8; }
    for (auto &booked : pm.Player_booked) { booked = false; }
    Preset[0].use_Wavetable = true;
    assert(pm.Select_player_for_note(0, 62, -1) == 15);
    Preset[0].use_Wavetable = false;
    assert(pm.Select_player_for_note(0, 62, -1) == 0);
    // Equal timestamps keep the lowest eligible index in oldest-voice searches.
    assert(pm.Simplefind_oldest_player(true) == 0);
    pm.Player_booked[0] = true;
    assert(pm.Simplefind_oldest_player(true) == 1);
    std::mt19937 random(0x4C494C4C);
    for (int scenario = 0; scenario < 100000; ++scenario)
    {
        optimization = 0; // Compare with the former 16-voice profile; reduced profiles are removed.
        const uint8_t instrument = random() % INSTRUMENTS;
        const uint8_t note = 60 + random() % 5;
        const int track = static_cast<int>(random() % 5) - 1;
        for (auto &preset : Preset)
        {
            preset.use_Wavetable = random() % 2;
            preset.precedence = random() % 2;
            preset.file = random() % 3 == 0 ? FIRST_LIVE_SAMPLING_FILE : 0;
        }
        for (int id = 0; id < PLAYERS; ++id)
        {
            auto &voice = pm.voices[id];
            voice.playing = random() % 5 != 0;
            voice.powered = voice.playing && random() % 2;
            voice.sample = random() % 2;
            voice.protected_voice = random() % 3 == 0;
            voice.pending = voice.playing && random() % 4 == 0;
            voice.patch = random() % 5;
            voice.patch_wait = voice.pending ? random() % 5 : voice.patch;
            voice.instrument = random() % INSTRUMENTS;
            voice.note = 60 + random() % 5;
            voice.track = static_cast<int>(random() % 5) - 1;
            voice.pending_patch = voice.patch_wait;
            voice.pending_instrument = random() % INSTRUMENTS;
            voice.pending_note = 60 + random() % 5;
            voice.pending_track = static_cast<int>(random() % 5) - 1;
            voice.timestamp = random() % 20;
            pm.Player_booked[id] = voice.pending || random() % 4 == 0; // Begin_midi_batch reserves pending starts.
        }
        const int expected = pm.Legacy_select(instrument, note, track);
        const int actual = pm.Select_player_for_note(instrument, note, track);
        if (expected != actual)
        {
            std::cerr << "Mismatch at scenario " << scenario << ": " << expected << " vs " << actual << '\n';
            return 1;
        }
        for (bool powered : {false, true})
        {
            assert(pm.Simplefind_oldest_player(powered) == pm.Legacy_Simplefind_oldest_player(powered));
            for (bool playing : {false, true})
            {
                assert(pm.Smartfind_oldest_player(instrument, powered, playing) == pm.Legacy_Smartfind_oldest_player(instrument, powered, playing));
                assert(pm.Simplefind_oldest_sample_player(powered, playing) == pm.Legacy_Simplefind_oldest_sample_player(powered, playing));
            }
        }
    }
#ifdef REFACTORED
    // The centralized eligibility check also protects pending starts if a caller has not rebuilt bookings yet.
    pm.voices[0].pending = true; pm.Player_booked[0] = false;
    PlayersManager::Player_filter filter;
    assert(!pm.Can_select_player(0, filter));
#endif
    std::cout << "PASS: 100000 selection scenarios preserve legacy priorities, reservations, polyphony limits and tie-breaks\n";
}
'''

compiler = shutil.which('g++') or r'C:\msys64\ucrt64\bin\g++.exe'
with tempfile.TemporaryDirectory(prefix='lilla-selection-') as directory:
    cpp = Path(directory) / 'selection.cpp'
    exe = Path(directory) / ('selection.exe' if os.name == 'nt' else 'selection')
    cpp.write_text(prefix + legacy + production + tests, encoding='utf-8', newline='\r\n')
    flags = ['-DREFACTORED'] if refactored else []
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', *flags, str(cpp), '-o', str(exe)], check=True)
    environment = os.environ.copy()
    environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
    subprocess.run([str(exe)], check=True, env=environment)
