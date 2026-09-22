"""Exercise production sampler controls and pointer navigation with a fake display."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
def read(name):
    return (root / name).read_text(encoding='utf-8')
def header(name):
    return re.sub(r'^#(?:include|pragma).*$', '', read(name), flags=re.M)
def method(source, signature):
    return re.search(r'^' + re.escape(signature) + r'\n\{.*?^\}', source, re.M | re.S).group(0) + '\n'

prefix = r"""
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
#include <iostream>
#define FLASHMEM
using byte = uint8_t;
constexpr int BAR_ELEMENTS = 50;
constexpr uint16_t ILI9341_BLACK = 0, ILI9341_RED = 0xf800, ILI9341_YELLOW = 0xffe0, TEXT_COLOR = 0xffff, RED_OFF = 0x7800;
int constrain(int value, int low, int high) { return std::clamp(value, low, high); }
int display_coordinate_x(float col) { return 4 + 6 * col; }
int display_coordinate_y(float row) { return 4 + 15 * row; }
enum DS_field_name { field_DS_Menu, field_DS_Value };
enum DS_menu_element_name { value_DS_CancelRecording, value_DS_PauseRec, value_DS_MonoRec, value_DS_StereoRec, value_DS_Stop };
enum DS_value_name { value_DS_Recording, value_DS_Gain };
struct DS_pointer_struct { DS_field_name field_name; DS_menu_element_name menu_element; DS_value_name value_element; };
int recording = 0, Line_in_gain = 8, DS_menu_max = 0;
int element_Menu_DS[12] = {}, position_Menu_DS[12] = {};
int X_position_Menu_DS[12] = {}, Y_position_Menu_DS[12] = {}, dimension_voice_Menu_DS[12] = {};
bool DS_recording_led_visible = true, DS_recording_led_redraw = false;
struct Draw { int x, y, w, h; uint16_t color; };
struct Text { int x, y; uint16_t color; std::string value; };
struct Display
{
    int x = 0, y = 0;
    uint16_t color = 0;
    std::vector<Draw> rectangles;
    std::vector<Text> labels;
    std::vector<int> numbers;
    void fillRect(int x, int y, int w, int h, uint16_t c) { rectangles.push_back({x,y,w,h,c}); }
    void drawRect(int x, int y, int w, int h, uint16_t c) { rectangles.push_back({x,y,w,h,c}); }
    void setCursor(int a, int b) { x = a; y = b; }
    void setTextColor(uint16_t c) { color = c; }
    void print(const char *s) { labels.push_back({x,y,color,s}); }
    void print(int value) { numbers.push_back(value); }
    void print(float, int) {}
} tft;
void Cancel_text_reset_cursor(int x, int y, int) { tft.setCursor(x, y); }
void Frame_by_col_row(float, float, int, bool) {}
int descriptions = 0;
"""
source = read('lib/DisplaySampler/DisplaySampler.cpp')
production = header('lib/DisplaySampler/DisplaySampler.h')
for signature in [
    'void DisplaySampler::DS_set_recording_controls(bool visible)',
    'void DisplaySampler::DS_sampler_IO(void)',
    'void DisplaySampler::DS_bar(int channel, int value)',
    'uint16_t DisplaySampler::DS_calc_bar_color(float value)',
    'void DisplaySampler::DS_sampler_txt(bool color)',
    'void DisplaySampler::DS_show_gain(void)',
    'void DisplaySampler::DS_show_pointer_frame(const DS_pointer_struct pointer, const bool show)',
]:
    production += method(source, signature)
production += '\nvoid DisplaySampler::DS_Recording_description(int, bool, bool) { ++descriptions; }\nDisplaySampler Display_Sampler;\n'
production += header('lib/PointerSampler/PointerSampler.h')
source = read('lib/PointerSampler/PointerSampler.cpp')
for signature in ['void PointerSampler::Set_pointer_to_first_menu_element(void)', 'void PointerSampler::Move_pointer(const int value)', 'DS_pointer_struct PointerSampler::Get_pointer(void)']:
    production += method(source, signature)
production += '\nvoid PointerSampler::Print_pointer(void) {}\n'
checks = r"""
int main()
{
    PointerSampler pointer;
    pointer.Set_pointer_to_first_menu_element();
    pointer.Move_pointer(1);
    assert(pointer.Get_pointer().value_element == value_DS_Recording);
    Display_Sampler.DS_bar(0, 25);
    Display_Sampler.DS_sampler_txt(true);
    Display_Sampler.DS_show_gain();
    assert(tft.rectangles.empty() && tft.labels.empty());
    Display_Sampler.DS_set_recording_controls(true);
    assert(Display_Sampler.DS_has_recording_controls());
    assert(!DS_recording_led_visible);
    assert(tft.labels.size() == 4);
    const char *labels[] = {"PAUSE+REC", "LINE IN GAIN", "LEVEL L", "LEVEL R"};
    for (int row = 0; row < 4; ++row)
    {
        assert(tft.labels[row].value == labels[row]);
        assert(tft.labels[row].y == display_coordinate_y(8 + row));
    }
    assert(tft.labels[0].color == ILI9341_RED);
    assert(tft.numbers.back() == 9);
    Line_in_gain = 0;
    Display_Sampler.DS_show_gain();
    assert(tft.numbers.back() == 1);
    Line_in_gain = 15;
    Display_Sampler.DS_show_gain();
    assert(tft.numbers.back() == 16);
    Display_Sampler.DS_sampler_txt(true);
    assert(tft.labels.back().color == ILI9341_RED);
    Display_Sampler.DS_sampler_txt(false);
    assert(tft.labels.back().color == RED_OFF);
    pointer.Set_pointer_to_first_menu_element();
    pointer.Move_pointer(1);
    assert(pointer.Get_pointer().field_name == field_DS_Value);
    assert(pointer.Get_pointer().value_element == value_DS_Gain);
    pointer.Move_pointer(-1);
    assert(pointer.Get_pointer().field_name == field_DS_Menu);
    tft.rectangles.clear();
    Display_Sampler.DS_bar(0, 100);
    assert(tft.rectangles.size() == 50);
    assert(tft.rectangles.front().x < tft.rectangles.back().x);
    assert(tft.rectangles.front().y == display_coordinate_y(10));
    assert((tft.rectangles.back().color & 0xf800) == ILI9341_RED);
    Display_Sampler.DS_bar(0, -5);
    assert(tft.rectangles.back().w == 100 && tft.rectangles.back().color == ILI9341_BLACK);
    Display_Sampler.DS_bar(1, 1);
    assert(tft.rectangles.back().y == display_coordinate_y(11));
    Display_Sampler.DS_set_recording_controls(false);
    assert(descriptions == 1);
    const auto count = tft.rectangles.size();
    Display_Sampler.DS_bar(1, 50);
    assert(tft.rectangles.size() == count);
    pointer.Set_pointer_to_first_menu_element();
    pointer.Move_pointer(1);
    assert(pointer.Get_pointer().value_element == value_DS_Recording);
    for (bool controls : {false, true})
    {
        Display_Sampler.DS_set_recording_controls(controls);
        pointer.Set_pointer_to_first_menu_element();
        pointer.Move_pointer(-1);
        assert(pointer.Get_pointer().field_name == field_DS_Value);
        assert(pointer.Get_pointer().value_element == (controls ? value_DS_Gain : value_DS_Recording));
        pointer.Move_pointer(1);
        assert(pointer.Get_pointer().field_name == field_DS_Menu);
        assert(pointer.Get_pointer().menu_element == element_Menu_DS[0]);
    }
    for (const auto &draw : tft.rectangles)
    {
        assert(draw.y >= 120 && draw.y + draw.h <= 240);
    }
    std::cout << "Sampler display regression passed: visibility, rows, blink, horizontal bars and pointer\n";
}
"""
compiler = shutil.which('g++') or r'C:\msys64\ucrt64\bin\g++.exe'
with tempfile.TemporaryDirectory(prefix='lilla-sampler-display-') as directory:
    cpp = Path(directory) / 'display.cpp'
    exe = Path(directory) / ('display.exe' if os.name == 'nt' else 'display')
    cpp.write_text(prefix + production + checks, encoding='utf-8', newline='\r\n')
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    environment = os.environ.copy()
    environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
    subprocess.run([str(exe)], check=True, env=environment)
