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
constexpr int LOOP_FWD = 2, FIRST_RECORDING_FILE = 100;
namespace SdSoundSource { constexpr int PREVIEW = 65535; }
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
    float a_first_sample = 10350.25f, pitch_limit = 35;
    int16_t *Noclick_ptr = nullptr, *Wavetable_ptr = nullptr;
    uint8_t tables_bank_mask = 1;
    AudioFileSource source_now;
    struct Raw { void fast_open(int) {} } rawfile;
    PlayerLoopSlide loop_slide;
    void Set_source(const AudioFileSource &) {}
    template<class... Args> void Main_settings(Args...) { ++starts; }
    template<class... Args> void Main_settings_editing(Args...) { ++edits; }
    void Update_pitch() {}
    bool Apply_preset_edit(int patch, int instrument, const Preset_struct &preset, const AudioTables::Pointers &tables);
    void Reset_loop_slide();
};
'''
fixture += method(source, 'bool AudioPlayer::Apply_preset_edit(int patch, int instrument, const Preset_struct &preset, const AudioTables::Pointers &tables)')
fixture += method(source, 'void AudioPlayer::Reset_loop_slide(void)')
fixture += r'''
int main()
{
    AudioPlayer voice;
    Preset_struct preset;
    AudioTables::Pointers tables;
    preset.slide = 25;
    assert(!voice.Apply_preset_edit(1, 2, preset, tables));
    assert(voice.slide_active && !voice.use_Wavetable && voice.edits == 0);
    assert(voice.tables_bank_mask == 2);
    assert(voice.loop_slide.Phase() == 250.25f && voice.loop_slide.Describe(0).source == 10350);
    voice.loop_slide.Advance(549.75f); // First junction: current tail to A + 25.
    assert(voice.loop_slide.Describe(0).head == 10025);
    voice.loop_slide.Commit_lookahead(19);
    voice.loop_slide.Advance(20);
    const auto in_progress = voice.loop_slide.Describe(0);
    preset.slide = -10;
    assert(!voice.Apply_preset_edit(1, 2, preset, tables));
    assert(voice.loop_slide.Phase() == 820);
    assert(voice.loop_slide.Describe(0).head == in_progress.head);
    voice.loop_slide.Advance(80);
    assert(voice.loop_slide.Describe(0).source == 10125);
    voice.loop_slide.Advance(800);
    assert(voice.loop_slide.Describe(0).head == 10015); // Reverse from reached 10025, not from initial 10000.
    voice.loop_slide.Commit_lookahead(99);
    voice.loop_slide.Advance(100);
    preset.slide = 0;
    preset.use_Wavetable = true;
    assert(!voice.Apply_preset_edit(1, 2, preset, tables));
    assert(voice.slide_active && !voice.use_Wavetable && voice.slide_step == 0);
    voice.loop_slide.Advance(900 * 4);
    assert(voice.loop_slide.Describe(0).source == 10115); // Zero freezes the reached window.
    preset.slide = 30;
    assert(!voice.Apply_preset_edit(1, 2, preset, tables));
    voice.loop_slide.Advance(900);
    assert(voice.loop_slide.Describe(0).source == 10145);
    voice.warmup_for_play_again_flag = true;
    preset.slide = -5;
    assert(!voice.Apply_preset_edit(1, 2, preset, tables));
    assert(voice.starts == 1 && voice.edits == 0); // Pending Note On gets new settings; current voice is not restarted.
    voice.Reset_loop_slide(); // Start_playing invokes this reset for the actual incoming note.
    assert(voice.loop_slide.Phase() == 0 && voice.loop_slide.Describe(0).source == 10100);

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
