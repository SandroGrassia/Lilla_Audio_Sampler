"""Exercise production runtime codecs and versioned/legacy Patch SD round trips."""
from pathlib import Path
import os
import runpy
import subprocess
import tempfile

backup = runpy.run_path(str(Path(__file__).with_name("fram_backup_regression.py")))
base = backup["base"]
header = base["header"]
implementation = base["implementation"]
extra = header[header.index("    byte Read_Delay"):header.index("    byte Read_first_octave")]
extra += header[header.index("    byte Save_Sound"):header.index("    byte Save_DS_Recording")]
extra += "    bool Copy_Patch_from_RAM_to_SD(const int patch_id, File &file);\n"
extra += "    bool Copy_Patch_from_SD_to_RAM(const int patch_id, File &file);\n"
fixture = backup["fixture"].replace("class ArchivingManager {\npublic:\n", "class ArchivingManager {\npublic:\n" + extra)
methods = backup["methods"]
shared = base["shared"]
methods += shared[shared.index("// == overloads"):shared.index("enum AudioFileStorage")]
methods += implementation[implementation.index("byte ArchivingManager::Read_Delay"):implementation.index("byte ArchivingManager::Read_first_octave")]
methods += implementation[implementation.index("byte ArchivingManager::Save_Sound"):implementation.index("byte ArchivingManager::Save_DS_Recording")]
start = implementation.index("bool ArchivingManager::Copy_Patch_from_RAM_to_SD(const int patch_id, File &file)")
methods += implementation[start:implementation.index("bool ArchivingManager::Copy_Sound_from_RAM_to_SD", start)]
start = implementation.index("bool ArchivingManager::Copy_Patch_from_SD_to_RAM(const int patch_id, File &file)")
methods += implementation[start:implementation.index("bool ArchivingManager::Copy_Sound_from_SD_to_RAM", start)]
mock = backup["mock"].replace("    void close()", r"""
    int available() const { return position < size(); }
    int read()
    {
        uint8_t value = 0;
        return read(&value, 1) == 1 ? value : -1;
    }
    size_t println(const char *text)
    {
        const std::string line = std::string(text) + "\r\n";
        return write(reinterpret_cast<const uint8_t *>(line.data()), line.size());
    }
    size_t println(uint8_t value) { return println(std::to_string(value).c_str()); }
    void close()""")
