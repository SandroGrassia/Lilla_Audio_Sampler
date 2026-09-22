"""Exercise production capture assignment, replacement and circular PSRAM copies."""
from pathlib import Path
import re
import os
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/PlaybackPerformance.inc').read_text(encoding='utf-8')

def method(name):
    return re.search(r'^FLASHMEM static void ' + name + r'[^\n]*\n\{.*?^\}', source, re.M | re.S).group(0) + '\n'

prefix = r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>
#include <iostream>
#define FLASHMEM
constexpr int PLAYBACK_FILES=8, INSTRUMENTS=8, SOUNDS_MAX=2, FIRST_PLAYBACK_SOUND=4, FIRST_PLAYBACK_FILE=323, PATCHES_MAX=1, LOOP_FWD=2, REC=1, PLAYONLY=2, Psram=1;
constexpr uint32_t PATCH_CACHE_ARRAY_SAMPLES=1024;
struct Instrument { bool used=false; int sound_id=0, from_note=0,root_key=0,to_note=0, Filter=0; };
struct Patch_struct { bool used=false; int instruments=0; Instrument Instrument[8]; };
struct Sound_struct { bool used=false; int file=0,A=0,B=0,mode=2,Noclick=0,pan=0; };
struct AudioFileSource { int16_t file_id=-1; int storage=0; const int16_t *psram_ptr=nullptr; uint32_t samples=0; int8_t cache_id=-1; };
struct Delay_data_struct {};
Patch_struct Playback_patch, Patch[2];
Sound_struct Sound[12];
AudioFileSource Playback_sources[8];
int8_t Playback_partner[8]={-1,-1,-1,-1,-1,-1,-1,-1};
Delay_data_struct Playback_delay, Delay_data;
bool Playback_delay_valid=false, LS_stereo=false;
int Playback_return_patch=0, Patch_id_old=0, LS_state=PLAYONLY, LS_mode=LOOP_FWD, LS_XY_delta=512, LS_X_sample=1900, LS_buffer_dim=2048;
int16_t mono[2048], left[2048], right[2048], cache[8][1024];
int16_t *LS_buffer_mono_ptr=mono,*LS_buffer_L_ptr=left,*LS_buffer_R_ptr=right;
struct Recorder { bool writing=false; bool Is_writing() { return writing; } } LiveSampler;
struct Cache { int16_t *Reserve_playback(int slot,uint32_t samples) { assert(slot<8 && samples<=1024); return cache[slot]; } } PatchCache_Manager;
struct Midi { void Start() {} } Midi_reader;
std::string notice;
bool confirm=true, keys_ok=true, drain=true;
void Playback_notice(const char *s) { notice=s; }
void LS_refresh_LS_page() {}
bool Playback_confirm(const char *) { return confirm; }
bool Playback_keys(uint8_t (&k)[3]) { k[0]=48; k[1]=60; k[2]=72; return keys_ok; }
bool Playback_drain_players() { return drain; }
'''
checks = r'''
int main()
{
    for (int i=0;i<2048;++i)
    {
        mono[i]=i; left[i]=30000; right[i]=10000;
    }
    LS_state=REC;
    Playback_capture(0);
    assert(notice=="STOP RECORDING FIRST" && !Playback_patch.used);
    LS_state=PLAYONLY; LS_XY_delta=1024;
    Playback_capture(0);
    assert(notice=="LOOP TOO LONG FOR CACHE" && !Playback_patch.used);
    LS_XY_delta=512; LS_stereo=true;
    Playback_capture(2);
    assert(Playback_partner[2]==3 && Playback_partner[3]==2);
    assert(cache[2][0]==30000 && cache[3][512]==10000);
    assert(Sound[6].pan==-16 && Sound[7].pan==16);
    assert(Playback_patch.Instrument[2].root_key==Playback_patch.Instrument[3].root_key);
    confirm=false; Playback_capture(3);
    assert(cache[2][0]==30000 && Playback_partner[3]==2);
    confirm=true; LS_stereo=false;
    Playback_capture(3);
    assert(Playback_partner[2]==-1 && Playback_partner[3]==-1);
    assert(cache[2][0]==20000 && cache[3][0]==1900 && cache[3][148]==0);
    assert(Sound[6].pan==0 && Sound[7].pan==0);
    assert(Playback_patch.Instrument[2].root_key==60);
    LS_stereo=true; Playback_capture(1);
    assert(Playback_partner[1]==-1 && cache[1][0]==20000 && cache[2][0]==20000);
    Playback_capture(7);
    assert(Playback_partner[7]==-1 && cache[7][512]==20000);
    Playback_capture(4);
    assert(Playback_partner[4]==5);
    left[0]=123; right[0]=-123; LS_X_sample=0;
    Playback_capture(5);
    assert(cache[4][0]==123 && cache[5][0]==-123 && Sound[8].pan==-16);
    keys_ok=false; LS_stereo=false;
    Playback_capture(4);
    assert(cache[4][0]==123 && Playback_partner[4]==5);
    std::cout << "PASS: capture guards, wrap, stereo replacement, mono split, occupied next slot, slot 8 and cancellation\n";
}
'''
with tempfile.TemporaryDirectory(prefix='lilla-playback-') as directory:
    cpp = Path(directory) / 'capture.cpp'
    exe = Path(directory) / 'capture.exe'
    cpp.write_text(prefix + method('Playback_publish_source') + method('Playback_capture') + checks, encoding='utf-8')
    subprocess.run([shutil.which('g++'), '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    environment = os.environ.copy()
    environment['PATH'] = str(Path(shutil.which('g++')).parent) + os.pathsep + environment.get('PATH', '')
    subprocess.run([str(exe)], check=True, env=environment)
