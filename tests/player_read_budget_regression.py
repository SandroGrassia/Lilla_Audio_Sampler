"""Exercise production read forecasting, atomic admission and crossfade budgeting on the host."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
manager = (ROOT / 'lib/PlayersManager/PlayersManager.cpp').read_text(encoding='utf-8')
player = (ROOT / 'lib/AudioPlayer/AudioPlayer.cpp').read_text(encoding='utf-8')


def method(source, signature):
    """Extract the production definition, retaining its actual branches and arithmetic."""
    match = re.search(r'^' + re.escape(signature) + r'[^\n]*\n\{.*?^\}', source, re.M | re.S)
    assert match, signature
    return match.group(0) + '\n'


prefix = r'''
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <random>
#include "PlayerReadBudget.h"
#include "PlaybackProfile.h"
constexpr int PLAYERS = 16, AUDIO_BLOCK_SAMPLES = 128, AUDIO_PLAYER_DEADLINE_US = 2700;
constexpr int LOOP_FWD = 2, LOOP_FWD_REV = 3, LOOP_REV_FWD = 4;
constexpr int FIRST_RECORDING_FILE = 100, FIRST_LIVE_SAMPLING_FILE = 200;
bool Is_playback_file(int id) { return id >= 323 && id < 331; }
constexpr float MIN_PITCH = 0.1f;
int Patch_id = 0, LS_XY_delta = 10000, LS_buffer_dim = 32768;
uint32_t audio_update_time_micros = 0, audio_update_cycle = 1;
enum Storage { Flash, Psram };
struct AudioFileSource { Storage storage = Flash; };
float constrain(float value, float low, float high) { return value < low ? low : (value > high ? high : value); }
struct PresetStub
{
    bool precedence = false, use_Wavetable = false;
    int file = 0, mode = 0, A = 0, B = 9999, Noclick = 0;
    float pitch = 1;
    AudioFileSource source;
} Preset[16];

namespace RealVoice
{
struct AudioPlayer
{
    static constexpr int IDLE = 0;
    int state = 1, file_id = 0, file_id_wait = 0;
    bool use_Wavetable = false, use_Wavetable_wait = false, use_Wavetable_E = false;
    AudioFileSource source_now, source_wait;
    int mode_player = 0, mode_player_wait = 0, mode_player_E = 0;
    int A_Flash_sample = 0, B_Flash_sample = 9999, A_Flash_sample_wait = 0, B_Flash_sample_wait = 9999, A_Flash_sample_E = 0, B_Flash_sample_E = 9999;
    int delta_Noclick = 0, delta_Noclick_wait = 0, delta_Noclick_E = 0;
    float pitch_bend = 1, pitch_vibrato = 1, pitch_based_gain_correction = 1;
    void Update_pitch();
    float pitch = 1; // Already applied pitch can survive until an outgoing crossfade is harvested.
    float pitch_tune = 1, pitch_tune_wait = 1, pitch_note = 1, pitch_note_wait = 1;
    float pitch_limit = 2.8f, pitch_limit_wait = 16, pitch_limit_E = 24;
    bool pitch_tune_flag = false, warmup_for_play_again_flag = false, restart_flag = false, main_settings_editing_flag = false;
    bool idle = false, power_on = true, pending_note_released = false, patch_release_pending = false;
    bool rendered_block_valid = true, budget_tail_pending = false;
    int16_t block[128] = {}, budget_tail_sample = 0;
    float pan_gain_L = .5f, pan_gain_R = .7f, budget_tail_pan_L = 0, budget_tail_pan_R = 0;
    struct audio_block_t { int16_t data[128] = {}; } output[2];
    unsigned allocated = 0, released = 0, transmitted = 0;
    int fail_allocation = -1;
    audio_block_t *allocate() { const unsigned index = allocated++; return static_cast<int>(index) == fail_allocation ? nullptr : &output[index % 2]; }
    void release(audio_block_t *) { ++released; }
    void transmit(audio_block_t *, int) { ++transmitted; }
    void Render_budget_tail();
    unsigned closed = 0;
    void Close_source() { ++closed; }
    void My_LED(bool) {}
    bool Has_pending_note() const { return warmup_for_play_again_flag || restart_flag; }
    PlayerReadBudget::Plan Get_read_plan(bool pending = false, bool edited = false) const;
    float Reserved_read_us() const;
    float Current_read_us(uint32_t) const;
    void Retire_for_read_budget();
};
'''
production = ''.join(method(player, signature) for signature in [
    'PlayerReadBudget::Plan AudioPlayer::Get_read_plan',
    'float AudioPlayer::Reserved_read_us',
    'float AudioPlayer::Current_read_us',
    'void AudioPlayer::Retire_for_read_budget',
    'void AudioPlayer::Update_pitch',
] )
# Execute the real source-free rendering branch with deterministic allocation failures.
update = method(player, 'void AudioPlayer::update')
tail = update[update.index('    if (budget_tail_pending)'):update.index('    if (patch_release_pending')]
production += 'void AudioPlayer::Render_budget_tail()\n{\n' + tail + '}\n}\n'
production += r'''
struct AudioPlayer
{
    float cost = 0, extra_per_sample = 0.5f;
    bool playing = false, powered = true, protection = false, pending = false, editing = false;
    int instrument = 0, patch = 0, mix = 0;
    unsigned timestamp = 0, retired = 0;
    float Reserved_read_us() const { return playing ? cost : 0; }
    float Current_read_us(uint32_t samples) const { return playing ? extra_per_sample * samples : 0; }
    bool isPlaying() const { return playing; }
    bool isPoweredOn() const { return powered; }
    bool Has_pending_note() const { return pending; }
    bool Needs_restart_mix() const { return pending; }
    bool Has_pending_edit() const { return editing; }
    int Read_instrument() const { return instrument; }
    int Assigned_patch() const { return patch; }
    bool Read_precedence() const { return protection; }
    unsigned Read_time_stamp() const { return timestamp; }
    void Retire_for_read_budget() { ++retired; playing = pending = editing = false; }
    void Set_mix_samples(uint8_t samples) { mix = samples; }
    void Set_edit_mix_samples(uint8_t samples) { if (editing) { mix = samples; } }
};
struct PlayersManager
{
    using ReadSource = PlayerReadSource;
    AudioPlayer voices[PLAYERS];
    AudioPlayer *Player_ptr = voices;
    bool Player_booked[PLAYERS] = {}, restart_Player[PLAYERS] = {}, midi_batch_active = false;
    uint8_t restart_mix_first_player = 0, players_to_restart = 0;
    uint32_t budget_rejected_notes = 0, budget_retired_players = 0, budget_forced_protected = 0;
    float budget_reserved_us = 0, budget_crossfade_us = 0;
    PlayerReadBudget::Plan New_read_plan(uint8_t, float) const;
    float Reserved_read_us() const;
    bool Admit_read_budget(int, uint8_t, float);
    void Prepare_read_budget();
    void Calculate_and_set_mix_samples();
    int Get_span_for_all_cross_mix();
    float Get_cross_mix_time(int, int);
};
'''
budget_header = (ROOT / 'lib/PlayersManager/PlayersManager.h').read_text(encoding='utf-8')
budget_type = re.search(r'    struct ReadBudgetDiagnostics[^\n]*\n    \{.*?\n    \};', budget_header, re.S).group(0)
production = production.replace('struct PlayersManager\n{', 'struct PlayersManager\n{\n' + budget_type + '\n    bool read_diagnostics_enabled = true; // Exercise diagnostic capture alongside real scheduling decisions.\n    ReadBudgetDiagnostics read_budget_diagnostics; // Production scheduling evidence.\n')

production += ''.join(method(manager, signature) for signature in [
    'PlayerReadBudget::Plan PlayersManager::New_read_plan',
    'float PlayersManager::Reserved_read_us',
    'bool PlayersManager::Admit_read_budget',
    'void PlayersManager::Prepare_read_budget',
    'void PlayersManager::Calculate_and_set_mix_samples',
    'int PlayersManager::Get_span_for_all_cross_mix',
    'float PlayersManager::Get_cross_mix_time',
])

tests = r'''
int main()
{
    using namespace PlayerReadBudget;
    Plan p;
    p.span = 10000;
    assert(Estimate(p, 0) == 0);
    const float flash = Estimate(p, 128);
    p.source = PlayerReadSource::Psram;
    const float psram = Estimate(p, 128);
    p.source = PlayerReadSource::Ram;
    assert(flash > psram && psram > Estimate(p, 128));
    p.pitch = 40;
    assert(std::isinf(Estimate(p, 128)));
    p.pitch = 1;
    p.source = PlayerReadSource::Psram;
    p.loop = true; p.span = 300; p.crossfade = 100;
    assert(Estimate(p, 128) > psram);
    p.span = 1; p.pingpong = true;
    assert(std::isinf(Estimate(p, 128)));

    // Forecast the maximum modulation and clamp it to the actual source ceiling.
    RealVoice::AudioPlayer real;
    real.pitch_note = 2;
    assert(real.Get_read_plan().pitch == real.pitch_limit);
    real.pitch = 4; assert(real.Get_read_plan().pitch == 4); // Lowering a setting cannot hide the outgoing applied pitch.
    real.pitch = 1;
    real.source_now.storage = Psram; real.pitch_limit = 16;
    assert(std::abs(real.Get_read_plan().pitch - 2 * Maximum_modulation) < .0001f);
    real.source_wait.storage = Psram; real.pitch_note_wait = 8;
    const float old = real.Reserved_read_us();
    real.warmup_for_play_again_flag = true;
    const float pending = Estimate(real.Get_read_plan(true), 128);
    assert(real.Reserved_read_us() == pending && pending > old);
    assert(real.Reserved_read_us() < old + pending); // Sequential restart reserves max, crossfade adds the old segment separately.
    real.main_settings_editing_flag = true; real.pitch_limit_E = 24;
    real.block[127] = 1234;
    real.Retire_for_read_budget();
    assert(real.Reserved_read_us() == 0 && real.closed == 1 && real.budget_tail_pending);
    assert(real.budget_tail_sample == 1234 && real.budget_tail_pan_L == .5f);
    assert(!real.Has_pending_note() && !real.main_settings_editing_flag && !real.power_on);
    real.Retire_for_read_budget(); assert(real.closed == 1);
    real.fail_allocation = 1; real.Render_budget_tail();
    assert(real.budget_tail_pending && real.released == 1 && real.transmitted == 0);
    real.allocated = real.released = 0; real.fail_allocation = -1;
    real.Render_budget_tail();
    assert(!real.budget_tail_pending && real.transmitted == 2 && real.released == 2);
    assert(real.output[0].data[0] == 617 && real.output[0].data[127] == 0);
    assert(real.output[1].data[127] == 0 && real.closed == 1);

    RealVoice::AudioPlayer limit_test;
    for (const float ceiling : {MAX_PITCH_FLASH, MAX_PITCH_PSRAM, MAX_PITCH_WAVETABLE})
    {
        limit_test.pitch_limit = ceiling;
        limit_test.pitch_note = 30;
        limit_test.pitch_tune = 1.2f;
        limit_test.pitch_bend = 1.5f;
        limit_test.pitch_vibrato = 1.01f;
        limit_test.Update_pitch();
        assert(limit_test.pitch == ceiling);
    }
    assert(MAX_PITCH_FLASH == 5 && MAX_PITCH_PSRAM == 35 && MAX_PITCH_WAVETABLE == 35);
    assert(Limit_us == 1800);
    Plan high;
    high.pitch = 35; high.span = 10000;
    high.source = PlayerReadSource::Psram;
    assert(std::isfinite(Estimate(high, 128)));
    high.source = PlayerReadSource::Ram;
    assert(std::isfinite(Estimate(high, 128)));
    high.pitch = 36;
    assert(std::isinf(Estimate(high, 128)));
    PlayersManager forecast;
    Preset[0].source.storage = Psram; Preset[0].pitch = 8;
    assert(forecast.New_read_plan(0, 8).pitch == MAX_PITCH_PSRAM);
    Preset[0].use_Wavetable = true;
    assert(forecast.New_read_plan(0, 8).source == PlayerReadSource::Ram);
    Preset[0].file = FIRST_LIVE_SAMPLING_FILE;
    assert(forecast.New_read_plan(0, 8).source == PlayerReadSource::Psram);
    assert(forecast.New_read_plan(0, 8).pitch == MAX_PITCH_PSRAM);
    Preset[0] = {};

    // Insufficient eligible budget rejects atomically: no partial voice theft.
    PlayersManager denied;
    for (int i = 0; i < 3; ++i)
    {
        denied.voices[i].playing = true; denied.voices[i].cost = 600;
        denied.voices[i].instrument = i == 0 ? 0 : 1;
        denied.voices[i].protection = i != 0;
    }
    assert(!denied.Admit_read_budget(3, 0, 1000));
    assert(denied.budget_rejected_notes == 1 && denied.budget_retired_players == 0);
    for (int i = 0; i < 3; ++i) { assert(denied.voices[i].playing); }
    assert(!denied.Admit_read_budget(3, 0, INFINITY));

    // Multiple victims, preserving unrelated protected and already booked voices.
    PlayersManager multiple;
    for (int i = 0; i < 4; ++i)
    {
        multiple.voices[i].playing = true; multiple.voices[i].cost = 400;
        multiple.voices[i].timestamp = i; multiple.voices[i].powered = false;
    }
    multiple.voices[0].instrument = 1; multiple.voices[0].protection = true;
    multiple.Player_booked[1] = true;
    assert(multiple.Admit_read_budget(4, 0, 1000));
    assert(multiple.voices[0].playing && multiple.voices[1].playing);
    assert(!multiple.voices[2].playing && !multiple.voices[3].playing);
    assert(multiple.budget_retired_players == 2);

    // A cheaper replacement can retire its expensive old source immediately.
    PlayersManager cheaper;
    cheaper.voices[0].playing = cheaper.voices[1].playing = true;
    cheaper.voices[0].cost = 1400; cheaper.voices[1].cost = 500;
    assert(cheaper.Admit_read_budget(0, 0, 200));
    assert(!cheaper.voices[0].playing && cheaper.voices[1].playing);

    // Runtime growth is caught before reads, including edits of protected notes.
    PlayersManager edited;
    for (int i = 0; i < 3; ++i)
    {
        edited.voices[i].playing = true; edited.voices[i].cost = 1000;
        edited.voices[i].protection = i != 0;
    }
    edited.Prepare_read_budget();
    assert(edited.Reserved_read_us() <= Limit_us);
    assert(edited.budget_retired_players == 2 && edited.budget_forced_protected == 1);

    // Share the same spare time between restarts and edits; elapsed control work caps it further.
    PlayersManager fades;
    fades.voices[0].playing = fades.voices[1].playing = true;
    fades.voices[0].cost = fades.voices[1].cost = 875;
    fades.voices[0].pending = true; fades.voices[1].editing = true;
    fades.Prepare_read_budget();
    assert(fades.budget_reserved_us + fades.budget_crossfade_us <= Limit_us);
    assert(fades.voices[0].mix == 64 && fades.voices[1].mix == 0);
    const auto scheduled = fades.read_budget_diagnostics; // Allocation evidence must explain the accepted and rejected tiers.
    assert(scheduled.valid && scheduled.cycle == audio_update_cycle && scheduled.reserved_us == 1750);
    assert(scheduled.read_headroom_us == 50 && scheduled.deadline_headroom_us == 250 && scheduled.available_us == 50);
    assert(scheduled.transition[0] == 1 && scheduled.transition[1] == 2 && scheduled.first_player == 0);
    assert(scheduled.mix_samples[0] == 64 && scheduled.mix_samples[1] == 0);
    assert(std::abs(scheduled.assigned_us[0] - 44.8f) < .001f);
    assert(scheduled.minimum_us[1] > scheduled.available_before_us[1] && scheduled.minimum_us[0] == 0);
    assert(scheduled.crossfade_us == fades.budget_crossfade_us && scheduled.pre_players_us == 0);
    audio_update_time_micros = 1000;
    fades.Prepare_read_budget();
    assert(fades.voices[0].mix == 0 && fades.voices[1].mix == 0);
    assert(fades.read_budget_diagnostics.read_headroom_us == 50 && fades.read_budget_diagnostics.deadline_headroom_us == -750);
    assert(fades.read_budget_diagnostics.available_us == 0 && fades.read_budget_diagnostics.scheduler_elapsed_us == 1000 && fades.read_budget_diagnostics.pre_players_us == 1000);
    assert(fades.read_budget_diagnostics.mix_samples[0] == 0 && fades.read_budget_diagnostics.assigned_us[0] == 0);
    fades.voices[0].playing = fades.voices[1].playing = false;
    fades.voices[0].pending = fades.voices[1].editing = false;
    fades.Prepare_read_budget(); assert(fades.Reserved_read_us() == 0);
    assert(fades.read_budget_diagnostics.transition[0] == 0 && fades.read_budget_diagnostics.minimum_us[1] == 0 && fades.read_budget_diagnostics.crossfade_us == 0);
    audio_update_time_micros = 0;

    // Randomized admission: either commit a fitting set or leave every incumbent intact.
    std::mt19937 rng(20260917);
    for (int iteration = 0; iteration < 10000; ++iteration)
    {
        PlayersManager trial;
        for (int i = 0; i < PLAYERS; ++i)
        {
            auto &v = trial.voices[i];
            v.playing = true; v.cost = rng() % 300;
            v.powered = rng() % 2; v.protection = rng() % 2;
            v.instrument = rng() % 3; v.patch = rng() % 2;
            v.timestamp = rng(); trial.Player_booked[i] = rng() % 5 == 0;
        }
        const float incoming = rng() % 2000;
        const bool accepted = trial.Admit_read_budget(0, 0, incoming);
        if (accepted)
        {
            float total = fmaxf(trial.voices[0].Reserved_read_us(), incoming);
            for (int i = 1; i < PLAYERS; ++i) { total += trial.voices[i].Reserved_read_us(); }
            assert(total <= Limit_us);
        }
        else
        {
            for (const auto &v : trial.voices) { assert(v.playing && v.retired == 0); }
        }
    }
    std::cout << "PASS: read forecasts, retirement, atomic admission, protection, runtime growth and shared crossfade budget (10000 randomized admissions)\n";
}
'''

compiler = shutil.which('g++') or r'C:\msys64\ucrt64\bin\g++.exe'
with tempfile.TemporaryDirectory(prefix='lilla-read-budget-') as directory:
    cpp = Path(directory) / 'budget.cpp'
    exe = Path(directory) / ('budget.exe' if os.name == 'nt' else 'budget')
    cpp.write_text(prefix + production + tests, encoding='utf-8', newline='\r\n')
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', '-I' + str(ROOT / 'lib/AudioPlayer'), '-I' + str(ROOT / 'lib/config'), str(cpp), '-o', str(exe)], check=True)
    environment = os.environ.copy()
    environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
    subprocess.run([str(exe)], check=True, env=environment)
