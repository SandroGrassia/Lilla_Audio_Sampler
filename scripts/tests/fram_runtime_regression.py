"""Exercise production runtime codecs and versioned/legacy Patch SD round trips."""
from pathlib import Path
import os
import re
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
elements = (base["ROOT"] / "lib/SharedElements/SharedElements.h").read_text(encoding="utf-8")
mock += re.search(r"static constexpr int LPF_MAX = \d+;", elements).group(0) + "\n"
mock += delay_header[delay_header.index("struct Delay_data_struct"):delay_header.index("static constexpr int DELAY_DATA_DIM")]
mock += '\n#include <cmath>\n#define constrain(x,lo,hi) ((x)<(lo)?(lo):((x)>(hi)?(hi):(x)))\nconstexpr float AUDIO_SAMPLE_RATE = 44100.0f;\nconstexpr int AUDIO_BLOCK_SAMPLES = 128;\nconstexpr int DELAY_CACHE_CHANNEL_SAMPLES = 882128;\n'
mock += delay_header[delay_header.index("static constexpr int DELAY_CACHE_ACTIVE_SECONDS"):delay_header.index("static constexpr float depth_array")]
mock += '\nint Calc_delay_samples(int value);\nint Calc_delay_time_tenths(int value);\n'
delay_source = (base["ROOT"] / "lib/SharedDelay/SharedDelay.cpp").read_text(encoding="utf-8")
for signature in ["void Convert_legacy_delay", "int Calc_delay_samples", "int Calc_delay_time_tenths"]:
    mock += re.search(r'^' + signature + r'\(.*?^}', delay_source, re.M | re.S).group(0) + '\n'
