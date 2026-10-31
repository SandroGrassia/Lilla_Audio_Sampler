"""Exercise production Live FWD harvesting, write-head jumps and output ramps."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile
from production_source import method

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / 'lib/AudioPlayer/AudioPlayer.cpp').read_text(encoding='utf-8')
prefix = r'''
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include "PlayerAntialias.h"
#include "PlayerInterpolator.h"
constexpr int AUDIO_BLOCK_SAMPLES = 128, ONCE_FWD = 0, ONCE_REV = 1, LOOP_FWD = 2, LOOP_FWD_REV = 3, LOOP_REV = 5, IDLE_REQUEST = 2, REC = 1;
int LS_buffer_dim = 4096, LS_X_delta = 0, LS_state = REC;
bool LS_XY_lock = false;
constexpr float MIN_PITCH = 0.01f;
namespace SdSoundSource { constexpr int PREVIEW = -1; }
float constrain(float v, float lo, float hi) { return std::min(hi, std::max(lo, v)); }
constexpr float VIBRATO_STEP = 1;
namespace PlayerReadBudget { constexpr int Live_noclick_samples = 128; }
struct Recorder
{
    bool first_write_flag = false, writing = true;
    int Q_sample = 1023;
    bool Is_writing() const { return writing; }
} recorder;
int16_t memory[4096];
struct AudioPlayer
{
    bool LS_flag = true, use_Wavetable = false, live_forward_end = false, live_forward_empty = false;
    bool live_start_pending = false, live_restart_pending = false, live_fade_out = false, live_fade_in = false;
    bool warmup_for_play_again_flag = false, idle = false, power_on = true;
    inline static volatile bool live_unrecorded_notice = false;
    float live_restart_sample = 0.0f;
    int live_last_read_sample = 0;
    int mode_player = ONCE_FWD, state = 1, closes = 0;
    int A_Flash_sample = 0, B_Flash_sample = 4095, C_Flash_sample = 8190, B_Flash_sample_shifted = 4095;
    int Flash_first_RAM_sample = 0, Wavetable_length = 128, delta_Noclick = 0;
    int16_t *Wavetable_ptr = memory, *Noclick_ptr = memory;
    float a_first_sample = 0, a_sample = 0, b_sample = 0, pitch = 1.5f, pitch_note = 1.5f, pitch_tune = 1;
    float initial_index_offset = 0, interpolation_mix = 0;
    uint8_t tone_filter_level = 0;
    PlayerAntialias::Settings antialias;
    int harvested_samples = 0, read_last = -1, read_count = 0;
    int16_t samples_basket[5000] = {}, block[128] = {}, live_forward_last_sample = 0;
    float resampled_basket[128] = {};
    Recorder *Live_Sampler = &recorder;
    void Read_samples(int16_t *dst, int first, int count)
    {
        assert(count > 0 && count <= 5000);
        read_last = first + count - 1;
        read_count = count;
        for (int i = 0; i < count; ++i)
        {
            dst[i] = memory[((first + i) % LS_buffer_dim + LS_buffer_dim) % LS_buffer_dim];
        }
    }
    void Close_source() { ++closes; }
    void Fast_stop() { assert(false); }
    void Wavetable_harvest() { assert(false); }
    int Loop_period(int, int, int, int) { assert(false); return 1; }
    bool Fill_loop_samples(int16_t *, int, int, int, int, int, int, const int16_t *, int) { assert(false); return false; }
    void Flash_memory_harvest()
    {
        live_forward_end = Harvest_live_forward_end();
        if (!live_forward_end)
        {
            assert(Harvest_live_continuous());
        }
    }
    bool pitch_tune_flag = false, pitch_bend_flag = false, restart_flag = false, vibrato_flag = false;
    float pitch_tune_wait = 1, pitch_bend_wait = 1, pitch_bend = 1, pitch_vibrato = 1, pitch_limit = 35, pitch_based_gain_correction = 1;
    float vibrato_array_element_float = 0, modulation_depth = 1, vibrato_values[32] = {};
    uint8_t vibrato_array_element = 0, last_vibrato = 0;
    uint8_t *vibrato_array_last_element_ptr = &last_vibrato;
    float *vibrato_array_ptr = vibrato_values;
    int file_id = 320;
    void Update_pitch();
    void Apply_pitch_requests();
    void Prepare_live_forward_block();
    void Check_live_forward_boundary();
    void Shape_live_forward_block();
    void Harvest_samples(int outputs = 128);
    void Harvest_filtered_samples(int left, int right, float distance);
    bool Harvest_live_forward_end();
    bool Harvest_live_continuous();
    void Fade_live_forward_end();
    float Read_rendered_sample(int sample, float position) const;
    void Render()
    {
        live_forward_end = false;
        Prepare_live_forward_block();
        a_sample = a_first_sample;
        b_sample = a_sample + pitch * 127;
        initial_index_offset = a_sample - floorf(a_sample);
        Harvest_samples();
        Check_live_forward_boundary();
        for (int i = 0; i < 128; ++i)
        {
            block[i] = static_cast<int16_t>(Read_rendered_sample(i, initial_index_offset + i * pitch));
        }
        if (live_forward_end)
        {
            live_restart_pending = false;
            Fade_live_forward_end();
        }
        else
        {
            Shape_live_forward_block();
        }
    }
};
'''
checks = r'''
int main()
{
    for (int i = 0; i < LS_buffer_dim; ++i)
    {
        memory[i] = static_cast<int16_t>(10000 * sin(i * 0.021) + 1500 * cos(i * 0.047));
    }
    int16_t original[4096];
    std::copy_n(memory, 4096, original);
    // Mobile starts: zero/nonzero offset, wrap and fractional pitch. Unaffected starts stay intact.
    for (int q : {127, 1023, 4095})
    {
        recorder.Q_sample = q;
        for (int offset : {0, 1, 120, 4095})
        {
            LS_X_delta = offset;
            AudioPlayer p;
            p.live_start_pending = true;
            p.Prepare_live_forward_block();
            assert(p.A_Flash_sample == (q + (offset == 0 ? 1 : offset)) % LS_buffer_dim);
            assert(p.a_first_sample == p.A_Flash_sample);
        }
    }
    for (int scenario = 0; scenario < 4; ++scenario)
    {
        AudioPlayer p;
        p.live_start_pending = true;
        p.a_first_sample = 77;
        LS_XY_lock = scenario == 0;
        p.pitch = scenario == 1 ? 1.0f : 1.5f;
        recorder.writing = scenario != 2;
        recorder.first_write_flag = scenario == 3;
        recorder.Q_sample = 1023;
        p.Prepare_live_forward_block();
        assert(p.a_first_sample == 77);
    }
    LS_XY_lock = false;
    LS_X_delta = 0;
    recorder.writing = true;
    recorder.first_write_flag = false;
    // FIXED attacks move only when the prepared start belongs to the latest written block.
    LS_XY_lock = true;
    for (int q : {0, 63, 127, 1023, 4095})
    {
        recorder.Q_sample = q;
        for (int distance : {-1, 0, 1, 127, 128, 256})
        {
            const int fixed_start = (q - distance + LS_buffer_dim) % LS_buffer_dim;
            AudioPlayer p;
            p.A_Flash_sample = fixed_start;
            p.B_Flash_sample = fixed_start + LS_buffer_dim - 1;
            p.a_first_sample = fixed_start;
            p.live_start_pending = true;
            LS_X_delta = 777; // A stale mobile offset must not affect FIXED.
            p.Prepare_live_forward_block();
            const int expected = distance >= 0 && distance < 128 ? (q + 1) % LS_buffer_dim : fixed_start;
            assert(p.A_Flash_sample == expected && p.a_first_sample == expected);
            assert(p.B_Flash_sample == expected + LS_buffer_dim - 1);
            assert(!p.live_restart_pending && !p.live_fade_in && !p.live_fade_out);
        }
    }
    for (int scenario = 0; scenario < 4; ++scenario)
    {
        AudioPlayer p;
        recorder.Q_sample = 1023;
        recorder.first_write_flag = scenario == 0;
        recorder.writing = scenario != 1;
        LS_state = scenario == 3 ? 0 : REC;
        p.pitch = scenario == 2 ? 1.0f : 1.5f;
        p.A_Flash_sample = 1000;
        p.a_first_sample = 1000;
        p.live_start_pending = true;
        p.Prepare_live_forward_block();
        assert(p.a_first_sample == (scenario == 2 ? 1024 : 1000) && p.A_Flash_sample == p.a_first_sample);
    }
    recorder.first_write_flag = false;
    recorder.writing = true;
    LS_state = REC;
    for (bool fixed : {false, true})
    {
        LS_XY_lock = fixed;
        // Exact actual read endpoints differ with antialias when b_sample is an integer.
        for (bool filtered : {false, true})
        {
            AudioPlayer p;
            p.tone_filter_level = filtered ? 0x81 : 0;
            p.pitch = p.pitch_note = 2;
            p.a_first_sample = 769; // b=1023, the last written sample.
            recorder.Q_sample = 1023;
            p.Render();
            assert(p.live_last_read_sample == 1023 + filtered);
            assert(p.live_fade_out && p.live_restart_pending);
            assert(p.live_restart_sample == 1152);
        }
        // Exercise ramps and saved destinations with real interpolation/filtering, including wrap and STOP.
        for (bool filtered : {false, true})
        {
            for (int q : {127, 1023, 4095})
            {
                for (bool stop : {false, true})
                {
                    recorder.Q_sample = q;
                    recorder.writing = true;
                    LS_state = REC;
                    AudioPlayer p;
                    p.tone_filter_level = filtered ? 0x81 : 0;
                    p.a_first_sample = q - 200;
                    p.A_Flash_sample = q - 200;
                    p.B_Flash_sample = -999; // Full-cache FWD must never use B, including after STOP.
                    p.Render();
                    assert(p.live_fade_out && p.live_restart_pending && p.block[127] == 0);
                    const int destination = (q + 129) % LS_buffer_dim;
                    assert(p.live_restart_sample == destination);
                    recorder.Q_sample = (q + 128) % LS_buffer_dim;
                    recorder.writing = !stop;
                    LS_state = stop ? 0 : REC;
                    p.Render();
                    assert(p.live_fade_in && !p.live_restart_pending && p.block[0] == 0);
                    assert((static_cast<int>(p.a_sample) % LS_buffer_dim + LS_buffer_dim) % LS_buffer_dim == destination);
                    assert(p.state == 1 && p.closes == 0);
                }
            }
        }
        // A bend above unity crosses Q from the previous recorded block, including circular wrap.
        recorder.writing = true;
        LS_state = REC;
        for (int q : {127, 1023, 4095})
        {
            for (float bent_pitch : {1.0594631f, 2.0f})
            {
                AudioPlayer p;
                recorder.Q_sample = q;
                p.pitch = p.pitch_note = 1.0f;
                p.a_first_sample = q - 127;
                p.Render();
                assert(!p.live_restart_pending);
                recorder.Q_sample = (q + 128) % LS_buffer_dim;
                p.pitch = p.pitch_note = bent_pitch;
                p.Render();
                assert(p.live_fade_out && p.live_restart_pending && p.block[127] == 0);
                const int destination = (recorder.Q_sample + 129) % LS_buffer_dim;
                assert(p.live_restart_sample == destination);
                recorder.Q_sample = (recorder.Q_sample + 128) % LS_buffer_dim;
                p.Render();
                assert(p.live_fade_in && !p.live_restart_pending && p.block[0] == 0);
                assert(static_cast<int>(p.a_sample) % LS_buffer_dim == destination);
            }
        }
        // Full harvested intervals trigger only when crossing Q or ending in the latest written block.
        for (int q : {0, 63, 1023, 4095})
        {
            recorder.Q_sample = q;
            for (int first_offset : {-300, -127, 0, 1, 128})
            {
                AudioPlayer p;
                p.harvested_samples = 136;
                p.live_last_read_sample = q + first_offset + p.harvested_samples - 1;
                p.Check_live_forward_boundary();
                assert(p.live_restart_pending == (first_offset == -127 || first_offset == 0));
            }
        }
        // Endpoint inclusion is circular: Q and Q-127 belong, Q+1 and Q-128 do not.
        recorder.writing = true;
        LS_state = REC;
        for (int q : {0, 63, 127, 4095})
        {
            recorder.Q_sample = q;
            for (int distance : {-1, 0, 127, 128})
            {
                AudioPlayer p;
                p.live_last_read_sample = q - distance;
                p.harvested_samples = 1;
                p.Check_live_forward_boundary();
                assert(p.live_restart_pending == (distance >= 0 && distance < 128));
            }
        }
        AudioPlayer changed;
        changed.live_restart_pending = true;
        changed.mode_player = LOOP_FWD;
        changed.a_first_sample = 333;
        changed.Prepare_live_forward_block();
        assert(!changed.live_restart_pending && !changed.live_fade_in && changed.a_first_sample == 333);
    }
    LS_XY_lock = false;
    // Ramps include exact endpoints, work for either sign, and leave the cache untouched.
    for (bool up : {false, true})
    {
        for (int value : {-32768, 32767})
        {
            AudioPlayer p;
            std::fill_n(p.block, 128, value);
            p.live_fade_in = up;
            p.live_fade_out = !up;
            p.Shape_live_forward_block();
            assert(p.block[0] == (up ? 0 : value));
            assert(p.block[127] == (up ? value : 0));
            for (int i = 1; i < 128; ++i)
            {
                assert(std::abs(p.block[i] - p.block[i - 1]) <= 259);
            }
        }
    }
    assert(std::equal(original, original + 4096, memory));
    // Reproduce SYNC at +1 semitone while a sine is recorded into a previously silent full cache.
    // Advance the recorder after each Player block, matching the firmware's update order.
    for (bool filtered : {false, true})
    {
        std::fill_n(memory, 4096, 0);
        recorder.Q_sample = 1023;
        recorder.first_write_flag = false;
        recorder.writing = true;
        LS_state = REC;
        LS_X_delta = 0;
        LS_XY_lock = false;
        for (int i = 0; i <= recorder.Q_sample; ++i)
        {
            memory[i] = static_cast<int16_t>(20000 * sin(i * 0.03));
        }
        AudioPlayer p;
        p.live_start_pending = true;
        p.pitch = p.pitch_note = powf(2.0f, 1.0f / 12.0f);
        p.tone_filter_level = filtered ? 0x81 : 0;
        for (int packet = 0; packet < 20; ++packet)
        {
            p.Render();
            assert(p.a_sample > recorder.Q_sample);
            assert(!p.live_restart_pending);
            for (int i = 0; i < 128; ++i)
            {
                // The IIR history can produce a short initial tail, but not a sustained full-amplitude burst.
                const int limit = !filtered ? 0 : (packet == 0 ? 2500 : 20);
                assert(std::abs(static_cast<int>(p.block[i])) <= limit);
            }
            for (int i = 1; i <= 128; ++i)
            {
                memory[recorder.Q_sample + i] = static_cast<int16_t>(20000 * sin((recorder.Q_sample + i) * 0.03));
            }
            recorder.Q_sample += 128;
        }
    }
    std::copy_n(original, 4096, memory);
    // First fill retains the existing stop/fade, with and without antialias.
    recorder.first_write_flag = true;
    recorder.Q_sample = 255;
    for (bool filtered : {false, true})
    {
        AudioPlayer p;
        p.tone_filter_level = filtered ? 0x81 : 0;
        p.a_first_sample = 200;
        p.Render();
        assert(p.live_forward_end && p.state == IDLE_REQUEST && !p.live_restart_pending && p.block[127] == 0);
    }
    // Continuous full-cache reading ignores B after STOP for linear, sinc and antialias paths.
    recorder.first_write_flag = false;
    recorder.writing = false;
    LS_state = 0;
    for (float speed : {0.5f, 1.0f, 1.0594631f, 2.0f})
    {
        for (bool filtered : {false, true})
        {
            AudioPlayer p;
            p.pitch = p.pitch_note = speed;
            p.tone_filter_level = filtered ? 0x81 : 0;
            p.A_Flash_sample = 4000;
            p.a_first_sample = 4000;
            p.B_Flash_sample = -999;
            for (int i = 0; i < 150; ++i)
            {
                p.Render();
                assert(p.state == 1 && p.closes == 0 && !p.live_restart_pending);
                assert(p.a_first_sample >= 4000 && p.a_first_sample < 8096);
            }
        }
    }
    // Exact startup boundaries, including cache wrap, in both lock modes.
    recorder.writing = true;
    recorder.first_write_flag = false;
    LS_state = REC;
    for (bool fixed : {false, true})
    {
        LS_XY_lock = fixed;
        for (float speed : {1.0f, 0.99f, 0.5f})
        {
            for (int q : {0, 63, 127, 1023, 4095})
            {
                recorder.Q_sample = q;
                for (int distance : {-1, 0, 126, 127, 128, 254})
                {
                    AudioPlayer p;
                    const int start = (q - distance + 4096) % 4096;
                    p.A_Flash_sample = start;
                    p.a_first_sample = start;
                    p.live_start_pending = true;
                    p.pitch = speed;
                    p.Prepare_live_forward_block();
                    const bool forbidden = distance >= 0 && distance < (speed == 1 ? 127 : 128);
                    const int expected = forbidden ? (q + (speed == 1 ? 1 : -128) + 4096) % 4096 : start;
                    assert(p.a_first_sample == expected);
                }
            }
        }
    }
    // Real sinc prefetch, fractional rewind, STOP and circular restart.
    for (float speed : {0.25f, 0.5f, 0.9f, 0.99f})
    {
        for (int q : {0, 63, 1023, 4095})
        {
            for (bool stop : {false, true})
            {
                recorder.Q_sample = q;
                recorder.writing = true;
                LS_state = REC;
                AudioPlayer p;
                p.pitch = p.pitch_note = speed;
                p.a_first_sample = q + 0.25f;
                p.Render();
                assert(p.live_fade_out && p.live_restart_pending && p.block[127] == 0);
                const float saved = q + 0.25f - 128;
                assert(p.live_restart_sample == saved);
                recorder.Q_sample = (q + 128) % 4096;
                recorder.writing = !stop;
                LS_state = stop ? 0 : REC;
                p.Render();
                assert(p.live_fade_in && !p.live_restart_pending && p.block[0] == 0);
                assert(p.a_sample == fmodf(saved + 4096, 4096));
                const int last = (p.live_last_read_sample % 4096 + 4096) % 4096;
                assert((recorder.Q_sample - last + 4096) % 4096 < 256);
            }
        }
    }
    // Merely ending at Q does not cross it; the first extra sample does.
    recorder.writing = true;
    LS_state = REC;
    recorder.Q_sample = 1023;
    for (int end : {1022, 1023, 1024})
    {
        AudioPlayer p;
        p.pitch = 0.5f;
        p.a_sample = 1000.25f;
        p.live_last_read_sample = end;
        p.harvested_samples = end - 996 + 1;
        p.Check_live_forward_boundary();
        assert(p.live_restart_pending == (end > 1023));
    }
    // Pending bend/tune and vibrato are consumed only after the fade-in block.
    for (float speed : {0.5f, 1.5f})
    {
        AudioPlayer p;
        recorder.Q_sample = 1023;
        p.pitch = p.pitch_note = speed;
        p.a_first_sample = speed < 1 ? 1023.25f : 823.0f;
        p.Render();
        assert(p.live_restart_pending);
        p.pitch_tune_wait = 2;
        p.pitch_bend_wait = 1.1f;
        p.pitch_tune_flag = p.pitch_bend_flag = p.vibrato_flag = true;
        std::fill_n(p.vibrato_values, 32, 1.01f);
        p.Apply_pitch_requests();
        assert(p.pitch == speed && p.pitch_tune_flag && p.pitch_bend_flag && p.vibrato_array_element_float == 0);
        recorder.Q_sample += 128;
        p.Render();
        assert(p.live_fade_in && p.pitch == speed);
        p.Apply_pitch_requests();
        assert(!p.pitch_tune_flag && !p.pitch_bend_flag && p.vibrato_array_element_float == 1);
        assert(std::abs(p.pitch - speed * 2 * 1.1f * 1.01f) < 0.0001f);
    }
    std::cout << "PASS: mobile/fixed starts, actual filtered endpoints, circular collisions, ramps, STOP, mode changes and first fill\n";
}
'''
signatures = [
    'void AudioPlayer::Update_pitch(void)',
    'void AudioPlayer::Prepare_live_forward_block(void)',
    'void AudioPlayer::Check_live_forward_boundary(void)',
    'void AudioPlayer::Shape_live_forward_block(void)',
    'void AudioPlayer::Harvest_samples(int outputs)',
    'void AudioPlayer::Harvest_filtered_samples(int left, int right, float distance)',
    'bool AudioPlayer::Harvest_live_forward_end(void)',
    'bool AudioPlayer::Harvest_live_continuous(void)',
    'void AudioPlayer::Fade_live_forward_end(void)',
    'float AudioPlayer::Read_rendered_sample(int sample, float position) const',
]
compiler = shutil.which('g++') or 'C:/msys64/ucrt64/bin/g++.exe'
env = os.environ.copy()
env['PATH'] = str(Path(compiler).parent) + os.pathsep + env.get('PATH', '')
with tempfile.TemporaryDirectory(prefix='lilla-live-forward-') as folder:
    cpp = Path(folder) / 'test.cpp'
    exe = Path(folder) / 'test.exe'
    pitch_start = source.index('        if (!live_restart_pending)', source.index('void AudioPlayer::update(void)'))
    pitch_end = source.index('        if (main_settings_editing_flag)', pitch_start)
    pitch_requests = 'void AudioPlayer::Apply_pitch_requests()\n{\n' + source[pitch_start:pitch_end] + '}\n'
    cpp.write_text(prefix + ''.join(method(source, s) for s in signatures) + pitch_requests + checks, encoding='utf-8')
    lib = ROOT / 'lib/AudioPlayer'
    subprocess.run([compiler, '-std=c++20', '-O2', '-Wall', '-Wextra', '-Werror', '-I' + str(lib), str(cpp), str(lib / 'PlayerAntialias.cpp'), str(lib / 'PlayerInterpolator.cpp'), '-o', str(exe)], check=True, env=env)
    subprocess.run([str(exe)], check=True, env=env)
