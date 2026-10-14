"""Exercise SD alphabetical indexing, PCM trimming, persistent recipes and preview buffering."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile
from file_registry_fixture import ROOT, REGISTRY_FIXTURE

fixture = REGISTRY_FIXTURE.replace('    uint8_t readArray(', '    uint8_t writeLong(uint32_t address, uint32_t value) { return writeArray(address, 4, reinterpret_cast<uint8_t *>(&value)); }\n    uint8_t readArray(')
fixture += r'''
#include <map>
#include <memory>
#include <string>
#include <strings.h>
#include <iostream>
#include <cstdlib>
using std::min;
constexpr int BUILTIN_SDCARD = 0, AUDIO_BLOCK_SAMPLES = 128;
struct File
{
    std::shared_ptr<std::vector<uint8_t>> data;
    std::string filename;
    size_t cursor = 0, next = 0;
    bool directory = false;
    explicit operator bool() const { return data != nullptr || directory; }
    bool isDirectory() const { return directory; }
    const char *name() const { return filename.c_str(); }
    size_t size() const { return data ? data->size() : 0; }
    size_t position() const { return cursor; }
    bool seek(size_t position) { cursor = position; return data && position <= data->size(); }
    size_t read(void *out, size_t bytes)
    {
        if (!data || cursor > data->size())
        {
            return 0;
        }
        const size_t count = std::min(bytes, data->size() - cursor);
        memcpy(out, data->data() + cursor, count);
        cursor += count;
        return count;
    }
    void close() { data.reset(); directory = false; }
    File openNextFile();
};
struct SDMock
{
    bool present = true;
    std::vector<std::string> order;
    std::map<std::string, std::shared_ptr<std::vector<uint8_t>>> files;
    bool begin(int) { return present; }
    File open(const char *path)
    {
        if (!present)
        {
            return {};
        }
        if (strcmp(path, "/LILLA_AUDIO") == 0)
        {
            File file;
            file.directory = true;
            return file;
        }
        const std::string name = strrchr(path, '/') + 1;
        const auto found = files.find(name);
        return found == files.end() ? File{} : File{found->second, name};
    }
    void add(const std::string &name, std::vector<uint8_t> bytes)
    {
        if (files.count(name) == 0)
        {
            order.push_back(name);
        }
        files[name] = std::make_shared<std::vector<uint8_t>>(std::move(bytes));
    }
} SD;
File File::openNextFile()
{
    return next == SD.order.size() ? File{} : SD.open(("/LILLA_AUDIO/" + SD.order[next++]).c_str());
}
struct Mp3Import
{
    bool Open(File &, uint32_t &, uint16_t &, uint32_t) { return false; }
    bool Read(int16_t *, uint32_t) { return false; }
    bool Seek(uint32_t) { return false; }
};
constexpr int IRQ_SOFTWARE = 0;
bool NVIC_IS_ENABLED(int) { return true; }
void NVIC_DISABLE_IRQ(int) {}
void NVIC_ENABLE_IRQ(int) {}
struct audio_block_t { int16_t data[128]; };
struct AudioStream
{
    audio_block_t output = {};
    int transmitted = 0;
    AudioStream(int, void *) {}
    virtual void update() = 0;
    audio_block_t *allocate() { return &output; }
    void transmit(audio_block_t *) { ++transmitted; }
    void release(audio_block_t *) {}
};
'''
for path in ('lib/SdAudioLibrary/SdAudioLibrary.h', 'lib/SdAudioLibrary/SdAudioLibrary.cpp'):
    source = (ROOT / path).read_text(encoding='utf-8')
    source = re.sub(r'^#(?:include|pragma)[^\n]*\n', '', source, flags=re.M)
    fixture += source.replace('__asm__ volatile("dmb" ::: "memory");', '')
fixture += r'''
int main()
{
    reset_registry();
    SD.add("z.raw", {1, 0, 2, 0, 3, 0, 4, 0});
    SD.add("Beta.RAW", {5, 0, 6, 0, 7, 0, 8, 0});
    SD.add("alpha.raw", {9, 0, 10, 0, 11, 0, 12, 0});
    SD.add("readme.txt", {1, 2});
    const auto archive = LillaFram.memory;
    assert(SdAudioLibrary::Begin());
    assert(SdAudioLibrary::Count() == 3);
    char name[SdAudioLibrary::NAME_BYTES];
    assert(SdAudioLibrary::Name(0, name) && strcmp(name, "alpha.raw") == 0);
    assert(SdAudioLibrary::Name(1, name) && strcmp(name, "Beta.RAW") == 0);
    assert(SdAudioLibrary::Name(2, name) && strcmp(name, "z.raw") == 0);
    assert(std::equal(archive.begin(), archive.begin() + SdAudioLibrary::ADDRESS, LillaFram.memory.begin()));
    SdAudioReader reader;
    int16_t samples[4] = {};
    assert(reader.Open("alpha.raw") && reader.Samples() == 4);
    assert(reader.Seek(1) && reader.Read(samples, 2) && samples[0] == 10 && samples[1] == 11);
    assert(!reader.Read(samples, 2));
    assert(SdAudioLibrary::Save(1, "alpha.raw", 1, 2));
    assert(FileNameRegistry::Bind_numeric(1) && FileNameRegistry::Save());
    assert(SdAudioLibrary::Begin() && SdAudioLibrary::Assigned(1) && SdAudioLibrary::Samples(1) == 2);
    assert(SdAudioLibrary::Read(1, samples, 0, 2) && samples[0] == 10 && samples[1] == 11);
    assert(SdAudioLibrary::Read(1, samples, 1, 1) && samples[0] == 11);
    assert(!SdAudioLibrary::Read(1, samples, 1, 2));
    SD.present = false;
    assert(SdAudioLibrary::Begin() && SdAudioLibrary::Count() == 0 && SdAudioLibrary::Assigned(1) && SdAudioLibrary::Samples(1) == 0);
    SD.present = true;
    assert(SdAudioLibrary::Begin() && SdAudioLibrary::Samples(1) == 2);
    LillaFram.budget = 10;
    assert(!SdAudioLibrary::Save(2, "z.raw", 0, 3));
    LillaFram.budget = -1;
    assert(FileNameRegistry::Bind_numeric(2) && FileNameRegistry::Save());
    assert(SdAudioLibrary::Begin() && !SdAudioLibrary::Assigned(2));
    SdAudioPreview preview;
    assert(preview.Open("z.raw") && preview.Trim(1, 2));
    for (int i = 0; i < 4; ++i)
    {
        assert(preview.Service());
    }
    preview.update();
    assert(preview.transmitted == 1);
    for (int i = 0; i < 128; ++i)
    {
        assert(preview.output.data[i] == (i % 2 == 0 ? 2 : 3));
    }
    preview.Stop();
    preview.update();
    assert(preview.transmitted == 1);
    SD.add(std::string(64, 'a') + ".raw", {0, 0});
    assert(!SdAudioLibrary::Begin() && SdAudioLibrary::Count() == 0 && SdAudioLibrary::Error() != nullptr);
    std::cout << "SD library regression passed\n";
}
'''
compiler = shutil.which('g++') or 'C:/msys64/ucrt64/bin/g++.exe'
with tempfile.TemporaryDirectory(prefix='lilla-sd-audio-') as directory:
    cpp = Path(directory) / 'test.cpp'
    exe = Path(directory) / 'test.exe'
    cpp.write_text(fixture, encoding='utf-8')
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    environment = os.environ.copy()
    environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
    subprocess.run([str(exe)], check=True, env=environment)
