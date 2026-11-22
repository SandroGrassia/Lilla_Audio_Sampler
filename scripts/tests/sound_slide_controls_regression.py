"""Exercise SOUND cursor visibility and signed SLIDE rendering from production code."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile
from production_source import method

ROOT = Path(__file__).resolve().parents[2]
def read(path):
    return (ROOT / path).read_text(encoding='utf-8')

shared = read('lib/SharedSound/SharedSound.h')
pointer = read('lib/PointerSound/PointerSound.cpp')
display = read('lib/DisplaySound/DisplaySound.cpp')
header = read('lib/DisplaySound/DisplaySound.h')
types = shared[shared.index('enum S_field_name'):shared.index('extern uint32_t S_sd_files')]
coordinates = '\n'.join(line for line in header.splitlines() if 'static constexpr float S_column_row_' in line and '[2] = {' in line)
fixture = r'''
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
#define FLASHMEM
constexpr int LOOP_FWD = 2, TEXT_COLOR = 1, ILI9341_YELLOW = 2;
struct PresetData { int mode = 0, slide = 0; } Preset[1];
struct Print { int x, y; std::string text; };
struct Tft
{
    int x = 0, y = 0;
    std::vector<Print> prints;
    void setCursor(int a, int b) { x = a; y = b; }
    void setTextColor(int) {}
    void print(const char *text) { prints.push_back({x, y, text}); }
    void print(int value) { prints.push_back({x, y, std::to_string(value)}); }
} tft;
int display_coordinate_x(float column) { return static_cast<int>(column * 6); }
int display_coordinate_y(float row) { return static_cast<int>(4 + row * 15); }
void Cancel_text_reset_cursor(int x, int y, int) { tft.prints.clear(); tft.setCursor(x, y); }
'''
fixture += types + '\nstruct DisplaySound\n{\n' + coordinates + r'''
    void Show_pointer_frame(S_field_description_struct, bool) {}
    void Show_Slide_value(int instrument_id);
} Display_Sound;
struct PointerSound
{
    S_field_description_struct pointer = {field_S_Menu, 0, value_S_File};
    void Move_pointer(int value, int S_menu_max, bool slide_visible);
};
'''
fixture += method(display, 'FLASHMEM void DisplaySound::Show_Slide_value(int instrument_id)')
fixture += method(pointer, 'FLASHMEM void PointerSound::Move_pointer(const int value, const int S_menu_max, bool slide_visible)')
fixture += r'''
int main()
{
    for (bool visible : {false, true})
    {
        PointerSound cursor;
        cursor.Move_pointer(-1, 2, visible);
        assert(cursor.pointer.value_element == value_S_Noclick);
        cursor.Move_pointer(1, 2, visible);
        assert(cursor.pointer.field_name == field_S_Menu && cursor.pointer.menu_element == 0);
        cursor.pointer = {field_S_Value, 0, value_S_PlayMode};
        cursor.Move_pointer(1, 2, visible);
        assert(cursor.pointer.value_element == (visible ? value_S_Slide : value_S_Noclick));
        if (visible)
        {
            cursor.Move_pointer(1, 2, true);
            assert(cursor.pointer.value_element == value_S_Noclick);
        }
        cursor.Move_pointer(-1, 2, visible);
        assert(cursor.pointer.value_element == (visible ? value_S_Slide : value_S_PlayMode));
        if (visible)
        {
            cursor.Move_pointer(-1, 2, true);
            assert(cursor.pointer.value_element == value_S_PlayMode);
        }
    }
    for (int mode = 0; mode < 6; ++mode)
    {
        Preset[0].mode = mode;
        for (int slide : {-2000, -1, 0, 1, 2000})
        {
            Preset[0].slide = slide;
            Display_Sound.Show_Slide_value(0);
            if (mode != LOOP_FWD)
            {
                assert(tft.prints.empty());
                continue;
            }
            assert(tft.prints[0].text == "SLIDE");
            assert(tft.prints[0].x == display_coordinate_x(DisplaySound::S_column_row_Pitch[0]));
            assert(tft.prints[1].x - tft.prints[0].x == 6 * 6); // Five letters plus one blank character.
            assert(tft.prints.back().text == std::to_string(slide));
            assert(slide <= 0 || tft.prints[1].text == "+");
        }
    }
    assert(DisplaySound::S_column_row_NoClick[0] + 4 < 53);
}
'''
# Labels share the requested REL column; the value and its cursor remain inside the display.
for label in ('REL', 'NOCLK'):
    assert re.search(r'display_coordinate_x\(41\.5\).*?tft\.print\("' + label + r'"\)', display, re.S)
compiler = shutil.which('g++') or 'C:/msys64/ucrt64/bin/g++.exe'
with tempfile.TemporaryDirectory() as folder:
    cpp = Path(folder) / 'controls.cpp'
    exe = Path(folder) / 'controls.exe'
    cpp.write_text(fixture, encoding='utf-8')
    environment = os.environ.copy()
    environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True, env=environment)
    subprocess.run([str(exe)], check=True, env=environment)
print('PASS: signed SLIDE display, layout and conditional cursor navigation')