delay_header = (base["ROOT"] / "lib/SharedDelay/SharedDelay.h").read_text(encoding="utf-8")
mock += delay_header[delay_header.index("struct Delay_data_struct"):delay_header.index("static constexpr int DELAY_DATA_DIM")]
mock += r"""
#define FLASHMEM
#define bitRead(value, bit) (((value) >> (bit)) & 1U)
#define bitWrite(value, bit, flag) ((value) = ((value) & ~(1U << (bit))) | ((flag) << (bit)))
#define PRINT_ERROR(value) ((void)(value))
constexpr int FIRST_RECORDING_FILE = 2000;
int Capture_new_patch = -1, pending_file = -1;
Delay_data_struct Capture_patch_delay{};
bool Capture_pending(int file) { return file == pending_file; }
struct InfoStub { int Raw_file_samples(int) { return 100000; } } Info;
"""
tests = r"""
int main()
{
    ArchivingManager archive;
    assert(archive.Factory_reset_FRAM() == 0);
    Sound[799] = {};
    Sound[799].used = true;
    Sound[799].B = 12345;
    Sound[799].pitch = -12;
    Sound[799].data = 31;
    assert(archive.Save_Sound(799) == 0);
    Sound[799] = {};
    assert(archive.Read_Sound(799) == 0 && Sound[799].B == 12345 && Sound[799].data == 31);
    Patch[199] = {};
    Patch[199].used = true;
    Patch[199].instruments = 2;
    for (int index : {0, 7})
    {
        auto &i = Patch[199].Instrument[index];
        i.used = true;
        i.sound_id = index == 0 ? 799 : 256;
        i.root_key = 60 + index;
        i.to_note = 127;
        i.Filter.frequency_time = 19;
    }
    Delay_data_struct delay{99, -10, 0xA5, 2, 39, 90, 359, 5};
    assert(archive.Save_Delay(199, delay) == 0);
    assert(archive.Save_Patch(199) == 0);
    const auto expected = Patch[199];
    Patch[199] = {};
    assert(archive.Read_Patch(199) == 0 && Patch[199] == expected);
    Delay_data_struct actual{};
    assert(archive.Read_Delay(199, actual) == 0 && memcmp(&actual, &delay, sizeof(delay)) == 0);
    assert(archive.Save_Sound(800) == 11 && archive.Save_Patch(200) == 11);
    File wire{std::make_shared<std::vector<uint8_t>>()};
    assert(archive.Copy_Patch_from_RAM_to_SD(199, wire));
    Patch[198] = {};
    assert(archive.Copy_Patch_from_SD_to_RAM(198, wire) && Patch[198] == expected);
    auto malformed = *wire.data;
    malformed.pop_back();
    File truncated{std::make_shared<std::vector<uint8_t>>(malformed)};
    assert(!archive.Copy_Patch_from_SD_to_RAM(198, truncated) && Patch[198] == expected);
    File legacy{std::make_shared<std::vector<uint8_t>>()};
    const uint8_t *raw = reinterpret_cast<const uint8_t *>(&expected);
    for (size_t offset = 0; offset < sizeof(expected); ++offset)
    {
        legacy.println(raw[offset]);
    }
    assert(sizeof(expected) == 130);
    assert(archive.Copy_Patch_from_SD_to_RAM(198, legacy) && Patch[198] == expected);
    // Invalid numeric input is rejected without changing the destination.
    (*wire.data)[15] = 'X';
    assert(!archive.Copy_Patch_from_SD_to_RAM(198, wire) && Patch[198] == expected);
    LillaFram.clear_io();
    LillaFram.fail_at = 1;
    assert(archive.Read_Patch(198) == 2 && Patch[198] == expected);
    LillaFram.clear_io();
    LillaFram.memory[0x100 + 199 * 192] ^= 1;
    actual = delay;
    assert(archive.Read_Delay(199, actual) == 12 && memcmp(&actual, &delay, sizeof(delay)) == 0);
    // New capture delay remains in RAM and pending audio cannot reach persistent metadata.
    LillaFram.clear_io();
    Capture_new_patch = 198;
    Capture_patch_delay = delay;
    const auto before_capture = LillaFram.memory;
    assert(archive.Save_Delay(198, delay) == 0);
    actual = {};
    assert(archive.Read_Delay(198, actual) == 0 && memcmp(&actual, &delay, sizeof(delay)) == 0);
    assert(LillaFram.memory == before_capture);
    Patch[198] = expected;
    pending_file = Sound[799].file;
    assert(archive.Save_Sound(799) == 14);
    Patch[198].Instrument[0].sound_id = 799;
    assert(archive.Save_Patch(198) == 14 && Capture_new_patch == 198);
    assert(LillaFram.memory == before_capture);
    File unsaved{std::make_shared<std::vector<uint8_t>>()};
    assert(!archive.Copy_Patch_from_RAM_to_SD(198, unsaved) && unsaved.size() == 0);
    // Dropping an unsaved patch must not demand nonexistent RAW files.
    Patch[198].used = false;
    assert(archive.Save_Patch(198) == 0 && Capture_new_patch == -1);
    Patch[198].used = true;
    Capture_new_patch = 198;
    pending_file = -1;
    assert(archive.Save_Sound(799) == 0 && archive.Save_Patch(198) == 0 && Capture_new_patch == -1);
    actual = {};
    assert(archive.Read_Delay(198, actual) == 0 && memcmp(&actual, &delay, sizeof(delay)) == 0);
    std::puts("PASS: runtime high IDs, signed fields, Delay preservation, SD round trip, padded legacy import and atomic rejection");
}
"""
compiler = base["compiler"]
with tempfile.TemporaryDirectory(prefix="lilla-runtime-") as directory:
    source = Path(directory) / "runtime.cpp"
    executable = Path(directory) / "runtime.exe"
    source.write_text(base["prefix"] + mock + base["runtime"] + fixture + methods + tests, encoding="utf-8", newline="\r\n")
    subprocess.run([compiler, "-std=c++20", "-Wall", "-Wextra", "-Werror", str(source), "-o", str(executable)], check=True)
    environment = os.environ.copy()
    environment["PATH"] = str(Path(compiler).parent) + os.pathsep + environment.get("PATH", "")
    subprocess.run([str(executable)], env=environment, check=True)
