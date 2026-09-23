"""Exercise production capture, source lookup, cache pinning and deferred RAW writes."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]

def body(name):
    return re.sub(r'^#(?:include|pragma)[^\n]*\n', '', (root / name).read_text(encoding='utf-8'), flags=re.M)

prefix = r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#define FLASHMEM
constexpr int INSTRUMENTS=8, SOUNDS_MAX=40, PATCHES_MAX=4, PLAYERS=2, FIRST_RECORDING_FILE=20, FIRST_LIVE_SAMPLING_FILE=22, PATCH_CACHE_ARRAY_COUNT=8;
constexpr uint32_t PATCH_CACHE_ARRAY_SAMPLES=1024;
constexpr int REC=1, PLAYONLY=2, LOOP_FWD=2, EN_PB_Select=0, ILI9341_RED=0, ILI9341_WHITE=1;
enum AudioFileStorage { Flash, Psram };
struct AudioFileSource { int16_t file_id=-1; AudioFileStorage storage=Flash; const int16_t *psram_ptr=nullptr; uint32_t samples=0; int8_t cache_id=-1; };
struct Delay_data_struct { int samples=0; };
struct Instrument { bool used=false; int sound_id=0, from_note=0,root_key=0,to_note=0,Filter=0; bool operator==(const Instrument &) const = default; };
struct Patch_struct { bool used=false; int instruments=0; ::Instrument Instrument[8]; };
struct Sound_struct { bool used=false; int file=0,A=0,B=0,mode=2,Noclick=0,pan=0; bool operator==(const Sound_struct &) const = default; };
struct Preset_struct { bool active=false; int file=0; };
struct RecordingData { bool consistent=false; int bytes=0; } Recording[1];
Patch_struct Patch[PATCHES_MAX+1], Patch_cache_P;
Sound_struct Sound[SOUNDS_MAX+2], S_Sound_cache_P[SOUNDS_MAX];
Preset_struct Preset[INSTRUMENTS];
Delay_data_struct Delay_data{37};
int Patch_id=PATCHES_MAX, Patch_id_old=0, LS_state=PLAYONLY, LS_mode=LOOP_FWD, LS_XY_delta=512, LS_X_sample=1900, LS_buffer_dim=2048;
bool LS_stereo=false, cancel=false, stuck=false;
int midi_starts=0, midi_stops=0, creates=0, scans=0, fail_file=-1, corrupt_file=-1;
int16_t mono[2048], left[2048], right[2048], cache[8][1024];
int16_t *LS_buffer_mono_ptr=mono,*LS_buffer_L_ptr=left,*LS_buffer_R_ptr=right;
char name_file[22][10];
std::map<std::string,std::vector<uint8_t>> flash;
std::vector<std::string> messages;
struct Screen { void fillRoundRect(int,int,int,int,int,int) {} void setTextColor(int) {} void setCursor(int,int) {} void print(const char *s) { messages.emplace_back(s); } } tft;
void Show_popup_text(const char *text, int, int, int = 0) { messages.emplace_back(text); }
void Show_popup_text(const char *first, const char *second, int, int, int = 0) { messages.emplace_back(first); messages.emplace_back(second); }
struct Recorder { bool writing=false; bool Is_writing() { return writing; } } LiveSampler;
struct Midi { void Start() { ++midi_starts; } void Stop() { ++midi_stops; } } Midi_reader;
struct Players { void Stop_all_players() {} uint16_t Get_cache_reference_mask() { return 0; } } Players_Manager;
struct PlayerStub { bool isPlaying() { return stuck; } } Player[PLAYERS];
void AudioNoInterrupts() {} void AudioInterrupts() {} void Clear_UI_events() {}
uint32_t millis() { static uint32_t tick=0; return ++tick; }
void Read_encoder(int,int &choice,int,int,int) { choice=1; }
bool Read_pushbutton(int);
void LS_refresh_LS_page() {} void P_Update_Patches_number() {}
bool P_Verify_is_Patch_original(const int patch_id);
bool S_Verify_is_Sound_original(int id) { return Sound[id] == S_Sound_cache_P[id]; }
int Get_sound_id(int patch, int instrument) { return Patch[patch].Instrument[instrument].sound_id; }
struct SerialStub { void print(const char *) {} void println(bool) {} } Serial;
int S_Get_Patch_id_free() { for(int i=0;i<PATCHES_MAX;++i) {
    if (!Patch[i].used)
    {
        return i;
    }
} return -1; }
void S_Copy_all_Sound_to_Sound_cache_P() { std::copy(std::begin(Sound),std::begin(Sound)+SOUNDS_MAX,std::begin(S_Sound_cache_P)); }
struct SerialFlashFile {
    std::string name; uint32_t offset=0;
    explicit operator bool() const { return flash.count(name)!=0; }
    uint32_t write(const void *src,uint32_t count) {
            if (std::stoi(name)==fail_file)
            {
                return 0;
            }
        auto &data=flash.at(name); assert(offset+count<=data.size());
        memcpy(data.data()+offset,src,count); offset+=count; return count;
    }
    uint32_t read(void *dst,uint32_t count) {
        auto &data=flash.at(name); assert(offset+count<=data.size());
        memcpy(dst,data.data()+offset,count); offset+=count;
            if (std::stoi(name)==corrupt_file)
            {
                static_cast<uint8_t *>(dst)[0]^=1;
            }
        return count;
    }
    void seek(uint32_t n) { offset=n; } void close() {}
};
struct FlashStub {
    bool exists(const char *n) { return flash.count(n)!=0; }
    bool create(const char *n,uint32_t size) { ++creates; flash[n].resize(size); return true; }
    SerialFlashFile open(const char *n) { return {n,0}; }
    void wait() {} bool remove(const char *n) { return flash.erase(n)!=0; }
} SerialFlash;
struct FlashFileRegisterParser { static uint32_t length(int id) { auto it=flash.find(name_file[id]); return it==flash.end()?0:it->second.size(); } };
struct Scanner { void Read_all_file_data() { ++scans; } } File_scanner;
'''
ui = r'''
bool Read_pushbutton(int) { return cancel || !Capture_learn_key; }
struct Shifters { void Update() {
    if (Capture_learn_key)
    {
        Capture_learn_note=61;
    }
} } Shifters_manager;
'''
checks = r'''
int main()
{
    for(int i=0;i<22;++i) { snprintf(name_file[i],10,"%d.raw",i); }
    for(int i=0;i<2048;++i) { mono[i]=i; left[i]=30000; right[i]=10000; }
    for(int i=0;i<8;++i) { PatchCache_Manager.Set_cache_pointer(i,cache[i]); }
    Patch[0].used=true;
    Patch[0].instruments=1;
    Patch[0].Instrument[0]={true,30,55,55,55,0};
    Sound[30].used=true;
    Patch_cache_P=Patch[0];
    S_Copy_all_Sound_to_Sound_cache_P();
    Patch[PATCHES_MAX].Instrument[0]={true,SOUNDS_MAX,60,60,60,0};
    assert(P_Verify_is_Patch_original(Patch_id_old));
    Sound[30].pan=4;
    Capture_sound(0);
    assert(!Patch[1].used && messages.back()=="SAVE CURRENT PATCH FIRST");
    Sound[30].pan=0;
    // Cancel and guard failures allocate neither metadata nor Flash.
    cancel=true; Capture_sound(0); cancel=false;
    assert(!Patch[1].used && creates==0 && !Capture_learn_key);
    LS_state=REC; Capture_sound(0); LS_state=PLAYONLY;
    assert(!Patch[1].used);
    // Stereo capture gets a normal patch and normal Sound IDs, with only one key prompt.
    LS_stereo=true; messages.clear(); Capture_sound(2);
    assert(Capture_new_patch==1 && Patch_id_old==1 && Patch[1].used && Patch[1].instruments==2);
    assert(Capture_patch_delay.samples==37 && creates==0);
    assert(std::count(messages.begin(),messages.end(),"ROOT KEY")==1);
    int soundL=Patch[1].Instrument[2].sound_id, soundR=Patch[1].Instrument[3].sound_id;
    int fileL=Sound[soundL].file, fileR=Sound[soundR].file;
    assert(soundL<SOUNDS_MAX && soundR<SOUNDS_MAX && fileL<FIRST_RECORDING_FILE);
    for(int i=2;i<=3;++i) { const auto &v=Patch[1].Instrument[i]; assert(v.root_key==61 && v.from_note==61 && v.to_note==61); }
    assert(Sound[soundL].pan==-16 && Sound[soundR].pan==16);
    int16_t value=0; assert(Capture_read(fileL,&value,512,1) && value==30000);
    assert(!Capture_read(fileL,&value,513,1) && !Capture_read(fileL,&value,-1,1));
    // Leaving the patch or resetting normal caches cannot evict unsaved audio.
    PatchCache_Manager.Set_required_files(Preset); PatchCache_Manager.Release_unreferenced_caches(0); PatchCache_Manager.Begin();
    assert(PatchCache_Manager.Get_source(fileL).storage==Psram);
    // Copy-on-write replacement must preserve a clone sharing the first source.
    Sound[9]=Sound[soundL]; LS_stereo=false; cancel=true; Capture_sound(2); cancel=false;
    // confirm accepts a press, but ROOT KEY cancellation leaves capture untouched.
    assert(Sound[soundL].file==fileL);
    Capture_sound(2);
    assert(Sound[soundL].file!=fileL && Capture_find(fileL)->audio.psram_ptr[0]==30000);
    // Directly test mono capture in an empty slot, including circular wrap and exact length.
    Capture_sound(4);
    int monoSound=Patch[1].Instrument[4].sound_id, monoFile=Sound[monoSound].file;
    auto *monoSource=Capture_find(monoFile);
    assert(monoSource->audio.psram_ptr[0]==1900 && monoSource->audio.psram_ptr[148]==0 && monoSource->audio.samples==513);
    // A failed second file keeps all sources in PSRAM; retry skips completed files.
    fail_file=fileR; assert(!Capture_materialize());
    assert(Capture_find(Sound[soundL].file)->written && !Capture_find(fileR)->written);
    assert(PatchCache_Manager.Get_source(fileR).storage==Psram);
    const int firstCreates=creates; fail_file=-1; assert(Capture_materialize());
    assert(creates==firstCreates+3); // Retry right, then write mono and the preserved clone source.
    assert(flash.at(name_file[monoFile]).size()==1026);
    const int completedCreates=creates; assert(Capture_materialize() && creates==completedCreates);
    // Publication removes only temporary pinning; standard cache playback remains valid.
    Preset[2]={true,fileL}; Preset[3]={true,fileR}; Preset[4]={true,monoFile};
    Capture_finish_save();
    assert(Capture_find(fileL)==nullptr && PatchCache_Manager.Get_source(fileL).storage==Psram);
    // Full source capacity fails before assigning a patch/Sound or writing any file.
    for(int i=0;i<8;++i) { Capture_sources[i]={{static_cast<int16_t>(i+10),Psram,cache[i],10,static_cast<int8_t>(i)},false}; Sound[i+20].used=true; Sound[i+20].file=i+10; }
    Capture_sound(6); assert(!Patch[1].Instrument[6].used && creates==completedCreates);
    // Readback mismatch never publishes the file and retains its PSRAM data.
    CaptureSource bad={{19,Psram,cache[0],10,0},false}; corrupt_file=19;
    assert(!Capture_write(bad) && !bad.written && !SerialFlash.exists(name_file[19]));
    std::cout << "PASS: normal IDs, root, cancel, circular capture, cache pinning, deferred save, retry, capacity and verification\n";
}
'''

program = prefix + body('lib/CaptureSources/CaptureSources.h') + body('lib/CaptureSources/CaptureSources.cpp') + ui
program += body('lib/PatchCacheManager/PatchCacheManager.h') + body('lib/PatchCacheManager/PatchCacheManager.cpp')
main_source = (root / 'src/main.cpp').read_text(encoding='utf-8')
for name in ['P_Verify_if_Instrument_original', 'P_Verify_is_Patch_original']:
    program += re.search(r'^bool ' + name + r'[^\n]*\n\{.*?^\}', main_source, re.M | re.S).group(0) + '\n'
capture_start = main_source.index('// Live capture creates ordinary patch/Sound metadata;')
capture_end = main_source.index('\nvoid Require_FRAM(byte result)\n{', capture_start)
program += '\nPatchCacheManager PatchCache_Manager;\n' + main_source[capture_start:capture_end] + checks
with tempfile.TemporaryDirectory(prefix='lilla-live-capture-') as directory:
    cpp = Path(directory) / 'capture.cpp'
    exe = Path(directory) / 'capture.exe'
    cpp.write_text(program, encoding='utf-8')
    compiler = shutil.which('g++')
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    environment = os.environ.copy()
    environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
    subprocess.run([str(exe)], check=True, env=environment)
