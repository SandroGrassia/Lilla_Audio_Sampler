"""Exercise the production recorder and completion paths across attack, fade and packet boundaries."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
header = (root / 'lib/StereoSampler/StereoSampler.h').read_text(encoding='utf-8')
source = (root / 'lib/StereoSampler/StereoSampler.cpp').read_text(encoding='utf-8')
main = (root / 'src/main.cpp').read_text(encoding='utf-8')
production = re.sub(r'^#(?:include|pragma).*$', '', header + '\n' + source, flags=re.M)
completion = re.search(r'                if \(Recording\[recording\]\.packets == 0\).*?                DS_update_recordings\(\);', main, re.S).group(0)
assert main.count('DirectSampler.Stop_and_wait();') == 4
assert 'DirectSampler.Book_stop();' not in main
assert 'Was_cancelled' not in main + header + source

prefix = r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <vector>
using byte = uint8_t;
constexpr int AUDIO_BLOCK_SAMPLES = 128, PACKET_DIM = 65536, PACKET_BLOCKS = 256;
struct Entry { int first_packet = 0, packets = 0; bool consistent = true; } Recording[30];
int VFS_FAT_table[1024], recording = 0, spi_users = 0, writes = 0, saves = 0, startup_waits = 0;
bool active = false;
int available_mask = 3, releases[2] = {}, received[2] = {};
byte save_error = 0;
std::vector<std::vector<int16_t>> captured;
struct audio_block_t { int16_t data[AUDIO_BLOCK_SAMPLES] = {}; };
struct AudioStream {
    audio_block_t block[2];
    AudioStream(int, audio_block_t**) {}
    virtual void update() = 0;
    audio_block_t* receiveReadOnly(int channel)
    {
        if ((available_mask & (1 << channel)) == 0)
        {
            return nullptr;
        }
        ++received[channel];
        std::fill_n(block[channel].data, AUDIO_BLOCK_SAMPLES, channel == 0 ? 3000 : 1000);
        return &block[channel];
    }
    void release(audio_block_t* value)
    {
        assert(value != nullptr);
        const int channel = value == &block[0] ? 0 : 1;
        assert(value == &block[channel]);
        assert(++releases[channel] <= received[channel]);
    }
};
struct LillaSerialFlashFile {
    int pos = 0;
    bool opened = false;
    void packet_fast_open(int) { assert(!opened); pos = 0; opened = true; }
    void write(const void* data, int bytes)
    {
        assert(opened);
        pos += bytes;
        ++writes;
        captured.emplace_back(bytes / sizeof(int16_t));
        memcpy(captured.back().data(), data, bytes);
    }
    int position() { return pos; }
    void close() { opened = false; }
};
struct LillaFRAM_2x512 { static constexpr byte ERROR_0 = 0; };
struct ArchivingManager {
    byte Save_DS_Recording(int id) { assert(!Recording[id].consistent || !active); ++saves; return save_error; }
} Archive;
struct SerialStub { template<class T> void println(T) {} } Serial;
#define F(x) x
void AudioStartUsingSPI() { ++spi_users; active = true; }
void AudioStopUsingSPI() { --spi_users; active = false; }
void delayMicroseconds(int);
void delay(int);
'''
support = r'''
StereoSampler DirectSampler(Archive);
void delayMicroseconds(int) { DirectSampler.update(); }
void delay(int milliseconds)
{
    assert(milliseconds == 20 && DirectSampler.Is_recording());
    ++startup_waits;
    // A 20 ms interval always includes at least six 128-sample audio blocks at 44.1 kHz.
    for (int i = 0; i < 6; ++i)
    {
        DirectSampler.update();
    }
}
void Require_FRAM(byte result) { assert(result == 0); }
int DS_get_next_Recording(int) { return -1; }
void DS_read_Recording(int) { assert(!DirectSampler.Is_recording()); }
void DS_update_recordings() {}
'''
checks = r'''
int main()
{
    for (bool stereo : {false, true})
    {
        for (int blocks : {0, 3, 248, 250})
        {
            recording = 0;
            DirectSampler.Stop_and_wait(); // An idle stop must not affect the next take.
            const int previous_waits = startup_waits;
            const int initial_writes = writes;
            DirectSampler.Start(0, 7, recording, stereo);
            assert(startup_waits == previous_waits + 1 && writes == initial_writes + 6 * (stereo ? 2 : 1));
            for (int i = 0; i < blocks; ++i)
            {
                DirectSampler.update();
            }
            const int previous_writes = writes;
            DirectSampler.Stop_and_wait();
            assert(!DirectSampler.Is_recording() && spi_users == 0);
            assert(writes == previous_writes + 3 * (stereo ? 2 : 1));
            finish();
            assert(Recording[0].consistent && Recording[0].packets == (blocks >= 248 ? 2 : 1));
        }
        recording = 0;
        DirectSampler.Start(0, 7, recording, stereo);
        for (int i = 0; i < 3; ++i)
        {
            DirectSampler.update();
        }
        DirectSampler.Stop_and_wait();
        const int writes_after_stop = writes;
        DirectSampler.Stop_and_wait();
        assert(writes == writes_after_stop);
        assert(!DirectSampler.Is_recording());
        DirectSampler.Start(0, stereo ? 1 : 0, recording, stereo);
        for (int i = 0; i < 260 && DirectSampler.Is_recording(); ++i)
        {
            DirectSampler.update();
        }
        assert(!DirectSampler.Is_recording() && spi_users == 0);
        finish();
    }
    for (bool stereo : {false, true})
    {
        for (int mask = 0; mask < 4; ++mask)
        {
            available_mask = mask;
            captured.clear();
            const int channels = stereo ? 2 : 1;
            const int left = (mask & 1) != 0 ? 3000 : 0;
            const int right = (mask & 2) != 0 ? 1000 : 0;
            DirectSampler.Start(0, 7, 0, stereo);
            assert(DirectSampler.Missing_blocks_left() == ((mask & 1) != 0 ? 0u : 6u));
            assert(DirectSampler.Missing_blocks_right() == ((mask & 2) != 0 ? 0u : 6u));
            DirectSampler.update();
            DirectSampler.Stop_and_wait();
            assert(captured.size() == static_cast<size_t>(10 * channels));
            for (int block = 0; block < 10; ++block)
            {
                for (int channel = 0; channel < channels; ++channel)
                {
                    const int base = stereo ? (channel == 0 ? left : right) : (left + right) / 2;
                    for (int sample = 0; sample < AUDIO_BLOCK_SAMPLES; ++sample)
                    {
                        const int index = block * AUDIO_BLOCK_SAMPLES + sample;
                        const double gain = block < 3 ? index / 384.0 : (block >= 7 ? (384 - (block - 7) * 128 - sample) / 384.0 : 1.0);
                        const int expected = static_cast<int16_t>(base * gain);
                        const int actual = captured[block * channels + channel][sample];
                        assert(std::abs(actual - expected) <= (gain == 1.0 || base == 0 ? 0 : 1)); // Allow one LSB from the production float ramp's rounding.
                    }
                }
            }
            assert(DirectSampler.Missing_blocks_left() == ((mask & 1) != 0 ? 0u : 10u));
            assert(DirectSampler.Missing_blocks_right() == ((mask & 2) != 0 ? 0u : 10u));
            assert(releases[0] == received[0] && releases[1] == received[1]);
            DirectSampler.Start(0, stereo ? 1 : 0, 0, stereo);
            for (int i = 0; i < 260 && DirectSampler.Is_recording(); ++i)
            {
                DirectSampler.update();
            }
            assert(!DirectSampler.Is_recording() && spi_users == 0);
            save_error = 1;
            DirectSampler.Start(0, 7, 0, stereo);
            assert(!DirectSampler.Is_recording() && DirectSampler.Storage_error() == 1);
            assert(releases[0] == received[0] && releases[1] == received[1]);
            save_error = 0;
        }
    }
    std::cout << "PASS: mono/stereo capture, missing L/R/both, exact samples and ramps, counters, packet rollover, manual/automatic stop and error releases\n";
}
'''
compiler = shutil.which('g++') or r'C:\msys64\ucrt64\bin\g++.exe'
environment = os.environ.copy()
environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
with tempfile.TemporaryDirectory(prefix='lilla-sampler-stop-') as directory:
    cpp = Path(directory) / 'test.cpp'
    exe = Path(directory) / 'test.exe'
    cpp.write_text(prefix + production + support + '\nvoid finish()\n{\n' + completion + '\n}\n' + checks, encoding='utf-8', newline='\r\n')
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True, env=environment)
    subprocess.run([str(exe)], check=True, env=environment)
