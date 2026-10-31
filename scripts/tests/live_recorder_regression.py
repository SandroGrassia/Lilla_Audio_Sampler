"""Check recorder ownership, mono rounding, fades and circular writes with shared inputs."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def source(path):
    return re.sub(r'^\s*#(?:include|pragma)[^\n]*', '', (ROOT / path).read_text(encoding='utf-8'), flags=re.M)


prefix = r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
constexpr int AUDIO_BLOCK_SAMPLES = 128;
constexpr int AUDIO_BLOCK_BYTES = 256;
int LS_buffer_dim = 4096;
struct audio_block_t
{
    int16_t data[AUDIO_BLOCK_SAMPLES];
    int references = 2;
};
class AudioStream
{
public:
    audio_block_t *pending[2] = {};
    AudioStream(int channels, audio_block_t **) { assert(channels == 2); }
    virtual void update() = 0;
    audio_block_t *receiveReadOnly(unsigned channel)
    {
        auto *block = pending[channel];
        pending[channel] = nullptr;
        return block;
    }
    void release(audio_block_t *block)
    {
        assert(block != nullptr && block->references == 2);
        --block->references;
    }
};
'''


stop_reverse = r"""
constexpr int EMPTY = 0, REC = 1, PLAYONLY = 2, ONCE_FWD = 0, ONCE_REV = 1, LIVE_SAMPLING = 2, PATCHES_MAX = 10, SOUNDS_MAX = 10, LS_menu_elements = 5;
enum { value_LS_Recording, value_LS_Stop, value_LS_StopReverse, value_LS_MonoStereo, value_LS_Erase };
bool LS_stop_reverse_pending = false, LS_XY_lock = false, LS_stereo = false, Menu_LS[5] = {};
int LS_state = REC, LS_menu_max = 0, LS_Q_sample = 0, LS_X_sample = 0, LS_X_delta = 0, LS_Y_sample = 0, LS_XY_delta = 500, LS_mode = ONCE_FWD, LS_sound_id = SOUNDS_MAX;
int Patch_id = PATCHES_MAX, Lilla_state = LIVE_SAMPLING;
StereoLiveSampler LiveSampler;
struct SoundValue { int mode = ONCE_FWD; } Sound[SOUNDS_MAX + 2];
struct Manager
{
    int updates = 0, edits = 0;
    void Update_Preset_mode(int patch, int instrument) { assert(patch == PATCHES_MAX && instrument < (LS_stereo ? 2 : 1)); ++updates; }
    void Multicast_main_settings_editing(int, int) { ++edits; }
} Players_Manager;
struct Display
{
    int draws = 0;
    void Play_mode() { ++draws; }
    void Start_point() { ++draws; }
    void Loop_time() { ++draws; }
    void Show_wave(int) { ++draws; }
} Display_LiveSampler;
struct Pointer { void Show_pointer(bool) {} } Pointer_LiveSampler;
void AudioNoInterrupts() {}
void AudioInterrupts() {}
"""
main_source = (ROOT / 'src/main.cpp').read_text(encoding='utf-8')
for name in ['LS_complete_stop_reverse', 'LS_update_menu_elements']:
    stop_reverse += re.search(r'^void ' + name + r'\(void\)\n\{.*?^\}', main_source, re.M | re.S).group(0) + '\n'
stop_reverse += r"""
void Check_stop_reverse()
{
    for (int state : {EMPTY, REC, PLAYONLY})
    {
        LS_state = state;
        LS_update_menu_elements();
        assert(Menu_LS[value_LS_StopReverse] == (state == REC));
        assert(Menu_LS[value_LS_Stop] == (state == REC));
        assert(LS_menu_max == 1);
    }
    static int16_t mono[4096] = {}, left[4096] = {}, right[4096] = {};
    LiveSampler.LS_buffer_mono_ptr = mono;
    LiveSampler.LS_buffer_L_ptr = left;
    LiveSampler.LS_buffer_R_ptr = right;
    for (bool stereo : {false, true})
    {
        for (bool immediate : {false, true})
        {
            LS_stereo = stereo;
            LS_mode = ONCE_FWD;
            LS_XY_lock = false;
            LS_X_sample = 42;
            LS_stop_reverse_pending = true;
            Players_Manager.updates = Players_Manager.edits = 0;
            LiveSampler.Reset();
            LiveSampler.Start(stereo);
            if (!immediate)
            {
                for (int i = 0; i < 3; ++i) { LiveSampler.update(); }
                LiveSampler.Q_sample = LS_buffer_dim - 129; // The stop ramp crosses the circular boundary.
            }
            LiveSampler.Stop();
            for (int i = 0; i < (immediate ? 6 : 3); ++i)
            {
                LS_complete_stop_reverse();
                assert(LS_stop_reverse_pending && LS_X_sample == 42 && LS_mode == ONCE_FWD);
                LiveSampler.update();
            }
            assert(!LiveSampler.Is_writing());
            LS_complete_stop_reverse();
            assert(!LS_stop_reverse_pending && LS_XY_lock && LS_mode == ONCE_REV);
            assert(LS_X_sample == (immediate ? 767 : 255) && LS_X_sample == LiveSampler.Q_sample);
            assert(LS_Y_sample == LS_X_sample + LS_XY_delta);
            assert(Players_Manager.updates == (stereo ? 2 : 1) && Players_Manager.edits == Players_Manager.updates);
            LS_complete_stop_reverse();
            assert(Players_Manager.updates == (stereo ? 2 : 1)); // Completion is consumed once.
        }
    }
}
"""

checks = r'''
int main()
{
    for (bool stereo : {false, true})
    {
        StereoLiveSampler recorder;
        int16_t mono[4096] = {};
        int16_t left[4096] = {};
        int16_t right[4096] = {};
        std::fill_n(mono, 4096, 3210);
        std::fill_n(left, 4096, 4567);
        std::fill_n(right, 4096, -6789);
        recorder.LS_buffer_mono_ptr = mono;
        recorder.LS_buffer_L_ptr = left;
        recorder.LS_buffer_R_ptr = right;
        recorder.Reset();
        recorder.Start(stereo);
        auto run = [&]()
        {
            const int next = (recorder.Q_sample + 1 + AUDIO_BLOCK_SAMPLES) % LS_buffer_dim;
            int16_t saved_mono[AUDIO_BLOCK_SAMPLES], saved_left[AUDIO_BLOCK_SAMPLES], saved_right[AUDIO_BLOCK_SAMPLES];
            std::copy_n(mono + next, AUDIO_BLOCK_SAMPLES, saved_mono);
            std::copy_n(left + next, AUDIO_BLOCK_SAMPLES, saved_left);
            std::copy_n(right + next, AUDIO_BLOCK_SAMPLES, saved_right);
            audio_block_t input_l;
            audio_block_t input_r;
            std::fill(std::begin(input_l.data), std::end(input_l.data), 12001);
            std::fill(std::begin(input_r.data), std::end(input_r.data), -3001);
            recorder.pending[0] = &input_l;
            recorder.pending[1] = &input_r;
            recorder.update();
            // Recording must preserve the next block during first fill, wrap, restart and stop fades.
            assert(std::equal(saved_mono, saved_mono + AUDIO_BLOCK_SAMPLES, mono + next));
            assert(std::equal(saved_left, saved_left + AUDIO_BLOCK_SAMPLES, left + next));
            assert(std::equal(saved_right, saved_right + AUDIO_BLOCK_SAMPLES, right + next));
            assert(input_l.references == 1 && input_r.references == 1);
            for (int sample = 0; sample < AUDIO_BLOCK_SAMPLES; ++sample)
            {
                assert(input_l.data[sample] == 12001 && input_r.data[sample] == -3001);
            }
        };
        for (int block = 0; block < 4; ++block)
        {
            run();
        }
        assert(recorder.Q_sample == 511 && recorder.Is_writing());
        assert((stereo ? left[0] : mono[0]) == 0);
        for (int sample = 384; sample < 512; ++sample)
        {
            assert(stereo ? left[sample] == 12001 && right[sample] == -3001 : mono[sample] == 4499);
        }
        recorder.Stop();
        run();
        assert(recorder.Is_writing());
        run();
        assert(recorder.Is_writing());
        run();
        assert(!recorder.Is_writing());
        assert((stereo ? left[895] : mono[895]) < (stereo ? left[512] : mono[512]));
        recorder.Start(stereo);
        for (int block = 0; block < 26; ++block)
        {
            run();
        }
        assert(recorder.Q_sample == 127 && !recorder.first_write_flag);
        assert(stereo ? left[0] == 12001 && right[0] == -3001 : mono[0] == 4499);
        assert((stereo ? left[128] : mono[128]) != 0);
        audio_block_t only_left = {};
        recorder.pending[0] = &only_left;
        recorder.update();
        assert(only_left.references == 1 && recorder.Q_sample == 255);
        recorder.update();
        assert(recorder.Q_sample == 383);
        recorder.Stop();
        for (int i = 0; i < 3; ++i)
        {
            recorder.update();
        }
        assert(!recorder.Is_writing());
        recorder.Reset();
        recorder.Start(stereo);
        for (int block = 0; block < 96; ++block)
        {
            run();
            if (block >= 64)
            {
                // SYNC FWD at high pitch crosses Q into the previous pass, including buffer wrap.
                for (int pitch : {2, 4, 8})
                {
                    for (int sample = 0; sample < AUDIO_BLOCK_SAMPLES; ++sample)
                    {
                        const int position = (recorder.Q_sample - AUDIO_BLOCK_SAMPLES + sample * pitch + LS_buffer_dim) % LS_buffer_dim;
                        assert(stereo ? left[position] == 12001 && right[position] == -3001 : mono[position] == 4499);
                    }
                }
            }
        }
        for (int mask = 0; mask < 4; ++mask)
        {
            recorder.Reset();
            recorder.Stop(); // A stale stop must not leak into the next recording.
            recorder.Start(stereo);
            auto missing_run = [&]()
            {
                audio_block_t l = {}, r = {};
                std::fill_n(l.data, AUDIO_BLOCK_SAMPLES, 12001);
                std::fill_n(r.data, AUDIO_BLOCK_SAMPLES, -3001);
                recorder.pending[0] = (mask & 1) != 0 ? &l : nullptr;
                recorder.pending[1] = (mask & 2) != 0 ? &r : nullptr;
                recorder.update();
                assert(l.references == ((mask & 1) != 0 ? 1 : 2));
                assert(r.references == ((mask & 2) != 0 ? 1 : 2));
            };
            for (int i = 0; i < 4; ++i)
            {
                missing_run();
            }
            const int l = (mask & 1) != 0 ? 12001 : 0;
            const int r = (mask & 2) != 0 ? -3001 : 0;
            assert(recorder.Is_writing() && recorder.Q_sample == 511);
            for (int i = 384; i < 512; ++i)
            {
                assert(stereo ? left[i] == l && right[i] == r : mono[i] == (l >> 1) + (r >> 1));
            }
            recorder.Q_sample = LS_buffer_dim - 1;
            recorder.Stop();
            for (int i = 0; i < 3; ++i)
            {
                missing_run();
            }
            assert(!recorder.Is_writing() && recorder.Q_sample == 383);
            assert(stereo ? left[0] == l && right[0] == r : mono[0] == (l >> 1) + (r >> 1));
            recorder.Reset();
            recorder.Start(stereo);
            recorder.Stop(); // Stop during attack: finish attack before starting the three-block decay.
            for (int i = 0; i < 6; ++i)
            {
                missing_run();
            }
            assert(!recorder.Is_writing() && recorder.Q_sample == 767);
        }
    }
    Check_stop_reverse();
    std::cout << "PASS: STOP+REVERSE final position and menu visibility; shared inputs unchanged/released, mono rounding, stereo separation, fades, wrap and missing inputs\n";
}
'''

compiler = shutil.which('g++') or 'C:/msys64/ucrt64/bin/g++.exe'
environment = os.environ.copy()
environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
production = source('lib/StereoLiveSampler/StereoLiveSampler.h') + source('lib/StereoLiveSampler/StereoLiveSampler.cpp')
with tempfile.TemporaryDirectory(prefix='lilla-live-recorder-') as folder:
    cpp = Path(folder) / 'test.cpp'
    exe = Path(folder) / 'test.exe'
    cpp.write_text(prefix + production + stop_reverse + checks, encoding='utf-8', newline='\r\n')
    subprocess.run([compiler, '-std=c++20', '-O2', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True, env=environment)
    subprocess.run([str(exe)], check=True, env=environment)
