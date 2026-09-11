"""Exercise production FRAM archiving methods with a bounded, fault-injecting driver."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile
import zlib

ROOT = Path(__file__).resolve().parents[1]
header = (ROOT / "lib/ArchivingManager/ArchivingManager.h").read_text(encoding="utf-8")
implementation = (ROOT / "lib/ArchivingManager/ArchivingManager.cpp").read_text(encoding="utf-8")
layout = header[header.index("    struct alignas(4)"):header.index("\npublic:")]
declarations = header[header.index("    static constexpr byte FRAM_ERROR_CRC"):header.rindex("};")]
methods = implementation[implementation.index("namespace\n"):implementation.index("void ArchivingManager::Save_CC_lowpass_filter")]

prefix = r"""
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>
using byte = uint8_t;
constexpr int INSTRUMENTS = 8;
constexpr int RECORDINGS = 30;
struct LillaFRAM_2x512 {
    static constexpr uint32_t TOTAL_SIZE = 131072;
    static constexpr byte ERROR_0 = 0, ERROR_11 = 11;
    std::vector<uint8_t> memory = std::vector<uint8_t>(TOTAL_SIZE, 0xA5);
    int calls = 0, fail_at = 0;
    int stop_after = -1;
    std::vector<std::pair<uint32_t, uint32_t>> writes;
    byte failure = 2;
    void clear_io() { calls = 0; fail_at = 0; stop_after = -1; writes.clear(); }
    void reset() { std::fill(memory.begin(), memory.end(), 0xA5); clear_io(); }
    byte writeArray(uint32_t address, uint32_t count, uint8_t *source) {
        assert(count > 0 && count <= 134 && address + count <= TOTAL_SIZE);
        writes.emplace_back(address, count);
        if (++calls == fail_at) return failure;
        if (stop_after >= 0) {
            const uint32_t accepted = std::min(count, static_cast<uint32_t>(stop_after));
            memcpy(memory.data() + address, source, accepted);
            stop_after -= accepted;
            return accepted == count ? ERROR_0 : failure;
        }
        memcpy(memory.data() + address, source, count);
        return ERROR_0;
    }
    byte readArray(uint32_t address, uint32_t count, uint8_t *destination) {
        assert(count > 0 && count <= 136 && address + count <= TOTAL_SIZE);
        memcpy(destination, memory.data() + address, count);
        if (++calls == fail_at) return failure;
        return ERROR_0;
    }
} LillaFram;
"""
fixture = "class ArchivingManager {\npublic:\n" + layout + declarations + "};\n"
tests = r"""
template<class T, class Write, class Read>
void exercise(uint32_t address, Write write, Read read) {
    LillaFram.reset();
    T source{}, destination{};
    auto *bytes = reinterpret_cast<uint8_t *>(&source);
    for (size_t i = 0; i < sizeof(T); ++i) bytes[i] = static_cast<uint8_t>(i * 37 + 11);
    assert(write(source) == 0);
    assert(memcmp(LillaFram.memory.data() + address, &source, sizeof(T)) == 0);
    assert(std::all_of(LillaFram.memory.begin(), LillaFram.memory.begin() + address, [](uint8_t value) { return value == 0xA5; }));
    assert(std::all_of(LillaFram.memory.begin() + address + sizeof(T), LillaFram.memory.end(), [](uint8_t value) { return value == 0xA5; }));
    assert(read(destination) == 0 && memcmp(&source, &destination, sizeof(T)) == 0);
    const int read_calls = LillaFram.calls / 2;
    for (int failure_call = 1; failure_call <= read_calls; ++failure_call) {
        memset(&destination, 0xCC, sizeof(T));
        T previous = destination;
        LillaFram.calls = 0; LillaFram.fail_at = failure_call;
        assert(read(destination) == 2 && LillaFram.calls == failure_call);
        assert(memcmp(&destination, &previous, sizeof(T)) == 0);
        LillaFram.calls = 0;
        assert(write(source) == 2 && LillaFram.calls == failure_call);
    }
}
template<class T, class Write, class Read>
void invalid(Write write, Read read) {
    LillaFram.reset();
    T value{};
    memset(&value, 0xCC, sizeof(T));
    T previous = value;
    assert(write(value) == 11 && read(value) == 11 && LillaFram.calls == 0);
    assert(memcmp(&value, &previous, sizeof(T)) == 0);
}
using A = ArchivingManager;
using Patch = A::FRAM_Patch_struct;
Patch patterned_patch(uint8_t seed) {
    Patch patch{};
    auto *bytes = reinterpret_cast<uint8_t *>(&patch);
    for (size_t i = 0; i < sizeof(Patch); ++i) bytes[i] = static_cast<uint8_t>(i * 37 + seed);
    return patch;
}
void exercise_patch(uint8_t id) {
    A archive;
    const uint32_t address = 0x100 + id * 192;
    Patch old = patterned_patch(11), fresh = patterned_patch(23), actual{};
    LillaFram.reset();
    assert(archive.FRAM_Write_patch(id, old) == 0);
    const auto baseline = LillaFram.memory;
    assert(LillaFram.writes.size() == 3);
    assert(LillaFram.writes[0].first == address && LillaFram.writes[0].second == 128);
    assert(LillaFram.writes[1].first == address + 128 && LillaFram.writes[1].second == 60);
    assert(LillaFram.writes[2].first == address + 188 && LillaFram.writes[2].second == 4);
    assert(archive.FRAM_Read_patch(id, actual) == 0);
    assert(memcmp(&actual, &old, 188) == 0 && actual.crc32 == EXPECTED_PATTERN_CRC);
    assert(std::all_of(baseline.begin(), baseline.begin() + address, [](uint8_t value) { return value == 0xA5; }));
    assert(std::all_of(baseline.begin() + address + 192, baseline.end(), [](uint8_t value) { return value == 0xA5; }));
    const Patch original_source = patterned_patch(11);
    assert(memcmp(&old, &original_source, sizeof(old)) == 0);

    // Every payload/CRC byte is protected, and rejected reads do not publish anything.
    for (uint32_t byte_index = 0; byte_index < 192; ++byte_index) {
        LillaFram.memory = baseline; LillaFram.clear_io();
        LillaFram.memory[address + byte_index] ^= 0x80;
        memset(&actual, 0xCC, sizeof(actual));
        const Patch previous = actual;
        assert(archive.FRAM_Read_patch(id, actual) == A::FRAM_ERROR_CRC);
        assert(memcmp(&actual, &previous, sizeof(actual)) == 0);
    }
    for (int failure_call : {1, 2}) {
        LillaFram.memory = baseline; LillaFram.clear_io(); LillaFram.fail_at = failure_call;
        const Patch previous = actual;
        assert(archive.FRAM_Read_patch(id, actual) == 2 && LillaFram.calls == failure_call);
        assert(memcmp(&actual, &previous, sizeof(actual)) == 0);
    }
    for (int failure_call : {1, 2, 3}) {
        LillaFram.memory = baseline; LillaFram.clear_io(); LillaFram.fail_at = failure_call;
        assert(archive.FRAM_Write_patch(id, fresh) == 2 && LillaFram.calls == failure_call);
        assert(LillaFram.writes.size() == static_cast<size_t>(failure_call));
    }

    // Simulate power loss after every possible prefix, including a torn CRC.
    for (int cut = 0; cut <= 192; ++cut) {
        LillaFram.memory = baseline; LillaFram.clear_io(); LillaFram.stop_after = cut;
        assert(archive.FRAM_Write_patch(id, fresh) == (cut == 192 ? 0 : 2));
        LillaFram.clear_io();
        memset(&actual, 0xCC, sizeof(actual));
        const Patch previous = actual;
        const byte result = archive.FRAM_Read_patch(id, actual);
        if (result == 0) {
            assert(memcmp(&actual, &old, 188) == 0 || memcmp(&actual, &fresh, 188) == 0);
            if (cut == 192) assert(memcmp(&actual, &fresh, 188) == 0);
        } else {
            assert(result == A::FRAM_ERROR_CRC);
            assert(memcmp(&actual, &previous, sizeof(actual)) == 0);
        }
    }
    for (uint8_t blank : {0, 255}) {
        LillaFram.reset();
        std::fill(LillaFram.memory.begin() + address, LillaFram.memory.begin() + address + 192, blank);
        assert(archive.FRAM_Read_patch(id, actual) == A::FRAM_ERROR_CRC);
    }
}
template<class T, class Write, class Read>
void exercise_nested(uint8_t patch_id, uint32_t offset, Write write, Read read) {
    A archive;
    const uint32_t address = 0x100 + patch_id * 192;
    Patch seed = patterned_patch(11), actual{};
    LillaFram.reset();
    assert(archive.FRAM_Write_patch(patch_id, seed) == 0);
    const auto baseline = LillaFram.memory;
    T source{}, destination{};
    memset(&source, 0x37, sizeof(source));
    Patch expected = seed;
    memcpy(reinterpret_cast<uint8_t *>(&expected) + offset, &source, sizeof(source));
    LillaFram.clear_io();
    assert(write(source) == 0 && LillaFram.writes.size() == 3);
    assert(archive.FRAM_Read_patch(patch_id, actual) == 0 && memcmp(&actual, &expected, 188) == 0);
    assert(read(destination) == 0 && memcmp(&destination, &source, sizeof(source)) == 0);
    assert(std::equal(baseline.begin(), baseline.begin() + address, LillaFram.memory.begin()));
    assert(std::equal(baseline.begin() + address + 192, baseline.end(), LillaFram.memory.begin() + address + 192));

    // Corruption elsewhere in the Patch must block both nested reads and updates.
    LillaFram.memory[address + 10] ^= 1;
    const auto corrupt = LillaFram.memory;
    const T previous = destination;
    LillaFram.clear_io();
    assert(read(destination) == A::FRAM_ERROR_CRC && memcmp(&destination, &previous, sizeof(destination)) == 0);
    assert(write(source) == A::FRAM_ERROR_CRC && LillaFram.writes.empty() && LillaFram.memory == corrupt);

    for (int failure_call : {1, 2}) {
        LillaFram.memory = baseline; LillaFram.clear_io(); LillaFram.fail_at = failure_call;
        assert(read(destination) == 2 && memcmp(&destination, &previous, sizeof(destination)) == 0);
    }
    for (int failure_call = 1; failure_call <= 5; ++failure_call) {
        LillaFram.memory = baseline; LillaFram.clear_io(); LillaFram.fail_at = failure_call;
        assert(write(source) == 2 && LillaFram.calls == failure_call);
        if (failure_call <= 2) assert(LillaFram.writes.empty() && LillaFram.memory == baseline);
    }
}
void exercise_sound(uint16_t id) {
    A archive;
    using Sound = A::FRAM_Sound_struct;
    const uint32_t address = 0x9700 + id * 32;
    Sound old{}, fresh{}, actual{};
    for (size_t i = 0; i < sizeof(Sound); ++i) {
        reinterpret_cast<uint8_t *>(&old)[i] = static_cast<uint8_t>(i * 37 + 11);
        reinterpret_cast<uint8_t *>(&fresh)[i] = static_cast<uint8_t>(i * 37 + 23);
    }
    const Sound original_source = old;
    LillaFram.reset();
    assert(archive.FRAM_Write_sound(id, old) == 0);
    const auto baseline = LillaFram.memory;
    assert(LillaFram.writes.size() == 2);
    assert(LillaFram.writes[0].first == address && LillaFram.writes[0].second == 28);
    assert(LillaFram.writes[1].first == address + 28 && LillaFram.writes[1].second == 4);
    assert(memcmp(&old, &original_source, sizeof(old)) == 0);
    assert(archive.FRAM_Read_sound(id, actual) == 0);
    assert(memcmp(&actual, &old, 28) == 0 && actual.crc32 == EXPECTED_SOUND_CRC);
    assert(std::all_of(baseline.begin(), baseline.begin() + address, [](uint8_t value) { return value == 0xA5; }));
    assert(std::all_of(baseline.begin() + address + 32, baseline.end(), [](uint8_t value) { return value == 0xA5; }));
    for (uint32_t byte_index = 0; byte_index < 32; ++byte_index) {
        LillaFram.memory = baseline; LillaFram.clear_io();
        LillaFram.memory[address + byte_index] ^= 0x80;
        memset(&actual, 0xCC, sizeof(actual));
        const Sound previous = actual;
        assert(archive.FRAM_Read_sound(id, actual) == A::FRAM_ERROR_CRC);
        assert(memcmp(&actual, &previous, sizeof(actual)) == 0);
    }
    LillaFram.memory = baseline; LillaFram.clear_io(); LillaFram.fail_at = 1;
    const Sound previous = actual;
    assert(archive.FRAM_Read_sound(id, actual) == 2 && LillaFram.calls == 1);
    assert(memcmp(&actual, &previous, sizeof(actual)) == 0);
    for (int failure_call : {1, 2}) {
        LillaFram.memory = baseline; LillaFram.clear_io(); LillaFram.fail_at = failure_call;
        assert(archive.FRAM_Write_sound(id, fresh) == 2 && LillaFram.calls == failure_call);
        assert(LillaFram.writes.size() == static_cast<size_t>(failure_call));
    }
    for (int cut = 0; cut <= 32; ++cut) {
        LillaFram.memory = baseline; LillaFram.clear_io(); LillaFram.stop_after = cut;
        assert(archive.FRAM_Write_sound(id, fresh) == (cut == 32 ? 0 : 2));
        LillaFram.clear_io();
        memset(&actual, 0xCC, sizeof(actual));
        const Sound before_read = actual;
        const byte result = archive.FRAM_Read_sound(id, actual);
        if (result == 0) {
            assert(memcmp(&actual, &old, 28) == 0 || memcmp(&actual, &fresh, 28) == 0);
            if (cut == 32) assert(memcmp(&actual, &fresh, 28) == 0);
        } else {
            assert(result == A::FRAM_ERROR_CRC && memcmp(&actual, &before_read, sizeof(actual)) == 0);
        }
    }
    for (uint8_t blank : {0, 255}) {
        LillaFram.reset();
        std::fill(LillaFram.memory.begin() + address, LillaFram.memory.begin() + address + 32, blank);
        assert(archive.FRAM_Read_sound(id, actual) == A::FRAM_ERROR_CRC);
    }
}
int main() {
    const uint8_t check[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    assert(FRAM_Calculate_crc32(check, sizeof(check)) == 0xCBF43926UL);
    ArchivingManager archive;
    using A = ArchivingManager;
    for (uint8_t patch : {0, 199}) {
        exercise_patch(patch);
        exercise_nested<A::FRAM_Patch_delay_struct>(patch, 160, [&](const auto &v) { return archive.FRAM_Write_delay(patch, v); }, [&](auto &v) { return archive.FRAM_Read_delay(patch, v); });
        for (uint8_t instrument : {0, 7}) {
            exercise_nested<A::FRAM_Instrument_struct>(patch, 32 + instrument * 16, [&](const auto &v) { return archive.FRAM_Write_instrument(patch, instrument, v); }, [&](auto &v) { return archive.FRAM_Read_instrument(patch, instrument, v); });
            exercise_nested<A::FRAM_Instrument_filter_struct>(patch, 40 + instrument * 16, [&](const auto &v) { return archive.FRAM_Write_filter(patch, instrument, v); }, [&](auto &v) { return archive.FRAM_Read_filter(patch, instrument, v); });
        }
    }
    for (uint16_t sound : {0, 255, 256, 799}) exercise_sound(sound);
    for (uint8_t recording : {0, 29}) exercise<A::FRAM_Recording_struct>(0xFB00 + recording * 8, [&](const auto &v) { return archive.FRAM_Write_recording(recording, v); }, [&](auto &v) { return archive.FRAM_Read_recording(recording, v); });
    exercise<A::FRAM_CC_settings_struct>(0xFC04, [&](const auto &v) { return archive.FRAM_Write_CC_settings(v); }, [&](auto &v) { return archive.FRAM_Read_CC_settings(v); });
    exercise<A::FRAM_System_struct>(0xFC00, [&](const auto &v) { return archive.FRAM_Write_system(v); }, [&](auto &v) { return archive.FRAM_Read_system(v); });
    for (uint8_t patch : {200, 255}) {
        invalid<A::FRAM_Patch_struct>([&](const auto &v) { return archive.FRAM_Write_patch(patch, v); }, [&](auto &v) { return archive.FRAM_Read_patch(patch, v); });
        invalid<A::FRAM_Patch_delay_struct>([&](const auto &v) { return archive.FRAM_Write_delay(patch, v); }, [&](auto &v) { return archive.FRAM_Read_delay(patch, v); });
        invalid<A::FRAM_Instrument_struct>([&](const auto &v) { return archive.FRAM_Write_instrument(patch, 0, v); }, [&](auto &v) { return archive.FRAM_Read_instrument(patch, 0, v); });
        invalid<A::FRAM_Instrument_filter_struct>([&](const auto &v) { return archive.FRAM_Write_filter(patch, 0, v); }, [&](auto &v) { return archive.FRAM_Read_filter(patch, 0, v); });
    }
    for (uint8_t instrument : {8, 255}) {
        invalid<A::FRAM_Instrument_struct>([&](const auto &v) { return archive.FRAM_Write_instrument(0, instrument, v); }, [&](auto &v) { return archive.FRAM_Read_instrument(0, instrument, v); });
        invalid<A::FRAM_Instrument_filter_struct>([&](const auto &v) { return archive.FRAM_Write_filter(0, instrument, v); }, [&](auto &v) { return archive.FRAM_Read_filter(0, instrument, v); });
    }
    for (uint16_t sound : {800, 65535}) invalid<A::FRAM_Sound_struct>([&](const auto &v) { return archive.FRAM_Write_sound(sound, v); }, [&](auto &v) { return archive.FRAM_Read_sound(sound, v); });
    for (uint8_t recording : {30, 255}) invalid<A::FRAM_Recording_struct>([&](const auto &v) { return archive.FRAM_Write_recording(recording, v); }, [&](auto &v) { return archive.FRAM_Read_recording(recording, v); });
    std::cout << "PASS: all 16 FRAM operations, physical offsets, first/last IDs, Sound IDs above 255, region isolation, buffer limits, invalid IDs and injected I/O failures\n";
}
"""
tests = tests.replace("EXPECTED_PATTERN_CRC", hex(zlib.crc32(bytes((i * 37 + 11) & 255 for i in range(188)))))
tests = tests.replace("EXPECTED_SOUND_CRC", hex(zlib.crc32(bytes((i * 37 + 11) & 255 for i in range(28)))))
compiler = shutil.which("g++") or r"C:\msys64\ucrt64\bin\g++.exe"
with tempfile.TemporaryDirectory(prefix="lilla-fram-") as directory:
    cpp = Path(directory) / "fram.cpp"
    exe = Path(directory) / ("fram.exe" if os.name == "nt" else "fram")
    cpp.write_text(prefix + fixture + methods + tests, encoding="utf-8", newline="\r\n")
    subprocess.run([compiler, "-std=c++20", "-Wall", "-Wextra", "-Werror", str(cpp), "-o", str(exe)], check=True)
    environment = os.environ.copy()
    environment["PATH"] = str(Path(compiler).parent) + os.pathsep + environment.get("PATH", "")
    subprocess.run([str(exe)], check=True, env=environment)
