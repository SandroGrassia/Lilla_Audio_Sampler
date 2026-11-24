"""Verify continuous SLIDE edits through the production voice edit and loop scheduler."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile
from production_source import method

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / 'lib/AudioPlayer/AudioPlayer.cpp').read_text(encoding='utf-8')
fixture = r'''
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include "PlayerLoopSlide.h"
constexpr int LOOP_FWD = 2, LOOP_REV = 5, FIRST_RECORDING_FILE = 100;
constexpr int ONCE_FWD = 0, ONCE_REV = 1, LOOP_FWD_REV = 3, LOOP_REV_FWD = 4, FIRST_LIVE_SAMPLING_FILE = 200, AUDIO_BLOCK_SAMPLES = 128;
constexpr int LS_buffer_dim = 32768, LS_XY_delta = 4095, LS_X_sample = 0, LS_Q_sample = 8192, LS_X_delta = 0;
constexpr bool LS_XY_lock = false;
constexpr float MAX_PITCH_LIVE = 35, MAX_PITCH_WAVETABLE = 35;
namespace SdSoundSource { constexpr int PREVIEW = 65535; bool Is_sd(int) { return false; } }
namespace PlayerReadBudget { constexpr int Live_noclick_samples = 32; }
struct SerialStub { template<class T> void print(T) {} template<class T> void println(T) {} void println() {} } Serial;
#define F(x) x
float Mirror(float center, float value) { return 2 * center - value; }
enum Storage { Flash, Psram };
struct AudioFileSource
{
    int file_id = 1;
    Storage storage = Psram;
    int16_t *psram_ptr = nullptr;
    int cache_id = 0;
    uint32_t first_sample = 0, samples = 50000;
};
struct Preset_struct
{
    int file = 1, mode = LOOP_FWD, A = 10000, B = 10999, Noclick = 100, slide = 0;
    bool use_Wavetable = false;
    AudioFileSource source;
};
struct AudioTables { struct Pointers { int16_t *noclick = nullptr, *wavetable = nullptr; uint8_t bank_mask = 2; }; };
void AudioStartUsingSPI() {}
float Sample_pitch_limit(bool cached) { return cached ? 35 : 5; }
float Playback_pitch_limit(bool, bool, bool) { return 5; }
struct AudioPlayer
{
    enum class PlayerState { Idle, Playing };
    PlayerState state = PlayerState::Playing;
    bool warmup_for_play_again_flag = false, restart_flag = false, main_settings_editing_flag = false;
    bool LS_flag = false, use_Wavetable = true, slide_active = false, spi_in_use = false;
    int local_patch = 1, instrument_id = 2, file_id = 1, patch_id_wait = 1, instrument_id_wait = 2, file_id_wait = 1;
    int mode_player = LOOP_FWD, A_Flash_sample = 10000, B_Flash_sample = 10999, delta_Noclick = 100;
    int slide_step = 0, slide_rendered_ahead = -1, edits = 0, starts = 0;
    float a_first_sample = 10350.25f, pitch_limit = 35, pitch = 1;
    int16_t *Noclick_ptr = nullptr, *Wavetable_ptr = nullptr;
    uint8_t tables_bank_mask = 1;
    AudioFileSource source_now;
    struct Raw { void fast_open(int) {} } rawfile;
    PlayerLoopSlide loop_slide;
    int slide_step_E = 0, mode_player_E = LOOP_FWD, A_Flash_sample_E = 10000, B_Flash_sample_E = 10999, delta_Noclick_E = 100;
    int Flash_first_RAM_sample = 10100, Flash_first_RAM_sample_E = 10100, Wavetable_length = 900, Wavetable_length_E = 900;
    int C_Flash_sample = 11798, C_Flash_sample_E = 11798, B_Flash_sample_shifted = 10899, B_Flash_sample_shifted_E = 10899;
    bool use_Wavetable_E = false, snubber_flag = false, mix_flag = false, live_restart_pending = false;
    float pitch_limit_E = 35, a_first_sample_E = 0, a_sample = 0, b_sample = 0, initial_index_offset = 0;
    float volume_gain = 1, velocity_gain = 1, ADSR_gain = .5f, pitch_based_gain_correction = 1;
    float raw_first_value_cache[AUDIO_BLOCK_SAMPLES] = {};
    int16_t *Noclick_E_ptr = nullptr, *Wavetable_E_ptr = nullptr;
    uint8_t tables_bank_mask_E = 0;
    int mix_samples = 32, harvested = 0;
    void Set_source(const AudioFileSource &) {}
    template<class... Args> void Main_settings(Args...) { ++starts; }
    template<class... Args> void Main_settings_editing(Args... args) { ++edits; Prepare_edit(args...); }
    void Prepare_edit(uint8_t mode_in, int A_value_in, int B_value_in, uint16_t delta_Noclick_in, bool use_Wavetable_in, int16_t *p_Noclick_in, int16_t *p_Wavetable_in, uint8_t tables_bank_mask_in, int16_t slide_in);
    void Apply_queued_edit();
    void Harvest_samples(int count) { harvested += count; loop_slide.Queue_step(slide_step / pitch); loop_slide.Advance(count * pitch); }
    float Read_rendered_sample(int, float) { return 123; }
    void Update_pitch() {}
    bool Apply_preset_edit(int patch, int instrument, const Preset_struct &preset, const AudioTables::Pointers &tables);
    void Reset_loop_slide();
};
'''
fixture += method(source, 'bool AudioPlayer::Apply_preset_edit(int patch, int instrument, const Preset_struct &preset, const AudioTables::Pointers &tables)')
fixture += method(source, 'void AudioPlayer::Reset_loop_slide(void)')
prepare = method(source, 'void AudioPlayer::Main_settings_editing(uint8_t mode_in, int A_value_in, int B_value_in, uint16_t delta_Noclick_in, bool use_Wavetable_in, int16_t *p_Noclick_in, int16_t *p_Wavetable_in, uint8_t tables_bank_mask_in, int16_t slide_in)')
fixture += prepare.replace('AudioPlayer::Main_settings_editing', 'AudioPlayer::Prepare_edit')
edit_start = source.index('        if (main_settings_editing_flag)', source.index('void AudioPlayer::update(void)'))
edit_end = source.index('        // Harvest for samples', edit_start)
fixture += 'void AudioPlayer::Apply_queued_edit()\n{\nfloat a_first_sample_real = 0, F_basket_sample = 0;\n' + source[edit_start:edit_end] + '}\n'
fixture += r'''
void Check_voice(bool reverse)
{
    AudioPlayer voice;
    Preset_struct preset;
    AudioTables::Pointers tables;
    voice.mode_player = preset.mode = reverse ? LOOP_REV : LOOP_FWD;
    voice.a_first_sample = (reverse ? 10899 : 10100) + 250.25f;
    const auto source = [reverse](int window, int phase) { return window + (reverse ? 899 - phase : 100 + phase); };
    const auto head = [reverse](int window) { return window + (reverse ? 999 : 0); };
    preset.slide = 25;
    assert(!voice.Apply_preset_edit(1, 2, preset, tables));
    assert(voice.slide_active && !voice.use_Wavetable && voice.edits == 0);
    assert(voice.tables_bank_mask == 2);
    assert(voice.loop_slide.Phase() == 250.25f && voice.loop_slide.Describe(0).source == source(10000, 250));
    voice.loop_slide.Advance(549.75f); // First junction: current tail to A + 25.
    assert(voice.loop_slide.Describe(0).head == head(10025));
    voice.loop_slide.Commit_lookahead(19);
    voice.loop_slide.Advance(20);
    const auto in_progress = voice.loop_slide.Describe(0);
    preset.slide = -10;
    assert(!voice.Apply_preset_edit(1, 2, preset, tables));
    assert(voice.loop_slide.Phase() == 820);
    assert(voice.loop_slide.Describe(0).head == in_progress.head);
    voice.loop_slide.Advance(80);
    assert(voice.loop_slide.Describe(0).source == source(10025, 0));
    voice.loop_slide.Advance(800);
    assert(voice.loop_slide.Describe(0).head == head(10015)); // Reverse motion from reached 10025, not from initial 10000.
    voice.loop_slide.Commit_lookahead(99);
    voice.loop_slide.Advance(100);
    preset.slide = 0;
    preset.use_Wavetable = true;
    assert(!voice.Apply_preset_edit(1, 2, preset, tables));
    assert(voice.slide_active && !voice.use_Wavetable && voice.slide_step == 0);
    voice.loop_slide.Advance(900 * 4);
    assert(voice.loop_slide.Describe(0).source == source(10015, 0)); // Zero freezes the reached window.
    preset.slide = 30;
    assert(!voice.Apply_preset_edit(1, 2, preset, tables));
    voice.loop_slide.Advance(900);
    assert(voice.loop_slide.Describe(0).source == source(10045, 0));
    voice.warmup_for_play_again_flag = true;
    preset.slide = -5;
    assert(!voice.Apply_preset_edit(1, 2, preset, tables));
    assert(voice.starts == 1 && voice.edits == 0); // Pending Note On gets new settings; current voice is not restarted.
    voice.Reset_loop_slide(); // Start_playing invokes this reset for the actual incoming note.
    assert(voice.loop_slide.Phase() == 0 && voice.loop_slide.Describe(0).source == source(10000, 0));

    voice.warmup_for_play_again_flag = false;
    preset.mode = reverse ? LOOP_FWD : LOOP_REV;
    assert(voice.Apply_preset_edit(1, 2, preset, tables)); // Changing playback direction uses the geometry crossmix.
    assert(voice.edits == 1);

    // Activating SLIDE must retain a junction already prefetched by the stationary reader.
    AudioPlayer prefetched;
    prefetched.mode_player = preset.mode = reverse ? LOOP_REV : LOOP_FWD;
    prefetched.a_first_sample = (reverse ? 10899 : 10100) + 797.5f;
    prefetched.slide_rendered_ahead = 4;
    preset.slide = 25;
    assert(!prefetched.Apply_preset_edit(1, 2, preset, tables));
    assert(prefetched.loop_slide.Describe(3).head == head(10000));
    assert(prefetched.loop_slide.Describe(903).head == head(10025));

    AudioPlayer pitched;
    pitched.mode_player = preset.mode = reverse ? LOOP_REV : LOOP_FWD;
    pitched.a_first_sample = reverse ? 10899 : 10100;
    pitched.pitch = 2;
    preset.slide = 20;
    assert(!pitched.Apply_preset_edit(1, 2, preset, tables));
    pitched.loop_slide.Advance(900);
    assert(pitched.loop_slide.Describe(0).source == source(10010, 0));
    pitched.pitch = 4;
    assert(!pitched.Apply_preset_edit(1, 2, preset, tables));
    pitched.loop_slide.Advance(900);
    assert(pitched.loop_slide.Describe(0).source == source(10015, 0));
}

void Check_trim(bool reverse)
{
    for (int tail : {0, 32, 128})
    {
        for (int step : {-20, 0, 20})
        {
            for (int change = 0; change < 4; ++change)
            {
                for (float phase : {250.25f, 850.25f})
                {
                    AudioPlayer voice;
                    Preset_struct preset;
                    AudioTables::Pointers tables;
                    voice.mode_player = preset.mode = reverse ? LOOP_REV : LOOP_FWD;
                    voice.use_Wavetable = false;
                    voice.slide_active = true;
                    voice.slide_step = preset.slide = step;
                    voice.pitch = 2.5f;
                    voice.mix_samples = tail;
                    voice.loop_slide.Reset(10000, 10999, 100, 8, 0, 49999, reverse);
                    voice.loop_slide.Advance(9000 + phase); // Ten cycles have already moved the window by 80 samples.
                    voice.a_first_sample = (reverse ? 10899 : 10100) + phase;
                    const auto before = voice.loop_slide.Get_position();
                    const int from_delta = change == 0 || change == 2 ? 10 : 0;
                    preset.A += from_delta;
                    preset.B += change == 1 || change == 2 ? 10 : 0;
                    preset.Noclick += change == 3 ? 10 : 0;
                    preset.use_Wavetable = step == 0;
                    assert(voice.Apply_preset_edit(1, 2, preset, tables));
                    assert(voice.main_settings_editing_flag && !voice.use_Wavetable_E);
                    voice.Apply_queued_edit();
                    const auto after = voice.loop_slide.Get_position();
                    assert(after.source == before.source && after.fraction == before.fraction);
                    assert(after.first == before.first + static_cast<int64_t>(from_delta) * (1LL << 24));
                    assert(voice.slide_active && !voice.use_Wavetable && !voice.main_settings_editing_flag);
                    assert(voice.starts == 0 && voice.harvested == tail && voice.mix_flag == (tail != 0));
                    const int length = preset.B - preset.A + 1, period = length - preset.Noclick;
                    voice.loop_slide.Advance(period - voice.loop_slide.Phase());
                    const int first = 10080 + from_delta + static_cast<int>(step / voice.pitch);
                    assert(voice.loop_slide.Describe(0).source == first + (reverse ? length - 1 - preset.Noclick : preset.Noclick));
                    if (step == 0)
                    {
                        voice.loop_slide.Advance(period * 5);
                        assert(voice.loop_slide.Describe(0).source == first + (reverse ? length - 1 - preset.Noclick : preset.Noclick));
                    }
                    else
                    {
                        voice.Reset_loop_slide(); // Only a new note returns to the edited preset's original window.
                        assert(voice.loop_slide.Phase() == 0);
                        assert(voice.loop_slide.Describe(0).source == (reverse ? preset.B - preset.Noclick : preset.A + preset.Noclick));
                    }
                }
            }
        }
    }

    // Repeated trims preserve fractional window motion instead of rounding it away at every edit.
    PlayerLoopSlide loop;
    loop.Reset(10000, 10999, 100, .25f, 0, 49999, reverse);
    loop.Advance(900 + 250.25f);
    const auto before = loop.Get_position();
    loop.Reframe(before, 10, 990, 100, .25f);
    loop.Advance(890 - loop.Phase());
    assert(loop.Get_position().first == before.first + static_cast<int64_t>(10.25 * (1LL << 24)));

    // Expansion at the file edge keeps the entire resized window inside the source.
    loop.Reset(49000, 49999, 100, 0, 0, 49999, reverse);
    loop.Advance(250.25f);
    const auto edge = loop.Get_position();
    loop.Reframe(edge, 0, 1200, 100, 0);
    assert(loop.Get_position().source == edge.source);
    assert(loop.Get_position().first == static_cast<int64_t>(48800) * (1LL << 24));

    // If trimming removes the current sample, land at the nearest valid point of the reached window.
    loop.Reset(10080, 11079, 100, 0, 0, 49999, reverse);
    loop.Advance(250.25f);
    const auto excluded = loop.Get_position();
    loop.Reframe(excluded, reverse ? 0 : 600, 400, 100, 0);
    assert(loop.Phase() == 0);
    assert(loop.Get_position().source == (reverse ? 10379 : 10780));
}

int main()
{
    Check_voice(false);
    Check_voice(true);
    Check_trim(false);
    Check_trim(true);

    // A queued change can be replaced before its first endpoint has been rendered.
    PlayerLoopSlide loop;
    loop.Reset(100, 199, 20, 10, 0, 999);
    loop.Advance(30.5f);
    loop.Queue_step(20);
    loop.Queue_step(-15);
    assert(loop.Phase() == 30.5f);
    loop.Advance(29.5f);
    assert(loop.Describe(0).head == 85);
    loop.Commit_lookahead(4);
    loop.Queue_step(0);
    assert(loop.Describe(0).head == 85);
    loop.Advance(80);
    assert(loop.Describe(0).head == 85);

    // Preserve destinations that interpolation has read before the audible cursor reaches them.
    loop.Reset(100, 199, 20, 10, 0, 999);
    loop.Advance(57);
    loop.Commit_lookahead(4);
    loop.Queue_step(30);
    assert(loop.Describe(3).head == 110);
    assert(loop.Describe(83).head == 140);

    // At either file edge, reversing starts immediately from the clamped window; no accumulated overshoot.
    for (int direction : {-1, 1})
    {
        loop.Reset(400, 499, 20, direction * 200, 0, 999);
        loop.Advance(80 * 100);
        loop.Queue_step(-direction * 7);
        loop.Advance(80);
        assert(loop.Describe(0).source == (direction > 0 ? 893 : 7) + 20);
    }

    // Rapid alternating edits exercise bounded history, short periods, fractional pitch and lookahead.
    for (int overlap : {0, 2, 3})
    {
        const int span = 6, period = span - overlap;
        loop.Reset(400, 405, overlap, 0, 0, 999);
        int positions[2000] = {};
        std::fill(positions, positions + 2000, 400);
        int64_t cursor = 0, rendered = -1;
        float fraction = 0;
        for (int block = 0; block < 500; ++block)
        {
            const int step = block % 2 == 0 ? 1 : -1;
            loop.Queue_step(step);
            const int64_t frontier = std::max(rendered, cursor - (fraction == 0 ? 1 : 0));
            const int target = std::max<int64_t>(1, frontier + overlap < 0 ? 0 : (frontier + overlap) / period + 1);
            for (int cycle = target; cycle < 2000; ++cycle)
            {
                positions[cycle] = std::clamp(positions[cycle - 1] + step, 0, 994);
            }
            const int left = loop.History(3);
            assert(left == 3); // All context needed at minimum pitch survives frequent edits.
            for (int offset = -left; offset < 7; ++offset)
            {
                const int64_t sample = cursor + offset;
                const int cycle = static_cast<int>(std::floor(static_cast<double>(sample) / period));
                const int phase = sample - cycle * period;
                const auto segment = loop.Describe(offset);
                const int first = positions[std::max(0, cycle)];
                assert(segment.source == first + overlap + phase);
                if (segment.head >= 0)
                {
                    assert(segment.head == positions[std::max(0, cycle + 1)] + phase - (span - 2 * overlap));
                }
            }
            loop.Commit_lookahead(6);
            rendered = std::max(rendered, cursor + 6);
            loop.Advance(1.28f);
            const float next = fraction + 1.28f;
            cursor += static_cast<int>(next);
            fraction = next - static_cast<int>(next);
        }
        assert(loop.History(484) >= 3); // Sudden high-pitch filtering requests only retained source history.
    }
    puts("PASS: continuous SLIDE edits, frozen windows, reversal, queued notes, interpolation lookahead and rapid edits");
}
'''
compiler = shutil.which('g++') or 'C:/msys64/ucrt64/bin/g++.exe'
with tempfile.TemporaryDirectory() as folder:
    cpp = Path(folder) / 'slide_edit.cpp'
    exe = Path(folder) / 'slide_edit.exe'
    cpp.write_text(fixture, encoding='utf-8')
    environment = os.environ.copy()
    environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
    subprocess.run([compiler, '-std=c++20', '-O2', '-Wall', '-Wextra', '-Werror', '-I' + str(ROOT / 'lib/AudioPlayer'), str(cpp), str(ROOT / 'lib/AudioPlayer/PlayerLoopSlide.cpp'), '-o', str(exe)], check=True, env=environment)
    subprocess.run([str(exe)], check=True, env=environment)
