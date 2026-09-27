"""Run the production RAW exporter against fault-injectable flash and SD mocks."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
main = (root / 'src/main.cpp').read_text(encoding='utf-8')
production = re.search(r'^bool DS_export_raw_to_SD\(void\)\n\{.*?^\}', main, re.M | re.S).group(0)
prefix = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#include <iostream>
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
bool card_ok, mkdir_ok, sync_ok, close_ok, directory_exists;
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
        if (++reads == fail_read || position + count > flash[path].size())
        {
            return 0;
        }
        memcpy(buffer, flash[path].data() + position, count);
        position += count;
        return count;
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
    uint32_t fileSize() { return files[path].size(); }
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
    bool exists(const char *path) { return std::string(path) == "/LILLARAW_EXPORT" ? directory_exists : files.count(path); }
    bool mkdir(const char *) { directory_exists = mkdir_ok; return mkdir_ok; }
    bool remove(const char *path) { return files.erase(path); }
} SD;
'''
tests = r'''
void reset(int bytes, bool stereo)
{
    files.clear();
    flash.clear();
    card_ok = mkdir_ok = sync_ok = close_ok = true;
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
int main()
{
    for (bool stereo : {false, true})
    {
        for (int bytes : {1, 256, 258, PACKET_DIM, PACKET_DIM + 258, 2 * PACKET_DIM})
        {
            reset(bytes, stereo);
            assert(DS_export_raw_to_SD());
            assert(files.size() == (stereo ? 2u : 1u));
            for (int channel = 0; channel < (stereo ? 2 : 1); ++channel)
            {
                const auto &actual = files[stereo ? (channel ? "/LILLARAW_EXPORT/0R.raw" : "/LILLARAW_EXPORT/0L.raw") : "/LILLARAW_EXPORT/0M.raw"];
                assert(actual.size() == static_cast<size_t>(bytes));
                for (int offset = 0; offset < bytes; ++offset)
                {
                    const int packet = 3 + offset / PACKET_DIM * (stereo ? 2 : 1) + channel;
                    assert(actual[offset] == flash[name_packet[packet]][offset % PACKET_DIM]);
                }
            }
        }
    }
    for (int fault = 0; fault < 10; ++fault)
    {
        reset(258, true);
        switch (fault)
        {
            case 0: card_ok = false; break;
            case 1: mkdir_ok = false; break;
            case 2: fail_open = 1; break;
            case 3: fail_open = 2; break;
            case 4: fail_read = 3; break;
            case 5: fail_write = 3; break;
            case 6: sync_ok = false; break;
            case 7: close_ok = false; break;
            case 8: flash.erase(name_packet[4]); break;
            case 9: Recording[0].bytes = PACKET_DIM + 1; break;
        }
        assert(!DS_export_raw_to_SD());
        assert(files.empty());
    }
    reset(256, false);
    files["/LILLARAW_EXPORT/0M.raw"] = {42};
    assert(DS_export_raw_to_SD());
    assert(files["/LILLARAW_EXPORT/0M.raw"] == std::vector<byte>{42});
    assert(files.count("/LILLARAW_EXPORT/1M.raw"));
    std::cout << "RAW export regression passed: exact mono/stereo bytes, SD/flash failures, rollback, existing files\n";
}
'''
compiler = shutil.which('g++') or r'C:\msys64\ucrt64\bin\g++.exe'
with tempfile.TemporaryDirectory(prefix='lilla-raw-export-') as directory:
    cpp = Path(directory) / 'export.cpp'
    exe = Path(directory) / ('export.exe' if os.name == 'nt' else 'export')
    cpp.write_text(prefix + production + tests, encoding='utf-8', newline='\r\n')
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    environment = os.environ.copy()
    environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
    subprocess.run([str(exe)], check=True, env=environment)
