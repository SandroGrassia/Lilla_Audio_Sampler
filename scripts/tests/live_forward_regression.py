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
constexpr int BASKET_DIM = 5000;
namespace PlayerReadBudget { constexpr int Live_noclick_samples = 32; }
struct Recorder
{
    bool first_write_flag = false, writing = true;
    int Q_sample = 1023;
    bool Is_writing() const { return writing; }
} recorder;
int16_t memory[32768];
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
    float initial_index_offset = 0, interpolation_mix = 0, live_sinc_mix_start = 0;
    bool live_sinc_start_pending = false, verify_recorded_reads = false;
    uint8_t tone_filter_level = 0;
    PlayerAntialias::Settings antialias;
    int harvested_samples = 0, read_last = -1, read_count = 0, read_total = 0;
    int16_t samples_basket[5000] = {}, block[128] = {}, live_forward_last_sample = 0;
    float resampled_basket[128] = {};
    Recorder *Live_Sampler = &recorder;
    void Read_samples(int16_t *dst, int first, int count)
    {
        assert(count > 0 && count <= 5000);
        if (verify_recorded_reads && recorder.first_write_flag && recorder.Q_sample < LS_buffer_dim - 1)
        {
            const int position = (first % LS_buffer_dim + LS_buffer_dim) % LS_buffer_dim;
            const int tail = std::min(count, LS_buffer_dim - position);
            if (position + tail - 1 > recorder.Q_sample || count - tail - 1 > recorder.Q_sample)
            {
                std::cerr << "Unrecorded read: mode=" << mode_player << " pitch=" << pitch << " Q=" << recorder.Q_sample << " A=" << A_Flash_sample << " a=" << a_sample << " first=" << first << " count=" << count << " mix=" << interpolation_mix << "\n";
                assert(false);
            }
        }
        read_last = first + count - 1;
        read_count = count;
        read_total += count;
        for (int i = 0; i < count; ++i)
        {
            dst[i] = memory[((first + i) % LS_buffer_dim + LS_buffer_dim) % LS_buffer_dim];
        }
    }
    void Close_source() { ++closes; }
    void Fast_stop() { assert(false); }
    void Wavetable_harvest() { assert(false); }
    int Loop_period(int first, int last, int crossfade, uint8_t mode) const;
    bool Fill_loop_samples(int16_t *destination, int count, int phase, int first, int last, int crossfade, uint8_t mode, const int16_t *noclick, int period);
    void Loop_memory_harvest();
    void Flash_memory_harvest()
    {
        if (mode_player >= LOOP_FWD)
        {
            Loop_memory_harvest();
            return;
        }
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
    void Edit_geometry(int mode_player_E, int A_Flash_sample_E, int B_Flash_sample_E);
    int Live_loop_segment(int phase, int first, int last, int &source, int &blend_source) const;
    bool Live_loop_contains(int phase, int count, int position) const;
    bool Live_loop_samples_recorded(int phase, int count, int write_position) const;
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
        changed.Edit_geometry(LOOP_FWD, changed.A_Flash_sample, changed.B_Flash_sample);
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
    // First-fill mobile SYNC: start with the available interpolation samples, never read unwritten memory.
    LS_buffer_dim = 32768;
    LS_XY_lock = false;
    LS_X_delta = 0;
    LS_state = REC;
    recorder.writing = true;
    for (int i = 0; i < LS_buffer_dim; ++i)
    {
        memory[i] = static_cast<int16_t>(12000 * sin(i * 0.03) + 3000 * cos(i * 0.1));
    }
    for (int mode : {ONCE_FWD, LOOP_FWD})
    {
        for (float speed : {0.01f, 0.25f, 0.5f, 0.9f, 0.99f, 0.9999f, 1.0f, 1.0594631f, 2.0f, 35.0f})
        {
            for (int q : {-1, 0, 1, 2, 63, 64, 126, 127, 128, 255, 1023, 32766})
            {
                recorder.Q_sample = q;
                recorder.first_write_flag = true;
                AudioPlayer p;
                p.mode_player = mode;
                p.pitch = p.pitch_note = speed;
                p.A_Flash_sample = p.a_first_sample = mode == ONCE_FWD ? q + 1 : q - 128;
                p.B_Flash_sample = p.A_Flash_sample + (mode == ONCE_FWD ? LS_buffer_dim - 1 : 5292);
                p.live_start_pending = true;
                p.verify_recorded_reads = true;
                AudioPlayer::live_unrecorded_notice = false;
                const int start = std::max(0, q - (speed == 1 ? 127 : 128));
                const bool allowed = speed <= 1 && ceilf(start + 127 * speed) <= q;
                p.Render();
                assert(p.live_forward_end == !allowed && AudioPlayer::live_unrecorded_notice == !allowed);
                if (allowed)
                {
                    assert(p.a_sample == start && p.A_Flash_sample == start && p.state == 1);
                    assert(!p.live_fade_out && !p.live_restart_pending);
                    if (speed == 1)
                    {
                        assert(p.read_total == 128 && p.read_last == q);
                        for (int i = 0; i < 128; ++i)
                        {
                            assert(p.block[i] == memory[start + i]);
                        }
                    }
                }
                else
                {
                    assert(p.state == IDLE_REQUEST && p.read_last == -1);
                    for (int value : p.block)
                    {
                        assert(value == 0);
                    }
                }
            }
        }
        // Follow a growing cache until the sinc window is available, including pitches almost at unity.
        for (float speed : {0.01f, 0.5f, 0.99f, 0.9999f})
        {
            recorder.Q_sample = 127;
            recorder.first_write_flag = true;
            AudioPlayer p;
            p.mode_player = mode;
            p.pitch = p.pitch_note = speed;
            p.A_Flash_sample = p.a_first_sample = mode == ONCE_FWD ? 128 : -1;
            p.B_Flash_sample = p.A_Flash_sample + (mode == ONCE_FWD ? LS_buffer_dim - 1 : 5292);
            p.live_start_pending = true;
            p.verify_recorded_reads = true;
            bool restored = false;
            for (int cycle = 0; cycle < 250; ++cycle)
            {
                p.Render();
                assert(p.state == 1 && !p.live_forward_end && !p.live_restart_pending);
                if (cycle == 0)
                {
                    assert(p.interpolation_mix == 0 && p.read_last <= 127);
                }
                if (p.live_sinc_start_pending && p.interpolation_mix > 0)
                {
                    const float position = p.initial_index_offset;
                    assert(p.Read_rendered_sample(0, position) == PlayerInterpolator::Read(p.samples_basket, position, 0));
                    assert(std::abs(p.Read_rendered_sample(127, position + 127 * speed) - PlayerInterpolator::Read(p.samples_basket, position + 127 * speed, PlayerInterpolator::Mix(speed))) < 0.01f);
                }
                if (!p.live_sinc_start_pending)
                {
                    assert(p.interpolation_mix == PlayerInterpolator::Mix(speed));
                    restored = true;
                    break;
                }
                recorder.Q_sample += 128;
            }
            assert(restored);
        }
    }
    LS_XY_lock = true;
    // Independent source-sequence oracle: the first 32 samples are an attack-only prefix.
    LS_buffer_dim = 16384;
    for (int i = 0; i < LS_buffer_dim; ++i)
    {
        memory[i] = static_cast<int16_t>(15000 * sin(i * 0.03));
    }
    const auto sources = [](int first, int span, int phase)
    {
        struct Sample { int tail, head, weight; };
        if (phase < 0)
        {
            return Sample{(first + ((phase + 32) % span + span) % span) % LS_buffer_dim, -1, 0};
        }
        const int index = 32 + phase % (span - 32);
        const int offset = index - (span - 32);
        return Sample{(first + index) % LS_buffer_dim, offset >= 0 ? (first + offset) % LS_buffer_dim : -1, offset};
    };
    const auto value_at = [&](int first, int span, int phase)
    {
        const auto r = sources(first, span, phase);
        return r.head < 0 ? memory[r.tail] : static_cast<int16_t>((static_cast<int32_t>(memory[r.tail]) * (31 - r.weight) + static_cast<int32_t>(memory[r.head]) * r.weight) / 31);
    };
    // Render the initial attack and several returns at slow, unity and fast pitch, with and without IIR.
    recorder.first_write_flag = false;
    recorder.writing = false;
    LS_state = 0;
    for (int first : {0, 16000})
    {
        for (int span : {5293, 16384})
        {
            for (float speed : {0.01f, 0.5f, 0.9999f, 1.0f, 1.5f, 35.0f})
            {
                for (bool filtered : {false, true})
                {
                    AudioPlayer p;
                    p.mode_player = LOOP_FWD;
                    p.pitch = speed;
                    p.pitch_note = 1;
                    p.tone_filter_level = filtered ? 0x81 : 0;
                    p.A_Flash_sample = p.a_first_sample = first;
                    p.B_Flash_sample = first + span - 1;
                    p.live_start_pending = true;
                    p.Noclick_ptr = nullptr;
                    p.delta_Noclick = 128;
                    const int period = span - 32;
                    // Directly cover exact endpoints, then exercise the complete render/filter pipeline.
                    int16_t seam[34];
                    assert(p.Fill_loop_samples(seam, 34, period - 32, first, p.B_Flash_sample, 32, LOOP_FWD, nullptr, period));
                    for (int i = 0; i < 34; ++i)
                    {
                        assert(seam[i] == value_at(first, span, period - 32 + i));
                    }
                    for (int packet = 0; packet < 14; ++packet)
                    {
                        if (packet == 3)
                        {
                            p.a_first_sample = first + span - 28.25f; // Enter a fractional seam even at low pitch.
                        }
                        p.Render();
                        const float phase = p.a_sample - first - 32;
                        const int left = p.interpolation_mix > 0 ? 3 : (p.antialias.mix > 0 ? p.antialias.history : 0);
                        const int right = p.interpolation_mix > 0 ? 4 : (p.antialias.mix > 0 ? 1 : 0);
                        const float distance = p.b_sample - p.a_sample;
                        const int count = left > 0 ? static_cast<int>(floorf(phase - floorf(phase) + left + distance)) + right + 1 : static_cast<int>(ceilf(phase + distance)) - static_cast<int>(floorf(phase)) + 1;
                        int read_phase = static_cast<int>(floorf(phase)) - left;
                        if (phase >= 0)
                        {
                            read_phase = (read_phase % period + period) % period;
                        }
                        for (int i = 0; i < count; ++i)
                        {
                            assert(p.samples_basket[i] == value_at(first, span, read_phase + i));
                        }
                        assert(p.a_first_sample == first + 32 + fmodf(phase + distance + speed, static_cast<float>(period)));
                        if (packet == 0 && speed == 1)
                        {
                            for (int i = 0; i < 128; ++i)
                            {
                                assert(p.block[i] == memory[(first + i) % LS_buffer_dim]);
                            }
                        }
                    }
                }
            }
        }
    }
    // First-fill checks inspect raw attack history and both sources if the requested window reaches a mix.
    for (int first : {0, 16000})
    {
        const int span = 5293;
        for (float speed : {0.01f, 0.5f, 0.9999f, 1.0f, 1.5f, 35.0f})
        {
            for (bool filtered : {false, true})
            {
                const int left = speed < 1 ? 3 : (filtered && speed > 1 ? PlayerAntialias::History(speed) : 0);
                const int right = speed < 1 ? 4 : (filtered && speed > 1 ? 1 : 0);
                const int count = left > 0 ? static_cast<int>(floorf(127 * speed)) + left + right + 1 : static_cast<int>(ceilf(127 * speed)) + 1;
                for (int q : {-1, 126, 127, 2000, 6000, 16382, 16383})
                {
                    AudioPlayer p;
                    p.mode_player = LOOP_FWD;
                    p.pitch = speed;
                    p.pitch_note = 1;
                    p.tone_filter_level = filtered ? 0x81 : 0;
                    p.A_Flash_sample = p.a_first_sample = first;
                    p.B_Flash_sample = first + span - 1;
                    p.live_start_pending = true;
                    recorder.first_write_flag = true;
                    recorder.Q_sample = q;
                    bool valid = true;
                    for (int i = 0; i < count; ++i)
                    {
                        const auto r = sources(first, span, -32 - left + i);
                        valid = valid && r.tail <= q && (r.head < 0 || r.head <= q);
                    }
                    AudioPlayer::live_unrecorded_notice = false;
                    p.Render();
                    assert(p.live_forward_end == !valid && AudioPlayer::live_unrecorded_notice == !valid);
                    if (!valid)
                    {
                        assert(p.state == IDLE_REQUEST && p.read_last == -1);
                    }
                    for (int phase : {0, span - 80, span - 50})
                    {
                        bool all_recorded = true;
                        bool contains = false;
                        for (int i = 0; i < count; ++i)
                        {
                            const auto r = sources(first, span, phase + i);
                            all_recorded = all_recorded && r.tail <= q && (r.head < 0 || r.head <= q);
                            contains = contains || r.tail == q || (r.head >= 0 && r.head == q);
                        }
                        assert(p.Live_loop_samples_recorded(phase, count, q) == all_recorded);
                        if (q >= 0)
                        {
                            assert(p.Live_loop_contains(phase, count, q) == contains);
                        }
                    }
                }
            }
        }
    }
    // Q in either endpoint triggers a ramp; restart geometry and pitch freeze follow the shortened period.
    recorder.first_write_flag = false;
    for (int first : {50, 16000})
    {
        const int span = 5293;
        const int period = span - 32;
        for (float speed : {0.01f, 0.5f, 0.9999f, 1.0f, 1.5f, 35.0f})
        {
            for (bool filtered : {false, true})
            {
                for (int q_offset : {5, span - 27})
                {
                    for (bool stop : {false, true})
                    {
                        AudioPlayer p;
                        p.mode_player = LOOP_FWD;
                        p.pitch = speed;
                        p.pitch_note = 1;
                        p.tone_filter_level = filtered ? 0x81 : 0;
                        p.A_Flash_sample = first;
                        p.B_Flash_sample = first + span - 1;
                        p.a_first_sample = first + span - 28.25f;
                        recorder.Q_sample = (first + q_offset) % LS_buffer_dim;
                        recorder.writing = true;
                        LS_state = REC;
                        p.Render();
                        assert(p.live_fade_out && p.live_restart_pending && p.block[127] == 0);
                        const float original_phase = p.a_sample - first - 32;
                        float expected = p.live_restart_sample;
                        if (speed < 1)
                        {
                            expected = first + 32 + fmodf(original_phase - 128 + period, period);
                        }
                        else if (speed == 1)
                        {
                            expected = p.a_sample;
                        }
                        recorder.Q_sample = (recorder.Q_sample + 128) % LS_buffer_dim;
                        if (speed > 1)
                        {
                            const int left = filtered ? PlayerAntialias::History(speed) : 0;
                            const int count = left > 0 ? left + static_cast<int>(floorf(127 * speed)) + 2 : static_cast<int>(ceilf(127 * speed)) + 1;
                            const int phase = (static_cast<int>(expected - first - 32) - left + period) % period;
                            for (int i = 0; i < count - 1; ++i)
                            {
                                const auto r = sources(first, span, phase + i);
                                assert(r.tail != recorder.Q_sample && r.head != recorder.Q_sample);
                            }
                        }
                        p.pitch_bend_wait = 1.1f;
                        p.pitch_bend_flag = true;
                        p.Apply_pitch_requests();
                        assert(p.pitch == speed && p.pitch_bend_flag);
                        recorder.writing = !stop;
                        LS_state = stop ? 0 : REC;
                        p.Render();
                        assert(p.live_fade_in && !p.live_restart_pending && p.a_sample == expected && p.block[0] == 0);
                        assert(p.pitch == speed);
                        p.Apply_pitch_requests();
                        assert(!p.pitch_bend_flag);
                    }
                }
            }
        }
    }
    // Exact final sample exclusion also applies to both endpoints of a mixed sample.
    recorder.writing = true;
    LS_state = REC;
    for (int offset : {4, 5, 6})
    {
        for (bool head : {false, true})
        {
            AudioPlayer p;
            p.mode_player = LOOP_FWD;
            p.pitch = 1;
            p.A_Flash_sample = 50;
            p.B_Flash_sample = 5342;
            p.a_sample = 50 + 5293 - 32 + 5 - 127;
            p.b_sample = p.a_sample + 127;
            recorder.Q_sample = head ? 50 + offset : 5342 - 31 + offset;
            p.Check_live_forward_boundary();
            assert(p.live_restart_pending == (offset < 5));
        }
    }
    // The initial 32-sample prefix is preserved by unity replay and reset on each new note.
    AudioPlayer attack;
    attack.mode_player = LOOP_FWD;
    attack.pitch = attack.pitch_note = 1;
    attack.A_Flash_sample = 50;
    attack.B_Flash_sample = 5342;
    attack.a_first_sample = 50;
    recorder.Q_sample = 60;
    attack.Render();
    assert(attack.live_restart_pending);
    recorder.Q_sample += 128;
    attack.Render();
    assert(attack.live_fade_in && attack.a_sample == 50);
    attack.live_restart_pending = true;
    attack.Edit_geometry(LOOP_FWD_REV, 50, 5342);
    attack.Prepare_live_forward_block();
    assert(!attack.live_restart_pending && !attack.live_fade_in);
    attack.mode_player = LOOP_FWD;
    attack.live_start_pending = true;
    recorder.writing = false;
    LS_state = 0;
    attack.Render();
    assert(attack.a_sample == 50 && attack.block[0] == memory[50]);
    LS_buffer_dim = 4096;
    // A rejected filtered attack must not poison the cached filter settings for a later note at the same pitch.
    AudioPlayer rejected_filter;
    rejected_filter.mode_player = LOOP_FWD;
    rejected_filter.pitch = rejected_filter.pitch_note = 1.5f;
    rejected_filter.tone_filter_level = 0x81;
    rejected_filter.live_start_pending = true;
    recorder.first_write_flag = true;
    recorder.Q_sample = 100;
    rejected_filter.Render();
    assert(rejected_filter.live_forward_end && rejected_filter.antialias.mix > 0);
    recorder.first_write_flag = false;
    rejected_filter.live_start_pending = true;
    rejected_filter.state = 1;
    rejected_filter.Render();
    assert(!rejected_filter.live_forward_end && rejected_filter.antialias.mix > 0);
    // First fill lands behind the predicted Q; an early STOP cannot expose never-recorded memory.
    for (bool early_stop : {false, true})
    {
        AudioPlayer p;
        p.mode_player = LOOP_FWD;
        p.pitch = p.pitch_note = 2;
        p.A_Flash_sample = 0;
        p.B_Flash_sample = 2000;
        p.a_first_sample = 900;
        recorder.first_write_flag = true;
        recorder.writing = true;
        recorder.Q_sample = 1000;
        LS_state = REC;
        p.Render();
        assert(p.live_restart_pending && p.live_restart_sample == 874);
        p.read_last = -1;
        recorder.writing = !early_stop;
        LS_state = early_stop ? 0 : REC;
        recorder.Q_sample += early_stop ? 0 : 128;
        p.Render();
        if (early_stop)
        {
            assert(p.live_forward_end && p.state == IDLE_REQUEST && p.read_last == -1);
        }
        else
        {
            assert(p.live_fade_in && !p.live_restart_pending && p.a_sample == 874);
            assert(p.read_last == 1128);
        }
    }
    // Full-cache STOP still completes the booked forward jump; geometry edits cancel it.
    recorder.first_write_flag = false;
    recorder.writing = true;
    recorder.Q_sample = 1000;
    LS_state = REC;
    AudioPlayer stopped_loop;
    stopped_loop.mode_player = LOOP_FWD;
    stopped_loop.pitch = stopped_loop.pitch_note = 2;
    stopped_loop.B_Flash_sample = 2000;
    stopped_loop.a_first_sample = 900;
    stopped_loop.Render();
    assert(stopped_loop.live_restart_pending);
    const float saved_loop = stopped_loop.live_restart_sample;
    recorder.writing = false;
    LS_state = 0;
    stopped_loop.Render();
    assert(stopped_loop.live_fade_in && stopped_loop.a_sample == saved_loop);
    stopped_loop.live_restart_pending = true;
    stopped_loop.Edit_geometry(ONCE_FWD, 0, 2000);
    stopped_loop.Prepare_live_forward_block();
    assert(!stopped_loop.live_restart_pending && !stopped_loop.live_fade_in);
    // Fractional replays need one extra sinc sample: reject if that last sample is still unrecorded.
    for (int q : {267, 268})
    {
        AudioPlayer p;
        p.mode_player = LOOP_FWD;
        p.pitch = p.pitch_note = 0.5f;
        p.B_Flash_sample = 500;
        p.live_restart_pending = true;
        p.live_restart_sample = 200.75f;
        recorder.first_write_flag = true;
        recorder.Q_sample = q;
        recorder.writing = false;
        LS_state = 0;
        AudioPlayer::live_unrecorded_notice = false;
        p.Render();
        assert(p.live_forward_end == (q == 267));
        assert(AudioPlayer::live_unrecorded_notice == (q == 267));
        if (q == 267)
        {
            assert(p.read_last == -1 && p.state == IDLE_REQUEST && !p.live_fade_in);
        }
        else
        {
            assert(p.live_fade_in && p.a_sample == 200.75f && p.read_last == 268);
        }
    }
    // At large cache indices, float rounding can add a source sample to the sinc window.
    LS_buffer_dim = 1764096;
    for (int q : {1000130, 1000131})
    {
        AudioPlayer p;
        p.mode_player = LOOP_FWD;
        p.pitch = p.pitch_note = 0.9999f;
        p.B_Flash_sample = LS_buffer_dim - 1;
        p.live_restart_pending = true;
        p.live_restart_sample = 1000000.0f;
        recorder.first_write_flag = true;
        recorder.Q_sample = q;
        p.Prepare_live_forward_block();
        assert(p.live_forward_end == (q == 1000130));
        assert(p.live_fade_in == (q == 1000131));
    }
    LS_buffer_dim = 4096;
    // Ordinary file loops retain their NoClick table.
    AudioPlayer file_loop;
    file_loop.LS_flag = false;
    int16_t blend[2] = {123, 456};
    int16_t loop_output[8];
    assert(file_loop.Fill_loop_samples(loop_output, 8, 0, 0, 9, 2, LOOP_FWD, blend, 8));
    for (int i = 0; i < 6; ++i)
    {
        assert(loop_output[i] == memory[i + 2]);
    }
    assert(loop_output[6] == 123 && loop_output[7] == 456);
    std::cout << "PASS: mobile/fixed starts, actual filtered endpoints, circular collisions, ramps, STOP, mode changes, first fill, attack-preserving 32-sample Live loop mix and collision ramps\n";
}
'''
signatures = [
    'int AudioPlayer::Live_loop_segment(int phase, int first, int last, int &source, int &blend_source) const',
    'bool AudioPlayer::Live_loop_contains(int phase, int count, int position) const',
    'bool AudioPlayer::Live_loop_samples_recorded(int phase, int count, int write_position) const',
    'int AudioPlayer::Loop_period(int first, int last, int crossfade, uint8_t mode) const',
    'bool AudioPlayer::Fill_loop_samples(int16_t *destination, int count, int phase, int first, int last, int crossfade, uint8_t mode, const int16_t *noclick, int period)',
    'void AudioPlayer::Loop_memory_harvest(void)',
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
    edit_start = source.index('            if (mode_player != mode_player_E || A_Flash_sample != A_Flash_sample_E')
    edit_end = source.index('            a_first_sample = a_first_sample_E;', edit_start)
    edit_geometry = 'void AudioPlayer::Edit_geometry(int mode_player_E, int A_Flash_sample_E, int B_Flash_sample_E)\n{\n' + source[edit_start:edit_end] + 'A_Flash_sample = A_Flash_sample_E; B_Flash_sample = B_Flash_sample_E;\n}\n'
    cpp.write_text(prefix + ''.join(method(source, s) for s in signatures) + pitch_requests + edit_geometry + checks, encoding='utf-8')
    lib = ROOT / 'lib/AudioPlayer'
    subprocess.run([compiler, '-std=c++20', '-O2', '-Wall', '-Wextra', '-Werror', '-I' + str(lib), str(cpp), str(lib / 'PlayerAntialias.cpp'), str(lib / 'PlayerInterpolator.cpp'), '-o', str(exe)], check=True, env=env)
    subprocess.run([str(exe)], check=True, env=env)
