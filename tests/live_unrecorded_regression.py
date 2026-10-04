"""Exercise the production live first-pass boundary and final output fade."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
player = (ROOT / "lib/AudioPlayer/AudioPlayer.cpp").read_text(encoding="utf-8")

def method(signature):
    return re.search(r"^" + re.escape(signature) + r"[^\n]*\n\{.*?^\}", player, re.M | re.S).group(0)

prefix = r"""
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
constexpr int AUDIO_BLOCK_SAMPLES = 128, ONCE_FWD = 0, IDLE_REQUEST = 2;
int LS_buffer_dim = 1024;
bool LS_XY_lock = false;
struct Recorder
{
    bool first_write_flag = true;
    bool writing = false;
    bool Is_writing() const { return writing; }
    int Q_sample = 255;
} recorder;
struct AudioPlayer
{
    inline static volatile bool live_unrecorded_notice = false;
    bool LS_flag = true, live_forward_empty = false;
    int A_Flash_sample = 0;
    int mode_player = ONCE_FWD, state = 1, reads = 0;
    float a_sample = 128, b_sample = 382, pitch = 2, a_first_sample = 0;
    int16_t samples_basket[8192] = {}, block[AUDIO_BLOCK_SAMPLES] = {}, live_forward_last_sample = 0;
    Recorder *LiveSampler_ptr = &recorder;
    void Read_samples(int16_t *destination, int first, int count)
    {
        assert(first >= 0 && first < LS_buffer_dim && count > 0);
        assert(!recorder.first_write_flag || first + count - 1 <= recorder.Q_sample);
        ++reads;
        for (int i = 0; i < count; ++i)
        {
            destination[i] = 12000;
        }
    }
    bool Harvest_live_continuous(void);
    bool Harvest_live_forward_end(void);
    void Fade_live_forward_end(void);
};
"""
checks = r"""
int main()
{
    AudioPlayer p;
    // A high-pitch block crosses the write head. Only recorded memory may be read.
    assert(p.Harvest_live_forward_end() && p.reads == 1 && !p.live_forward_empty);
    for (int i = 0; i < 255; ++i)
    {
        assert(p.samples_basket[i] == 12000);
    }
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; ++i)
    {
        p.block[i] = p.samples_basket[2 * i];
    }
    p.Fade_live_forward_end();
    assert(AudioPlayer::live_unrecorded_notice);
    assert(p.block[0] == 12000 && p.block[127] == 0 && p.state == IDLE_REQUEST);
    for (int i = 1; i < AUDIO_BLOCK_SAMPLES; ++i)
    {
        assert(p.block[i] <= p.block[i - 1] && p.block[i] >= 0);
        assert(p.block[i - 1] - p.block[i] <= 95);
    }

    // A new note in unwritten memory is silent; an existing voice has a continuous tail.
    p.a_sample = 300; p.b_sample = 554; p.reads = 0;
    assert(p.Harvest_live_forward_end() && p.live_forward_empty && p.reads == 0);
    p.Fade_live_forward_end();
    for (auto sample : p.block)
    {
        assert(sample == 0);
    }
    p.live_forward_last_sample = -9000;
    p.Fade_live_forward_end();
    assert(p.block[0] == -9000 && p.block[127] == 0);

    // Fractional interpolation must not fetch an unwritten neighbour.
    p.a_sample = 254.5f; p.b_sample = 256.5f; p.reads = 0;
    assert(p.Harvest_live_forward_end() && p.reads == 1);
    assert(p.samples_basket[0] == 12000 && p.samples_basket[2] == 12000);
    // Reaching the last recorded sample exactly remains valid.
    p.a_sample = 128; p.b_sample = 255;
    assert(!p.Harvest_live_forward_end());
    recorder.Q_sample = 511; p.b_sample = 382;
    assert(!p.Harvest_live_forward_end());

    // Reset, wrapped/negative coordinates, and first complete fill.
    recorder.Q_sample = -1;
    assert(p.Harvest_live_forward_end() && p.live_forward_empty);
    recorder.Q_sample = 255; p.a_sample = -1; p.b_sample = 253;
    assert(p.Harvest_live_forward_end() && p.live_forward_empty);
    p.a_sample = 1152; p.b_sample = 1406;
    assert(p.Harvest_live_forward_end() && !p.live_forward_empty);
    recorder.Q_sample = LS_buffer_dim - 1;
    assert(!p.Harvest_live_forward_end());
    recorder.Q_sample = 255; recorder.first_write_flag = false;
    assert(!p.Harvest_live_forward_end());

    // FIXED playback must stop at the same unrecorded boundary, including an empty start.
    recorder.first_write_flag = true; LS_XY_lock = true;
    assert(p.Harvest_live_forward_end() && !p.live_forward_empty);
    p.a_sample = 300; p.b_sample = 554;
    assert(p.Harvest_live_forward_end() && p.live_forward_empty);
    AudioPlayer::live_unrecorded_notice = false;
    p.Fade_live_forward_end();
    assert(AudioPlayer::live_unrecorded_notice && p.state == IDLE_REQUEST);
    p.a_sample = 0; p.b_sample = 127;
    assert(!p.Harvest_live_forward_end());

    // Other modes and ordinary files retain their existing paths.
    p.a_sample = 300; p.b_sample = 554;
    LS_XY_lock = false; p.mode_player = 2;
    assert(!p.Harvest_live_forward_end());
    p.mode_player = ONCE_FWD; p.LS_flag = false;
    assert(!p.Harvest_live_forward_end());
    // Sustained notes continue over multiple real mono/stereo buffer turns at different pitches.
    p.LS_flag = true;
    recorder.first_write_flag = false;
    recorder.writing = true;
    for (bool fixed : {false, true})
    {
        LS_XY_lock = fixed;
        for (int dimension : {882048, 1764096})
        {
            LS_buffer_dim = dimension;
            for (float speed : {0.5f, 0.6674199f, 1.0f, 2.0f})
            {
                p.pitch = speed;
                p.A_Flash_sample = dimension - 64;
                p.a_first_sample = p.A_Flash_sample;
                p.state = 1;
                int wraps = 0;
                for (int packet = 0; packet < 90000; ++packet)
                {
                    p.a_sample = p.a_first_sample;
                    p.b_sample = p.a_sample + p.pitch * 127;
                    assert(!p.Harvest_live_forward_end());
                    assert(p.Harvest_live_continuous());
                    assert(p.state == 1);
                    assert(p.a_first_sample >= p.A_Flash_sample && p.a_first_sample < p.A_Flash_sample + dimension);
                    float advance = p.a_first_sample - p.a_sample;
                    if (advance < 0)
                    {
                        advance += dimension;
                        ++wraps;
                    }
                    assert(std::abs(advance - p.pitch * 128) < 0.6f);
                }
                assert(wraps >= 3);
            }
        }
    }
    recorder.writing = false;
    assert(!p.Harvest_live_continuous());
    recorder.writing = true;
    p.mode_player = 2;
    assert(!p.Harvest_live_continuous());
    p.mode_player = ONCE_FWD;
    p.LS_flag = false;
    assert(!p.Harvest_live_continuous());
    std::cout << "PASS: live first-pass boundary, interpolation, fade, reset and scope\n";
}
"""
compiler = shutil.which("g++") or "C:/msys64/ucrt64/bin/g++.exe"
env = os.environ.copy()
env["PATH"] = str(Path(compiler).parent) + os.pathsep + env.get("PATH", "")
with tempfile.TemporaryDirectory(prefix="live-unrecorded-") as folder:
    cpp = Path(folder) / "test.cpp"
    exe = Path(folder) / "test.exe"
    cpp.write_text(prefix + method("bool AudioPlayer::Harvest_live_continuous(") + method("bool AudioPlayer::Harvest_live_forward_end(") + method("void AudioPlayer::Fade_live_forward_end(") + checks, encoding="utf-8")
    subprocess.run([compiler, "-std=c++20", "-Wall", "-Wextra", "-Werror", str(cpp), "-o", str(exe)], check=True, env=env)
    subprocess.run([str(exe)], check=True, env=env)
