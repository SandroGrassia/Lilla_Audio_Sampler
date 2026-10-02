"""Verify the embedded recording and exercise the production SD import with fault injection."""
from pathlib import Path
import hashlib
import os
import re
import shutil
import struct
import subprocess
import tempfile
import sys
from file_registry_fixture import REGISTRY_FIXTURE

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / 'src/ZeroRaw.cpp').read_text(encoding='utf-8')
initializer = re.search(r'const int16_t zeroraw\[43996\] PROGMEM = \{([^}]+)\};', source).group(1)
samples = [int(value) for value in initializer.split(',')]
assert len(samples) == 43996
import math
assert samples[0] == samples[-1] == 0
assert all(value == round(32767 * math.sin(2 * math.pi * 261 * index / 43995)) for index, value in enumerate(samples))
assert abs(1200 * math.log2((261 * 44100 / 43995) / (440 * 2 ** (-9 / 12)))) < 0.02

main = (ROOT / 'src/main.cpp').read_text(encoding='utf-8')
parsers = main[main.index('static constexpr uint32_t SET_AUDIO_MAX_RAW_BYTES'):main.index('FLASHMEM\nbool SET_Copy_audio_files_from_SD_to_Flash')]
method = re.search(r'^bool SET_Copy_audio_files_from_SD_to_Flash\(bool &flash_changed\)\n\{.*?^\}', main, re.M | re.S).group(0)
case_start = re.search(r'^            case \d+: // import RAW files from SD', main, re.M).start()
case_end = main.index('            case ', case_start + len('            case '))
menu_case = re.sub(r'case \d+:', 'case 4:', main[case_start:case_end], count=1)

