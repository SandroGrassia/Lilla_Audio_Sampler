"""Exercise production Auto-Tune with a host FFT retaining the Teensy 50% overlap."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / 'src/main.cpp').read_text(encoding='utf-8')
feeder = re.search(r'^class AutoTuneInput : public AudioStream\n\{.*?^\};', source, re.M | re.S).group(0)
tune = re.search(r'^FLASHMEM const char \*S_Auto_tune_pitch\(int sound_id\)\n\{.*?^\}', source, re.M | re.S).group(0)

prefix = r'''
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>
#define FLASHMEM
#define DMAMEM
using std::max;
using std::abs;
constexpr double pi = 3.14159265358979323846;
constexpr int AUDIO_BLOCK_SAMPLES = 128;
constexpr float AUDIO_SAMPLE_RATE = 44100;
constexpr int LOOP_FWD = 2, LOOP_FWD_REV = 3, LOOP_REV_FWD = 4, LOOP_REV = 5;
template <typename T> T constrain(T value, T low, T high) { return std::clamp(value, low, high); }
void AudioNoInterrupts() {}
void AudioInterrupts() {}
uint32_t tick = 0;
uint32_t millis() { return tick; }
void yield();
struct audio_block_t { int16_t data[128]; };
int allocation_failures = 0;
void deliver(const audio_block_t *block);
class AudioStream
{
public:
    AudioStream(int, void *) {}
    virtual void update() = 0;
    audio_block_t *allocate()
    {
        static audio_block_t block;
        if (allocation_failures > 0)
        {
            --allocation_failures;
            return nullptr;
        }
        return &block;
    }
    void transmit(audio_block_t *block) { deliver(block); }
    void release(audio_block_t *) {}
};
struct HostFFT
{
    std::vector<int16_t> pending;
    std::array<int16_t, 1024> last_window{};
    std::array<float, 512> magnitudes{};
    bool ready = false;
    int frames = 0;
    bool available()
    {
        const bool result = ready;
        ready = false;
        return result;
    }
    float read(unsigned int bin) const { return magnitudes.at(bin); }
    void update()
    {
        if (pending.size() < 1024)
        {
            return;
        }
        std::copy_n(pending.begin(), 1024, last_window.begin());
        for (unsigned int bin = 0; bin < 512; ++bin)
        {
            double real = 0, imag = 0;
            for (unsigned int i = 0; i < 1024; ++i)
            {
                const double value = pending[i] * (0.5 - 0.5 * std::cos(2 * pi * i / 1024));
                real += value * std::cos(2 * pi * bin * i / 1024);
                imag -= value * std::sin(2 * pi * bin * i / 1024);
            }
            magnitudes[bin] = std::hypot(real, imag) / (1024 * 32768);
        }
        pending.erase(pending.begin(), pending.begin() + 512);
        ready = true;
        ++frames;
    }
} AutoTune_fft;
void deliver(const audio_block_t *block) { AutoTune_fft.pending.insert(AutoTune_fft.pending.end(), block->data, block->data + 128); }
enum Storage { Flash, Psram };
struct AudioFileSource { Storage storage = Psram; const int16_t *psram_ptr = nullptr; uint32_t samples = 0; };
struct SoundData { int mode = LOOP_FWD, file = 0, pitch = 0; uint32_t A = 0, B = 0; } Sound[1];
struct PresetData { bool use_Wavetable = false; int Noclick = 0; } Preset[1];
int Instrument_id = 0;
std::vector<int16_t> recording;
std::vector<int16_t> wavetable;
std::vector<int16_t> noclick;
AudioFileSource selected_source;
bool table_ok = true, flash_ok = true;
int flash_reads = 0;
struct Cache { AudioFileSource Get_source(int) { return selected_source; } } PatchCache_Manager;
struct AudioTables
{
    struct Pointers { int16_t *wavetable = nullptr; int16_t *noclick = nullptr; };
    Pointers Get_active_pointers(int, const PresetData &) { return table_ok ? Pointers{::wavetable.data(), ::noclick.data()} : Pointers{}; }
} Audio_tables;
struct LillaSerialFlashFile
{
    static bool Read_audio_samples(int, int16_t *out, uint32_t first, uint32_t count)
    {
        ++flash_reads;
        assert(first + count <= recording.size());
        if (flash_ok)
        {
            std::copy_n(recording.data() + first, count, out);
        }
        return flash_ok;
    }
};
'''

tests = r'''
void configure(unsigned int length, int amplitude = 20000, bool table = false, int mode = LOOP_FWD, int crossfade = 0)
{
    assert(AutoTune_input.Idle());
    Sound[0] = {mode, 0, 0, 0, length - 1};
    Preset[0] = {table, crossfade};
    recording.resize(length);
    for (unsigned int i = 0; i < length; ++i)
    {
        recording[i] = std::lround(amplitude * std::sin(2 * pi * i / length));
    }
    wavetable = recording;
    noclick.assign(crossfade, 123);
    selected_source = {Psram, recording.data(), length};
    table_ok = flash_ok = true;
}
void expect_error(const char *expected)
{
    const int before = Sound[0].pitch;
    const char *error = S_Auto_tune_pitch(0);
    assert(error != nullptr && std::strcmp(error, expected) == 0);
    assert(Sound[0].pitch == before);
}
void expect_frame(std::array<int16_t, 1024> expected)
{
    int32_t sum = 0, amplitude = 0;
    for (int16_t value : expected)
    {
        sum += value;
    }
    const int32_t mean = sum / 1024;
    for (int16_t value : expected)
    {
        amplitude = max(amplitude, abs(static_cast<int32_t>(value) - mean));
    }
    for (int16_t &value : expected)
    {
        value = (static_cast<int32_t>(value) - mean) * 30000 / amplitude;
    }
    assert(expected == AutoTune_fft.last_window);
}
int main()
{
    // Repeated calls on different periods must end with exactly the new 1024 samples.
    for (unsigned int length : {100u, 674u, 675u, 1000u, 100u})
    {
        configure(length, 20000, length <= 674);
        assert(S_Auto_tune_pitch(0) == nullptr);
        std::array<int16_t, 1024> expected;
        for (unsigned int i = 0; i < 1024; ++i)
        {
            expected[i] = recording[i % length];
        }
        expect_frame(expected);
        const double frequency = 44100.0 / length * std::pow(2.0, Sound[0].pitch / 192.0);
        const double midi = 69 + 12 * std::log2(frequency / 440);
        assert(std::abs(midi - std::round(midi)) * 100 <= 3.126);
    }
    configure(100, 12, true);
    assert(S_Auto_tune_pitch(0) == nullptr); // Quiet but periodic audio is usable.
    configure(100, 0, true);
    expect_error("AUTO-TUNE: NO SIGNAL");
    configure(100, 20000, true, 0);
    expect_error("AUTO-TUNE: SELECT LOOP MODE");
    configure(100, 20000, true);
    table_ok = false;
    expect_error("AUTO-TUNE: TABLES UNAVAILABLE");
    configure(1000);
    selected_source.storage = Flash;
    flash_ok = false;
    expect_error("AUTO-TUNE: READ FAILED");
    flash_ok = true;
    const int before_reads = flash_reads;
    assert(S_Auto_tune_pitch(0) == nullptr && flash_reads == before_reads + 1);

    // Verify the same forward/reverse NoClick period and ping-pong endpoints as playback.
    for (int mode : {LOOP_FWD, LOOP_REV, LOOP_FWD_REV, LOOP_REV_FWD})
    {
        const bool pingpong = mode == LOOP_FWD_REV || mode == LOOP_REV_FWD;
        configure(800, 20000, false, mode, pingpong ? 0 : 100);
        assert(S_Auto_tune_pitch(0) == nullptr);
        std::vector<int16_t> cycle;
        if (pingpong)
        {
            cycle = recording;
            for (int i = 798; i > 0; --i)
            {
                cycle.push_back(recording[i]);
            }
        }
        else if (mode == LOOP_FWD)
        {
            cycle.assign(recording.begin() + 100, recording.end() - 100);
            cycle.insert(cycle.end(), noclick.begin(), noclick.end());
        }
        else
        {
            cycle = noclick;
            cycle.insert(cycle.end(), recording.begin() + 100, recording.end() - 100);
            std::reverse(cycle.begin(), cycle.end());
        }
        std::array<int16_t, 1024> expected;
        for (unsigned int i = 0; i < 1024; ++i)
        {
            expected[i] = cycle[i % cycle.size()];
        }
        expect_frame(expected);
    }
    for (unsigned int length : {1024u, 4410u})
    {
        configure(length);
        assert(S_Auto_tune_pitch(0) == nullptr);
        std::array<int16_t, 1024> expected;
        std::copy_n(recording.end() - 1024, 1024, expected.begin());
        expect_frame(expected);
    }
    configure(100, 20000, true);
    allocation_failures = 40;
    expect_error("AUTO-TUNE: FFT TIMEOUT");
    assert(!AutoTune_input.Idle());
    assert(S_Auto_tune_pitch(0) == nullptr); // Drain a timed-out window before restarting.
    assert(AutoTune_input.Idle());
    std::cout << "PASS: short/quiet loops, repeat requests, 50% FFT overlap, playback periods, Flash/PSRAM, errors and timeout recovery\n";
}
'''

runtime = '\nAutoTuneInput AutoTune_input;\nvoid yield() { AutoTune_input.update(); AutoTune_fft.update(); tick += 3; }\n'
compiler = shutil.which('g++') or r'C:\msys64\ucrt64\bin\g++.exe'
with tempfile.TemporaryDirectory(prefix='lilla-autotune-') as directory:
    cpp = Path(directory) / 'autotune.cpp'
    exe = Path(directory) / ('autotune.exe' if os.name == 'nt' else 'autotune')
    cpp.write_text(prefix + feeder + runtime + tune + tests, encoding='utf-8', newline='\r\n')
    subprocess.run([compiler, '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    environment = os.environ.copy()
    environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
    subprocess.run([str(exe)], check=True, env=environment)
