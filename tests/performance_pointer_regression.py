"""Exercise production Performance field memory across instruments, menus and shortcuts."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]

def read(name):
    return (root / name).read_text(encoding='utf-8')

def strip(text):
    return '\n'.join(line for line in text.splitlines() if not line.startswith(('#include', '#pragma')))

shared = read('lib/SharedPerformance/SharedPerformance.h')
types = shared[shared.index('enum P_field_name'):shared.index('extern int8_t instrument_on_position')]
prefix = r"""
#include <cassert>
#include <cstdint>
#include <iostream>
#define FLASHMEM
struct SerialStub
{
    template<class T> void print(T) {}
    template<class T> void println(T) {}
} Serial;
int Patch_id = 0;
struct PatchData { int instruments = 3; } Patch[1];
int8_t instrument_on_position[8] = {0, 3, 6};
int P_line_of_instrument[8] = {0, 0, 0, 1, 0, 0, 2};
""" + types + r"""
struct DisplayStub
{
    P_field_description_struct erased{}, shown{};
    void P_show_pointer_frame(P_field_description_struct pointer, bool show)
    {
        if (show)
        {
            shown = pointer;
        }
        else
        {
            erased = pointer;
        }
    }
} Display_Performance;
"""
code = prefix + strip(read('lib/PointerPerformance/PointerPerformance.h')) + '\n' + strip(read('lib/PointerPerformance/PointerPerformance.cpp'))
code += r"""
int main()
{
    PointerPerformance pointer;
    pointer.Set_pointer_to_Patch();
    pointer.Move_pointer(1, 4);
    pointer.Move_pointer_from_Instrument_to_inside();
    assert(pointer.Get_pointer().element == value_P_Hifi);
    for (int field = value_P_Hifi; field <= value_P_Gain; ++field)
    {
        while (pointer.Get_pointer().element != field)
        {
            pointer.Move_pointer(1, 4);
        }
        pointer.Move_pointer_from_inside_to_Instrument();
        const auto old = pointer.Get_pointer();
        // Changing instrument preserves the selected field, including the last-to-first route via menu/Patch.
        if (old.instrument_line == 2)
        {
            pointer.Set_pointer_to_Patch();
        }
        pointer.Move_pointer(1, 4);
        pointer.Move_pointer_from_Instrument_to_inside();
        assert(pointer.Get_pointer().element == field);
        assert(pointer.Get_pointer().instrument_id == instrument_on_position[(old.instrument_line + 1) % 3]);
        assert(Display_Performance.erased.field_name == field_P_Instrument);
        assert(Display_Performance.shown.field_name == field_P_Instrument_inside && Display_Performance.shown.element == field);
    }
    pointer.Move_pointer(1, 4);
    assert(pointer.Get_pointer().element == value_P_Hifi);
    pointer.Move_pointer(-1, 4);
    assert(pointer.Get_pointer().element == value_P_Gain);
    pointer.Set_pointer_to_RootKey(6);
    pointer.Move_pointer_from_inside_to_Instrument();
    pointer.Move_pointer(-1, 4);
    pointer.Move_pointer_from_Instrument_to_inside();
    assert(pointer.Get_pointer().element == value_P_RootKey && pointer.Get_pointer().instrument_id == 3);
    pointer.Move_pointer(-1, 4);
    assert(pointer.Get_pointer().element == value_P_Midi);
    pointer.Move_pointer_from_inside_to_Instrument();
    pointer.Set_pointer_to_first_menu_voice();
    pointer.Move_pointer(-1, 4);
    pointer.Move_pointer_from_Instrument_to_inside();
    assert(pointer.Get_pointer().element == value_P_Midi && pointer.Get_pointer().instrument_id == 6);
    pointer.Set_pointer_to_last_instrument(0);
    pointer.Move_pointer_from_Instrument_to_inside();
    assert(pointer.Get_pointer().element == value_P_Midi && pointer.Get_pointer().instrument_id == 0);
    std::cout << "PASS: remembered Performance fields, MIDI across instruments, wrapping and RootKey shortcuts\n";
}
"""
compiler = shutil.which('g++') or 'C:/msys64/ucrt64/bin/g++.exe'
environment = os.environ.copy()
environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
with tempfile.TemporaryDirectory(prefix='performance-pointer-') as folder:
    cpp = Path(folder) / 'test.cpp'
    exe = Path(folder) / 'test.exe'
    cpp.write_text(code, encoding='utf-8', newline='\r\n')
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True, env=environment)
    subprocess.run([str(exe)], check=True, env=environment)
