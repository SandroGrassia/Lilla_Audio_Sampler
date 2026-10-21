"""Exercise SD-only RAW preparation, PCM conversions, duplicate skipping and failures and the index."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
main = (ROOT / 'src/main.cpp').read_text(encoding='utf-8')
parsers = '\n'.join(re.search(r'static bool ' + name + r'\([^\n]+\)\n\{.*?^\}', main, re.M | re.S).group(0) for name in ['SET_WAV_raw_data', 'SET_AIFF_raw_data'])
helpers = (ROOT / 'scripts/tests/audio_import_cases.inc').read_text(encoding='utf-8').split('void audio_import_rejected')[0]
fixture = r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <new>
size_t array_allocation_limit=SIZE_MAX;
void *operator new[](size_t size, const std::nothrow_t &) noexcept
{
    if (size > array_allocation_limit)
    {
        return nullptr;
    }
    try
    {
        return ::operator new[](size);
    }
    catch (...)
    {
        return nullptr;
    }
}
#define FLASHMEM
constexpr int BUILTIN_SDCARD=0, FILE_WRITE=1;
int write_budget=-1;
int read_budget=-1;
struct SerialMock
{
    std::string log;
    void println(const char *text) { log+=text; log+='\n'; }
    void printf(const char *format, ...) { char text[1024]; va_list args; va_start(args,format); vsnprintf(text,sizeof(text),format,args); va_end(args); log+=text; }
} Serial;
std::string key(std::string path) { std::transform(path.begin(),path.end(),path.begin(),[](unsigned char c){return std::tolower(c);}); return path; }
struct Node { std::string path; bool directory=false; std::vector<uint8_t> bytes; };
struct File
{
    std::shared_ptr<Node> node;
    size_t cursor=0, next=0;
    explicit operator bool() const { return bool(node); }
    bool isDirectory() const { return node && node->directory; }
    const char *name() const { return strrchr(node->path.c_str(),'/')+1; }
    size_t size() const { return node ? node->bytes.size():0; }
    size_t position() const { return cursor; }
    bool seek(size_t value) { cursor=value; return node && value<=size(); }
    int read(void *out, size_t bytes) { if(!node || cursor>size()) { return 0; } if(read_budget>=0) { bytes=std::min(bytes,static_cast<size_t>(read_budget)); read_budget-=bytes; } const size_t count=std::min(bytes,size()-cursor); memcpy(out,node->bytes.data()+cursor,count); cursor+=count; return count; }
    size_t write(const uint8_t *data, size_t bytes) { if(!node) { return 0; } if(write_budget>=0) { bytes=std::min(bytes,static_cast<size_t>(write_budget)); write_budget-=bytes; } node->bytes.resize(cursor+bytes); memcpy(node->bytes.data()+cursor,data,bytes); cursor+=bytes; return bytes; }
    void flush() {}
    void close() { node.reset(); }
    File openNextFile();
};
struct SDMock
{
    std::map<std::string,std::shared_ptr<Node>> nodes;
    bool present=true;
    std::string fail_rename_to;
    bool begin(int) { return present; }
    bool mediaPresent() { return present; }
    bool exists(const char *path) { return present && nodes.count(key(path)); }
    bool mkdir(const char *path) { if(!present) {return false;} auto n=std::make_shared<Node>(); n->path=path; n->directory=true; nodes[key(path)]=n; return true; }
    bool remove(const char *path) { return present && nodes.erase(key(path)); }
    bool rename(const char *from,const char *to) { if(key(to)==fail_rename_to) {fail_rename_to.clear(); return false;} if(!exists(from) || exists(to)) {return false;} auto n=nodes.at(key(from)); nodes.erase(key(from)); n->path=to; nodes[key(to)]=n; return true; }
    void add(const std::string &path, const std::vector<uint8_t> &bytes) { auto n=std::make_shared<Node>(); n->path=path; n->bytes=bytes; nodes[key(path)]=n; }
    File open(const char *path,int mode=0) { if(!present) {return {};} if(mode==FILE_WRITE && !exists(path)) {add(path,{});} auto it=nodes.find(key(path)); return it==nodes.end()?File{}:File{it->second}; }
    const std::vector<uint8_t> &bytes(const char *path) { return nodes.at(key(path))->bytes; }
} SD;
File File::openNextFile()
{
    if(!node || !SD.present) {return {};}
    size_t index=0;
    const auto prefix=key(node->path)+"/";
    for(auto it=SD.nodes.rbegin();it!=SD.nodes.rend();++it)
    {
        const auto &[path,item]=*it;
        if(path.starts_with(prefix) && path.find('/',prefix.size())==std::string::npos && index++==next) {++next; return File{item};}
    }
    return {};
}
// MP3_IMPLEMENTATION
'''
mp3 = 'struct Mp3Import { bool Open(File &,uint32_t &,uint16_t &) {return false;} bool Read(int16_t *,uint32_t) {return false;} };'
if '--mp3' in sys.argv:
    mp3 = re.sub(r'^#(?:include|pragma)[^\n]*\n','',(ROOT/'include/Mp3Import.h').read_text(),flags=re.M)
    mp3 += (ROOT/'src/Mp3Import.cpp').read_text().replace('#include "Mp3Import.h"','')
fixture = fixture.replace('// MP3_IMPLEMENTATION',mp3)
for path in ['lib/UserAudioImport/UserAudioImport.h','lib/UserAudioImport/UserAudioImport.cpp','lib/SdSoundSource/SdSoundSource.h','lib/SdSoundSource/SdSoundSource.cpp']:
    text=(ROOT/path).read_text()
    text=re.sub(r'^#(?:include [<"](?:SD.h|UserAudioImport.h|Mp3Import.h|SdSoundSource.h)[>"]|pragma once)[^\n]*\n','',text,flags=re.M)
    fixture+=text+'\n'
fixture+=parsers+'\n'+helpers
fixture+=r'''
void reset()
{
    SD.nodes.clear(); SD.fail_rename_to.clear(); SD.present=true; write_budget=-1; read_budget=-1; array_allocation_limit=SIZE_MAX; Serial.log.clear();
    SD.mkdir("/EXTERNAL_AUDIO");
}
UserAudioImport::Report run() {return UserAudioImport::Run(SET_WAV_raw_data,SET_AIFF_raw_data);}
std::vector<UserAudioImport::Entry> index_entries()
{
    const auto &bytes=SD.bytes(UserAudioImport::INDEX_PATH);
    UserAudioImport::Header header;
    memcpy(&header,bytes.data(),sizeof(header));
    assert(header.magic==0x4C524157 && header.version==1 && header.sample_rate==44100 && header.channels==1 && header.bits==16);
    assert(bytes.size()==sizeof(header)+header.entries*sizeof(UserAudioImport::Entry));
    std::vector<UserAudioImport::Entry> entries(header.entries);
    memcpy(entries.data(),bytes.data()+sizeof(header),entries.size()*sizeof(entries[0]));
    return entries;
}
void test_sd_import()
{
    reset();
    SD.mkdir("/LILLA_AUDIO");
    SD.add("/LILLA_AUDIO/Existing.raw",{8,9});
    SD.add("/EXTERNAL_AUDIO/z.RAW",{1,2,3,4});
    SD.add("/EXTERNAL_AUDIO/alpha.wav",audio_file(false,1200,2,true));
    SD.add("/EXTERNAL_AUDIO/Beta.AIFF",audio_file(true,1500,2,true,true));
    SD.add("/EXTERNAL_AUDIO/readme.txt",{1});
    const auto original=SD.bytes("/EXTERNAL_AUDIO/alpha.wav");
    auto report=run();
    assert(report.copied==3 && report.skipped==1 && report.errors==0 && report.indexed==4 && report.index_ready);
    assert(SD.bytes("/LILLA_AUDIO/alpha.raw")==audio_samples(1200,2,false,true));
    assert(SD.bytes("/LILLA_AUDIO/Beta.raw")==audio_samples(1500,2,false,true));
    assert(SD.bytes("/EXTERNAL_AUDIO/alpha.wav")==original);
    auto entries=index_entries();
    assert(std::string(entries[0].name)=="alpha.raw" && std::string(entries[1].name)=="Beta.raw" && std::string(entries[2].name)=="Existing.raw" && std::string(entries[3].name)=="z.raw");
    assert(entries[0].bytes==2400 && entries[0].samples==1200);
    assert(Serial.log.find("CONVERT:")!=std::string::npos && Serial.log.find("INDEX OK:")!=std::string::npos);
    assert(Serial.log.find("INDEX OK: 1 ")<Serial.log.find("COPY:"));
    assert(Serial.log.find("INDEX: updating final")>Serial.log.find("CONVERT:"));
    Serial.log.clear();
    SD.add("/EXTERNAL_AUDIO/z.RAW",{9,8});
    report=run();
    assert(report.errors==0 && report.copied==0 && report.skipped==4 && SD.bytes("/LILLA_AUDIO/z.raw")==std::vector<uint8_t>({1,2,3,4}));
    assert(Serial.log.find("already exists; unchanged")!=std::string::npos);
    assert(Serial.log.find("creating initial")==std::string::npos);
    assert(Serial.log.find("COPY:")==std::string::npos && Serial.log.find("CONVERT:")==std::string::npos);
    SD.add("/EXTERNAL_AUDIO/New.wav",audio_file(false,1200,1));
    SD.add("/EXTERNAL_AUDIO/Existing.mp3",{0,0}); // Existing RAW skips even invalid input without decoding.
    report=run();
    assert(report.copied==1 && report.skipped==5 && report.errors==0 && report.indexed==5);
    assert(SD.bytes("/LILLA_AUDIO/Existing.raw")==std::vector<uint8_t>({8,9}));
    reset();
    const std::vector<uint8_t> large(4*1024*1024,0x52);
    SD.add("/EXTERNAL_AUDIO/long.raw",large);
    assert(run().errors==0 && SD.bytes("/LILLA_AUDIO/long.raw")==large); // No Flash-only 3 MiB truncation.
    reset();
    SD.mkdir("/LILLA_AUDIO");
    array_allocation_limit=4096;
    SD.add("/LILLA_AUDIO/old.raw",{1,2});
    SD.add("/EXTERNAL_AUDIO/old.wav",audio_file(false,1000,1));
    write_budget=10;
    report=run();
    assert(report.errors>0 && SD.bytes("/LILLA_AUDIO/old.raw")==std::vector<uint8_t>({1,2}));
    assert(!SD.exists(UserAudioImport::INDEX_PATH));
    write_budget=-1;
    report=run();
    assert(report.errors==0 && report.index_ready);
    SD.add("/EXTERNAL_AUDIO/old.wav",audio_file(false,1200,1));
    const auto saved=SD.bytes("/LILLA_AUDIO/old.raw");
    SD.fail_rename_to=key("/LILLA_AUDIO/old.raw");
    report=run();
    assert(report.errors==0 && report.copied==0 && report.skipped==1 && SD.bytes("/LILLA_AUDIO/old.raw")==saved);
    assert(!SD.fail_rename_to.empty());
    assert(!SD.exists("/LILLA_AUDIO/.user-import.previous"));
    reset();
    SD.add("/EXTERNAL_AUDIO/broken.wav",{1,2,3,4});
    SD.add("/EXTERNAL_AUDIO/odd.raw",{1,2,3});
    SD.add("/EXTERNAL_AUDIO/valid.raw",{1,2});
    report=run();
    assert(report.errors==2 && report.copied==1 && report.index_ready && report.indexed==1);
    reset();
    SD.mkdir("/LILLA_AUDIO");
    for(int i=4095;i>=0;--i) {char name[64]; snprintf(name,sizeof(name),"/LILLA_AUDIO/%04d.raw",i); SD.add(name,{1,2});}
    report=run();
    assert(report.errors==0 && report.indexed==4096);
    entries=index_entries();
    assert(std::string(entries[0].name)=="0000.raw" && std::string(entries[4095].name)=="4095.raw");
    for(size_t i=1;i<entries.size();++i)
    {
        assert(strcasecmp(entries[i-1].name,entries[i].name)<0);
        assert(entries[i].bytes==2 && entries[i].samples==1);
    }
    assert(!SD.exists("/LILLA_AUDIO/.lilla-index.tmp") && !SD.exists("/LILLA_AUDIO/.lilla-index.sort"));
    assert(SdSoundSource::Catalog_count()==4096);
    UserAudioImport::Entry last_entry;
    assert(SdSoundSource::Catalog_entry(4095,last_entry) && std::string(last_entry.name)=="4095.raw");
    assert(!SdSoundSource::Catalog_entry(4096,last_entry));
    uint32_t last_position=0;
    assert(SdSoundSource::Find_catalog("4095.RAW",4096,last_position) && last_position==4095);
    assert(SdSoundSource::Select(4095,1));
    int16_t last_sample=0;
    assert(SdSoundSource::Load(SdSoundSource::PREVIEW,&last_sample) && last_sample==0x0201);
    SdSoundSource::Unbind_all();
    SD.add("/LILLA_AUDIO/overflow.raw",{1,2});
    report=run();
    assert(report.errors>0 && !report.index_ready && !SD.exists(UserAudioImport::INDEX_PATH));
    for(int count : {0,1,3,15,31,1023})
    {
        reset();
        SD.mkdir("/LILLA_AUDIO");
        array_allocation_limit=4096;
        for(int i=0;i<count;++i)
        {
            char name[64];
            snprintf(name,sizeof(name),"/LILLA_AUDIO/%04d.raw",i);
            SD.add(name,{1,2,3,4});
        }
        report=run();
        assert(report.index_ready && report.errors==0 && report.indexed==static_cast<uint32_t>(count));
        entries=index_entries();
        for(int i=0;i<count;++i)
        {
            char name[32];
            snprintf(name,sizeof(name),"%04d.raw",i);
            assert(std::string(entries[i].name)==name && entries[i].bytes==4 && entries[i].samples==2);
        }
    }
    // Fail initial writing, merging, final header/data writing and publication.
    for(int budget : {0,792,900,2376,2400,2500})
    {
        reset();
        SD.mkdir("/LILLA_AUDIO");
        for(const char *name : {"/LILLA_AUDIO/z.raw","/LILLA_AUDIO/a.raw","/LILLA_AUDIO/M.raw"})
        {
            SD.add(name,{1,2});
        }
        write_budget=budget;
        report=run();
        assert(report.errors==1 && !report.index_ready && report.indexed==0 && !SD.exists(UserAudioImport::INDEX_PATH));
        assert(!SD.exists("/LILLA_AUDIO/.lilla-index.tmp") && !SD.exists("/LILLA_AUDIO/.lilla-index.sort"));
        write_budget=-1;
        assert(run().index_ready);
    }
    for(int budget : {0,264,1584,1848})
    {
        reset();
        SD.mkdir("/LILLA_AUDIO");
        for(const char *name : {"/LILLA_AUDIO/z.raw","/LILLA_AUDIO/a.raw","/LILLA_AUDIO/M.raw"})
        {
            SD.add(name,{1,2});
        }
        read_budget=budget;
        report=run();
        assert(report.errors==1 && !report.index_ready && !SD.exists(UserAudioImport::INDEX_PATH));
        read_budget=-1;
        assert(run().index_ready);
    }
    SD.fail_rename_to=key(UserAudioImport::INDEX_PATH);
    assert(!run().index_ready && !SD.exists(UserAudioImport::INDEX_PATH));
    assert(run().index_ready);
    reset();
    assert(run().index_ready);
    SD.add("/EXTERNAL_AUDIO/new.raw",{1,2});
    SD.fail_rename_to=key("/LILLA_AUDIO/new.raw");
    report=run();
    assert(report.copied==0 && report.errors==1 && report.index_ready && report.indexed==0);
    assert(!SD.exists("/LILLA_AUDIO/new.raw") && !SD.exists("/LILLA_AUDIO/.user-import.tmp"));
    write_budget=2; // Audio succeeds; the final index update cannot be written.
    report=run();
    assert(report.copied==1 && report.errors==1 && !report.index_ready);
    assert(SD.bytes("/LILLA_AUDIO/new.raw")==std::vector<uint8_t>({1,2}));
    write_budget=-1;
    report=run();
    assert(report.copied==0 && report.skipped==1 && report.index_ready && report.indexed==1);
    reset(); SD.present=false; assert(run().errors==1);
    reset(); SD.remove("/EXTERNAL_AUDIO"); assert(run().errors==1);
    reset();
    SD.add("/EXTERNAL_AUDIO/z.raw",{1,2,3,4,5,6,7,8});
    SD.add("/EXTERNAL_AUDIO/a.raw",{9,10});
    assert(run().index_ready && SdSoundSource::Catalog_count()==2);
    uint32_t found=99;
    assert(SdSoundSource::Find_catalog("Z.RAW",2,found) && found==1);
    assert(!SdSoundSource::Find_catalog("missing.raw",2,found));
    assert(SdSoundSource::Select(1,3));
    assert(SdSoundSource::Samples(SdSoundSource::PREVIEW)==3);
    assert(std::string(SdSoundSource::Name(SdSoundSource::PREVIEW))=="z.raw");
    int16_t preview[3]={};
    assert(SdSoundSource::Load(SdSoundSource::PREVIEW,preview));
    assert(preview[0]==0x0201 && preview[2]==0x0605);
    SdSoundSource::Bind(SdSoundSource::PREVIEW,preview);
    const int saved_id=SdSoundSource::Save(1,2);
    assert(saved_id>=SdSoundSource::FIRST && saved_id<SdSoundSource::PREVIEW);
    SdSoundSource::Unbind_all();
    assert(SdSoundSource::Prepare(saved_id) && SdSoundSource::Samples(saved_id)==2);
    int16_t saved_audio[2]={};
    assert(SdSoundSource::Load(saved_id,saved_audio));
    assert(saved_audio[0]==0x0403 && saved_audio[1]==0x0605);
    SdSoundSource::Bind(saved_id,saved_audio);
    int16_t readback[2]={};
    assert(SdSoundSource::Read(saved_id,readback,0,2) && readback[0]==0x0403);
    assert(!SdSoundSource::Read(saved_id,readback,1,2));
    SD.add("/EXTERNAL_AUDIO/0.raw",{1,2});
    assert(run().index_ready && SdSoundSource::Catalog_count()==3);
    assert(SdSoundSource::Load(saved_id,saved_audio) && saved_audio[0]==0x0403);
    SD.present=false;
    assert(!SdSoundSource::Select(0,1));
    assert(SdSoundSource::Samples(SdSoundSource::PREVIEW)==3 && std::string(SdSoundSource::Name(SdSoundSource::PREVIEW))=="z.raw");
    assert(SdSoundSource::Read(saved_id,readback,0,2) && readback[0]==0x0403);
    assert(SdSoundSource::Catalog_count()==0);
    SD.present=true;
    SD.remove("/LILLA_AUDIO/z.raw");
    assert(!SdSoundSource::Load(saved_id,saved_audio));
    assert(std::string(SdSoundSource::Name(saved_id))=="z.raw");
    // A removed sidecar identity still referenced by a Sound must never be reused.
    assert(SdSoundSource::Select(0,1));
    int16_t single=0;
    assert(SdSoundSource::Load(SdSoundSource::PREVIEW,&single));
    SdSoundSource::Bind(SdSoundSource::PREVIEW,&single);
    const int skipped_id=SdSoundSource::Save(0,0,[](int id) { return id==SdSoundSource::FIRST+1; });
    assert(skipped_id==SdSoundSource::FIRST+2);
    // Corrupt or absent reference metadata leaves the saved identity unavailable.
    char reference_path[64];
    snprintf(reference_path,sizeof(reference_path),"/LILLA_AUDIO/.lilla-sources/%05d.ref",skipped_id);
    assert(SD.remove(reference_path));
    SD.add(reference_path,{1,2,3});
    assert(!SdSoundSource::Prepare(skipped_id));
    assert(!SdSoundSource::Prepare(skipped_id+1));
    assert(!SdSoundSource::Select(999,3));
    std::cout<<"SD preparation: RAW/PCM, full copies, duplicate skipping, failure preservation, index 4096 and overflow passed\n";
}
'''
wrapper = re.search(r'FLASHMEM void SET_Prepare_user_audio\(void\)\n\{.*?^\}', main, re.M | re.S).group(0)
fixture += r'''
bool audio_enabled=true, drain_ok=true;
int result=0, answer=0, import_calls=0;
constexpr int IRQ_SOFTWARE=0, ILI9341_WHITE=1, ILI9341_RED=2, ILI9341_BLACK=3, ILI9341_GREEN=4;
bool NVIC_IS_ENABLED(int) { return audio_enabled; }
void AudioNoInterrupts() { audio_enabled=false; }
void AudioInterrupts() { audio_enabled=true; }
struct Control {bool running=true; bool Is_running() {return running;} void Start() {running=true;} void Stop() {running=false;} } Trigger, Midi_reader;
bool LS_Capture_drain() {Midi_reader.Stop(); audio_enabled=true; return drain_ok;}
template<class... T> void Show_popup_text(T...) {}
void delay(int) {}
void Clear_UI_events() {}
struct ImportProbe
{
    static UserAudioImport::Report Run(UserAudioImport::PcmParser, UserAudioImport::PcmParser)
    {
        assert(!audio_enabled && !Trigger.running && !Midi_reader.running);
        ++import_calls;
        return {};
    }
};
#define UserAudioImport ImportProbe
'''
fixture += wrapper + r'''
#undef UserAudioImport
void test_suspension()
{
    for (bool initially_running : {false,true})
    {
        audio_enabled=Trigger.running=Midi_reader.running=initially_running;
        import_calls=0; drain_ok=true;
        SET_Prepare_user_audio();
        assert(import_calls==1 && audio_enabled==initially_running && Trigger.running==initially_running && Midi_reader.running==initially_running);
        drain_ok=false;
        SET_Prepare_user_audio();
        assert(import_calls==1 && audio_enabled==initially_running && Trigger.running==initially_running && Midi_reader.running==initially_running);
    }
    std::cout<<"SD preparation: direct start, suspended audio/MIDI and state restoration passed\n";
}
'''
compiler=shutil.which('g++') or 'C:/msys64/ucrt64/bin/g++.exe'
with tempfile.TemporaryDirectory(prefix='lilla-user-audio-') as directory:
    build=Path(directory)
    checks='int main() { test_sd_import(); test_suspension(); }'
    if '--mp3' in sys.argv:
        ffmpeg=shutil.which('ffmpeg') or next((ROOT/'.pio/test-tools').glob('**/ffmpeg*.exe'),None)
        if ffmpeg is None: raise RuntimeError('MP3 tests require ffmpeg')
        for rate in [44100,48000,22050]:
            subprocess.run([str(ffmpeg),'-v','error','-f','lavfi','-i',f'aevalsrc=0.1*sin(2*PI*440*t)|0.2*sin(2*PI*660*t):s={rate}:d=0.3','-c:a','libmp3lame',str(build/f'{rate}.mp3')],check=True)
        checks='#include <fstream>\n#include <iterator>\nconst char *folder=R"('+directory+')";\n'
        checks+=r'''
int main()
{
    test_sd_import();
    test_suspension();
    for(int rate:{44100,48000,22050})
    {
        reset();
        std::ifstream input(std::string(folder)+"/"+std::to_string(rate)+".mp3",std::ios::binary);
        assert(input);
        std::vector<uint8_t> encoded{std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
        SD.add("/EXTERNAL_AUDIO/source.mp3",encoded);
        File file=SD.open("/EXTERNAL_AUDIO/source.mp3");
        Mp3Import decoder;
        uint32_t bytes=0; uint16_t channels=0;
        assert(decoder.Open(file,bytes,channels));
        std::vector<int16_t> decoded(bytes/2*channels);
        assert(decoder.Read(decoded.data(),bytes/2));
        std::vector<int16_t> expected(bytes/2);
        for(size_t i=0;i<expected.size();++i) {expected[i]=channels==1?decoded[i]:static_cast<int16_t>((static_cast<int32_t>(decoded[2*i])+decoded[2*i+1])/2);}
        auto report=run();
        assert(report.errors==0 && report.copied==1 && report.indexed==1);
        const auto &actual=SD.bytes("/LILLA_AUDIO/source.raw");
        assert(actual.size()==bytes && memcmp(actual.data(),expected.data(),bytes)==0);
        assert(SD.bytes("/EXTERNAL_AUDIO/source.mp3")==encoded);
    }
    std::cout<<"MP3 SD conversion: exact mono PCM at 44.1/48/22.05 kHz passed\n";
}
'''
    cpp=build/'test.cpp';exe=build/'test.exe'
    cpp.write_text(fixture+checks,encoding='utf-8')
    subprocess.run([compiler,'-std=c++20','-Wall','-Wextra','-Werror','-I',str(ROOT/'.pio/libdeps/teensy41/dr_libs'),str(cpp),'-o',str(exe)],check=True)
    env=os.environ.copy();env['PATH']=str(Path(compiler).parent)+os.pathsep+env.get('PATH','')
    subprocess.run([str(exe)],check=True,env=env)
