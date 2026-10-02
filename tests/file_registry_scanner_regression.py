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
    uint32_t location = 0, bytes = 0;
    uint32_t getFlashAddress() { return location; }
    uint32_t size() { return bytes; }
    void close() {}
};
struct FlashStub
{
    std::map<std::string, SerialFlashFile> files;
    SerialFlashFile open(const char *name)
    {
        const auto it = files.find(name);
        return it == files.end() ? SerialFlashFile{} : it->second;
    }
} SerialFlash;
'''
methods = ''
for path, name in [('lib/SharedElements/SharedElements.cpp', 'Get_file_name'), ('lib/SharedVFS/SharedVFS.cpp', 'Get_packet_name')]:
    source = (ROOT / path).read_text(encoding='utf-8')
    methods += re.search(r'^const char \*' + name + r'\([^\n]+\)\n\{.*?^\}', source, re.M | re.S).group(0) + '\n'
header = (ROOT / 'lib/LillaSerialFlash/LillaSerialFlash.h').read_text(encoding='utf-8')
methods += header[header.index('class FlashFileRegisterParser'):]
source = (ROOT / 'lib/LillaSerialFlash/LillaSerialFlash.cpp').read_text(encoding='utf-8')
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
