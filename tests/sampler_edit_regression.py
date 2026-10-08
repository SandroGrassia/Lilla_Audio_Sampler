"""Exercise the production Sampler editor entry, stereo trim, rollback and reload."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
main = (root / 'src/main.cpp').read_text(encoding='utf-8')
shared = (root / 'lib/SharedSampler/SharedSampler.cpp').read_text(encoding='utf-8')


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
struct Sound_struct
{
    uint32_t A = 0, B = 0;
    int file = 0, pan = 0, gain = 28;
    bool operator==(const Sound_struct &) const = default;
};
struct DS_Trim { uint32_t first, last; };
struct InstrumentData { bool used = false; };
struct PatchData { int instruments = 0; InstrumentData Instrument[INSTRUMENTS]; };
PatchData Patch[PATCHES_MAX + 1];
Sound_struct Sound[SOUNDS_MAX + 2], DS_sound_before_edit[2], S_Sound_cache_P[SOUNDS_MAX];
struct Entry { bool stereo = true; } Recording[2];
struct PresetData { bool use_Wavetable = false; uint32_t A = 0, B = 0; } Preset[2];
int recording = 0, Patch_id = PATCHES_MAX, Instrument_id = 0, Sound_id = SOUNDS_MAX;
int Lilla_state = DIRECT_SAMPLING, Lilla_state_0 = DIRECT_SAMPLING;
int samples_in_file, Noclick_max, S_trim_step, trim_speed = 0, S_slicing_window, S_pointer;
int maps = 0, multicasts = 0, audio_depth = 0, saves = 0, S_menu_max = 0;
bool S_sound_original, tables_ok = true, solo_flag = false;
enum { value_S_Return, value_S_Clone, value_S_Drop, value_S_Exit, value_S_SaveExit };
bool S_Menu[5];
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
    void Multicast_main_settings_editing(int, int) { ++multicasts; }
} Players_Manager;
void AudioNoInterrupts() { ++audio_depth; }
void AudioInterrupts() { --audio_depth; }
void Require_FRAM(byte result) { assert(result == 0); }
void Clear_UI_events() {}
void S_Set_Sound_SOLO_OFF() { solo_flag = false; }
void P_Update_all_maps_Instrument_for_notes() { ++maps; }
void Golive_DIRECT_SAMPLING() { Lilla_state = DIRECT_SAMPLING; }
int DS_get_samples_in_Recording(int) { return 1000; }
int Get_samples_in_raw_file(int) { return 1000; }
int S_Calc_Noclick_max(bool) { return 10; }
int S_Calc_trim_step(int) { return 1; }
int S_Get_sounds_free() { return 1; }
void DS_set_DS_Sampling_Patch()
{
    Sound[SOUNDS_MAX] = Sound[SOUNDS_MAX + 1] = {};
    Patch[PATCHES_MAX].Instrument[0].used = Patch[PATCHES_MAX].Instrument[1].used = true;
}
bool S_Fill_all_tables()
{
    assert(audio_depth == 1);
    if (tables_ok)
    {
        for (int i = 0; i < Patch[PATCHES_MAX].instruments; ++i)
        {
            Preset[i].A = Sound[SOUNDS_MAX + i].A;
            Preset[i].B = Sound[SOUNDS_MAX + i].B;
        }
    }
    return tables_ok;
}
'''
production = method(shared, 'DS_Trim DS_get_trim(const Sound_struct *channels, bool stereo)')
for signature in [
    'bool S_Verify_is_Sound_original(const int sound_id)',
    'void S_Select_menu_elements(void)',
    'bool DS_load_recording_sounds(void)',
    'void DS_open_sound_edit(int channel)',
    'bool DS_close_sound_edit(bool save)',
]:
    production += method(main, signature)

checks = r'''
int main()
{
    assert(DS_load_recording_sounds());
    assert(Patch[PATCHES_MAX].instruments == 2);
    DS_open_sound_edit(0);
    assert(Lilla_state == SOUND_EDIT && Instrument_id == 0 && Sound_id == SOUNDS_MAX);
    assert(S_Menu[value_S_Exit] && S_Menu[value_S_SaveExit] && !S_Menu[value_S_Clone]);
    Sound[SOUNDS_MAX].A = 40;
    Sound[SOUNDS_MAX].B = 600;
    DS_open_sound_edit(1);
    assert(DS_sound_before_edit[0].A == 0);
    Sound[SOUNDS_MAX + 1].A = 20;
    Sound[SOUNDS_MAX + 1].B = 700;
    assert(DS_close_sound_edit(true));
    assert(Archive.first == 20 && Archive.last == 700 && saves == 1);
    assert(Preset[0].A == 20 && Preset[1].A == 20 && Preset[0].B == 700 && Preset[1].B == 700);
    assert(Lilla_state == DIRECT_SAMPLING && audio_depth == 0);
    assert(DS_load_recording_sounds() && Sound[SOUNDS_MAX + 1].A == 20);
    DS_open_sound_edit(1);
    Sound[SOUNDS_MAX + 1].A = 50;
    Sound[SOUNDS_MAX + 1].gain = 70;
    assert(DS_close_sound_edit(false));
    assert(Sound[SOUNDS_MAX + 1].A == 20 && Sound[SOUNDS_MAX + 1].gain == 28 && saves == 1);
    DS_open_sound_edit(0);
    Sound[SOUNDS_MAX].A = 45;
    tables_ok = false;
    assert(!DS_close_sound_edit(true) && Lilla_state == SOUND_EDIT && saves == 1);
    assert(Sound[SOUNDS_MAX].A == 45 && audio_depth == 0);
    assert(!DS_close_sound_edit(false) && Sound[SOUNDS_MAX].A == 45);
    tables_ok = true;
    assert(DS_close_sound_edit(false));
    Recording[0].stereo = false;
    assert(DS_load_recording_sounds() && Patch[PATCHES_MAX].instruments == 1);
    assert(!Patch[PATCHES_MAX].Instrument[1].used && Sound[SOUNDS_MAX].pan == 0);
    DS_open_sound_edit(0);
    Sound[SOUNDS_MAX].A = 100;
    Sound[SOUNDS_MAX].B = 500;
    assert(DS_close_sound_edit(true));
    assert(Archive.first == 100 && Archive.last == 500);
    recording = -1;
    assert(DS_load_recording_sounds() && Patch[PATCHES_MAX].instruments == 0);
    Lilla_state_0 = MIDI_LOOP;
    S_Select_menu_elements();
    assert(!S_Menu[value_S_Exit] && !S_Menu[value_S_SaveExit] && S_Menu[value_S_Return]);
    std::cout << "PASS: editor channel selection, stereo union, saved playback, EXIT rollback, reload and failed preparation\n";
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