arduino = '\n#pragma once\n#include <cstddef>\n#include <cstdint>\n#define FLASHMEM\n#define PROGMEM\n#define F(text) text\nusing elapsedMillis = unsigned long;\n'
flash = '\n#pragma once\n#include <Arduino.h>\n#include <algorithm>\n#include <cassert>\n#include <cstring>\n#include <map>\n#include <string>\n#include <vector>\nstruct FlashStub;\nextern FlashStub SerialFlash;\nclass SerialFlashFile\n{\npublic:\n    std::string name;\n    uint32_t offset = 0;\n    SerialFlashFile() = default;\n    explicit SerialFlashFile(const std::string &value) : name(value) {}\n    explicit operator bool() const;\n    uint32_t size() const;\n    uint32_t write(const void *source, uint32_t count);\n    uint32_t read(void *destination, uint32_t count);\n    void seek(uint32_t value) { offset = value; }\n    void close() {}\n};\nstruct FlashStub\n{\n    std::map<std::string, std::vector<uint8_t>> files;\n    std::vector<std::string> created;\n    uint32_t capacity = 64u * 1024u * 1024u;\n    uint32_t allocated = 0;\n    int erases = 0, writes = 0, waits = 0, reads = 0;\n    int fail_write_at = -1, fail_read_at = -1, corrupt_at = -1;\n    bool fail_create = false, fail_remove = false;\n    std::string fail_open;\n    const void *first_write_source = nullptr;\n    size_t directory_index = 0;\n    SerialFlashFile open(const char *name)\n    {\n        return files.count(name) && fail_open != name ? SerialFlashFile(name) : SerialFlashFile();\n    }\n    bool create(const char *name, uint32_t count)\n    {\n        const uint32_t start = (allocated + 255u) & ~255u;\n        if (fail_create || files.count(name) || start + count > capacity)\n        {\n            return false;\n        }\n        allocated = start + count;\n        files[name].assign(count, 0xFF);\n        created.emplace_back(name);\n        return true;\n    }\n    bool remove(const char *name)\n    {\n        return !fail_remove && files.erase(name) != 0;\n    }\n    void eraseAll()\n    {\n        ++erases;\n        files.clear();\n        created.clear();\n        allocated = 0;\n    }\n    void wait() { ++waits; }\n    bool ready() { return true; }\n    void readID(unsigned char *id) { id[0] = 0xEF; id[1] = 0x40; id[2] = 0x20; }\n    void opendir() { directory_index = 0; }\n    bool readdir(char *name, size_t capacity, uint32_t &count)\n    {\n        if (directory_index == files.size())\n        {\n            return false;\n        }\n        auto item = files.begin();\n        std::advance(item, directory_index++);\n        assert(item->first.size() < capacity);\n        std::strcpy(name, item->first.c_str());\n        count = item->second.size();\n        return true;\n    }\n};\ninline FlashStub SerialFlash;\ninline SerialFlashFile::operator bool() const { return !name.empty() && SerialFlash.files.count(name); }\ninline uint32_t SerialFlashFile::size() const { return *this ? SerialFlash.files.at(name).size() : 0; }\ninline uint32_t SerialFlashFile::write(const void *source, uint32_t count)\n{\n    assert(*this && count > 0 && count <= 256 && offset + count <= size());\n    if (SerialFlash.writes++ == 0)\n    {\n        SerialFlash.first_write_source = source;\n    }\n    if (static_cast<int>(offset) == SerialFlash.fail_write_at)\n    {\n        return count - 1;\n    }\n    auto &data = SerialFlash.files.at(name);\n    std::memcpy(data.data() + offset, source, count);\n    if (SerialFlash.corrupt_at >= static_cast<int>(offset) && SerialFlash.corrupt_at < static_cast<int>(offset + count))\n    {\n        data[SerialFlash.corrupt_at] ^= 1;\n    }\n    offset += count;\n    return count;\n}\ninline uint32_t SerialFlashFile::read(void *destination, uint32_t count)\n{\n    ++SerialFlash.reads;\n    assert(*this && count > 0 && count <= 256 && offset + count <= size());\n    if (static_cast<int>(offset) == SerialFlash.fail_read_at)\n    {\n        return count - 1;\n    }\n    std::memcpy(destination, SerialFlash.files.at(name).data() + offset, count);\n    offset += count;\n    return count;\n}\n'
fixture = '\n#include <SerialFlash.h>\n#include <ZeroRaw.h>\n#include <cstdio>\n#include <memory>\n#include <strings.h>\nstruct Entry\n{\n    std::string name;\n    std::vector<uint8_t> data;\n    bool directory = false;\n    int fail_at = -1, fail_result = 0, read_limit = 256;\n};\nstd::vector<std::shared_ptr<Entry>> entries;\nclass File\n{\npublic:\n    std::shared_ptr<Entry> entry;\n    size_t next = 0, offset = 0;\n    File() = default;\n    explicit File(std::shared_ptr<Entry> value) : entry(value) {}\n    explicit operator bool() const { return entry != nullptr; }\n    bool isDirectory() const { return entry && entry->directory; }\n    const char *name() const { return entry->name.c_str(); }\n    uint32_t size() const { return entry->data.size(); }\n    void close() { entry.reset(); }\n    File openNextFile()\n    {\n        assert(isDirectory());\n        return next < entries.size() ? File(entries[next++]) : File();\n    }\n    int read(void *destination, unsigned int count)\n    {\n        if (entry->fail_at >= 0 && static_cast<int>(offset) >= entry->fail_at)\n        {\n            return entry->fail_result;\n        }\n        const uint32_t accepted = std::min({count, size() - static_cast<uint32_t>(offset), static_cast<uint32_t>(entry->read_limit)});\n        std::memcpy(destination, entry->data.data() + offset, accepted);\n        offset += accepted;\n        return accepted;\n    }\n};\nstruct SDStub\n{\n    bool available = true, directory_exists = true;\n    int opens = 0, fail_open_at = -1;\n    bool begin(int) { return available; }\n    bool exists(const char *) { return directory_exists; }\n    File open(const char *)\n    {\n        if (++opens == fail_open_at || !directory_exists)\n        {\n            return File();\n        }\n        auto root = std::make_shared<Entry>();\n        root->directory = true;\n        return File(root);\n    }\n} SD;\nstruct SerialStub\n{\n    template<class... T> void print(T...) {}\n    template<class... T> void println(T...) {}\n    template<class... T> void printf(T...) {}\n} Serial;\nstruct Controller\n{\n    bool running = true;\n    bool Is_running() const { return running; }\n    void Start() { running = true; }\n    void Stop() { running = false; }\n} Trigger, Midi_reader;\nstruct ShiftersStub\n{\n    void Update() {}\n    void Switch_led(int, bool) {}\n} Shifters_manager;\nstruct ScannerStub\n{\n    int scans = 0;\n    void Read_all_file_data() { ++scans; }\n} File_scanner;\nconstexpr int BUILTIN_SDCARD = 0, EN_PB_Select = 0, LED_Tools = 0;\nint result = 0, SET_menu = 4, reloaded = 0, tables_rebuilt = 0;\nbool TOOLS_pushbutton = true, confirm_import = true;\nvoid delay(int) {}\nvoid Delete_text_row(int) {}\nvoid Clear_UI_events() {}\nint Read_encoder_simple(int) { return confirm_import ? 1 : 0; }\nbool Read_pushbutton(int) { return true; }\nint Get_raw_files_volume() { return 0; }\nint Get_raw_files() { return SerialFlash.files.size(); }\nunsigned long Get_flash_size() { return SerialFlash.capacity; }\nfloat SET_eraseBytesPerSecond(const unsigned char *) { return 512281.4f; }\nvoid AudioNoInterrupts() {}\nvoid AudioInterrupts() {}\nbool P_Quiesce_audio_players()\n{\n    Trigger.Stop();\n    Midi_reader.Stop();\n    return true;\n}\nbool S_Fill_all_tables() { ++tables_rebuilt; return true; }\nvoid VFS_Make_VFS() { assert(SerialFlash.files.count("0.raw")); }\nvoid DS_seed_all_Recordings() {}\nvoid Reload_system_state()\n{\n    assert(File_scanner.scans > 0);\n    ++reloaded;\n    Trigger.Start();\n    Midi_reader.Start();\n}\n'
methods = sorted(set(re.findall(r'Display_(?:Manager|Storage|Setup)\.(\w+)\(', method + menu_case)))
display = 'struct DisplayStub\n{\n    int completed = 0, invalid = 0, duplicates = 0, reported_files = 0;\n    unsigned long reported_bytes = 0;\n'
for name in methods:
    if name.endswith('_files_report'):
        display += f'    void {name}(unsigned long bytes, int files, int, int) {{ reported_bytes = bytes; reported_files = files; }}\n'
        continue
    action = '++completed;' if name.endswith('_job_done') else '++invalid;' if name.endswith('_invalid_audio') else '++duplicates;' if name.endswith('_duplicate') else ''
    display += f'    template<class... T> void {name}(T...) {{ {action} }}\n'
