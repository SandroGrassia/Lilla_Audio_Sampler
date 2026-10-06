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
completion = re.search(r'                if \(DirectSampler.Was_cancelled\(\).*?                DS_update_recordings\(\);', main, re.S).group(0)
assert main.count('DirectSampler.Stop_and_wait();') == 4
assert 'DirectSampler.Book_stop();' not in main
assert main.count('if (DirectSampler.Was_cancelled() && !Recording[recording].consistent)') == 5

prefix = r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
using byte = uint8_t;
constexpr int AUDIO_BLOCK_SAMPLES = 128, PACKET_DIM = 65536, PACKET_BLOCKS = 256;
struct Entry { int first_packet = 0, packets = 0; bool consistent = true; } Recording[30];
int VFS_FAT_table[1024], recording = 0, spi_users = 0, writes = 0, erases = 0, saves = 0;
bool active = false;
struct audio_block_t { int16_t data[AUDIO_BLOCK_SAMPLES] = {}; };
struct AudioStream {
    audio_block_t block;
    AudioStream(int, audio_block_t**) {}
    virtual void update() = 0;
    audio_block_t* receiveReadOnly(int) { return &block; }
    void release(audio_block_t*) {}
};
struct LillaSerialFlashFile {
    int pos = 0;
    bool opened = false;
    void packet_fast_open(int) { assert(!opened); pos = 0; opened = true; }
    void write(byte*, int bytes) { assert(opened); pos += bytes; ++writes; }
    int position() { return pos; }
    void close() { opened = false; }
};
struct LillaFRAM_2x512 { static constexpr byte ERROR_0 = 0; };
struct ArchivingManager {
    byte Save_DS_Recording(int id) { assert(!Recording[id].consistent || !active); ++saves; return 0; }
} Archive;
struct SerialStub { template<class T> void println(T) {} } Serial;
#define F(x) x
void AudioStartUsingSPI() { ++spi_users; active = true; }
void AudioStopUsingSPI() { --spi_users; active = false; }
void delayMicroseconds(int);
'''
support = r'''
StereoSampler DirectSampler(Archive);
void delayMicroseconds(int) { DirectSampler.update(); }
void Require_FRAM(byte result) { assert(result == 0); }
void Require_VFS(bool result) { assert(result); }
void P_Invalidate_recording_cache(int) { assert(!DirectSampler.Is_recording()); }
bool VFS_Clean_up_VFS()
{
    assert(!DirectSampler.Is_recording() && DirectSampler.Was_cancelled());
    assert(!Recording[recording].consistent);
    erases += Recording[recording].packets;
    Recording[recording] = {};
    return true;
}
int DS_get_next_Recording(int) { return -1; }
void DS_read_Recording(int) { assert(!DirectSampler.Is_recording()); }
void DS_update_recordings() {}
'''
checks = r'''
int main()
{
    for (bool stereo : {false, true})
    {
        for (int blocks : {0, 1, 2})
        {
            recording = 0;
            DirectSampler.Start(0, 7, recording, stereo);
            for (int i = 0; i < blocks; ++i)
            {
                DirectSampler.update();
            }
            const int previous_writes = writes;
            const int previous_erases = erases;
            DirectSampler.Stop_and_wait();
            assert(!DirectSampler.Is_recording() && DirectSampler.Was_cancelled());
            assert(writes == previous_writes && spi_users == 0);
            finish();
            assert(recording == -1 && Recording[0].packets == 0);
            assert(erases == previous_erases + (blocks > 0 ? 1 : 0));
        }
        for (int blocks : {3, 254, 256})
        {
            recording = 0;
            DirectSampler.Book_stop(); // A stale request must not affect the next take.
            DirectSampler.Start(0, 7, recording, stereo);
            assert(!DirectSampler.Was_cancelled());
            for (int i = 0; i < blocks; ++i)
            {
                DirectSampler.update();
            }
            const int previous_writes = writes;
            DirectSampler.Stop_and_wait();
            assert(!DirectSampler.Was_cancelled() && spi_users == 0);
            assert(writes == previous_writes + 3 * (stereo ? 2 : 1));
            finish();
            assert(Recording[0].consistent && Recording[0].packets == (blocks >= 254 ? 2 : 1));
        }
        recording = 0;
        DirectSampler.Start(0, 7, recording, stereo);
        for (int i = 0; i < 3; ++i)
        {
            DirectSampler.update();
        }
        for (int i = 0; i < 3; ++i)
        {
            DirectSampler.Book_stop(); // Repeated requests must not restart the fade.
            DirectSampler.update();
        }
        assert(!DirectSampler.Is_recording() && !DirectSampler.Was_cancelled());
        DirectSampler.Start(0, stereo ? 1 : 0, recording, stereo);
        for (int i = 0; i < 260 && DirectSampler.Is_recording(); ++i)
        {
            DirectSampler.update();
        }
        assert(!DirectSampler.Is_recording() && !DirectSampler.Was_cancelled() && spi_users == 0);
        finish();
    }
    std::cout << "PASS: mono/stereo attack cancellation, cleanup, stale requests, three-block fade, packet rollover, repeated stop and automatic stop\n";
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