mock += r"""
#define FLASHMEM
#define bitRead(value, bit) (((value) >> (bit)) & 1U)
#define bitWrite(value, bit, flag) ((value) = ((value) & ~(1U << (bit))) | ((flag) << (bit)))
#define PRINT_ERROR(value) ((void)(value))
int Capture_new_patch = -1, pending_file = -1;
Delay_data_struct Capture_patch_delay{};
bool Capture_pending(int file) { return file == pending_file; }
struct InfoStub { int Raw_file_samples(int) { return 100000; } } Info;
"""
tests = r"""
bool Equal_delay(const Delay_data_struct &a, const Delay_data_struct &b)
{
    return a.samples == b.samples && a.samples_LR == b.samples_LR && a.instrument_route == b.instrument_route && a.modulation_source == b.modulation_source && a.modulation_depth == b.modulation_depth && a.modulation_frequency == b.modulation_frequency && a.modulation_phase_LR == b.modulation_phase_LR && a.loop_gain == b.loop_gain && a.lpf_cutoff == b.lpf_cutoff && a.compressor_threshold == b.compressor_threshold && a.compressor == b.compressor && a.reverb_level == b.reverb_level && a.reverb_decay == b.reverb_decay && a.reverb_damping == b.reverb_damping;
}
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
        i.lock = index == 0 ? 38 : 80;
        i.root_key = 60 + index;
        i.to_note = 127;
        i.Filter.frequency_time = 19;
    }
    Delay_data_struct delay{99, -10, 0xA5, 2, 39, 90, 359, 5};
    delay.lpf_cutoff = 20;
    delay.compressor = true;
    delay.reverb_level = 37;
    delay.reverb_decay = 28;
    delay.reverb_damping = 72;
    assert(archive.Save_Delay(199, delay) == 0);
    assert(archive.Save_Patch(199) == 0);
    const auto expected = Patch[199];
    Patch[199] = {};
    assert(archive.Read_Patch(199) == 0 && Patch[199] == expected);
    Delay_data_struct actual{};
    assert(archive.Read_Delay(199, actual) == 0);
    assert(actual.samples == delay.samples && actual.samples_LR == delay.samples_LR && actual.instrument_route == delay.instrument_route);
    assert(actual.modulation_source == delay.modulation_source && actual.modulation_depth == delay.modulation_depth && actual.modulation_frequency == delay.modulation_frequency && actual.modulation_phase_LR == delay.modulation_phase_LR);
    assert(actual.loop_gain == delay.loop_gain && actual.lpf_cutoff == delay.lpf_cutoff && actual.compressor == delay.compressor); // Compare fields, not runtime-struct padding.
    assert(Equal_delay(actual, delay));
    ArchivingManager::FRAM_Patch_delay_struct old_delay{};
    old_delay.samples = 20;
    old_delay.loop_gain = 5;
    assert(archive.FRAM_Write_delay(197, old_delay) == 0);
    assert(archive.Read_Delay(197, actual) == 0 && actual.samples == 24 && actual.loop_gain == 65 && actual.lpf_cutoff == LPF_MAX && !actual.compressor);
    assert(actual.reverb_level == 0 && actual.reverb_decay == 15 && actual.reverb_damping == 50);
    ArchivingManager::FRAM_Patch_delay_struct stored{};
    assert(archive.FRAM_Read_delay(197, stored) == 0 && stored.samples == 20 && stored.reserved[0] == 0);
    assert(archive.Save_Delay(197, actual) == 0);
    assert(archive.FRAM_Read_delay(197, stored) == 0 && stored.samples == 24 && stored.reserved[0] == 1);
    assert(archive.Read_Delay(197, actual) == 0 && actual.samples == 24 && actual.loop_gain == 65 && actual.lpf_cutoff == LPF_MAX && !actual.compressor);

    // Version-1 patches with an unused cutoff byte keep the default; all new cutoff indices round-trip.
    stored.reserved[0] = 1;
    stored.reserved[1] = 0;
    assert(archive.FRAM_Write_delay(197, stored) == 0);
    assert(archive.Read_Delay(197, actual) == 0 && actual.lpf_cutoff == LPF_MAX && !actual.compressor);
    for (int cutoff = 0; cutoff <= LPF_MAX; ++cutoff)
    {
        actual.lpf_cutoff = cutoff;
        assert(archive.Save_Delay(197, actual) == 0);
        assert(archive.FRAM_Read_delay(197, stored) == 0 && stored.reserved[1] == cutoff + 1);
        Delay_data_struct loaded{};
        assert(archive.Read_Delay(197, loaded) == 0 && loaded.lpf_cutoff == cutoff);
    }
    for (bool enabled : {false, true})
    {
        actual.compressor = enabled;
        assert(archive.Save_Delay(197, actual) == 0);
        assert(archive.FRAM_Read_delay(197, stored) == 0 && stored.reserved[2] == enabled);
        Delay_data_struct loaded{};
        assert(archive.Read_Delay(197, loaded) == 0 && loaded.compressor == enabled);
    }
    assert(actual.compressor_threshold == -6);
    for (int db = -24; db <= -2; ++db)
    {
        actual.compressor_threshold = db;
        assert(archive.Save_Delay(197, actual) == 0);
        assert(archive.FRAM_Read_delay(197, stored) == 0 && stored.reserved[3] == db + 25);
        Delay_data_struct loaded{};
        assert(archive.Read_Delay(197, loaded) == 0 && loaded.compressor_threshold == db);
    }
    stored.reserved[3] = 24;
    assert(archive.FRAM_Write_delay(197, stored) == 0);
    assert(archive.Read_Delay(197, actual) != 0 && actual.compressor_threshold == -2);
    stored.reserved[3] = 0;
    assert(archive.FRAM_Write_delay(197, stored) == 0);
    assert(archive.Read_Delay(197, actual) == 0 && actual.compressor_threshold == -6);
    for (int index : {4, 5, 6})
    {
        assert(archive.Save_Delay(197, actual) == 0);
        assert(archive.FRAM_Read_delay(197, stored) == 0);
        stored.reserved[index] = 255;
        assert(archive.FRAM_Write_delay(197, stored) == 0);
        const auto previous = actual;
        assert(archive.Read_Delay(197, actual) != 0 && Equal_delay(actual, previous));
    }
    const auto before_invalid = actual;
    stored.reserved[1] = LPF_MAX + 2;
    assert(archive.FRAM_Write_delay(197, stored) == 0);
    assert(archive.Read_Delay(197, actual) != 0);
    assert(Equal_delay(actual, before_invalid));
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
    // All TONE settings survive FRAM and SD round trips without changing stored numeric values.
    for (uint8_t level = 0; level <= 80; level = level == 0 ? 30 : level + 1)
    {
        Patch[198] = expected;
        Patch[198].Instrument[0].lock = level;
        const auto level_patch = Patch[198];
        assert(archive.Save_Patch(198) == 0);
        Patch[198] = {};
        assert(archive.Read_Patch(198) == 0 && Patch[198] == level_patch);
        File level_wire{std::make_shared<std::vector<uint8_t>>()};
        assert(archive.Copy_Patch_from_RAM_to_SD(198, level_wire));
        Patch[198] = {};
        assert(archive.Copy_Patch_from_SD_to_RAM(198, level_wire) && Patch[198] == level_patch);
    }
    // Legacy HIFI values migrate once, preserving their cutoff including 3.8 kHz.
    const uint8_t migrated[] = {0, 50, 45, 42, 40, 38};
    for (uint8_t old_level = 0; old_level <= 5; ++old_level)
    {
        Patch[198] = expected;
        Patch[198].Instrument[0].lock = old_level;
        assert(archive.Save_Patch(198) == 0); // Write the historical byte encoding.
        File old_wire{std::make_shared<std::vector<uint8_t>>()};
        assert(archive.Copy_Patch_from_RAM_to_SD(198, old_wire));
        auto migrated_patch = expected;
        migrated_patch.Instrument[0].lock = migrated[old_level];
        Patch[198] = {};
        assert(archive.Read_Patch(198) == 0 && Patch[198] == migrated_patch);
        assert(archive.Save_Patch(198) == 0);
        Patch[198] = {};
        assert(archive.Read_Patch(198) == 0 && Patch[198] == migrated_patch);
        Patch[198] = {};
        assert(archive.Copy_Patch_from_SD_to_RAM(198, old_wire) && Patch[198] == migrated_patch);
    }
    Patch[198] = expected;
    ArchivingManager::FRAM_Patch_struct invalid_level{};
    assert(archive.FRAM_Read_patch(198, invalid_level) == 0);
    invalid_level.Instrument[0].lock = 6;
    assert(archive.FRAM_Write_patch(198, invalid_level) == 0);
    assert(archive.Read_Patch(198) == ArchivingManager::FRAM_ERROR_SOURCE && Patch[198] == expected);
    assert(archive.Save_Patch(198) == 0);
    auto invalid_patch = expected;
    invalid_patch.Instrument[0].lock = 6;
    File invalid_legacy{std::make_shared<std::vector<uint8_t>>()};
    const auto *invalid_raw = reinterpret_cast<const uint8_t *>(&invalid_patch);
    for (size_t offset = 0; offset < sizeof(invalid_patch); ++offset)
    {
        invalid_legacy.println(invalid_raw[offset]);
    }
    assert(!archive.Copy_Patch_from_SD_to_RAM(198, invalid_legacy) && Patch[198] == expected);
    // Invalid numeric input is rejected without changing the destination.
    (*wire.data)[15] = 'X';
    assert(!archive.Copy_Patch_from_SD_to_RAM(198, wire) && Patch[198] == expected);
    LillaFram.clear_io();
    LillaFram.fail_at = 1;
    assert(archive.Read_Patch(198) == 2 && Patch[198] == expected);
    LillaFram.clear_io();
    LillaFram.memory[0x100 + 199 * 192] ^= 1;
    actual = delay;
    assert(archive.Read_Delay(199, actual) == 12 && Equal_delay(actual, delay));
    // New capture delay remains in RAM and pending audio cannot reach persistent metadata.
    LillaFram.clear_io();
    Capture_new_patch = 198;
    Capture_patch_delay = delay;
    const auto before_capture = LillaFram.memory;
    assert(archive.Save_Delay(198, delay) == 0);
    actual = {};
    assert(archive.Read_Delay(198, actual) == 0 && Equal_delay(actual, delay));
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
    assert(archive.Read_Delay(198, actual) == 0 && Equal_delay(actual, delay));
    std::puts("PASS: runtime high IDs, signed fields, Delay preservation, SD round trip, padded legacy import and atomic rejection");
}
"""
compiler = base["compiler"]
with tempfile.TemporaryDirectory(prefix="lilla-runtime-") as directory:
    source = Path(directory) / "runtime.cpp"
    executable = Path(directory) / "runtime.exe"
    source.write_text(base["prefix"] + mock + base["runtime"] + fixture + methods + tests, encoding="utf-8", newline="\r\n")
    subprocess.run([compiler, '-include', str(Path(__file__).resolve().parent / 'sd_sound_source_stub.h'), "-std=c++20", "-Wall", "-Wextra", "-Werror", str(source), "-o", str(executable)], check=True)
    environment = os.environ.copy()
    environment["PATH"] = str(Path(compiler).parent) + os.pathsep + environment.get("PATH", "")
    subprocess.run([str(executable)], env=environment, check=True)
