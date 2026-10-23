"""Exercise production Sound source notices and their nonblocking expiry."""
from pathlib import Path
import re
import os
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = (root / "lib/DisplaySound/DisplaySound.cpp").read_text(encoding="utf-8")
methods = "\n".join(re.search(r"^(?:FLASHMEM )?void DisplaySound::" + name + r"\(.*?^}", source, re.M | re.S).group(0) for name in ("Show_source_notice", "Show_read_sd_notice", "Update_source_notice"))
sd_source = (root / "lib/SdSoundSource/SdSoundSource.cpp").read_text(encoding="utf-8")
diagnose = re.search(r"^FLASHMEM SdSoundSource::SourceStatus SdSoundSource::Diagnose\(.*?^}", sd_source, re.M | re.S).group(0)
fixture = r'''
#include <cassert>
#include <cstdint>
#include <string>
#define FLASHMEM
constexpr int ILI9341_WHITE = 1, ILI9341_RED = 2, WAVE_POPUP_Y_OFFSET = 50;
uint32_t now = 0;
uint32_t millis() { return now; }
#include <cstring>
#include <cstdio>
#include <vector>
#include <map>
struct File
{
    std::string path;
    size_t position = 0;
    static inline std::map<std::string, std::string> files;
    static inline std::vector<std::string> entries;
    operator bool() const { return !path.empty(); }
    bool isDirectory() const { return path == "/AUDIO_FILES"; }
    File openNextFile() { return position < entries.size() ? File{entries[position++]} : File{}; }
    const char *name() const { return path.c_str(); }
    size_t size() const { return files[path].size(); }
    int read(void *destination, size_t count) { memcpy(destination, files[path].data(), count); return count; }
};
struct Sd
{
    bool present = false;
    bool mediaPresent() { return present; }
    File open(const char *path) { return File::files.count(path) ? File{path} : File{}; }
    bool exists(const char *path) { return File::files.count(path) != 0; }
} SD;
struct SdSoundSource
{
    static constexpr int PREVIEW = 32767;
    enum class SourceStatus { MissingCard, EmptyLibrary, MissingReference, MissingFile, Unavailable };
    struct Reference { uint32_t magic = 0x3153444C; char name[256] = "wanted.raw"; uint32_t checksum = 17; };
    struct Loaded { Reference reference; };
    static inline bool loaded = true;
    static Loaded *Find(int) { static Loaded value; return loaded ? &value : nullptr; }
    static void Reference_path(int, char (&path)[64]) { strcpy(path, "/reference"); }
    static uint32_t Checksum(const Reference &) { return 17; }
    static bool Is_sd(int file) { return file >= 1024 && file <= PREVIEW; }
    static SourceStatus Diagnose(int file);
};
int S_flash_first = -1;
std::string message, action;
int draws = 0;
void Show_popup_text(const char *text, const char *next, int fg, int bg, int offset)
{
    assert(fg == ILI9341_WHITE && bg == ILI9341_RED && offset == WAVE_POPUP_Y_OFFSET);
    message = text;
    action = next;
    assert(message.size() <= 49 && action.size() <= 49);
    ++draws;
}
void Show_popup_text(const char *text, int fg, int bg, int offset)
{
    Show_popup_text(text, "", fg, bg, offset);
}
struct DisplaySound
{
    int source_notice_file = -1;
    uint8_t source_notice_reason = 0;
    uint32_t source_notice_started = 0;
    bool source_notice_visible = false;
    int restored = 0;
    void Show_source_notice(int file, bool restart = true);
    void Show_read_sd_notice();
    void Update_source_notice(int instrument_id);
    void Show_wave(int instrument_id, bool show_notice)
    {
        assert(instrument_id == 3 && !show_notice);
        ++restored;
    }
};
'''
checks = r'''
int main()
{
    DisplaySound display;
    display.Show_source_notice(17, false);
    assert(message == "NO AUDIO FILES IN FLASH" && action == "PLEASE IMPORT AUDIO FILES");
    now = 4499;
    display.Show_source_notice(17, false);
    display.Update_source_notice(3);
    assert(display.restored == 0 && display.source_notice_started == 0);
    now = 4500;
    display.Update_source_notice(3);
    assert(display.restored == 1 && !display.source_notice_visible);
    const int previous_draws = draws;
    display.Show_source_notice(17, false);
    display.Update_source_notice(3);
    assert(draws == previous_draws && display.restored == 1);
    S_flash_first = 5;
    display.Show_source_notice(17, false);
    assert(message == "SOUND FILE MISSING FROM FLASH");
    display.Show_source_notice(1024, false);
    assert(message == "SD CARD NOT INSERTED");
    SD.present = true;
    display.Show_source_notice(1024, false);
    assert(message == "NO CONVERTED AUDIO FILES ON SD");
    File::files["/AUDIO_FILES"] = "";
    File::files["other.raw"] = "audio";
    File::entries = {"other.raw"};
    display.Show_source_notice(1024, false);
    assert(message == "SOUND FILE MISSING FROM SD");
    File::files["/AUDIO_FILES/wanted.raw"] = "audio";
    display.Show_source_notice(1024, false);
    assert(message == "SD AUDIO COULD NOT BE LOADED");
    SdSoundSource::loaded = false;
    display.Show_source_notice(1024, false);
    assert(message == "SOUND SD REFERENCE UNAVAILABLE");
    SdSoundSource::Reference reference;
    File::files["/reference"] = std::string(reinterpret_cast<const char *>(&reference), sizeof(reference));
    display.Show_source_notice(1024, false);
    assert(message == "SD AUDIO COULD NOT BE LOADED");
    File::files.erase("/AUDIO_FILES/wanted.raw");
    display.Show_source_notice(1024, false);
    assert(message == "SOUND FILE MISSING FROM SD");
    assert(SdSoundSource::Diagnose(SdSoundSource::PREVIEW) == SdSoundSource::SourceStatus::Unavailable);
    File::entries = {".lilla-index", "empty.raw"};
    File::files[".lilla-index"] = "stale catalog";
    File::files["empty.raw"] = "";
    display.Show_source_notice(1024, false);
    assert(message == "NO CONVERTED AUDIO FILES ON SD");
    now += 4500;
    display.Update_source_notice(3);
    display.Show_source_notice(1024);
    assert(display.source_notice_visible && display.source_notice_started == now);
    now = UINT32_MAX - 2000;
    display.Show_source_notice(1025);
    now += 4500;
    display.Update_source_notice(3);
    assert(!display.source_notice_visible && display.restored == 3);
    display.Show_read_sd_notice();
    assert(message == "READ SD AUDIO FILES" && action.empty());
    assert(display.source_notice_visible);
    now += 4499;
    display.Update_source_notice(3);
    assert(display.source_notice_visible && display.restored == 3);
    ++now;
    display.Update_source_notice(3);
    assert(!display.source_notice_visible && display.restored == 4);
}
'''
compiler = shutil.which("g++") or shutil.which("clang++")
if compiler is None:
    raise SystemExit("A C++ compiler is required")
with tempfile.TemporaryDirectory(prefix="lilla-source-notice-") as directory:
    cpp = Path(directory) / "notice.cpp"
    exe = Path(directory) / "notice.exe"
    cpp.write_text(fixture + diagnose + methods + checks, encoding="utf-8")
    subprocess.run([compiler, "-std=c++20", str(cpp), "-o", str(exe)], check=True)
    environment = os.environ.copy()
    environment["PATH"] = str(Path(compiler).parent) + os.pathsep + environment.get("PATH", "")
    subprocess.run([str(exe)], check=True, env=environment)
print("PASS: absent SD, empty libraries, missing files/references, load errors, stale catalog, 4.5-second expiry, redraw suppression, retry and timer rollover")
