"""Exercise production loop SD code with an in-memory, fault-injectable SD card."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
main = (ROOT / 'src/main.cpp').read_text(encoding='utf-8')
header = (ROOT / 'lib/SharedLoop/SharedLoop.h').read_text(encoding='utf-8')
shared = (ROOT / 'lib/SharedLoop/SharedLoop.cpp').read_text(encoding='utf-8')


def without_includes(text):
    return re.sub(r'^\s*#(?:include|pragma)[^\n]*', '', text, flags=re.M)


def method(signature):
    found = re.search(r'^' + re.escape(signature) + r'\n\{.*?^\}', main, re.M | re.S)
    assert found, signature
    return found.group(0) + '\n'


prefix = r'''
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <vector>
#define FLASHMEM
#define DMAMEM
#define F(x) x
using byte = uint8_t;
using String = std::string;
using elapsedMillis = uint32_t;
constexpr int TRACKS = 4;
constexpr int BUILTIN_SDCARD = 0;
constexpr int O_RDONLY = 0, O_WRONLY = 1, O_CREAT = 2, O_TRUNC = 4;
struct SerialStub
{
    template<class T> void print(T) {}
    template<class T> void println(T) {}
} Serial;
struct Faults
{
    bool begin = false, mkdir = false, sync = false, close = false, seek = false;
    size_t write_limit = SIZE_MAX, second_read_limit = SIZE_MAX;
    std::set<String> open, remove, rename;
} faults;
struct Node { String text; };
struct FsFile
{
    std::shared_ptr<Node> node;
    size_t position = 0;
    bool error = false, second = false;
    explicit operator bool() const { return bool(node); }
    int read()
    {
        if (second && position >= faults.second_read_limit)
        {
            error = true;
            return -1;
        }
        return node && position < node->text.size() ? uint8_t(node->text[position++]) : -1;
    }
    int peek() const { return node && position < node->text.size() ? uint8_t(node->text[position]) : -1; }
    size_t write(const char *data, size_t count)
    {
        const size_t available = position < faults.write_limit ? faults.write_limit - position : 0;
        const size_t written = std::min(count, available);
        node->text.append(data, written);
        position += written;
        error |= written != count;
        return written;
    }
    size_t println(uint8_t value)
    {
        const String line = std::to_string(value) + "\r\n";
        return write(line.data(), line.size());
    }
    bool sync() const { return !faults.sync && !error; }
    bool close() { node.reset(); return !faults.close; }
    bool seekSet(size_t offset)
    {
        if (faults.seek)
        {
            return false;
        }
        position = offset;
        second = true;
        return true;
    }
    bool getError() const { return error; }
    size_t curPosition() const { return position; }
    size_t fileSize() const { return node->text.size(); }
};
struct SDStub
{
    struct Backend
    {
        std::map<String, std::shared_ptr<Node>> files;
        FsFile open(const char *path, int mode)
        {
            if (faults.open.count(path))
            {
                return {};
            }
            if (mode & O_TRUNC)
            {
                files[path] = std::make_shared<Node>();
            }
            const auto it = files.find(path);
            return it == files.end() ? FsFile{} : FsFile{it->second};
        }
    } sdfs;
    std::set<String> directories;
    bool begin(int) const { return !faults.begin; }
    bool exists(const char *path) const { return directories.count(path) || sdfs.files.count(path); }
    bool mkdir(const char *path)
    {
        if (faults.mkdir)
        {
            return false;
        }
        directories.insert(path);
        return true;
    }
    bool remove(const char *path)
    {
        return !faults.remove.count(path) && sdfs.files.erase(path) != 0;
    }
    bool rename(const char *from, const char *to)
    {
        if (faults.rename.count(String(from) + ">" + to) || !sdfs.files.count(from) || exists(to))
        {
            return false;
        }
        sdfs.files[to] = sdfs.files[from];
        sdfs.files.erase(from);
        return true;
    }
} SD;
String LOOP_Filename_midi_loop(int id) { return std::to_string(id) + ".loop"; }
'''

globals_start = main.index('DMAMEM uint32_t LOOP_time_order')
globals_end = main.index('// Pointer', globals_start)
production = without_includes(header) + without_includes(shared) + main[globals_start:globals_end]
production += method('void LOOP_reset_all_data(void)')
production += 'void LOOP_stop_and_reset_runnig_loop_data() { LOOP_reset_all_data(); }\n'
production += method('void LOOP_set_time_order(int track)')
codec_start = main.index('// Loop SD v1:')
codec_end = main.index('bool LOOP_Look_for_midi_loop_in_SD(int loop_id)\n{', codec_start)
production += main[codec_start:codec_end]
for signature in ['int LOOP_Get_first_loop_id_free(void)', 'int LOOP_Get_next_loop_id_in_SD(int loop_id)', 'int LOOP_Get_previous_loop_id_in_SD(int loop_id)']:
    production += method(signature)

reader = (ROOT / 'lib/MidiReader/MidiReader.cpp').read_text(encoding='utf-8')
production += 'constexpr int MIDI_LOOP = 1; int Lilla_state = MIDI_LOOP; unsigned long LOOP_Clock() { return 123; }\n'
for name, condition in [('Learn_on', 'if ((Lilla_state == MIDI_LOOP) && LOOP_learn_flag && LOOP_elements < LOOP_EVENTS)'), ('Learn_off', 'if ((Lilla_state == MIDI_LOOP) && LOOP_learn_flag && LOOP_elements > 0 && LOOP_elements < LOOP_EVENTS)')]:
    start = reader.index(condition)
    position = reader.index('{', start)
    depth = 1
    end = position + 1
    while depth:
        depth += (reader[end] == '{') - (reader[end] == '}')
        end += 1
    production += f'void {name}(uint8_t midi_channel, uint8_t note_number, uint8_t velocity)\n{{\n' + reader[start:end] + '\n}\n'

production += r'''
int result;
LOOP_field_description_struct LOOP_local_pointer{};
struct PointerStub
{
    void Show_pointer(bool) {}
    void Set_pointer_to_first_menu_element() {}
    LOOP_field_description_struct Get_pointer() { return {}; }
} Pointer_MidiLoop;
struct DisplayStub { void Show_menu() {} void Show_loop_id() {} } Display_MidiLoop;
void Clear_UI_events() {}
void LOOP_select_menu_elements() {}
'''
ui_start = main.index('                    case value_LOOP_Save:\n')
ui_end = main.index('                    case value_LOOP_Delete:\n', ui_start)
production += 'void Save_UI(int action)\n{\n    switch (action)\n    {\n' + main[ui_start:ui_end] + '    }\n}\n'

tests = r'''
const String path = "/LILLALOOP/3.loop";
void seed(uint32_t count)
{
    LOOP_reset_all_data();
    LOOP_time = 10000;
    LOOP_stretch_int = 150;
    LOOP_stretch = 1.5f;
    LOOP_id = 7;
    LOOP_original = false;
    for (int track = 0; track < TRACKS; ++track)
    {
        LOOP_events[track] = count;
        LOOP_slide[track] = 100 * track;
        LOOP_pitch_int[track] = track - 2;
        for (uint32_t event = 0; event < count; ++event)
        {
            LOOP_element[track][event] = {int(event), uint8_t(track), uint8_t(40 + event % 80), uint8_t(event % 128), event % 2 == 0};
        }
    }
}
void put(const String &name, const String &text)
{
    SD.sdfs.files[name] = std::make_shared<Node>(Node{text});
}
String encoded()
{
    FsFile file{std::make_shared<Node>()};
    assert(LOOP_Compile_midi_loop_file(file));
    return file.node->text;
}
String legacy()
{
    FsFile file{std::make_shared<Node>()};
    assert(LOOP_Write_bytes(file, &LOOP_time, sizeof(LOOP_time)));
    for (int track = 0; track < TRACKS; ++track)
    {
        const uint8_t count = LOOP_events[track];
        assert(LOOP_Write_bytes(file, &count, sizeof(count)));
    }
    assert(LOOP_Write_bytes(file, LOOP_slide, sizeof(LOOP_slide)));
    assert(LOOP_Write_bytes(file, LOOP_pitch_int, sizeof(LOOP_pitch_int)));
    assert(LOOP_Write_bytes(file, &LOOP_stretch_int, sizeof(LOOP_stretch_int)));
    for (int track = 0; track < TRACKS; ++track)
    {
        assert(LOOP_Write_bytes(file, LOOP_element[track], LOOP_events[track] * sizeof(LOOP_struct)));
    }
    return file.node->text;
}
void rejected(const String &text)
{
    seed(1);
    const auto previous = LOOP_element[0][0];
    put(path, text);
    assert(!LOOP_Copy_midi_loop_from_SD_to_RAM(3));
    assert(LOOP_id == 7 && !LOOP_original && LOOP_events[0] == 1 && LOOP_time == 10000);
    assert(memcmp(&previous, &LOOP_element[0][0], sizeof(previous)) == 0);
}
String change_byte(String text, size_t index, const String &value)
{
    size_t position = text.find('\n') + 1;
    for (size_t i = 0; i < index; ++i)
    {
        position = text.find('\n', position) + 1;
    }
    text.replace(position, text.find('\n', position) - position + 1, value + "\r\n");
    return text;
}
int main()
{
    static_assert(std::is_same_v<std::remove_reference_t<decltype(LOOP_events[0])>, uint32_t>);
    static_assert(std::is_same_v<std::remove_reference_t<decltype(LOOP_time_order[0][0])>, uint32_t>);
    static_assert(std::is_same_v<std::remove_reference_t<decltype(LOOP_play_event[0])>, uint32_t>);
    seed(LOOP_EVENTS);
    const String valid = encoded();
    put(path, valid);
    LOOP_reset_all_data();
    assert(LOOP_Copy_midi_loop_from_SD_to_RAM(3));
    assert(LOOP_events[3] == LOOP_EVENTS && LOOP_time == 10000 && LOOP_pitch_int[3] == 1 && LOOP_slide[3] == 300 && LOOP_stretch == 1.5f);
    assert(LOOP_element[3][LOOP_EVENTS - 1].time == int(LOOP_EVENTS - 1));
    assert(encoded() == valid);
    // Rotation uses 32-bit indices, including beyond byte range in the future-capacity build.
    LOOP_element[0][LOOP_EVENTS - 1].time = -1;
    LOOP_set_time_order(0);
    assert(LOOP_time_order[0][0] == LOOP_EVENTS - 1 && LOOP_time_order[0][1] == 0);
    seed(2);
    const String old = legacy();
    put(path, old);
    LOOP_reset_all_data();
    assert(LOOP_Copy_midi_loop_from_SD_to_RAM(3) && LOOP_events[3] == 2 && LOOP_element[3][1].note_number == 41);
    const String small = encoded();
    for (size_t length = 0; length < small.size(); ++length)
    {
        rejected(small.substr(0, length));
    }
    rejected(small + "0\r\n");
    rejected("LILLALOOP 2\r\n" + small.substr(sizeof(LOOP_SD_MAGIC) - 1));
    rejected(change_byte(small, 0, "5")); // Track count.
    for (const char *bad : {"256", "-1", "x", "", "12x", "999999", " 1"})
    {
        rejected(change_byte(small, 4, bad));
    }
    rejected(change_byte(change_byte(small, 4, "0"), 5, "0")); // Zero duration.
    rejected(change_byte(small, 6, "0")); // Empty master.
    rejected(change_byte(small, 9, "255")); // Huge 32-bit event count.
    rejected(change_byte(small, 25, "127")); // Slide outside duration.
    rejected(change_byte(small, 38, "25")); // Pitch.
    rejected(change_byte(small, 54, "0")); // Stretch.
    rejected(change_byte(small, 61, "127"));
    rejected(change_byte(small, 62, "16")); // MIDI channel.
    rejected(change_byte(small, 63, "128")); // Note.
    rejected(change_byte(small, 64, "128")); // Velocity.
    rejected(change_byte(small, 65, "2")); // Bool representation.
    // Legacy export with the old extra 8 bytes in each array must not be accepted.
    size_t array_end = 0;
    for (int line = 0; line < 22; ++line)
    {
        array_end = old.find('\n', array_end) + 1;
    }
    String malformed = old;
    for (int i = 0; i < 16; ++i)
    {
        malformed.insert(array_end, "0\r\n");
    }
    rejected(malformed);
    put(path, small);
    seed(1);
    faults.seek = true;
    assert(!LOOP_Copy_midi_loop_from_SD_to_RAM(3) && LOOP_events[0] == 1);
    faults = {};
    faults.second_read_limit = small.size() - 4;
    assert(!LOOP_Copy_midi_loop_from_SD_to_RAM(3));
    assert(LOOP_events[0] == 0 && LOOP_events[3] == 0 && LOOP_time == 0 && LOOP_id == NEW_LOOP && !LOOP_original);
    faults = {};
    // First save creates the missing directory, and failure paths retain the previous file.
    SD.sdfs.files.clear();
    assert(LOOP_Get_first_loop_id_free() == 0);
    seed(2);
    assert(LOOP_Copy_midi_loop_from_RAM_to_SD(3));
    const String previous = SD.sdfs.files[path]->text;
    seed(3);
    LOOP_pitch_int[0] = 99;
    assert(!LOOP_Copy_midi_loop_from_RAM_to_SD(3) && SD.sdfs.files[path]->text == previous);
    seed(3);
    faults.write_limit = 30;
    assert(!LOOP_Copy_midi_loop_from_RAM_to_SD(3) && SD.sdfs.files[path]->text == previous);
    faults = {};
    faults.sync = true;
    assert(!LOOP_Copy_midi_loop_from_RAM_to_SD(3) && SD.sdfs.files[path]->text == previous);
    faults = {};
    faults.close = true;
    assert(!LOOP_Copy_midi_loop_from_RAM_to_SD(3) && SD.sdfs.files[path]->text == previous);
    faults = {};
    faults.rename.insert(path + ">" + path + ".bak");
    assert(!LOOP_Copy_midi_loop_from_RAM_to_SD(3) && SD.sdfs.files[path]->text == previous);
    faults = {};
    faults.rename.insert(path + ".tmp>" + path);
    assert(!LOOP_Copy_midi_loop_from_RAM_to_SD(3) && SD.sdfs.files[path]->text == previous);
    faults.rename.insert(path + ".bak>" + path);
    assert(!LOOP_Copy_midi_loop_from_RAM_to_SD(3));
    assert(!SD.exists(path.c_str()) && SD.sdfs.files[path + ".bak"]->text == previous);
    assert(LOOP_Get_next_loop_id_in_SD(2) == 3 && LOOP_Get_previous_loop_id_in_SD(4) == 3);
    faults = {};
    assert(LOOP_Copy_midi_loop_from_SD_to_RAM(3) && LOOP_events[0] == 2);
    assert(LOOP_Copy_midi_loop_from_RAM_to_SD(3));
    assert(SD.sdfs.files[path + ".bak"]->text == previous);
    faults.remove.insert(path + ".bak");
    assert(!LOOP_Delete_midi_loop_from_SD(3) && SD.exists(path.c_str()));
    faults = {};
    assert(LOOP_Delete_midi_loop_from_SD(3) && !SD.exists(path.c_str()) && !SD.exists((path + ".bak").c_str()));
    assert(LOOP_Get_next_loop_id_in_SD(2) == 2);
    faults.begin = true;
    assert(!LOOP_Copy_midi_loop_from_SD_to_RAM(3) && !LOOP_Copy_midi_loop_from_RAM_to_SD(3));
    // UI must keep both the dirty flag and the original ID on failure.
    seed(3);
    Save_UI(value_LOOP_Save);
    assert(LOOP_id == 7 && !LOOP_original);
    Save_UI(value_LOOP_SaveAsNew);
    assert(LOOP_id == 7 && !LOOP_original);
    faults = {};
    faults.write_limit = 10;
    Save_UI(value_LOOP_SaveAsNew);
    assert(LOOP_id == 7 && !LOOP_original);
    faults = {};
    Save_UI(value_LOOP_SaveAsNew);
    assert(LOOP_id == 0 && LOOP_original);
    LOOP_original = false;
    Save_UI(value_LOOP_Save);
    assert(LOOP_id == 0 && LOOP_original);
    // Exercise production recording conditions: the last slot is usable and no later event writes past it.
    LOOP_learning_track = 0;
    LOOP_elements = 0;
    LOOP_learn_flag = true;
    for (uint32_t event = 0; event < LOOP_EVENTS; ++event)
    {
        if (event % 2 == 0)
        {
            Learn_on(0, 60, 100);
        }
        else
        {
            Learn_off(0, 60, 0);
        }
    }
    assert(LOOP_elements == LOOP_EVENTS && LOOP_last_event == LOOP_EVENTS - 1 && !LOOP_learn_flag);
    LOOP_learn_flag = true; // Even an inconsistent flag cannot cause an out-of-bounds append.
    Learn_on(0, 61, 100);
    Learn_off(0, 61, 0);
    assert(LOOP_elements == LOOP_EVENTS && LOOP_element[0][LOOP_EVENTS - 1].note_number == 60);
    std::cout << "PASS: loop SD round trips, bounds, malformed/truncated files, second-pass failure, safe replacement/recovery; capacity=" << LOOP_EVENTS << '\n';
}
'''

compiler = shutil.which('g++') or r'C:\msys64\ucrt64\bin\g++.exe'
with tempfile.TemporaryDirectory(prefix='lilla-loop-') as directory:
    for capacity in [100, 1024]:
        cpp = Path(directory) / 'loop.cpp'
        exe = Path(directory) / ('loop.exe' if os.name == 'nt' else 'loop')
        variant = production.replace('LOOP_EVENTS = 100;', f'LOOP_EVENTS = {capacity};')
        cpp.write_text(prefix + variant + tests, encoding='utf-8', newline='\r\n')
        subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
        environment = os.environ.copy()
        environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
        subprocess.run([str(exe)], check=True, env=environment)