display += '} Display_Manager, Display_Storage, Display_Setup;\n'

tests = '\nvoid reset()\n{\n    SerialFlash = FlashStub();\n    SD = SDStub();\n    entries.clear();\n    Display_Manager = DisplayStub();\n    Display_Storage = DisplayStub();\n    Display_Setup = DisplayStub();\n    File_scanner = ScannerStub();\n    confirm_import = true;\n    Trigger.Start();\n    Midi_reader.Start();\n    reloaded = 0;\n    tables_rebuilt = 0;\n}\nstd::shared_ptr<Entry> add(const char *name, size_t bytes, bool directory = false)\n{\n    auto file = std::make_shared<Entry>();\n    file->name = name;\n    file->directory = directory;\n    file->data.resize(bytes);\n    for (size_t index = 0; index < bytes; ++index)\n    {\n        file->data[index] = static_cast<uint8_t>(index * 37 + 11);\n    }\n    entries.push_back(file);\n    return file;\n}\nvoid verify_fallback()\n{\n    const auto &data = SerialFlash.files.at("0.raw");\n    assert(data.size() == 87992);\n    assert(std::memcmp(data.data(), zeroraw, sizeof(zeroraw)) == 0);\n}\nint main()\n{\n    reset();\n    assert(ZeroRaw_ensure_file());\n    verify_fallback();\n    assert(SerialFlash.first_write_source == zeroraw);\n    assert(SerialFlash.writes == 344 && SerialFlash.waits == 344 && SerialFlash.reads == 0);\n    assert(ZeroRaw_ensure_file() && SerialFlash.writes == 344);\n\n    assert(ZeroRaw_ensure_file(true) && SerialFlash.writes == 344);\n    SerialFlash.files["0.raw"][1000] ^= 1;\n    assert(ZeroRaw_ensure_file(true));\n    verify_fallback();\n\n    for (uint32_t size : {0u, 1u, 3u})\n    {\n        reset();\n        SerialFlash.files["0.raw"].assign(size, 123);\n        assert(ZeroRaw_ensure_file());\n        verify_fallback();\n    }\n    reset();\n    SerialFlash.files["0.raw"] = {12, 34};\n    assert(ZeroRaw_ensure_file() && SerialFlash.writes == 0);\n    assert(SerialFlash.files.at("0.raw") == std::vector<uint8_t>({12, 34}));\n\n    for (int offset : {0, 256, 87808})\n    {\n        reset();\n        SerialFlash.fail_write_at = offset;\n        assert(!ZeroRaw_ensure_file());\n        assert(SerialFlash.files.count("0.raw") == 0);\n    }\n    reset();\n    SerialFlash.fail_create = true;\n    assert(!ZeroRaw_ensure_file());\n    reset();\n    SerialFlash.fail_open = "0.raw";\n    assert(!ZeroRaw_ensure_file());\n    reset();\n    SerialFlash.files["0.raw"] = {};\n    SerialFlash.fail_remove = true;\n    assert(!ZeroRaw_ensure_file() && SerialFlash.writes == 0);\n\n    bool changed = false;\n    reset();\n    SerialFlash.files["0.raw"] = {1, 2};\n    auto other = add("7.raw", 600);\n    add("subdirectory", 0, true);\n    assert(SET_Copy_audio_files_from_SD_to_Flash(changed) && changed);\n    verify_fallback();\n    assert(SerialFlash.created == std::vector<std::string>({"0.raw", "7.raw"}));\n    assert(SerialFlash.files.at("7.raw") == other->data && Display_Storage.completed == 1);\n\n    for (const char *name : {"0.raw", "0.RAW", "0.RaW"})\n    {\n        reset();\n        auto zero = add(name, 917580);\n        zero->read_limit = 256;\n        assert(SET_Copy_audio_files_from_SD_to_Flash(changed) && changed);\n        assert(SerialFlash.files.size() == 1 && SerialFlash.files.at("0.raw") == zero->data);\n    }\n    for (size_t size : {0u, 1u, 3u})\n    {\n        reset();\n        add("0.RAW", size);\n        assert(SET_Copy_audio_files_from_SD_to_Flash(changed));\n        verify_fallback();\n    }\n    reset();\n    assert(SET_Copy_audio_files_from_SD_to_Flash(changed));\n    verify_fallback();\n\n    for (int fault = 0; fault < 4; ++fault)\n    {\n        reset();\n        if (fault == 0)\n        {\n            confirm_import = false;\n        }\n        else if (fault == 1)\n        {\n            SD.available = false;\n        }\n        else if (fault == 2)\n        {\n            SD.directory_exists = false;\n        }\n        else\n        {\n            SD.fail_open_at = 1;\n        }\n        assert(!SET_Copy_audio_files_from_SD_to_Flash(changed) && !changed);\n        assert(SerialFlash.erases == 0 && Display_Storage.completed == 0);\n    }\n    reset();\n    SerialFlash.capacity = 87992;\n    add("7.raw", 256);\n    assert(!SET_Copy_audio_files_from_SD_to_Flash(changed) && changed);\n    verify_fallback();\n    assert(Display_Storage.completed == 0);\n    reset();\n    SerialFlash.capacity = 87000;\n    assert(!SET_Copy_audio_files_from_SD_to_Flash(changed) && changed);\n    assert(Display_Storage.completed == 0);\n\n    for (int error : {0, -1})\n    {\n        reset();\n        auto zero = add("0.raw", 600);\n        zero->fail_at = 256;\n        zero->fail_result = error;\n        assert(!SET_Copy_audio_files_from_SD_to_Flash(changed) && changed);\n        assert(Display_Storage.completed == 0 && SerialFlash.files.count("0.raw") == 0);\n    }\n    reset();\n    SD.fail_open_at = 2;\n    assert(!SET_Copy_audio_files_from_SD_to_Flash(changed) && changed);\n    assert(Display_Storage.completed == 0);\n    reset();\n    SerialFlash.fail_write_at = 87808;\n    assert(!SET_Copy_audio_files_from_SD_to_Flash(changed) && changed);\n    assert(Display_Storage.completed == 0);\n\n    reset();\n    run_menu_case();\n    verify_fallback();\n    assert(reloaded == 1 && File_scanner.scans == 1 && Trigger.Is_running());\n    reset();\n    confirm_import = false;\n    run_menu_case();\n    assert(tables_rebuilt == 1 && Trigger.Is_running() && SerialFlash.erases == 0);\n    reset();\n    SerialFlash.capacity = 87000;\n    run_menu_case();\n    assert(tables_rebuilt == 0 && reloaded == 0 && !Trigger.Is_running() && !Midi_reader.Is_running());\n    confirm_import = false;\n    run_menu_case();\n    assert(tables_rebuilt == 0 && !Trigger.Is_running());\n    confirm_import = true;\n    SerialFlash.capacity = 64u * 1024u * 1024u;\n    run_menu_case();\n    assert(reloaded == 1 && Trigger.Is_running());\n    std::puts("PASS: source bytes, direct Flash copy, preservation, fallback, filename case, invalid PCM, capacity, read/write faults, no audio readback, cancellation, audio recovery, inventory refresh");\n}\n'
fixture = REGISTRY_FIXTURE + fixture
fixture += '\nconstexpr int ILI9341_WHITE = 1, ILI9341_RED = 2;\nvoid Show_popup_text(const char *, int, int, int) {}\n'
tests = tests.replace('void reset()\n{', 'void reset()\n{\n    reset_registry();')
tests = tests.replace('SD.fail_open_at = 2;', 'SD.fail_open_at = 3;')
fixture += r"""
struct CaptureAudio { const void *psram_ptr = nullptr; };
struct CaptureStub { CaptureAudio audio; } Capture_sources[1];
void LS_Capture_collect() {}
void LS_Capture_notice(const char *) {}
"""
tests = tests.replace('int main()\n{', """int main()
{
    reset();
    Capture_sources[0].audio.psram_ptr = zeroraw;
    bool capture_changed = true;
    assert(!SET_Copy_audio_files_from_SD_to_Flash(capture_changed));
    assert(!capture_changed && SerialFlash.erases == 0 && SerialFlash.writes == 0);
    Capture_sources[0].audio.psram_ptr = nullptr;
""")
tests = tests.replace('    bool changed = false;', r"""
    bool imported = false;
    for (const char *name : {"7.raw", "7.RAW", "7.RaW"})
    {
        reset();
        auto raw = add(name, 600);
        auto zero = add("0.RAW", 800);
        add("notes.txt", 100);
        add("8.raw.bak", 200);
        assert(SET_Copy_audio_files_from_SD_to_Flash(imported) && imported);
        assert(SerialFlash.files.size() == 2);
        assert(SerialFlash.files.at("7.raw") == raw->data);
        assert(SerialFlash.files.at("0.raw") == zero->data);
    }
    reset();
    auto raw = add("MixedName.RAW", 600);
    assert(SET_Copy_audio_files_from_SD_to_Flash(imported) && imported);
    assert(SerialFlash.files.at("MixedName.raw") == raw->data);
    verify_fallback();
    bool changed = false;
""")
# Exercise the production RAW, WAV and AIFF import with the same SD/Flash mocks.
tests = tests.replace('    bool changed = false;', r'''
    reset();
    bool registry_changed = false;
    add("kick.raw", 256);
    add("snare.raw", 256);
    assert(SET_Copy_audio_files_from_SD_to_Flash(registry_changed));
    const int kick_id = FileNameRegistry::Find("kick.raw");
    const int snare_id = FileNameRegistry::Find("snare.raw");
    entries.clear();
    add("hat.raw", 256);
    add("snare.raw", 256);
    assert(SET_Copy_audio_files_from_SD_to_Flash(registry_changed));
    assert(FileNameRegistry::Find("kick.raw") == kick_id && FileNameRegistry::Find("snare.raw") == snare_id);
    assert(!SerialFlash.files.count("kick.raw"));
    entries.clear();
    add("kick.raw", 256);
    assert(SET_Copy_audio_files_from_SD_to_Flash(registry_changed));
    assert(FileNameRegistry::Load() && FileNameRegistry::Find("kick.raw") == kick_id);
    for (const char *invalid : {"P10.raw", "12345678901234567890123456789012.raw"})
    {
        reset();
        add(invalid, 256);
        assert(!SET_Copy_audio_files_from_SD_to_Flash(registry_changed));
        assert(!registry_changed && SerialFlash.erases == 0);
    }
    reset();
    LillaFram.budget = 10;
    add("kick.raw", 256);
    assert(!SET_Copy_audio_files_from_SD_to_Flash(registry_changed));
    assert(!registry_changed && SerialFlash.erases == 0);
    LillaFram.budget = -1;
    assert(FileNameRegistry::Load() && FileNameRegistry::Find("kick.raw") == -1);
    reset();
    for (int id = 1; id < 260; ++id)
    {
        assert(FileNameRegistry::Bind_numeric(id));
    }
    assert(FileNameRegistry::Save());
    add("kick.raw", 256);
    assert(!SET_Copy_audio_files_from_SD_to_Flash(registry_changed));
    assert(!registry_changed && SerialFlash.erases == 0);
    reset();
    bool changed = false;
''')
flash = flash.replace('    bool create(const char *name, uint32_t count)', '    bool exists(const char *name) { return files.count(name) != 0; }\n    bool create(const char *name, uint32_t count)')
fixture = fixture.replace('    void close() { entry.reset(); }', '    bool seek(uint32_t value)\n    {\n        if (value > size())\n        {\n            return false;\n        }\n        offset = value;\n        return true;\n    }\n    void close() { entry.reset(); }')
fixture = fixture.replace('    bool directory = false;', '    uint32_t max_read_end = 0;\n    bool directory = false;')
fixture = fixture.replace('        offset += accepted;', '        offset += accepted;\n        entry->max_read_end = std::max(entry->max_read_end, static_cast<uint32_t>(offset));')
audio_tests = (ROOT / 'tests/audio_import_cases.inc').read_text(encoding='utf-8')
tests = tests.replace('int main()\n{', audio_tests + '\nint main()\n{\n    test_audio_import();', 1)
method = method.replace('f.read(buf, input_bytes) != input_bytes', 'f.read(buf, input_bytes) != static_cast<int>(input_bytes)')
method = method.replace('f.read(buf, input_bytes) == input_bytes', 'f.read(buf, input_bytes) == static_cast<int>(input_bytes)')
fixture = fixture.replace('    void close() { entry.reset(); }', '    uint32_t position() { return offset; }\n    void close() { entry.reset(); }')
fixture += '\n' + (ROOT / 'src/Mp3Import.cpp').read_text(encoding='utf-8')
compiler = shutil.which('g++') or r'C:\msys64\ucrt64\bin\g++.exe'
with tempfile.TemporaryDirectory(prefix='lilla-zero-raw-') as directory:
    build = Path(directory)
    for name, contents in {'Arduino.h': arduino, 'SerialFlash.h': flash, 'SD.h': '#pragma once\n'}.items():
        (build / name).write_text(contents, encoding='utf-8', newline='\r\n')
    cpp = build / 'zero_raw_test.cpp'
    if '--mp3' in sys.argv:
        ffmpeg = shutil.which('ffmpeg') or next((ROOT / '.pio/test-tools').glob('**/ffmpeg*.exe'), None)
        if ffmpeg is None:
            raise RuntimeError('MP3 tests require ffmpeg on PATH or in .pio/test-tools')
        for name, rate, channels, duration, options in [('mono', 44100, 1, 0.2, ['-b:a', '128k']), ('stereo', 44100, 2, 0.2, ['-b:a', '192k']), ('vbr', 44100, 2, 0.2, ['-q:a', '4']), ('untagged', 44100, 1, 0.2, ['-write_xing', '0']), ('long', 44100, 2, 37, ['-q:a', '6']), ('rate48000', 48000, 2, 0.2, []), ('rate22050', 22050, 1, 0.2, [])]:
            signal = f'sine=frequency=440:sample_rate={rate}:duration={duration}' if channels == 1 else f'aevalsrc=0.1*sin(2*PI*440*t)|0.2*sin(2*PI*660*t):s={rate}:d={duration}'
            subprocess.run([str(ffmpeg), '-v', 'error', '-f', 'lavfi', '-i', signal, '-ac', str(channels), '-c:a', 'libmp3lame', '-metadata', 'title=Lilla import test', *options, str(build / (name + '.mp3'))], check=True)
        for rate in [8000, 11025, 12000, 16000, 22050, 24000, 32000, 48000]:
            for channels in [1, 2]:
                name = f'resample_{rate}_{channels}'
                signal = f'aevalsrc=0.1*sin(2*PI*440*t)|0.2*sin(2*PI*660*t):s={rate}:d=0.4'
                subprocess.run([str(ffmpeg), '-v', 'error', '-f', 'lavfi', '-i', signal, '-ac', str(channels), '-c:a', 'libmp3lame', '-q:a', '2', str(build / (name + '.mp3'))], check=True)
                subprocess.run([str(ffmpeg), '-v', 'error', '-i', str(build / (name + '.mp3')), '-ac', '1', '-ar', '44100', '-f', 's16le', str(build / (name + '.raw'))], check=True)
        subprocess.run([str(ffmpeg), '-v', 'error', '-f', 'lavfi', '-i', 'sine=frequency=440:sample_rate=48000:duration=37', '-c:a', 'libmp3lame', '-q:a', '6', str(build / 'long48000.mp3')], check=True)
        mp3_tests = '#include <fstream>\n#include <iterator>\nstatic const char *mp3_test_directory = R"(' + str(build) + ')";\n' + (ROOT / 'tests/mp3_import_cases.inc').read_text(encoding='utf-8')
        tests = tests.replace('int main()\n{', mp3_tests + '\nint main()\n{\n    test_mp3_import();', 1)
    cpp.write_text(fixture + display + parsers + method + '\nvoid run_menu_case()\n{\n    switch (4)\n    {\n' + menu_case + '\n    }\n}\n' + tests, encoding='utf-8', newline='\r\n')
    exe = build / ('zero_raw_test.exe' if os.name == 'nt' else 'zero_raw_test')
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', '-I', str(build), '-I', str(ROOT / 'include'), '-I', str(ROOT / '.pio/libdeps/teensy41/dr_libs'), str(cpp), str(ROOT / 'src/ZeroRaw.cpp'), '-o', str(exe)], check=True)
    environment = os.environ.copy()
    environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
    subprocess.run([str(exe)], check=True, env=environment, timeout=90)
print('PASS: 43996 sine samples, zero endpoints, middle-C pitch and Flash replacement')
