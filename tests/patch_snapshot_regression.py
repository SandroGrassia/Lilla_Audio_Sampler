"""Exercise production sparse rollback, allocation failures and nested snapshots."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/main.cpp').read_text(encoding='utf-8')
snapshot = re.search(r'^struct PatchEditSnapshot\n\{.*?^\};', source, re.M | re.S).group(0)

def method(signature):
    return re.search(r'^' + re.escape(signature) + r'\n\{.*?^\}', source, re.M | re.S).group(0)

prefix = r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <new>
#include <iostream>
constexpr int SOUNDS_MAX = 800, INSTRUMENTS = 8, IRQ_SOFTWARE = 0;
constexpr int PLAYBACK_FILES = 8;
bool Playback_active = false;
int Playback_partner[8] = {-1,-1,-1,-1,-1,-1,-1,-1};
bool irq_enabled = true, audio_tables_error_pending = false;
int allocations_until_failure = -1, live_allocations = 0, map_updates = 0;
void *operator new[](std::size_t size, const std::nothrow_t &) noexcept
{
    if (allocations_until_failure == 0)
    {
        return nullptr;
    }
    if (allocations_until_failure > 0)
    {
        --allocations_until_failure;
    }
    void *memory = std::malloc(size);
    if (memory != nullptr)
    {
        ++live_allocations;
    }
    return memory;
}
void operator delete[](void *memory) noexcept
{
    if (memory != nullptr)
    {
        --live_allocations;
        std::free(memory);
    }
}
void operator delete[](void *memory, std::size_t) noexcept
{
    operator delete[](memory);
}
bool NVIC_IS_ENABLED(int) { return irq_enabled; }
void AudioNoInterrupts() { irq_enabled = false; }
void AudioInterrupts() { irq_enabled = true; }
void P_Update_all_maps_Instrument_for_notes() { ++map_updates; }
struct Sound_struct { int value = 0; bool used = false; int gain = 0; };
struct Instrument { bool used = false; int sound_id = 0; };
struct Patch_struct { ::Instrument Instrument[INSTRUMENTS]; int instruments = 0; bool used = false; };
Sound_struct Sound[SOUNDS_MAX + 2 + PLAYBACK_FILES], S_Sound_cache_P[SOUNDS_MAX];
Patch_struct Patch[3];
int Patch_id = 0;
int Get_sound_id(int patch_id, int instrument_id) { return Patch[patch_id].Instrument[instrument_id].sound_id; }
int S_Get_sound_free()
{
    for (int id = 0; id < SOUNDS_MAX; ++id)
    {
        if (!Sound[id].used)
        {
            return id;
        }
    }
    return -1;
}
struct ArchiveStub
{
    int Read_Sound(int id) { Sound[id].value = -id - 1; return 0; }
    int Save_Patch(int) { return 0; }
    int Save_Delay(int, int) { return 0; }
    void Copy_Patch_from_RAM_to_SD(int) {}
} Archive;
void Require_FRAM(int result) { assert(result == 0); }
using Delay_data_struct = int;
int Delay_data = 1;
Patch_struct Patch_cache_P;
bool tables_succeed = true;
int persisted_sounds = 0;
struct ManagerStub
{
    void Release_softly_all_players(int) {}
} Players_Manager;
struct StatisticsStub
{
    void Reset_total_Players_per_instrument() {}
} Players_statistics;
int S_Get_Patch_id_free() { return 1; }
bool S_Fill_all_tables() { return tables_succeed; }
void S_Save_all_Sounds_changed() { ++persisted_sounds; }
void P_Update_Patches_number() {}
void S_Copy_all_Sound_to_Sound_cache_P() { memcpy(S_Sound_cache_P, Sound, sizeof(S_Sound_cache_P)); }
void P_Update_line_of_all_instruments() {}
void Golive_with_PERFORMANCE(int) {}
void Print_Patch(int) {}
'''

production = '\n'.join([snapshot, method('bool S_Pull_all_Sound_from_Sound_cache_P(PatchEditSnapshot *snapshot)'), method('bool S_Read_all_Sounds(PatchEditSnapshot *snapshot)'), method('bool S_Clone_Instrument(const int instrument_id, int &new_instrument, PatchEditSnapshot &snapshot)'), method('bool P_Save_current_patch_as_new(void)')])

checks = r'''
void reset()
{
    assert(live_allocations == 0);
    allocations_until_failure = -1;
    irq_enabled = true;
    audio_tables_error_pending = false;
    tables_succeed = true;
    persisted_sounds = 0;
    Patch_id = 0;
    for (auto &patch : Patch)
    {
        patch = {};
    }
    for (int id = 0; id < SOUNDS_MAX + 2; ++id)
    {
        Sound[id] = {id + 10, false, 4};
        if (id < SOUNDS_MAX)
        {
            S_Sound_cache_P[id] = Sound[id];
        }
    }
    Sound[3].used = true;
    S_Sound_cache_P[3] = Sound[3];
    Patch[0].Instrument[0] = {true, 3};
    Patch[0].Instrument[1] = {true, 3};
    Patch[0].instruments = 2;
    Patch[0].used = true;
    Patch_cache_P = Patch[0];
}
int main()
{
    reset();
    {
        PatchEditSnapshot snapshot;
        assert(snapshot.valid && snapshot.count == 1 && snapshot.capacity == INSTRUMENTS);
        assert(irq_enabled);
        Sound[3].value = 900;
        assert(snapshot.Capture_sound(3));
        assert(snapshot.Find_sound(3)->value == 13);
        Sound[700].value = 777;
        assert(S_Pull_all_Sound_from_Sound_cache_P(&snapshot));
        assert(snapshot.count == 2);
        Sound[600].value = 888;
        Patch_id = 1;
        snapshot.Restore();
        assert(Patch_id == 0 && Sound[3].value == 13 && Sound[700].value == 777);
        assert(Sound[600].value == 888);
    }
    reset();
    {
        PatchEditSnapshot outer;
        Sound[3].value = 25;
        {
            PatchEditSnapshot inner;
            Sound[3].value = 26;
            inner.Restore();
            assert(Sound[3].value == 25);
        }
        outer.Restore();
        assert(Sound[3].value == 13);
    }
    reset();
    {
        PatchEditSnapshot snapshot;
        int instrument_id = -1;
        assert(S_Clone_Instrument(0, instrument_id, snapshot));
        const int new_sound = Patch[0].Instrument[instrument_id].sound_id;
        assert(new_sound == 0 && Sound[new_sound].used && Sound[new_sound].gain == 0);
        snapshot.Restore();
        assert(!Sound[0].used && Sound[0].value == 10 && Sound[0].gain == 4);
        assert(!Patch[0].Instrument[instrument_id].used && Patch[0].instruments == 2);
    }
    reset();
    {
        PatchEditSnapshot snapshot;
        assert(snapshot.Capture_sound(SOUNDS_MAX));
        assert(snapshot.Capture_sound(SOUNDS_MAX + 1));
        Sound[SOUNDS_MAX].value = 0;
        Sound[SOUNDS_MAX + 1].value = 0;
        snapshot.Restore();
        assert(Sound[SOUNDS_MAX].value == SOUNDS_MAX + 10);
        assert(Sound[SOUNDS_MAX + 1].value == SOUNDS_MAX + 11);
    }
    reset();
    {
        PatchEditSnapshot snapshot;
        assert(S_Read_all_Sounds(&snapshot));
        assert(snapshot.count == SOUNDS_MAX);
        snapshot.Restore();
        for (int id = 0; id < SOUNDS_MAX + 2; ++id)
        {
            assert(Sound[id].value == id + 10);
        }
    }
    reset();
    {
        irq_enabled = false;
        allocations_until_failure = 0;
        PatchEditSnapshot snapshot;
        assert(!snapshot.valid && audio_tables_error_pending && !irq_enabled);
        snapshot.Restore();
        assert(!irq_enabled && Sound[3].value == 13);
    }
    reset();
    {
        PatchEditSnapshot snapshot;
        allocations_until_failure = 0;
        assert(!S_Read_all_Sounds(&snapshot));
        assert(!snapshot.valid && audio_tables_error_pending && irq_enabled);
        snapshot.Restore();
        for (int id = 0; id < SOUNDS_MAX; ++id)
        {
            assert(Sound[id].value == id + 10);
        }
    }
    reset();
    {
        PatchEditSnapshot snapshot;
        for (int id = 0; id < 30; ++id)
        {
            Sound[id].value += 1000;
        }
        allocations_until_failure = 0;
        assert(!S_Pull_all_Sound_from_Sound_cache_P(&snapshot));
        snapshot.Restore();
        for (int id = 0; id < 30; ++id)
        {
            assert(Sound[id].value == id + 10 + (id == 3 ? 0 : 1000));
        }
    }
    reset();
    {
        Sound[3].value = 999;
        assert(P_Save_current_patch_as_new());
        assert(Patch_id == 1 && persisted_sounds == 1 && irq_enabled);
        assert(Sound[3].value == 13);
        const int first = Patch[1].Instrument[0].sound_id;
        const int second = Patch[1].Instrument[1].sound_id;
        assert(first != second && Sound[first].value == 999 && Sound[second].value == 999);
    }
    reset();
    {
        Sound[3].value = 999;
        Sound[700].value = 777;
        tables_succeed = false;
        assert(!P_Save_current_patch_as_new());
        assert(Patch_id == 0 && !Patch[1].used && persisted_sounds == 0 && irq_enabled);
        assert(Sound[3].value == 999 && Sound[700].value == 777);
        assert(!Sound[0].used && Sound[0].value == 10 && !Sound[1].used && Sound[1].value == 11);
    }
    reset();
    {
        for (int id = 0; id < INSTRUMENTS; ++id)
        {
            Patch[0].Instrument[id] = {true, id};
            Sound[id].used = true;
        }
        Patch[0].instruments = INSTRUMENTS;
        Patch_cache_P = Patch[0];
        S_Copy_all_Sound_to_Sound_cache_P();
        allocations_until_failure = 1;
        assert(!P_Save_current_patch_as_new());
        assert(Patch_id == 0 && !Patch[1].used && !Sound[INSTRUMENTS].used && persisted_sounds == 0);
    }
    reset();
    std::cout << "PASS: sparse rollback, shared Sounds, nesting, clone, sampler channels, full reload and allocation failures\n";
}
'''

compiler = shutil.which('g++') or 'C:/msys64/ucrt64/bin/g++.exe'
environment = os.environ.copy()
environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
with tempfile.TemporaryDirectory(prefix='patch-snapshot-') as folder:
    cpp = Path(folder) / 'test.cpp'
    exe = Path(folder) / 'test.exe'
    cpp.write_text(prefix + production + checks, encoding='utf-8')
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True, env=environment)
    subprocess.run([str(exe)], check=True, env=environment)
