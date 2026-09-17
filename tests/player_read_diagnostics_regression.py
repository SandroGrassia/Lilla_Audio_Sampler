"""Exercise production read paths and diagnostic aggregation with host memory/file stubs."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
player = (root / 'lib/AudioPlayer/AudioPlayer.cpp').read_text(encoding='utf-8')
manager = (root / 'lib/PlayersManager/PlayersManager.cpp').read_text(encoding='utf-8')
header = (root / 'lib/PlayersManager/PlayersManager.h').read_text(encoding='utf-8')

def method(source, signature):
    return re.search(r'^' + re.escape(signature) + r'[^\n]*\n\{.*?^\}', source, re.M | re.S).group(0) + '\n'

prefix = r"""
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <vector>
#include "PlayerReadDiagnostics.h"
#include "PlayerReadBudget.h"
#define F(value) value
using byte = uint8_t;
constexpr uint8_t LOOP_FWD = 2, LOOP_FWD_REV = 3, LOOP_REV = 5;
constexpr int PLAYERS = 3, PACKET_DIM = 65536, LS_buffer_dim = 8;
constexpr uint32_t F_CPU_ACTUAL = 600000000;
uint32_t audio_update_cycle = 1, ARM_DWT_CYCCNT = 0, ARM_DEMCR = 0, ARM_DWT_CTRL = 0;
constexpr uint32_t ARM_DEMCR_TRCENA = 1, ARM_DWT_CTRL_CYCCNTENA = 1;
struct SerialStub
{
    template<class T> void print(T) {}
    template<class T> void println(T) {}
    void println() {}
} Serial;
const char *name_packet[4] = {};
namespace AudioTimingMonitor
{
enum Category { Flash, Psram, Zero, Players, Controls };
enum Event { Visited, ActivePlayer, Rendered, Tail, AllocationFailure, DeadlineStop, BudgetRetirement };
struct Scope { explicit Scope(Category) {} };
inline void Event_count(Event) {}
}
enum Storage { Flash, Psram };
struct Source { Storage storage = Flash; uint32_t samples = 0; const int16_t *psram_ptr = nullptr; };
struct File
{
    std::vector<byte> data = std::vector<byte>(131072);
    uint32_t base = 0, offset = 0;
    void seek(uint32_t value) { offset = value; }
    void close() {}
    void packet_fast_open(int value) { base = value * PACKET_DIM; offset = 0; }
    void read(void *destination, int count)
    {
        assert(base + offset + count <= data.size());
        memcpy(destination, data.data() + base + offset, count);
        offset += count;
        ARM_DWT_CYCCNT += count;
    }
};
class AudioPlayer
{
public:
    inline static bool read_diagnostics_enabled = true;
    PlayerReadDiagnostics read_diagnostics;
    static void Enable_read_diagnostics(bool enabled) { read_diagnostics_enabled = enabled; }
    Source source_now;
    File rawfile;
    bool recording_flag = false, stereo_flag = false, LS_flag = false, use_Wavetable = false, warmup_for_play_again_flag = false;
    int packet_delta = 0, first_packet = 0, local_timer = 0, state = 0;
    static constexpr int IDLE_REQUEST = 3;
    int16_t *FIFO = nullptr, *Wavetable_ptr = nullptr;
    int16_t samples_basket[5000] = {};
    float a_sample = 0, b_sample = 0, initial_index_offset = 0, a_first_sample = 0, pitch = 1;
    int Flash_first_RAM_sample = 0, Wavetable_length = 8, mode_player = 0;
    void Close_source() {}
    void Flash_memory_harvest() { Read_samples(samples_basket, 0, 32); }
    void Record_read(PlayerReadSource, int, uint16_t = 0);
    void Read_samples(int16_t *, int, int);
    int Loop_period(int, int, int, uint8_t) const;
    bool Fill_loop_samples(int16_t *, int, int, int, int, int, uint8_t, const int16_t *);
    void Wavetable_harvest();
    void Harvest_samples();
    const PlayerReadDiagnostics &Get_read_diagnostics() const { return read_diagnostics; }
};
"""
budget_type = re.search(r'    struct ReadBudgetDiagnostics[^\n]*\n    \{.*?\n    \};', header, re.S).group(0)
snapshot = re.search(r'    struct ReadDiagnosticsSnapshot\n    \{.*?\n    \};', header, re.S).group(0)
manager_stub = 'class PlayersManager\n{\npublic:\n' + budget_type + '\n' + snapshot + r"""
    using ReadSource = PlayerReadSource;
    ReadBudgetDiagnostics read_budget_diagnostics; // Current scheduling evidence for snapshot-coherence tests.
    bool read_diagnostics_enabled = true;
    AudioPlayer *Player_ptr;
    ReadDiagnosticsSnapshot read_diagnostics_last, read_diagnostics_peak, read_diagnostics_restart, read_diagnostics_gap;
    explicit PlayersManager(AudioPlayer *players) : Player_ptr(players) {}
    static bool Get_read_time_us(ReadSource, uint32_t, float &);
    static bool Get_read_usage_time_us(ReadSource, const PlayerReadUsage &, float &);
    void Enable_read_diagnostics(bool);
    void Collect_read_diagnostics();
    bool Copy_read_diagnostics(ReadDiagnosticsSnapshot &, ReadDiagnosticsSnapshot &, ReadDiagnosticsSnapshot &, ReadDiagnosticsSnapshot &) const;
};
"""
methods = ''.join(method(player, signature) for signature in ['void AudioPlayer::Record_read(', 'void AudioPlayer::Read_samples(', 'int AudioPlayer::Loop_period(', 'bool AudioPlayer::Fill_loop_samples(', 'void AudioPlayer::Wavetable_harvest(', 'void AudioPlayer::Harvest_samples('])
methods += ''.join(method(manager, signature) for signature in ['constexpr float Flash_read_time_us(', 'constexpr float Psram_read_time_us(', 'constexpr float Ram_read_time_us(', 'bool PlayersManager::Get_read_time_us(', 'bool PlayersManager::Get_read_usage_time_us(', 'void PlayersManager::Enable_read_diagnostics(', 'void PlayersManager::Collect_read_diagnostics(', 'bool PlayersManager::Copy_read_diagnostics('])
tests = r"""
void near(float a, float b) { assert(std::fabs(a - b) < 0.02f); }
int main()
{
    using S = PlayerReadSource;
    PlayerReadUsage sum;
    sum.Add(0); sum.Add(-1); assert(sum.operations == 0);
    sum.Add(1); sum.Add(9); sum.Add(100); sum.Add(4096); sum.Add(4096);
    assert(sum.samples == 8302 && sum.model_samples == 8312 && sum.operations == 5);
    float actual, one, nine, hundred, large;
    for (S source : {S::Flash, S::Psram, S::Ram})
    {
        assert(PlayersManager::Get_read_time_us(source, 1, one));
        assert(PlayersManager::Get_read_time_us(source, 9, nine));
        assert(PlayersManager::Get_read_time_us(source, 100, hundred));
        assert(PlayersManager::Get_read_time_us(source, 4096, large));
        assert(PlayersManager::Get_read_usage_time_us(source, sum, actual));
        near(actual, one + nine + hundred + large * 2);
    }
    sum.Add(4501); assert(!PlayersManager::Get_read_usage_time_us(S::Flash, sum, actual) && std::isinf(actual));
    assert(!PlayersManager::Get_read_time_us(static_cast<S>(99), 10, actual));
    assert(PlayersManager::Get_read_time_us(S::Ram, 0, actual) && actual == 0);
    AudioPlayer p;
    int16_t source[8] = {10,11,12,13,14,15,16,17}, output[32] = {};
    p.source_now = {Psram, 8, source};
    p.Read_samples(output, -2, 6);
    assert(output[0] == 0 && output[1] == 0 && output[2] == 10 && output[5] == 13);
    assert(p.read_diagnostics.sources[1].operations == 1 && p.read_diagnostics.sources[1].samples == 6);
    assert(p.read_diagnostics.flags & PlayerReadDiagnostics::PaddedRead);
    p.Read_samples(output, 6, 4); assert(output[0] == 16 && output[1] == 17 && output[2] == 0);
    p.read_diagnostics = {}; p.source_now.storage = Flash; p.LS_flag = true; p.FIFO = source;
    p.Read_samples(output, 6, 4);
    assert(output[0] == 16 && output[1] == 17 && output[2] == 10 && output[3] == 11);
    assert(p.read_diagnostics.sources[1].operations == 2 && p.read_diagnostics.sources[1].model_samples == 20);
    assert(p.read_diagnostics.flags & PlayerReadDiagnostics::LiveCopyProxy);
    p.read_diagnostics = {}; p.LS_flag = false; p.recording_flag = true;
    p.Read_samples(output, 32767, 4);
    assert(p.read_diagnostics.sources[0].operations == 2 && p.read_diagnostics.sources[0].samples == 4);
    assert(p.read_diagnostics.flags & PlayerReadDiagnostics::PacketOpen);
    p.read_diagnostics = {}; p.recording_flag = false; p.source_now = {Psram, 8, source};
    int16_t noclick[2] = {90,91};
    assert(p.Fill_loop_samples(output, 12, 0, 0, 7, 2, LOOP_FWD, noclick));
    assert(output[0] == 12 && output[4] == 90 && output[5] == 91 && output[6] == 12 && output[11] == 91);
    assert(p.read_diagnostics.sources[1].operations == 2 && p.read_diagnostics.sources[1].samples == 8);
    assert(p.read_diagnostics.sources[2].operations == 2 && p.read_diagnostics.sources[2].samples == 4);
    p.read_diagnostics = {};
    assert(p.Fill_loop_samples(output, 5, 7, 0, 7, 0, LOOP_FWD_REV, nullptr));
    assert(output[0] == 17 && output[1] == 16 && output[4] == 13);
    assert(p.read_diagnostics.sources[1].operations == 2);
    // A full 35x block crosses many short loop periods; guard both ends of the destination.
    int16_t guarded[4502];
    std::fill(std::begin(guarded), std::end(guarded), -1234);
    for (const uint8_t mode : {LOOP_FWD, LOOP_FWD_REV, LOOP_REV})
    {
        p.source_now = {Psram, 8, source};
        p.LS_flag = false;
        assert(p.Fill_loop_samples(guarded + 1, 4447, 0, 0, 7, 0, mode, nullptr));
        for (int i = 0; i < 4447; ++i)
        {
            const int phase = i % (mode == LOOP_FWD_REV ? 14 : 8);
            const int index = mode == LOOP_REV ? 7 - phase : (mode == LOOP_FWD_REV && phase >= 8 ? 14 - phase : phase);
            assert(guarded[i + 1] == source[index]);
        }
        assert(guarded[0] == -1234 && guarded[4448] == -1234);
        p.source_now.storage = Flash; p.LS_flag = true;
        assert(p.Fill_loop_samples(guarded + 1, 4447, 0, 6, 13, 0, mode, nullptr));
        for (int i = 0; i < 4447; ++i)
        {
            const int phase = i % (mode == LOOP_FWD_REV ? 14 : 8);
            const int index = mode == LOOP_REV ? 7 - phase : (mode == LOOP_FWD_REV && phase >= 8 ? 14 - phase : phase);
            assert(guarded[i + 1] == source[(6 + index) % 8]);
        }
        assert(guarded[0] == -1234 && guarded[4448] == -1234);
    }
    p.LS_flag = false; p.source_now = {Psram, 8, source};
    p.Wavetable_ptr = source; p.mode_player = LOOP_FWD; p.a_sample = 0.5f; p.b_sample = 4445.5f; p.pitch = 35;
    p.Wavetable_harvest();
    for (int i = 0; i < 4447; ++i)
    {
        assert(p.samples_basket[i] == source[i % 8]);
    }
    p.mode_player = 0;
    p.read_diagnostics = {}; p.Wavetable_ptr = source; p.a_sample = 6; p.b_sample = 10; p.use_Wavetable = true;
    p.Harvest_samples();
    assert(p.samples_basket[0] == 16 && p.samples_basket[1] == 17 && p.samples_basket[2] == 0);
    assert(p.read_diagnostics.sources[2].samples == 2 && p.read_diagnostics.harvests == 1);
    // A restart/edit has two harvest passes; changing source must not erase the first pass's counters.
    p.read_diagnostics = {}; p.use_Wavetable = false; p.source_now.storage = Flash; p.Harvest_samples();
    p.source_now = {Psram, 8, source}; p.Harvest_samples();
    assert(p.read_diagnostics.harvests == 2 && p.read_diagnostics.sources[0].operations == 1 && p.read_diagnostics.sources[1].operations == 1);
    AudioPlayer voices[PLAYERS];
    PlayersManager manager(voices);
    voices[0].read_diagnostics = p.read_diagnostics; voices[0].read_diagnostics.cycle = audio_update_cycle;
    voices[1].read_diagnostics.sources[2].Add(100); voices[1].read_diagnostics.cycle = audio_update_cycle;
    voices[2].read_diagnostics.sources[0].Add(4501); // Stale block must be excluded.
    manager.read_budget_diagnostics.valid = true; manager.read_budget_diagnostics.cycle = audio_update_cycle;
    manager.read_budget_diagnostics.reserved_us = 1200; manager.read_budget_diagnostics.mix_samples[0] = 32;
    manager.Collect_read_diagnostics();
    auto last = manager.read_diagnostics_last;
    near(last.total_estimated_us, last.estimated_us[0] + last.estimated_us[1]);
    assert(last.budget.valid && last.budget.cycle == last.cycle && last.budget.reserved_us == 1200 && last.budget.mix_samples[0] == 32);
    assert(last.estimated_us[2] == 0 && last.uncovered_operations == 0);
    ++audio_update_cycle;
    for (auto &voice : voices)
    {
        voice.read_diagnostics = {}; voice.read_diagnostics.cycle = audio_update_cycle;
    }
    manager.Collect_read_diagnostics();
    assert(manager.read_diagnostics_last.total_estimated_us == 0 && manager.read_diagnostics_last.blocks == 2);
    near(manager.read_diagnostics_peak.total_estimated_us, last.total_estimated_us);
    assert(!manager.read_diagnostics_last.budget.valid); // A stale scheduler cycle must not leak into the next block.
    assert(manager.read_diagnostics_peak.budget.valid && manager.read_diagnostics_peak.budget.mix_samples[0] == 32); // Peak retains its own allocation.
    voices[0].read_diagnostics.sources[0].Add(4501); manager.Collect_read_diagnostics();
    assert(std::isinf(manager.read_diagnostics_last.total_estimated_us) && manager.read_diagnostics_last.uncovered_operations == 1);
    // Pending edits/restarts alone do not qualify as an executed restart.
    manager.Enable_read_diagnostics(true);
    ++audio_update_cycle;
    for (auto &voice : voices)
    {
        voice.read_diagnostics = {}; voice.read_diagnostics.cycle = audio_update_cycle;
    }
    voices[0].read_diagnostics.flags = PlayerReadDiagnostics::Transition;
    voices[0].read_diagnostics.sources[1].Add(100);
    voices[0].read_diagnostics.harvest_cycles = 12000; // 20 us, exceeding the transfer-only estimate.
    manager.Collect_read_diagnostics();
    assert(manager.read_diagnostics_restart.blocks == 0 && manager.read_diagnostics_gap.blocks != 0);
    const auto first_gap_cycle = manager.read_diagnostics_gap.cycle;
    assert(!manager.read_diagnostics_gap.budget.valid); // Reset did not inherit a scheduling pass from the previous observation window.
    ++audio_update_cycle; voices[0].read_diagnostics.cycle = audio_update_cycle;
    voices[0].read_diagnostics.flags |= PlayerReadDiagnostics::RestartExecuted;
    manager.read_budget_diagnostics.valid = true; manager.read_budget_diagnostics.cycle = audio_update_cycle;
    manager.read_budget_diagnostics.mix_samples[0] = 48; manager.read_budget_diagnostics.pre_players_us = 333;
    voices[0].read_diagnostics.harvest_cycles = 6000; // Smaller positive gap must not replace the earlier maximum.
    manager.Collect_read_diagnostics();
    assert(manager.read_diagnostics_restart.restarted_players == 1 && manager.read_diagnostics_gap.cycle == first_gap_cycle);
    const auto restart_cycle = manager.read_diagnostics_restart.cycle;
    ++audio_update_cycle; voices[0].read_diagnostics.cycle = audio_update_cycle;
    voices[0].read_diagnostics.flags = 0;
    voices[0].read_diagnostics.harvest_cycles = 18000;
    manager.Collect_read_diagnostics();
    assert(manager.read_diagnostics_gap.cycle == audio_update_cycle && manager.read_diagnostics_restart.cycle == restart_cycle);
    const auto largest_gap_cycle = manager.read_diagnostics_gap.cycle;
    ++audio_update_cycle; voices[0].read_diagnostics.cycle = audio_update_cycle;
    voices[0].read_diagnostics.sources[1].Add(4501);
    voices[0].read_diagnostics.harvest_cycles = 6000000;
    manager.Collect_read_diagnostics();
    assert(manager.read_diagnostics_gap.cycle == largest_gap_cycle); // Uncovered/infinite estimates do not qualify.
    PlayersManager::ReadDiagnosticsSnapshot copy_last, copy_peak, copy_restart, copy_gap;
    assert(manager.Copy_read_diagnostics(copy_last, copy_peak, copy_restart, copy_gap));
    assert(copy_restart.cycle == restart_cycle && copy_gap.cycle == largest_gap_cycle);
    assert(copy_restart.budget.cycle == restart_cycle && copy_restart.budget.mix_samples[0] == 48 && copy_restart.budget.pre_players_us == 333);
    assert(!copy_gap.budget.valid); // Historical restart allocation must not be relabelled as the later gap block.
    manager.Enable_read_diagnostics(false);
    assert(!manager.Copy_read_diagnostics(copy_last, copy_peak, copy_restart, copy_gap));
    assert(copy_restart.cycle == restart_cycle && copy_gap.cycle == largest_gap_cycle); // Disable preserves evidence.
    manager.Enable_read_diagnostics(true);
    assert(manager.read_diagnostics_last.blocks == 0 && manager.read_diagnostics_peak.blocks == 0 && manager.read_diagnostics_restart.blocks == 0 && manager.read_diagnostics_gap.blocks == 0);
    manager.read_diagnostics_enabled = false;
    const auto blocks = manager.read_diagnostics_last.blocks;
    manager.Collect_read_diagnostics(); assert(manager.read_diagnostics_last.blocks == blocks);
    AudioPlayer::read_diagnostics_enabled = false;
    p.read_diagnostics = {}; p.Read_samples(output, 0, 4);
    assert(p.read_diagnostics.sources[1].operations == 0 && output[0] == 10);
    std::cout << "Player read diagnostics regression passed\n";
}
"""
# Verify the reset is before all early exits and all three production harvest sites use the wrapper.
update = method(player, 'void AudioPlayer::update(')
assert update.index('read_diagnostics = {};') < update.index('return;')
assert update.count('Harvest_samples();') == 3
assert update.count('PlayerReadDiagnostics::RestartExecuted') == update.count('Start_playing();') == 3
compiler = shutil.which('g++') or r'C:\msys64\ucrt64\bin\g++.exe'
with tempfile.TemporaryDirectory(prefix='player_read_diag_') as folder:
    cpp = Path(folder) / 'test.cpp'
    exe = Path(folder) / 'test.exe'
    cpp.write_text(prefix + manager_stub + methods + tests, encoding='utf-8')
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', '-I', str(root / 'lib/AudioPlayer'), str(cpp), '-o', str(exe)], check=True)
    environment = os.environ.copy()
    environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
    subprocess.run([str(exe)], check=True, env=environment)
