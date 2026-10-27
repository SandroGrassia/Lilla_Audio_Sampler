"""Exercise exclusive prelisten, line input mute, live routing and Sound menu transitions."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile
from production_source import method

root = Path(__file__).resolve().parents[2]
shared = (root / 'lib/SharedMixer/SharedMixer.cpp').read_text(encoding='utf-8')
main = (root / 'src/main.cpp').read_text(encoding='utf-8')
header = (root / 'lib/SharedSound/SharedSound.h').read_text(encoding='utf-8')
production = re.sub(r'^#include.*$', '', shared, flags=re.M)
production += method(main, 'void S_Select_menu_elements(void)')
menu = re.search(r'enum S_menu_elements_name\s*\{.*?\};', header, re.S).group(0)
menu += '\n' + '\n'.join(line for line in header.splitlines() if line.startswith(('static constexpr int S_menu_elements', 'static constexpr int S_row_menu', 'static constexpr char S_menu_char', 'static constexpr uint8_t S_dimension_voice_menu')))
menu += '\nuint8_t S_column_menu_element[S_menu_elements], S_position_menu[S_menu_elements];\nS_menu_elements_name S_element_menu[S_menu_elements];\n'
display = (root / 'lib/DisplaySound/DisplaySound.cpp').read_text(encoding='utf-8')
production += '\n' + method(display, 'void DisplaySound::Show_SOUND_menu(void)')

lineout_start = main.index('            case value_MX_Lineout:')
lineout_end = main.index('            case value_MX_Monitor:', lineout_start)
lineout_case = main[lineout_start:lineout_end]
production += '\nvoid handle_lineout()\n{\n    switch (0)\n    {\n' + lineout_case.replace('case value_MX_Lineout:', 'case 0:') + '\n    }\n}\n'

prelisten_end = main.index('            default:', lineout_end)
production += '\nvoid handle_prelisten()\n{\n    switch (0)\n    {\n' + main[lineout_end:prelisten_end].replace('case value_MX_Monitor:', 'case 0:') + '\n    }\n}\n'

solo_start = main.index('                case value_S_Solo:')
solo_end = main.index('                case value_S_OnlyPrelisten:', solo_start)
production += '\nvoid handle_solo(int choice)\n{\n    switch (choice)\n    {\n' + main[solo_start:solo_end] + '\n    }\n}\n'

mute_start = main.index('            case value_MX_Mute:')
mute_end = main.index('            case value_MX_Gain:', mute_start)
production += '\nvoid handle_mute()\n{\n    switch (0)\n    {\n' + main[mute_start:mute_end].replace('case value_MX_Mute:', 'case 0:') + '\n    }\n}\n'

prefix = r'''
#define FLASHMEM
#include <cassert>
#include <cstdint>
#include <iostream>
#include <cstring>
constexpr int MX_sources = 9, LINE_IN_source = 8, INSTRUMENTS = 8;
extern uint8_t MX_routing_source[MX_sources];
class PlayersManager
{
public:
    int routes[8] = {};
    int solo_releases = 0;
    void Update_Preset_volume(int, int, float) {}
    void Multicast_volume_for_instrument_edit(int) {}
    void Release_all_players_for_instrument_solo(int) { ++solo_releases; }
    void MX_multicast_change_routing(int instrument) { routes[instrument] = MX_routing_source[instrument]; }
};
class AmpliOutMuteIn
{
public:
    float level = -1;
    void gain(int channel, float value) { assert(channel == 1); level = value; }
};
constexpr int DIRECT_SAMPLING = 1, MIDI_LOOP = 2, PATCHES_MAX = 200;
int Lilla_state_0 = 0, Instrument_id = 0, Patch_id = 0, S_menu_max = 0, free_sounds = 1;
struct PatchData { int instruments = 2; } Patch[1];
bool S_Menu[10] = {};
int S_flash_first = -1, Sound_id = 0;
uint32_t S_sd_files = 0;
struct SoundData { int file = 0; } Sound[1];
bool solo_flag = false;
int S_solo_instrument = -1, volume_patch = 0;
float Patch_volume_gain(int) { return 1.0f; }
int S_Get_sounds_free() { return free_sounds; }
PlayersManager Players_Manager;
AmpliOutMuteIn MAIN_mixer_out_L, MAIN_mixer_out_R, PWM_mixer_out_L, PWM_mixer_out_R;
struct MixerPointer { int source = 0; } MX_local_pointer;
struct DisplayMixerStub
{
    void MX_source_values(int, bool) {}
    void MX_source_values_edit(int source) { assert(source == MX_local_pointer.source); }
} Display_Mixer;
constexpr int EN_PB_Value = 0;
bool value_pressed = false;
int value_rotation = 0;
bool Read_pushbutton(int) { const bool pressed = value_pressed; value_pressed = false; return pressed; }
int Read_encoder_simple(int) { const int rotation = value_rotation; value_rotation = 0; return rotation; }
void AudioNoInterrupts() {}
void AudioInterrupts() {}
constexpr int ILI9341_BLACK = 0, MENU_COLOR = 1;
int display_coordinate_x(int column) { return 4 + 6 * column; }
int display_coordinate_y(int row) { return 4 + 15 * row; }
struct TFTStub
{
    int x = 0, printed = 0;
    void fillRect(int, int, int, int, int) { printed = 0; }
    void setCursor(int column, int) { x = column; }
    void setTextColor(int) {}
    void print(const char *label) { assert(x + 6 * std::strlen(label) + 3 <= 320); ++printed; }
} tft;
struct SoundPointer { int menu_element = 0; } S_pointer;
class DisplaySound
{
public:
    int menu_first_visible = 0;
    void Show_SOUND_menu();
    void Show_pointer_frame(SoundPointer, bool) {}
    void Show_wave(int) {}
} Display_Sound;
struct PointerSoundStub { void Display_pointer() {} } Pointer_Sound;
int solo_maps = 0, restored_maps = 0;
void S_Map_one_Instrument_for_all_notes(int) { ++solo_maps; }
void P_Update_all_maps_Instrument_for_notes() { ++restored_maps; }
void DS_sync_stereo_parameters() {}
void Clear_UI_events() {}



'''
checks = r'''
int main()
{
    for (int source = 0; source < MX_sources; ++source)
    {
        assert(MX_Mute_allowed(source, false, -1));
        assert(MX_Mute_allowed(source, true, -1) == (source == LINE_IN_source));
        for (int solo = 0; solo < INSTRUMENTS; ++solo)
        {
            assert(MX_Mute_allowed(source, false, solo) == (source != solo));
        }
    }
    for (int source = 0; source < MX_sources; ++source)
    {
        MX_local_pointer.source = source;
        Instrument_id = source < INSTRUMENTS ? source : 0;
        MX_mute[source] = false;
        value_rotation = 1;
        handle_mute();
        assert(MX_mute[source]);
        value_rotation = 1;
        handle_mute();
        assert(MX_mute[source]);
        value_rotation = -1;
        handle_mute();
        assert(!MX_mute[source]);
        value_pressed = true;
        handle_mute();
        assert(MX_mute[source]);
        value_pressed = true;
        handle_mute();
        assert(!MX_mute[source]);
        solo_flag = true;
        S_solo_instrument = Instrument_id;
        value_rotation = 1;
        handle_mute();
        assert(MX_mute[source] == (source == LINE_IN_source));
        solo_flag = false;
        MX_mute[source] = false;
        Patch_id = PATCHES_MAX;
        Lilla_state_0 = DIRECT_SAMPLING;
        value_pressed = true;
        handle_mute();
        assert(MX_mute[source] == (source == LINE_IN_source));
        value_rotation = 1;
        handle_mute();
        assert(MX_mute[source] == (source == LINE_IN_source));
        Patch_id = 0;
        Lilla_state_0 = 0;
        MX_mute[source] = false;
    }
    Instrument_id = 0;
    PlayersManager players;
    AmpliOutMuteIn left, right, preleft, preright;
    for (int source = 0; source < MX_sources; ++source)
    {
        assert((MX_routing_source[source] & 1) == 0);
    }
    for (int previous = 0; previous < MX_sources; ++previous)
    {
        for (int selected = 0; selected < MX_sources; ++selected)
        {
            for (int track = 0; track < MX_sources; ++track)
            {
                MX_routing_source[track] = (track % 2) * 2;
                MX_mute[track] = true;
            }
            MX_Select_prelisten(previous, true);
            MX_Select_prelisten(selected, true);
            MX_Apply_routing(players, left, right, preleft, preright);
            for (int track = 0; track < MX_sources; ++track)
            {
                assert(MX_routing_source[track] == (track % 2) * 2 + (track == selected));
                assert(MX_mute[track]);
                if (track < LINE_IN_source)
                {
                    assert(players.routes[track] == MX_routing_source[track]);
                }
            }
            assert(left.level == 0 && right.level == 0 && preleft.level == 0 && preright.level == 0);
            MX_Select_prelisten(selected, false);
            for (int track = 0; track < MX_sources; ++track)
            {
                assert((MX_routing_source[track] & 1) == 0);
            }
            MX_Only_prelisten(selected);
            assert(MX_routing_source[selected] == 1 && !MX_mute[selected]);
            MX_Send_to_lineout(selected);
            assert(MX_routing_source[selected] == 3 && !MX_mute[selected]);
            MX_Only_prelisten(selected);
            assert(MX_routing_source[selected] == 1);
        }
    }
    MX_Only_prelisten(LINE_IN_source);
    MX_Apply_routing(players, left, right, preleft, preright);
    assert(left.level == 0 && right.level == 0 && preleft.level == 1 && preright.level == 1);
    MX_Send_to_lineout(LINE_IN_source);
    MX_Select_prelisten(0, true);
    MX_Apply_routing(players, left, right, preleft, preright);
    assert(left.level == 1 && right.level == 1 && preleft.level == 0 && preright.level == 0);
    for (int routing = 0; routing < 4; ++routing)
    {
        for (int mute = 0; mute < 2; ++mute)
        {
            MX_routing_source[LINE_IN_source] = routing;
            MX_mute[LINE_IN_source] = mute;
            MX_Apply_routing(players, left, right, preleft, preright);
            assert(left.level == (!mute && (routing & 2) ? 1 : 0));
            assert(preleft.level == (!mute && (routing & 1) ? 1 : 0));
            assert(right.level == left.level && preright.level == preleft.level);
        }
        MX_routing_source[Instrument_id] = routing;
        S_Select_menu_elements();
        assert(S_Menu[value_S_OnlyPrelisten] == (routing != 1));
        assert(S_Menu[value_S_SendToLineOut] == ((routing & 2) == 0));
        assert(S_Menu[value_S_BrowseSd] && !S_Menu[value_S_BrowseFlash] && !S_Menu[value_S_SaveSd]);
        assert(S_menu_max == (routing == 0 ? 6 : 5));
    }
    Lilla_state_0 = 0;
    for (int instrument = 0; instrument < INSTRUMENTS; ++instrument)
    {
        Instrument_id = MX_local_pointer.source = instrument;
        MX_Only_prelisten(instrument);
        S_Select_menu_elements();
        assert(S_Menu[value_S_SendToLineOut]);
        value_pressed = true;
        handle_lineout();
        assert(MX_routing_source[instrument] == 3 && Players_Manager.routes[instrument] == 3);
        assert(S_Menu[value_S_OnlyPrelisten] && !S_Menu[value_S_SendToLineOut]);
        value_pressed = true;
        handle_lineout();
        assert(MX_routing_source[instrument] == 1 && S_Menu[value_S_SendToLineOut]);
    }
    for (int source = 0; source < MX_sources; ++source)
    {
        MX_local_pointer.source = source;
        Instrument_id = source < INSTRUMENTS ? source : 0;
        MX_routing_source[source] = 0;
        value_rotation = 1;
        handle_lineout();
        assert(MX_routing_source[source] == 2);
        value_rotation = 1;
        handle_lineout();
        assert(MX_routing_source[source] == 2);
        value_rotation = -1;
        handle_lineout();
        assert(MX_routing_source[source] == 0);
        value_rotation = 1;
        handle_prelisten();
        assert(MX_routing_source[source] == 1);
        value_rotation = 1;
        handle_prelisten();
        assert(MX_routing_source[source] == 1);
        value_rotation = -1;
        handle_prelisten();
        assert(MX_routing_source[source] == 0);
        value_pressed = true;
        handle_prelisten();
        assert(MX_routing_source[source] == 1);
        value_pressed = true;
        handle_prelisten();
        assert(MX_routing_source[source] == 0);
    }
    MX_Only_prelisten(0);
    Instrument_id = MX_local_pointer.source = 1;
    value_rotation = 1;
    handle_prelisten();
    assert(MX_routing_source[0] == 0 && MX_routing_source[1] == 1);
    Instrument_id = 0;
    S_Select_menu_elements();
    assert(S_Menu[value_S_OnlyPrelisten] && S_Menu[value_S_SendToLineOut]);
    MX_Send_to_lineout(0);
    S_Select_menu_elements();
    assert(S_Menu[value_S_OnlyPrelisten] && !S_Menu[value_S_SendToLineOut]);
    solo_flag = true;
    S_Select_menu_elements();
    assert(S_Menu[value_S_ResetSolo] && !S_Menu[value_S_Solo]);
    solo_flag = false;
    S_Select_menu_elements();
    assert(!S_Menu[value_S_ResetSolo] && S_Menu[value_S_Solo]);
    DisplaySound display;
    for (int routing = 0; routing < 4; ++routing)
    {
        for (int solo = 0; solo < 2; ++solo)
        {
            solo_flag = solo;
            MX_routing_source[Instrument_id] = routing;
            S_Select_menu_elements();
            display.Show_SOUND_menu();
            for (int position = 0; position <= S_menu_max; ++position)
            {
                display.menu_first_visible = position;
                display.Show_SOUND_menu();
                assert(S_column_menu_element[position] != 255);
                const int element = S_element_menu[position];
                assert(S_Menu[element]);
                assert(S_dimension_voice_menu[element] == std::strlen(S_menu_char[element]));
            }
        }
    }
    MX_routing_source[Instrument_id] = 0;
    solo_flag = false;
    S_Select_menu_elements();
    Display_Sound.Show_SOUND_menu();
    S_pointer.menu_element = S_position_menu[value_S_Solo];
    MX_mute[Instrument_id] = true;
    handle_solo(value_S_Solo);
    assert(S_solo_instrument == Instrument_id && !MX_mute[Instrument_id]);
    assert(solo_flag && solo_maps == 1 && Players_Manager.solo_releases == 1);
    assert(S_Menu[value_S_ResetSolo] && !S_Menu[value_S_Solo]);
    assert(MX_routing_source[Instrument_id] == 0);
    S_pointer.menu_element = S_position_menu[value_S_ResetSolo];
    handle_solo(value_S_ResetSolo);
    assert(!solo_flag && restored_maps == 1 && S_Menu[value_S_Solo]);
    assert(S_Menu[value_S_OnlyPrelisten] && S_Menu[value_S_SendToLineOut]);
    MX_routing_source[Instrument_id] = 2;
    Lilla_state_0 = MIDI_LOOP;
    S_Select_menu_elements();
    assert(S_menu_max == 3 && S_Menu[value_S_BrowseSd] && !S_Menu[value_S_Clone] && !S_Menu[value_S_Drop]);
    Lilla_state_0 = 0;
    S_flash_first = 0;
    Sound[0].file = SdSoundSource::FIRST;
    S_Select_menu_elements();
    assert(S_Menu[value_S_BrowseFlash] && !S_Menu[value_S_BrowseSd] && !S_Menu[value_S_SaveSd]);
    assert(S_menu_max == 5 && S_Menu[value_S_Clone] && S_Menu[value_S_Drop]);
    Sound[0].file = SdSoundSource::PREVIEW;
    S_Select_menu_elements();
    assert(S_Menu[value_S_BrowseFlash] && !S_Menu[value_S_BrowseSd] && S_Menu[value_S_SaveSd]);
    assert(S_menu_max == 4 && !S_Menu[value_S_Clone] && !S_Menu[value_S_Drop]);
    Lilla_state_0 = DIRECT_SAMPLING;
    S_Select_menu_elements();
    assert(!S_Menu[value_S_BrowseFlash] && !S_Menu[value_S_BrowseSd] && !S_Menu[value_S_SaveSd]);
    assert(S_menu_max == 1 && S_Menu[value_S_Return] && !S_Menu[value_S_OnlyPrelisten] && !S_Menu[value_S_SendToLineOut]);
    std::cout << "PASS: exclusive prelisten on all nine sources, live routing, line input mute and Sound menus\n";
}
'''
compiler = shutil.which('g++') or 'C:/msys64/ucrt64/bin/g++.exe'
environment = os.environ.copy()
environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
with tempfile.TemporaryDirectory(prefix='mixer-prelisten-') as folder:
    cpp = Path(folder) / 'test.cpp'
    exe = Path(folder) / 'test.exe'
    cpp.write_text(prefix + menu + production + checks, encoding='utf-8')
    subprocess.run([compiler, '-include', str(Path(__file__).resolve().parent / 'sd_sound_source_stub.h'), '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), str(Path(__file__).with_name('sd_sound_source_stub.cpp').resolve()), '-o', str(exe)], check=True, env=environment)
    subprocess.run([str(exe)], check=True, env=environment)
