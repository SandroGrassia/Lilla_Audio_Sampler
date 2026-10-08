"""Exercise linked stereo editing, live presets, RETURN and automatic trim saving."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
main = (root / 'src/main.cpp').read_text(encoding='utf-8')
shared = (root / 'lib/SharedSampler/SharedSampler.cpp').read_text(encoding='utf-8')
elements = (root / 'lib/SharedElements/SharedElements.h').read_text(encoding='utf-8')


def method(source, signature):
    return re.search(r'^' + re.escape(signature) + r'\n\{.*?^\}', source, re.M | re.S).group(0) + '\n'


prefix = r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
using byte = uint8_t;
constexpr int SOUNDS_MAX = 800, PATCHES_MAX = 200, INSTRUMENTS = 8;
constexpr int FIRST_RECORDING_FILE = 260, DIRECT_SAMPLING = 1, SOUND_EDIT = 2, MIDI_LOOP = 3;
struct LillaFRAM_2x512 { static constexpr byte ERROR_0 = 0; };
// SOUND_STRUCT
struct DS_Trim { uint32_t first, last; };
struct InstrumentData { bool used = false; };
struct PatchData { int instruments = 0; InstrumentData Instrument[INSTRUMENTS]; };
PatchData Patch[PATCHES_MAX + 1];
Sound_struct Sound[SOUNDS_MAX + 2], DS_sound_before_edit[2], S_Sound_cache_P[SOUNDS_MAX];
struct Entry { bool stereo = true; } Recording[2];
struct PresetData
{
    bool use_Wavetable = false;
    Sound_struct settings{};
} Preset[2];
int recording = 0, Patch_id = PATCHES_MAX, Instrument_id = 0, Sound_id = SOUNDS_MAX;
int Lilla_state = DIRECT_SAMPLING, Lilla_state_0 = DIRECT_SAMPLING, volume_patch = 10;
int samples_in_file, Noclick_max, S_trim_step, trim_speed = 0, S_slicing_window, S_pointer;
int maps = 0, multicasts[2] = {}, audio_depth = 0, saves = 0, S_menu_max = 0;
int pitch_updates[2] = {}, volume_updates[2] = {}, pan_updates[2] = {}, midi_releases = 0;
bool S_sound_original, tables_ok = true, solo_flag = false;
enum { value_S_Return, value_S_Clone, value_S_Drop };
bool S_Menu[3];
struct ArchiveMock
{
    bool saved = false;
    uint32_t first = 0, last = 999;
    byte Read_DS_edit(int, uint32_t &a, uint32_t &b)
    {
        if (saved)
        {
            a = first;
            b = last;
        }
        return 0;
    }
    byte Save_DS_edit(int, uint32_t a, uint32_t b)
    {
        assert(audio_depth == 0);
        ++saves;
        saved = true;
        first = a;
        last = b;
        return 0;
    }
} Archive;
struct DisplayMock
{
    void Show_SOUND_page(int, int) {}
    void Show_SOUND_menu() {}
    void Show_wave(int) {}
} Display_Sound;
struct PointerMock
{
    void Set_pointer_to_first_menu_element() {}
    int Get_pointer() { return 0; }
} Pointer_Sound;
struct LedMock { void Restore_all_LED() {} } Performance_led_set;
struct PlayerMock
{
    void Multicast_main_settings_editing(int, int i) { ++multicasts[i]; }
    void Multicast_release_players(int) { ++midi_releases; }
    void Update_Preset_midi_channel(int, int i) { Preset[i].settings.data = Sound[SOUNDS_MAX + i].data; }
    void Update_Preset_attack_type(int, int i) { Preset[i].settings.data = Sound[SOUNDS_MAX + i].data; }
    void Update_Preset_pitch(int, int i) { Preset[i].settings.pitch = Sound[SOUNDS_MAX + i].pitch; }
    void Multicast_pitch_for_sound_edit(int i) { ++pitch_updates[i]; }
    void Update_Preset_volume(int, int i, float) { Preset[i].settings.gain = Sound[SOUNDS_MAX + i].gain; }
    void Multicast_volume_for_instrument_edit(int i) { ++volume_updates[i]; }
    void Update_Preset_pan(int, int i) { Preset[i].settings.pan = Sound[SOUNDS_MAX + i].pan; }
    void Multicast_pan(int i) { ++pan_updates[i]; }
    void Update_Preset_attack(int, int i) { Preset[i].settings.attack = Sound[SOUNDS_MAX + i].attack; }
    void Update_Preset_decay(int, int i) { Preset[i].settings.decay = Sound[SOUNDS_MAX + i].decay; }
    void Update_Preset_sustain(int, int i) { Preset[i].settings.sustain = Sound[SOUNDS_MAX + i].sustain; }
    void Update_Preset_release(int, int i) { Preset[i].settings.release = Sound[SOUNDS_MAX + i].release; }
} Players_Manager;
void AudioNoInterrupts() { ++audio_depth; }
void AudioInterrupts() { --audio_depth; }
void Require_FRAM(byte result) { assert(result == 0); }
void Clear_UI_events() {}
void S_Set_Sound_SOLO_OFF() { solo_flag = false; }
void S_Map_one_Instrument_for_all_notes(int) {}
void P_Update_all_maps_Instrument_for_notes() { ++maps; }
void Golive_DIRECT_SAMPLING() { Lilla_state = DIRECT_SAMPLING; }
int DS_get_samples_in_Recording(int) { return 1000; }
int Get_samples_in_raw_file(int) { return 1000; }
int S_Calc_Noclick_max(bool) { return 10; }
int S_Calc_trim_step(int) { return 1; }
int S_Get_sounds_free() { return 1; }
float Patch_volume_gain(int) { return 1; }
void DS_set_DS_Sampling_Patch()
{
    Sound[SOUNDS_MAX] = Sound[SOUNDS_MAX + 1] = {};
    Sound[SOUNDS_MAX].used = Sound[SOUNDS_MAX + 1].used = true;
    Sound[SOUNDS_MAX].pan = -16;
    Sound[SOUNDS_MAX + 1].pan = 16;
    Patch[PATCHES_MAX].Instrument[0].used = Patch[PATCHES_MAX].Instrument[1].used = true;
}
bool S_Rebuild_audio_tables(uint8_t)
{
    assert(audio_depth == 1);
    if (tables_ok)
    {
        for (int i = 0; i < Patch[PATCHES_MAX].instruments; ++i)
        {
            Preset[i].settings = Sound[SOUNDS_MAX + i];
        }
    }
    return tables_ok;
}
'''
sound_struct = elements[elements.index('struct Sound_struct'):elements.index('static constexpr uint8_t SIZE_OF_SOUND')]
prefix = prefix.replace('// SOUND_STRUCT', sound_struct + method(elements, 'inline bool operator==(const Sound_struct &lhs, const Sound_struct &rhs)'))
production = method(shared, 'DS_Trim DS_get_trim(const Sound_struct *channels, bool stereo)')
production += method(shared, 'void DS_copy_edited_parameters(const Sound_struct &before, const Sound_struct &edited, Sound_struct &other)')
for signature in [
    'bool S_Verify_is_Sound_original(const int sound_id)',
    'void S_Select_menu_elements(void)',
    'bool DS_load_recording_sounds(void)',
    'void DS_open_sound_edit(int channel)',
    'void DS_close_sound_edit(void)',
    'void DS_sync_stereo_parameters(void)',
    'bool S_Fill_tables(uint8_t instrument_id)',
]:
    production += method(main, signature)
finish_start = main.index('        if (Lilla_state_0 == DIRECT_SAMPLING)\n        {\n            if (Recording[recording].stereo)')
finish_end = main.index('        // Switch Sound or INSTRUMENT_EDIT', finish_start)
production += '\nvoid finish_edit(const Sound_struct &sound_before_edit)\n{\n' + main[finish_start:finish_end] + '}\n'

checks = r'''
void begin_edit()
{
    DS_sound_before_edit[0] = Sound[SOUNDS_MAX];
    DS_sound_before_edit[1] = Sound[SOUNDS_MAX + 1];
}
int main()
{
    assert(DS_load_recording_sounds());
    assert(Patch[PATCHES_MAX].instruments == 2);
    DS_open_sound_edit(0);
    assert(Lilla_state == SOUND_EDIT && Instrument_id == 0 && Sound_id == SOUNDS_MAX);
    assert(S_Menu[value_S_Return] && !S_Menu[value_S_Clone] && !S_Menu[value_S_Drop] && S_menu_max == 0);
    for (int channel : {0, 1})
    {
        DS_open_sound_edit(channel);
        begin_edit();
        auto &source = Sound[SOUNDS_MAX + channel];
        auto &other = Sound[SOUNDS_MAX + (channel ^ 1)];
        const uint16_t other_file = other.file;
        const int8_t old_pan = other.pan;
        source.gain = channel == 0 ? 37 : 30;
        source.pitch = channel == 0 ? 12 : -12;
        source.data = channel == 0 ? 5 : 10;
        source.attack = channel + 20;
        source.decay = channel + 30;
        source.sustain = channel + 40;
        source.release = channel + 10;
        finish_edit(DS_sound_before_edit[channel]);
        assert(other.gain == source.gain && other.pitch == source.pitch && other.data == source.data);
        assert(other.attack == source.attack && other.decay == source.decay && other.sustain == source.sustain && other.release == source.release);
        assert(other.file == other_file && other.pan == old_pan && saves == 0);
        assert(Preset[channel ^ 1].settings.gain == source.gain && Preset[channel ^ 1].settings.data == source.data);
        assert(Preset[channel ^ 1].settings.attack == source.attack && Preset[channel ^ 1].settings.release == source.release);
        assert(pitch_updates[channel ^ 1] > 0 && volume_updates[channel ^ 1] > 0);
        const int previous_releases = midi_releases;
        const int previous_pitch_updates = pitch_updates[channel ^ 1];
        AudioNoInterrupts();
        DS_sync_stereo_parameters();
        AudioInterrupts();
        assert(midi_releases == previous_releases && pitch_updates[channel ^ 1] == previous_pitch_updates);
        begin_edit();
        source.pan = channel == 0 ? -5 : 5;
        finish_edit(DS_sound_before_edit[channel]);
        assert(other.pan == source.pan && pan_updates[channel ^ 1] > 0);
    }
    assert(midi_releases == 2 && maps == 2);
    for (int channel : {0, 1})
    {
        DS_open_sound_edit(channel);
        begin_edit();
        auto &source = Sound[SOUNDS_MAX + channel];
        source.A = channel == 0 ? 40 : 80;
        source.B = channel == 0 ? 600 : 500;
        source.mode = channel + 1;
        source.Noclick = channel + 10;
        AudioNoInterrupts();
        assert(S_Fill_tables(channel));
        AudioInterrupts();
        finish_edit(DS_sound_before_edit[channel]);
        assert(Sound[SOUNDS_MAX].A == source.A && Sound[SOUNDS_MAX + 1].B == source.B);
        assert(Preset[0].settings.A == source.A && Preset[1].settings.B == source.B);
        assert(Preset[channel ^ 1].settings.mode == source.mode && Preset[channel ^ 1].settings.Noclick == source.Noclick);
        assert(multicasts[channel ^ 1] > 0);
        assert(Archive.first == source.A && Archive.last == source.B && saves == channel + 1);
    }
    begin_edit();
    Sound[SOUNDS_MAX + 1].A = 100;
    const auto published_other = Preset[0].settings;
    tables_ok = false;
    AudioNoInterrupts();
    assert(!S_Fill_tables(1));
    AudioInterrupts();
    assert(Sound[SOUNDS_MAX].A == 80 && Preset[0].settings == published_other);
    Sound[SOUNDS_MAX + 1] = DS_sound_before_edit[1]; // The editor restores a rejected candidate.
    finish_edit(DS_sound_before_edit[1]);
    assert(saves == 2);
    tables_ok = true;
    DS_close_sound_edit();
    assert(Lilla_state == DIRECT_SAMPLING && saves == 3 && Archive.first == 80 && Archive.last == 500);
    assert(Sound[SOUNDS_MAX].gain == 30 && Sound[SOUNDS_MAX + 1].gain == 30);
    assert(DS_load_recording_sounds() && Sound[SOUNDS_MAX].A == 80 && Sound[SOUNDS_MAX + 1].B == 500);
    Recording[0].stereo = false;
    assert(DS_load_recording_sounds() && Patch[PATCHES_MAX].instruments == 1);
    DS_open_sound_edit(0);
    begin_edit();
    Sound[SOUNDS_MAX].A = 100;
    Sound[SOUNDS_MAX].B = 400;
    finish_edit(DS_sound_before_edit[0]);
    assert(Archive.first == 100 && Archive.last == 400 && Sound[SOUNDS_MAX + 1].A == 80);
    DS_close_sound_edit();
    assert(saves == 5 && audio_depth == 0);
    recording = -1;
    assert(DS_load_recording_sounds() && Patch[PATCHES_MAX].instruments == 0);
    Lilla_state_0 = MIDI_LOOP;
    DS_sync_stereo_parameters(); // Other modes must not access the absent sampler recording.
    S_Select_menu_elements();
    assert(S_Menu[value_S_Return] && !S_Menu[value_S_Clone] && !S_Menu[value_S_Drop]);
    std::cout << "PASS: linked parameters in both directions, distinct sources, live stereo trim, rollback, autosave, RETURN and mono\n";
}
'''
compiler = shutil.which('g++') or 'C:/msys64/ucrt64/bin/g++.exe'
with tempfile.TemporaryDirectory(prefix='lilla-sampler-edit-') as directory:
    source = Path(directory) / 'edit.cpp'
    executable = Path(directory) / 'edit.exe'
    source.write_text(prefix + production + checks, encoding='utf-8', newline='\r\n')
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', str(source), '-o', str(executable)], check=True)
    environment = os.environ.copy()
    environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
    subprocess.run([str(executable)], env=environment, check=True)
