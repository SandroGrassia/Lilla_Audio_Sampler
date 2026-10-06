"""Verify production filename resolution and Flash inventory after a library replacement."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile
from file_registry_fixture import ROOT, REGISTRY_FIXTURE

prefix = r'''
#include <map>
#include <string>
constexpr int NAME_FILE_SIZE = 36, NAME_PACKET_SIZE = 10;
constexpr int FIRST_RECORDING_FILE = 260, FIRST_LIVE_SAMPLING_FILE = 320, RAW_FILES = 323, PACKETS = 4;
using elapsedMicros = int;
struct SerialStub
{
    template<class T> void print(T) {}
    template<class T> void println(T) {}
} Serial;
struct SerialFlashFile
{
    uint32_t address = 0, length = 0, offset = 0;
    uint16_t dirindex = 0;
    operator bool() { return address != 0; }
    uint32_t getFlashAddress() { return address; }
    uint32_t size() { return length; }
    void seek(uint32_t value) { offset = value; }
    uint32_t read(void *buffer, uint32_t bytes)
    {
        assert(address != 0);
        const uint32_t count = offset >= length ? 0 : std::min(bytes, length - offset);
        memset(buffer, 0x5a, count);
        offset += count;
        return count;
    }
    void close() {}
};
struct FlashStub
{
    std::map<std::string, SerialFlashFile> files;
    bool exists(const char *name) { return files.count(name) != 0; }
    SerialFlashFile open(const char *name)
    {
        const auto it = files.find(name);
        return it == files.end() ? SerialFlashFile{} : it->second;
    }
} SerialFlash;
const int16_t zeroraw[43996] = {111, -222, 333};
constexpr int RECORDINGS = 30, PACKET_DIM = 65536;
struct VFS_Recording { int first_packet = 0, packets = 0, bytes = 0; bool stereo = false; } Recording[RECORDINGS];
const int *Capture_find(int) { return nullptr; }
bool Capture_read(int, int16_t *, int, int) { return false; }
'''
methods = ''
for path, name in [('lib/SharedElements/SharedElements.cpp', 'Get_file_name'), ('lib/SharedVFS/SharedVFS.cpp', 'Get_packet_name')]:
    source = (ROOT / path).read_text(encoding='utf-8')
    methods += re.search(r'^const char \*' + name + r'\([^\n]+\)\n\{.*?^\}', source, re.M | re.S).group(0) + '\n'
header = (ROOT / 'lib/LillaSerialFlash/LillaSerialFlash.h').read_text(encoding='utf-8')
methods += re.sub(r'^#(?:include|pragma).*$', '', header, flags=re.M)
source = (ROOT / 'lib/LillaSerialFlash/LillaSerialFlash.cpp').read_text(encoding='utf-8')
methods += source[source.index('void LillaSerialFlashFile::fast_open'):source.index('void FlashFileRegisterParser::Read_all_file_data')]
methods += source[source.index('void FlashFileRegisterParser::Read_all_file_data'):source.index('/*\n** On-chip')]
checks = r'''
int main()
{
    reset_registry();
    assert(FileNameRegistry::Add("7.raw") == 1);
    assert(FileNameRegistry::Add("kick.raw") == 2);
    assert(FileNameRegistry::Save());
    SerialFlash.files = {{"0.raw", {100, 88000}}, {"7.raw", {100000, 2000}}, {"kick.raw", {102000, 4000}}, {"P0.raw", {120000, 65536}}};
    FlashFileRegisterParser::Read_all_file_data();
    assert(FlashFileRegisterParser::address(1) == 100000);
    assert(FlashFileRegisterParser::length(2) == 4000);
    assert(FlashFileRegisterParser::length(7) == 0); // An unassigned ID must not alias the numeric basename.
    assert(FlashFileRegisterParser::length(RAW_FILES) == 65536);
    assert(!FileNameRegistry::Numeric_available(7));
    SerialFlash.files.erase("kick.raw");
    FlashFileRegisterParser::Read_all_file_data();
    assert(FlashFileRegisterParser::address(2) == 0 && FlashFileRegisterParser::length(2) == 0);
    assert(FileNameRegistry::Find("kick.raw") == 2);
    SerialFlash.files["kick.raw"] = {200000, 6000};
    assert(FileNameRegistry::Load());
    FlashFileRegisterParser::Read_all_file_data();
    assert(FlashFileRegisterParser::address(2) == 200000 && FlashFileRegisterParser::length(2) == 6000);
    assert(!FlashFileRegisterParser::Zero_from_firmware());
    SerialFlash.files.erase("0.raw");
    FlashFileRegisterParser::Read_all_file_data();
    assert(FlashFileRegisterParser::Zero_from_firmware() && FlashFileRegisterParser::length(0) == sizeof(zeroraw));
    assert(SerialFlash.files.count("0.raw") == 0);
    LillaSerialFlashFile reader;
    reader.fast_open(0);
    assert(reader && reader.size() == sizeof(zeroraw) && reader.getFlashAddress() == 0);
    int16_t samples[4] = {};
    assert(reader.read(samples, 6) == 6 && memcmp(samples, zeroraw, 6) == 0);
    reader.seek(sizeof(zeroraw) - 2);
    assert(reader.read(samples, sizeof(samples)) == 2 && reader.read(samples, 2) == 0);
    reader.seek(UINT32_MAX);
    assert(reader.read(samples, 2) == 0);
    assert(LillaSerialFlashFile::Read_audio_samples(0, samples, 1, 2));
    assert(samples[0] == -222 && samples[1] == 333);
    assert(!LillaSerialFlashFile::Read_audio_samples(0, samples, 43995, 2));
    reader.packet_fast_open(0);
    assert(reader.read(samples, 2) == 2 && samples[0] == 0x5a5a);
    for (uint32_t bytes : {0u, 1u, 3u, 100u})
    {
        SerialFlash.files["0.raw"] = {100, bytes};
        FlashFileRegisterParser::Read_all_file_data();
        assert(!FlashFileRegisterParser::Zero_from_firmware() && FlashFileRegisterParser::length(0) == bytes);
        assert(SerialFlash.files.at("0.raw").size() == bytes);
        reader.fast_open(0);
        assert(reader && reader.size() == bytes);
        assert(reader.read(samples, 2) == std::min(bytes, 2u));
        if (bytes >= 2)
        {
            assert(samples[0] == 0x5a5a);
        }
    }
    puts("PASS: name-to-ID inventory, no numeric aliasing, missing/restored audio and packet isolation");
}
'''
compiler = shutil.which('g++') or r'C:\msys64\ucrt64\bin\g++.exe'
with tempfile.TemporaryDirectory(prefix='lilla-registry-scanner-') as directory:
    cpp = Path(directory) / 'scanner.cpp'
    exe = Path(directory) / 'scanner.exe'
    cpp.write_text(REGISTRY_FIXTURE + prefix + methods + checks, encoding='utf-8', newline='\r\n')
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    environment = os.environ.copy()
    environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
    subprocess.run([str(exe)], check=True, env=environment)
