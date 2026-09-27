"""Compare generated names against the original 323 file and 1024 packet names."""
from pathlib import Path
import hashlib
import os
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
headers = '\n'.join((ROOT / path).read_text(encoding='utf-8') for path in ['lib/SharedElements/SharedElements.h', 'lib/SharedVFS/SharedVFS.h', 'lib/SharedLiveSampler/SharedLiveSampler.h'])
constants = '\n'.join(re.search(r'^static constexpr int ' + name + r' = \d+;', headers, re.M).group(0) for name in ['NAME_FILE_SIZE', 'NAME_PACKET_SIZE', 'RAW_FILES', 'PACKETS', 'FIRST_RECORDING_FILE', 'FIRST_LIVE_SAMPLING_FILE'])
methods = []
for path, function in [('lib/SharedElements/SharedElements.cpp', 'Get_file_name'), ('lib/SharedVFS/SharedVFS.cpp', 'Get_packet_name')]:
    source = (ROOT / path).read_text(encoding='utf-8')
    methods.append(re.search(r'^const char \*' + function + r'\([^\n]+\)\n\{.*?^\}', source, re.M | re.S).group(0))

checks = r'''
int main()
{
    struct FileBuffer
    {
        char before = 'A';
        char name[NAME_FILE_SIZE];
        char after = 'Z';
    } file;
    struct PacketBuffer
    {
        char before = 'A';
        char name[NAME_PACKET_SIZE];
        char after = 'Z';
    } packet;
    for (int id = 0; id < RAW_FILES; ++id)
    {
        memset(file.name, 'X', sizeof(file.name));
        assert(Get_file_name(id, file.name) == file.name);
        assert(memchr(file.name, 0, sizeof(file.name)) != nullptr);
        assert(file.before == 'A' && file.after == 'Z');
        puts(file.name);
    }
    for (int id = 0; id < PACKETS; ++id)
    {
        memset(packet.name, 'X', sizeof(packet.name));
        assert(Get_packet_name(id, packet.name) == packet.name);
        assert(memchr(packet.name, 0, sizeof(packet.name)) != nullptr);
        assert(packet.before == 'A' && packet.after == 'Z');
        puts(packet.name);
    }
    char other[NAME_FILE_SIZE];
    Get_file_name(0, file.name);
    Get_file_name(RAW_FILES - 1, other);
    Get_packet_name(PACKETS - 1, packet.name);
    assert(strcmp(file.name, "0.raw") == 0);
    assert(strcmp(other, "Right.liv") == 0);
    assert(strcmp(packet.name, "P1023.raw") == 0);
}
'''
compiler = shutil.which('g++') or r'C:\msys64\ucrt64\bin\g++.exe'
with tempfile.TemporaryDirectory(prefix='lilla-file-names-') as directory:
    cpp = Path(directory) / 'names.cpp'
    exe = Path(directory) / ('names.exe' if os.name == 'nt' else 'names')
    cpp.write_text('#include <cassert>\n#include <cstdint>\n#include <cstdio>\n#include <cstring>\n' + constants + '\n' + '\n'.join(methods) + checks, encoding='utf-8', newline='\r\n')
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    environment = os.environ.copy()
    environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
    result = subprocess.run([str(exe)], check=True, env=environment, capture_output=True, text=True)
    assert hashlib.sha256(result.stdout.encode()).hexdigest() == '9dd3a5b24cb0815e887707a4d3a1cdb614b61b9fffe0910d71e6bbd12dc0c196'
print('PASS: all 1347 original names, null termination, buffer bounds and independent caller buffers')
