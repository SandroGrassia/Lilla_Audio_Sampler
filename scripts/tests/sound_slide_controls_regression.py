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
main = read('src/main.cpp')
types = shared[shared.index('enum S_field_name'):shared.index('extern uint32_t S_sd_files')]
coordinates = '\n'.join(line for line in header.splitlines() if 'static constexpr float S_column_row_' in line and '[2] = {' in line)
fixture = r'''
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
#define FLASHMEM
constexpr int LOOP_FWD = 2, LOOP_REV = 5, TEXT_COLOR = 1, ILI9341_YELLOW = 2;
struct PresetData { int mode = 0, slide = 0; } Preset[2];
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
    S_field_description_struct frame = {field_S_Menu, 0, value_S_File};
    void Show_pointer_frame(S_field_description_struct selected, bool show) { if (show) { frame = selected; } }
    void Show_Slide_value(int instrument_id);
} Display_Sound;
struct PointerSound
{
    S_field_description_struct pointer = {field_S_Menu, 0, value_S_File};
    void Move_pointer(int value, int S_menu_max, bool slide_visible);
    void Restore_pointer(int S_menu_max, bool slide_visible);
    void Display_pointer();
    S_field_description_struct Get_pointer();
};
PointerSound Pointer_Sound;
S_field_description_struct S_pointer;
int Instrument_id = 0, S_menu_max = 2;
'''
fixture += method(display, 'FLASHMEM void DisplaySound::Show_Slide_value(int instrument_id)')
fixture += method(pointer, 'FLASHMEM void PointerSound::Move_pointer(const int value, const int S_menu_max, bool slide_visible)')
fixture += method(pointer, 'FLASHMEM void PointerSound::Restore_pointer(int S_menu_max, bool slide_visible)')
fixture += method(pointer, 'void PointerSound::Display_pointer(void)')
fixture += method(pointer, 'S_field_description_struct PointerSound::Get_pointer(void)')
restore_start = main.index('                    // Restore the pointer within the selected Sound')
restore_end = main.index('                    Performance_led_set.Restore_all_LED();', restore_start)
fixture += 'void Restore_selected_sound()\n{\n' + main[restore_start:restore_end] + '}\n'
fixture += r'''
int main()
{
    // Two-instrument patch: preserve SLIDE only when the destination actually displays it.
    for (int origin : {LOOP_FWD, LOOP_REV})
    {
        Preset[0].mode = origin;
        for (int destination = 0; destination < 6; ++destination)
        {
            Preset[1].mode = destination;
            Instrument_id = 0;
            Pointer_Sound.pointer = {field_S_Value, 0, value_S_PlayMode};
            Pointer_Sound.Move_pointer(1, S_menu_max, true);
            assert(Pointer_Sound.pointer.value_element == value_S_Slide);
            Instrument_id = 1;
            Restore_selected_sound();
            const S_value_name expected = destination == LOOP_FWD || destination == LOOP_REV ? value_S_Slide : value_S_PlayMode;
            assert(S_pointer.field_name == field_S_Value && S_pointer.value_element == expected);
            assert(Pointer_Sound.Get_pointer().value_element == expected);
            assert(Display_Sound.frame.value_element == expected);
            Instrument_id = 0;
            Restore_selected_sound();
            assert(S_pointer.value_element == expected); // Returning keeps the valid selection.
        }
    }
    Preset[1].mode = 0;
    Instrument_id = 1;
    Pointer_Sound.pointer = {field_S_Value, 0, value_S_Pitch};
    Restore_selected_sound();
    assert(S_pointer.value_element == value_S_Pitch);
    Pointer_Sound.pointer = {field_S_Menu, S_menu_max + 1, value_S_File};
    Restore_selected_sound();
    assert(S_pointer.field_name == field_S_Menu && S_pointer.menu_element == S_menu_max);
    assert(Display_Sound.frame.menu_element == S_menu_max);

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
            if (mode != LOOP_FWD && mode != LOOP_REV)
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
