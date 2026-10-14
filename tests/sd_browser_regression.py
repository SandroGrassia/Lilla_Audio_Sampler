"""Exercise the production MIDI browser routing and persistent missing-audio overlay."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
main = (root / 'src/main.cpp').read_text(encoding='utf-8')
display = (root / 'lib/DisplaySound/DisplaySound.cpp').read_text(encoding='utf-8')
routing = main[main.index('bool S_SD_midi(uint8_t type, uint8_t channel, uint8_t key, uint8_t velocity)\n{'):]
methods = display[display.index('void DisplaySound::Mark_audio_unavailable'):display.index('void DisplaySound::Show_pointer_frame')].replace('FLASHMEM', '')
wave = display[display.index('void DisplaySound::Show_wave'):display.index('    int yp, yn, y0;', display.index('void DisplaySound::Show_wave'))] + '    ++draws;\n}\n'
fixture = r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <iostream>
constexpr int WAVE_MAX = 122, WAVEBOARD_HEIGHT = 97, X_WAVEBOARD_LEFT = 0, WAVEBOARD_WIDTH = 310, WAVE_BOARD_COLOR = 0;
constexpr int ILI9341_WHITE = 1, ILI9341_RED = 2, FIRST_LIVE_SAMPLING_FILE = 320;
int Patch_id = 0, Instrument_id = 0, draws = 0, popups = 0;
struct SoundData { int file = 1; } Sound[2];
struct InstrumentData { uint8_t root_key = 60; };
struct PatchData { InstrumentData Instrument[2]; } Patch[1];
int Get_sound_id(int, int instrument) { return instrument; }
uint8_t Get_midi_channel(int, int) { return 2; }
struct InfoData { int samples = 100; int Raw_file_samples(int) { return samples; } } Info;
struct DisplayData { void fillRect(int, int, int, int, int) {} } tft;
void Show_popup_text(const char *text, int foreground, int background, int offset)
{
    assert(strcmp(text, "AUDIO FILE UNAVAILABLE") == 0);
    assert(foreground == ILI9341_WHITE && background == ILI9341_RED && offset == 50);
    ++popups;
}
struct DisplaySound
{
    bool audio_unavailable[2] = {};
    void Mark_audio_unavailable(int);
    void Clear_audio_unavailable(int);
    void Show_audio_unavailable();
    void Show_wave(int);
};
bool S_sd_browsing = false, S_sd_unavailable = false, S_sd_new_instrument = false;
float pitch_from_note[128];
namespace midi { enum { NoteOn = 0x90, NoteOff = 0x80, ControlChange = 0xb0 }; }
struct Preview
{
    int on = 0, off = 0, silence = 0;
    uint8_t note = 0, volume = 0;
    float pitch = 0;
    void Note_on(uint8_t key, uint8_t velocity, float ratio) { ++on; note = key; volume = velocity; pitch = ratio; }
    void Note_off(uint8_t) { ++off; }
    void Silence() { ++silence; }
} SD_preview;
'''
checks = r'''
int main()
{
    DisplaySound display;
    display.Show_wave(0);
    assert(draws == 1 && popups == 0);
    Info.samples = 0;
    display.Show_wave(0);
    assert(draws == 1 && popups == 1);
    Info.samples = 100;
    display.Show_wave(0);
    assert(draws == 1 && popups == 2); // Restoring the source cannot dismiss the latch.
    display.Show_wave(1);
    assert(draws == 2 && popups == 2);
    display.Clear_audio_unavailable(0);
    display.Show_wave(0);
    assert(draws == 3 && popups == 2);
    display.Mark_audio_unavailable(0);
    display.Show_wave(0);
    assert(draws == 3 && popups == 3);
    for (int i = 0; i < 128; ++i) { pitch_from_note[i] = std::pow(2.0f, (i - 60) / 12.0f); }
    assert(!S_SD_midi(midi::NoteOn, 3, 72, 90) && SD_preview.on == 0);
    S_sd_browsing = true;
    assert(!S_SD_midi(midi::NoteOn, 1, 72, 90));
    assert(S_SD_midi(midi::NoteOn, 3, 72, 90));
    assert(SD_preview.on == 1 && SD_preview.pitch == 2.0f && SD_preview.volume == 90);
    assert(!S_SD_midi(midi::NoteOff, 3, 72, 0) && SD_preview.off == 1);
    assert(S_SD_midi(midi::ControlChange, 3, 123, 0) && SD_preview.silence == 1);
    S_sd_unavailable = true;
    assert(S_SD_midi(midi::NoteOn, 3, 72, 90) && SD_preview.on == 1);
    S_sd_unavailable = false;
    S_sd_new_instrument = true;
    assert(S_SD_midi(midi::NoteOn, 1, 60, 127) && SD_preview.pitch == 1.0f);
    std::cout << "SD browser routing and missing-audio overlay passed\n";
}
'''
compiler = shutil.which('g++') or 'C:/msys64/ucrt64/bin/g++.exe'
with tempfile.TemporaryDirectory(prefix='lilla-sd-browser-') as directory:
    cpp = Path(directory) / 'test.cpp'
    exe = Path(directory) / 'test.exe'
    cpp.write_text(fixture + methods + wave + routing + checks, encoding='utf-8')
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    environment = os.environ.copy()
    environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
    subprocess.run([str(exe)], check=True, env=environment)
