"""Exercise production TONE encoder limits, OFF transition and decimal rendering."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
main = (root / 'src/main.cpp').read_text(encoding='utf-8')
display = (root / 'lib/DisplayPerformance/DisplayPerformance.cpp').read_text(encoding='utf-8')
encoder = re.search(r'template <class T>\nbool Read_encoder\(.*?^\}', main, re.M | re.S).group(0)
start = main.index('            case value_P_Tone:')
end = main.index('            case value_P_Precedence:', start)
handler = main[start:end]
render = re.search(r'^void DisplayPerformance::P_show_Tone_value\(.*?^\}', display, re.M | re.S).group(0)
header = (root / 'lib/DisplayPerformance/DisplayPerformance.h').read_text(encoding='utf-8')
geometry = '\n'.join(re.findall(r'^    static constexpr [^\n]*(?:P_column_TONE_title|P_chars_instrument_element|P_column_instrument_element)[^\n]*', header, re.M))
code = r"""
#include <cassert>
#include <cstdint>
#include <string>
#include <type_traits>
#include <iostream>
int rotation = 0;
struct Encoder { int Get_rotation(int) { return rotation; } } Encoders_manager;
struct Instrument { uint8_t lock = 0; };
struct PatchData { struct Instrument Instrument[1]; } Patch[1];
constexpr int EN_PB_Value = 0, value_P_Tone = 0;
int Patch_id = 0, updates = 0, redraws = 0;
bool interrupts = true;
void AudioNoInterrupts() { interrupts = false; }
void AudioInterrupts() { interrupts = true; }
void P_Macro_Instrument_editing(int, int, int) { assert(interrupts); ++redraws; }
void P_UpdatePatchOriginalAndMenu() { assert(interrupts); }
struct Manager
{
    void Update_Preset_lock(int, int)
    {
        assert(!interrupts);
        ++updates;
    }
} Players_Manager;
int P_line_of_instrument[1] = {0};
constexpr int ILI9341_YELLOW = 1, ILI9341_WHITE = 2;
struct DisplayStub
{
    std::string text;
    void setTextColor(int) {}
    void print(const char *value) { text += value; }
    void print(char value) { text += value; }
    void print(int value) { text += std::to_string(value); }
} tft;
int cleared = 0;
int display_coordinate_x(float value) { return static_cast<int>(value * 6); }
void Cancel_text_reset_cursor(int, int, int width) { cleared = width; tft.text.clear(); }
struct DisplayPerformance
{
    // PRODUCTION_GEOMETRY
    int P_Instrument_pixels_y(int) { return 0; }
    void P_show_Tone_value(int, int, bool);
};
""" + encoder + '\n' + render + '\nvoid Step()\n{\n    const int instrument_id = 0, element = value_P_Tone;\n    switch (element)\n    {\n' + handler + '\n    }\n}\n'
code = code.replace("    // PRODUCTION_GEOMETRY", geometry)
code += r"""
int main()
{
    DisplayPerformance display;
    assert(display.P_column_instrument_element[0] == display.P_column_TONE_title && display.P_column_TONE_title == 7);
    assert(display.P_chars_instrument_element[0] == 3);
    display.P_show_Tone_value(0, 0, false);
    assert(tft.text == "OFF" && cleared == 3);
    rotation = -1;
    Step();
    assert(Patch[0].Instrument[0].lock == 0 && updates == 0);
    rotation = 1;
    for (int cutoff = 30; cutoff <= 80; ++cutoff)
    {
        Step();
        assert(Patch[0].Instrument[0].lock == cutoff);
        display.P_show_Tone_value(0, 0, true);
        assert(tft.text == std::to_string(cutoff / 10) + "." + std::to_string(cutoff % 10));
        assert(cleared == 3);
    }
    assert(updates == 51 && redraws == 51);
    Step();
    assert(Patch[0].Instrument[0].lock == 80 && updates == 51);
    rotation = -1;
    for (int cutoff = 79; cutoff >= 30; --cutoff)
    {
        Step();
        assert(Patch[0].Instrument[0].lock == cutoff);
    }
    Step();
    assert(Patch[0].Instrument[0].lock == 0);
    display.P_show_Tone_value(0, 0, false);
    assert(tft.text == "OFF" && cleared == 3);
    rotation = 0;
    const int before = updates;
    Step();
    assert(updates == before && interrupts);
    std::cout << "PASS: TONE OFF/3.0..8.0 selection, boundaries and one-decimal rendering\n";
}
"""
compiler = shutil.which('g++') or 'C:/msys64/ucrt64/bin/g++.exe'
env = os.environ.copy()
env['PATH'] = str(Path(compiler).parent) + os.pathsep + env.get('PATH', '')
with tempfile.TemporaryDirectory(prefix='tone-controls-') as folder:
    cpp = Path(folder) / 'test.cpp'
    exe = Path(folder) / 'test.exe'
    cpp.write_text(code, encoding='utf-8', newline='\r\n')
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True, env=env)
    subprocess.run([str(exe)], check=True, env=env)
