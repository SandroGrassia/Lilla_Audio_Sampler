"""Verify production mono/stereo WAV output, PCM bytes, and SD failure cleanup."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile
import wave

root = Path(__file__).resolve().parents[1]
main = (root / 'src/main.cpp').read_text(encoding='utf-8')
production = re.search(r'^bool DS_export_wav_to_SD\(void\)\n\{.*?^\}', main, re.M | re.S).group(0)
prefix = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
using byte = uint8_t;
constexpr int PACKET_DIM = 65536, VFS_PACKETS_MAX = 512, BUILTIN_SDCARD = 0;
constexpr int O_WRONLY = 1, O_CREAT = 2, O_EXCL = 4;
#define F(x) x
struct Entry { int first_packet, packets, bytes; bool stereo; };
Entry Recording[1];
int recording = 0;
const char *name_packet[512];
#include <cstdio>
#include <cstdint>
constexpr int NAME_PACKET_SIZE = 10;
const char *Get_packet_name(uint16_t id, char (&name)[NAME_PACKET_SIZE])
{
    snprintf(name, sizeof(name), "%s", name_packet[id]);
    return name;
}

std::string packet_names[512];
std::map<std::string, std::vector<byte>> flash, files;
bool card_ok, mkdir_ok, sync_ok, close_ok, size_ok, directory_exists;
int fail_open, fail_read, fail_write, opens, reads, writes;
struct Logger
{
    void print(const char *) {}
    void println(const char *) {}
} Serial;
struct SerialFlashFile
{
    std::string path;
    size_t position = 0;
    explicit operator bool() const { return flash.count(path); }
    uint32_t read(byte *buffer, uint32_t count)
    {
        if (position + count > flash[path].size())
        {
            return 0;
        }
        const uint32_t received = ++reads == fail_read ? count - 1 : count;
        memcpy(buffer, flash[path].data() + position, received);
        position += received;
        return received;
    }
    void close() {}
};
struct Flash
{
    SerialFlashFile open(const char *path) { return {path}; }
} SerialFlash;
struct FsFile
{
    std::string path;
    explicit operator bool() const { return !path.empty(); }
    size_t write(const byte *buffer, size_t count)
    {
        const size_t written = ++writes == fail_write ? count - 1 : count;
        files[path].insert(files[path].end(), buffer, buffer + written);
        return written;
    }
    bool sync() { return sync_ok; }
    uint32_t fileSize() { return files[path].size() + (size_ok ? 0 : 1); }
    bool close() { return close_ok; }
};
struct Card
{
    struct Native
    {
        FsFile open(const char *path, int)
        {
            if (++opens == fail_open || files.count(path))
            {
                return {};
            }
            files[path] = {};
            return {path};
        }
    } sdfs;
    bool begin(int) { return card_ok; }
    bool exists(const char *path) { return std::string(path) == "/LILLAWAV_EXPORT" ? directory_exists : files.count(path); }
    bool mkdir(const char *) { directory_exists = mkdir_ok; return mkdir_ok; }
    bool remove(const char *path) { return files.erase(path); }
} SD;
'''
tests = r'''
void reset(int bytes, bool stereo)
{
    files.clear();
    flash.clear();
    card_ok = mkdir_ok = sync_ok = close_ok = size_ok = true;
    directory_exists = false;
    fail_open = fail_read = fail_write = -1;
    opens = reads = writes = 0;
    Recording[0] = {3, (bytes + PACKET_DIM - 1) / PACKET_DIM, bytes, stereo};
    for (int i = 0; i < 512; ++i)
    {
        packet_names[i] = std::to_string(i);
        name_packet[i] = packet_names[i].c_str();
    }
    for (int i = 0; i < Recording[0].packets * (stereo ? 2 : 1); ++i)
    {
        auto &data = flash[name_packet[3 + i]];
        data.resize(PACKET_DIM);
        for (int j = 0; j < PACKET_DIM; ++j)
        {
            data[j] = static_cast<byte>(i * 17 + j);
        }
    }
}
uint32_t little_endian(const std::vector<byte> &bytes, size_t offset, int count)
{
    uint32_t result = 0;
    for (int i = 0; i < count; ++i)
    {
        result |= static_cast<uint32_t>(bytes[offset + i]) << (8 * i);
    }
    return result;
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    for (bool stereo : {false, true})
    {
        const int channels = stereo ? 2 : 1;
        for (int bytes : {2, 254, 256, 258, PACKET_DIM - 2, PACKET_DIM, PACKET_DIM + 2, PACKET_DIM + 258, 2 * PACKET_DIM})
        {
            reset(bytes, stereo);
            assert(DS_export_wav_to_SD());
            assert(files.size() == 1);
            const auto &actual = files.at(stereo ? "/LILLAWAV_EXPORT/0S.wav" : "/LILLAWAV_EXPORT/0M.wav");
            assert(actual.size() == 44 + static_cast<size_t>(bytes) * channels);
            assert(memcmp(actual.data(), "RIFF", 4) == 0);
            assert(little_endian(actual, 4, 4) == actual.size() - 8);
            assert(memcmp(actual.data() + 8, "WAVEfmt ", 8) == 0);
            assert(little_endian(actual, 16, 4) == 16);
            assert(little_endian(actual, 20, 2) == 1);
            assert(little_endian(actual, 22, 2) == static_cast<uint32_t>(channels));
            assert(little_endian(actual, 24, 4) == 44100);
            assert(little_endian(actual, 28, 4) == 44100u * channels * 2);
            assert(little_endian(actual, 32, 2) == 2u * channels);
            assert(little_endian(actual, 34, 2) == 16);
            assert(memcmp(actual.data() + 36, "data", 4) == 0);
            assert(little_endian(actual, 40, 4) == static_cast<uint32_t>(bytes) * channels);
            for (int frame = 0; frame < bytes / 2; ++frame)
            {
                for (int channel = 0; channel < channels; ++channel)
                {
                    for (int sample_byte = 0; sample_byte < 2; ++sample_byte)
                    {
                        const int offset = frame * 2 + sample_byte;
                        const int packet = 3 + offset / PACKET_DIM * channels + channel;
                        assert(actual[44 + (frame * channels + channel) * 2 + sample_byte] == flash[name_packet[packet]][offset % PACKET_DIM]);
                    }
                }
            }
            std::ofstream fixture(std::string(argv[1]) + "/" + std::to_string(channels) + "_" + std::to_string(bytes) + ".wav", std::ios::binary);
            fixture.write(reinterpret_cast<const char *>(actual.data()), actual.size());
            assert(fixture.good());
        }
        reset(258, stereo);
        assert(DS_export_wav_to_SD());
        const int total_reads = reads;
        const int total_writes = writes;
        for (int read = 1; read <= total_reads; ++read)
        {
            reset(258, stereo);
            fail_read = read;
            assert(!DS_export_wav_to_SD());
            assert(files.empty());
        }
        for (int write = 1; write <= total_writes; ++write)
        {
            reset(258, stereo);
            fail_write = write;
            assert(!DS_export_wav_to_SD());
            assert(files.empty());
        }
    }
    for (int fault = 0; fault < 14; ++fault)
    {
        reset(PACKET_DIM + 258, true);
        switch (fault)
        {
            case 0: card_ok = false; break;
            case 1: mkdir_ok = false; break;
            case 2: fail_open = 1; break;
            case 3: size_ok = false; break;
            case 4: flash.erase(name_packet[5]); break;
            case 5: flash.erase(name_packet[6]); break;
            case 6: sync_ok = false; break;
            case 7: close_ok = false; break;
            case 8: flash.erase(name_packet[4]); break;
            case 9: Recording[0].bytes = PACKET_DIM + 257; break;
            case 10: Recording[0].bytes = 2 * PACKET_DIM + 2; break;
            case 11: Recording[0].first_packet = VFS_PACKETS_MAX - 1; break;
            case 12: flash.erase(name_packet[3]); break;
            case 13: flash[name_packet[6]].resize(256); break;
        }
        assert(!DS_export_wav_to_SD());
        assert(files.empty());
    }
    for (bool stereo : {false, true})
    {
        reset(256, stereo);
        files["/LILLAWAV_EXPORT/0M.wav"] = {42};
        files["/LILLAWAV_EXPORT/1S.wav"] = {43};
        assert(DS_export_wav_to_SD());
        assert(files["/LILLAWAV_EXPORT/0M.wav"] == std::vector<byte>{42});
        assert(files["/LILLAWAV_EXPORT/1S.wav"] == std::vector<byte>{43});
        assert(files.count(stereo ? "/LILLAWAV_EXPORT/2S.wav" : "/LILLAWAV_EXPORT/2M.wav"));
    }
    std::cout << "WAV export regression passed: PCM headers, mono/stereo samples, packet boundaries, failure cleanup, existing files\n";
}
'''
compiler = shutil.which('g++') or r'C:\msys64\ucrt64\bin\g++.exe'
with tempfile.TemporaryDirectory(prefix='lilla-wav-export-') as directory:
    cpp = Path(directory) / 'export.cpp'
    exe = Path(directory) / ('export.exe' if os.name == 'nt' else 'export')
    cpp.write_text(prefix + production + tests, encoding='utf-8', newline='\r\n')
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    environment = os.environ.copy()
    environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
    subprocess.run([str(exe), directory], check=True, env=environment)
    fixtures = list(Path(directory).glob('*.wav'))
    assert len(fixtures) == 18
    for fixture in fixtures:
        channels, source_bytes = map(int, fixture.stem.split('_'))
        with wave.open(str(fixture), 'rb') as audio:
            assert audio.getnchannels() == channels
            assert audio.getsampwidth() == 2
            assert audio.getframerate() == 44100
            assert audio.getnframes() == source_bytes // 2
            assert audio.getcomptype() == 'NONE'
            assert len(audio.readframes(audio.getnframes())) == source_bytes * channels
    print('PASS: all 18 exported WAV fixtures read by Python wave')
