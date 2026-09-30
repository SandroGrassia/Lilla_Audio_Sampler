"""Exercise production preset preparation with missing and restored audio sources."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'lib/PlayersManager/PlayersManager.cpp').read_text(encoding='utf-8')
method = re.search(r'^bool PlayersManager::Build_presets_snapshot\(.*?^}', source, re.M | re.S).group(0)
fixture = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
constexpr int INSTRUMENTS = 8, PATCHES_MAX = 200, FIRST_LIVE_SAMPLING_FILE = 300;
struct SoundData { int file = 0; int A = 17; int B = 450; bool used = true; } Sound[8];
struct Instrument { bool used = false; int sound_id = 0; };
struct PatchData { Instrument Instrument[INSTRUMENTS]; } Patch[PATCHES_MAX + 1];
struct Preset_struct { bool active = false; int file = 0; };
struct AudioSource { uint32_t samples = 0; };
struct Cache
{
    uint32_t sizes[300] = {};
    AudioSource Get_source(int file) const { return {sizes[file]}; }
};
class PlayersManager
{
public:
    Cache *Cache_manager_ptr = nullptr;
    int builds[INSTRUMENTS] = {};
    int Get_sound_id(int patch, int instrument) { return Patch[patch].Instrument[instrument].sound_id; }
    Preset_struct Build_Preset(int patch, int instrument, float)
    {
        ++builds[instrument];
        return {true, Sound[Get_sound_id(patch, instrument)].file};
    }
    bool Build_presets_snapshot(int patch, float volume, Preset_struct (&presets)[INSTRUMENTS], uint16_t &mask);
};
'''
checks = r'''
int main()
{
    Cache cache;
    PlayersManager manager;
    manager.Cache_manager_ptr = &cache;
    Preset_struct presets[INSTRUMENTS];
    uint16_t mask = 0;
    for (int i = 0; i < INSTRUMENTS; ++i)
    {
        Sound[i].file = 10 + i;
        Patch[1].Instrument[i].used = i < 5;
        Patch[1].Instrument[i].sound_id = i;
        presets[i] = {true, 99}; // Previous active presets must not survive a missing source.
    }
    cache.sizes[10] = 500; // Available RAW.
    Sound[2].file = 260; // Missing recording.
    Sound[3].file = 261;
    cache.sizes[261] = 500; // Available recording or captured audio.
    Sound[4].file = FIRST_LIVE_SAMPLING_FILE;
    assert(manager.Build_presets_snapshot(1, 1.0f, presets, mask));
    assert(mask == ((1u << 0) | (1u << 3)));
    for (int i = 0; i < INSTRUMENTS; ++i)
    {
        const bool available = i == 0 || i == 3 || i == 4;
        assert(presets[i].active == available);
        assert(manager.builds[i] == (available ? 1 : 0));
        assert(Patch[1].Instrument[i].used == (i < 5));
        assert(Patch[1].Instrument[i].sound_id == i);
        assert(Sound[i].used && Sound[i].A == 17 && Sound[i].B == 450);
    }
    assert(Sound[1].file == 11 && Sound[2].file == 260);
    cache.sizes[11] = 500;
    cache.sizes[260] = 500;
    assert(manager.Build_presets_snapshot(1, 1.0f, presets, mask));
    assert(presets[1].active && presets[2].active && mask == 15);
    cache = {};
    Sound[4].file = 14;
    assert(manager.Build_presets_snapshot(1, 1.0f, presets, mask));
    assert(mask == 0);
    for (const auto &preset : presets)
    {
        assert(!preset.active);
    }
    std::puts("PASS: mixed missing audio, inactive presets, table exclusion, metadata preservation, restored files, live sources and all-missing patch");
}
'''
compiler = shutil.which('g++') or r'C:\msys64\ucrt64\bin\g++.exe'
with tempfile.TemporaryDirectory(prefix='lilla-missing-audio-') as directory:
    cpp = Path(directory) / 'missing_audio.cpp'
    exe = Path(directory) / 'missing_audio.exe'
    cpp.write_text(fixture + method + checks, encoding='utf-8', newline='\r\n')
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', str(cpp), '-o', str(exe)], check=True)
    environment = os.environ.copy()
    environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
    subprocess.run([str(exe)], check=True, env=environment)
