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
declarations = header[header.index("    struct FRAM_Repair_report"):header.rindex("};")]
settings_declarations = header[header.index("    byte Save_CC_lowpass_filter"):header.index("    byte Read_Delay")]
settings_declarations += header[header.index("    byte Read_first_octave"):header.index("    byte Save_Sound")]
settings_declarations += header[header.index("    byte Save_DS_Recording"):header.index("    byte Factory_reset_FRAM")]
methods = implementation[implementation.index("namespace\n{"):implementation.index("byte ArchivingManager::Set_FRAM_archive_state")]
methods += implementation[implementation.index("uint32_t ArchivingManager::FRAM_Get_patch_address"):implementation.index("byte ArchivingManager::Save_CC_lowpass_filter")]
methods += implementation[implementation.index("byte ArchivingManager::Save_CC_lowpass_filter"):implementation.index("byte ArchivingManager::Read_Delay")]
methods += implementation[implementation.index("byte ArchivingManager::Read_first_octave"):implementation.index("byte ArchivingManager::Save_Sound")]
methods += implementation[implementation.index("byte ArchivingManager::Save_DS_Recording"):implementation.index("String ArchivingManager::Filename_Patch")]
raw_read_start = methods.index("    byte FRAM_Read_bytes")
raw_read_end = methods.index("    template <class T>", raw_read_start)
methods = methods[:raw_read_start] + methods[raw_read_end:]
shared = (ROOT / "lib/SharedElements/SharedElements.h").read_text(encoding="utf-8")
runtime = shared[shared.index("struct Instrument_filter_data_struct"):shared.index("struct Instrument_filter_values_struct")]
runtime += shared[shared.index("struct Sound_struct"):shared.index("// == overloads")]
runtime += "\nPatch_struct Patch[PATCHES_MAX + 1];\nSound_struct Sound[SOUNDS_MAX + 2];\n"

prefix = r"""
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <vector>
#define F(x) x
struct SerialStub {
    void print(const char *text) { std::fputs(text, stdout); }
    void println(const char *text) { std::puts(text); }
} Serial;
using byte = uint8_t;
constexpr int INSTRUMENTS = 8;
constexpr int RECORDINGS = 30;
constexpr int PATCHES_MAX = 200;
constexpr int SOUNDS_MAX = 800;
constexpr int VFS_PACKETS_MAX = 512;
constexpr uint8_t Normalize_optimization(uint8_t value) { return value <= 2 ? value : 1; }
struct VFS_Recording { int first_packet = 0, packets = 0, bytes = 0; float seconds = 0; bool stereo = false, consistent = false; };
VFS_Recording Recording[RECORDINGS];
struct LillaFRAM_2x512 {
    static constexpr uint32_t TOTAL_SIZE = 131072;
    static constexpr byte ERROR_0 = 0, ERROR_11 = 11;
    std::vector<uint8_t> memory = std::vector<uint8_t>(TOTAL_SIZE, 0xA5);
    int calls = 0, fail_at = 0;
    int stop_after = -1;
    int corrupt_on_read = -1;
    std::vector<std::pair<uint32_t, uint32_t>> writes;
    byte failure = 2;
    void clear_io() { calls = 0; fail_at = 0; stop_after = -1; corrupt_on_read = -1; writes.clear(); }
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
        if (corrupt_on_read >= 0) { memory.at(corrupt_on_read) ^= 1; corrupt_on_read = -1; }
        memcpy(destination, memory.data() + address, count);
        if (++calls == fail_at) return failure;
        return ERROR_0;
    }
} LillaFram;
"""
fixture = "class ArchivingManager {\npublic:\n" + layout + settings_declarations + declarations + "};\n"
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
using StoredPatch = A::FRAM_Patch_struct;
StoredPatch patterned_patch(uint8_t seed) {
    StoredPatch patch{};
    auto *bytes = reinterpret_cast<uint8_t *>(&patch);
    for (size_t i = 0; i < sizeof(StoredPatch); ++i) bytes[i] = static_cast<uint8_t>(i * 37 + seed);
    return patch;
}
void exercise_patch(uint8_t id) {
    A archive;
    const uint32_t address = 0x100 + id * 192;
    StoredPatch old = patterned_patch(11), fresh = patterned_patch(23), actual{};
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
    const StoredPatch original_source = patterned_patch(11);
    assert(memcmp(&old, &original_source, sizeof(old)) == 0);

    // Every payload/CRC byte is protected, and rejected reads do not publish anything.
    for (uint32_t byte_index = 0; byte_index < 192; ++byte_index) {
        LillaFram.memory = baseline; LillaFram.clear_io();
        LillaFram.memory[address + byte_index] ^= 0x80;
        memset(&actual, 0xCC, sizeof(actual));
        const StoredPatch previous = actual;
        assert(archive.FRAM_Read_patch(id, actual) == A::FRAM_ERROR_CRC);
        assert(memcmp(&actual, &previous, sizeof(actual)) == 0);
    }
    for (int failure_call : {1, 2}) {
        LillaFram.memory = baseline; LillaFram.clear_io(); LillaFram.fail_at = failure_call;
        const StoredPatch previous = actual;
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
        const StoredPatch previous = actual;
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
    StoredPatch seed = patterned_patch(11), actual{};
    LillaFram.reset();
    assert(archive.FRAM_Write_patch(patch_id, seed) == 0);
    const auto baseline = LillaFram.memory;
    T source{}, destination{};
    memset(&source, 0x37, sizeof(source));
    StoredPatch expected = seed;
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
template<class T, class Write, class Read>
void exercise_crc_record(uint32_t address, uint32_t payload_bytes, Write write, Read read) {
    LillaFram.reset();
    T source{}, destination{};
    auto *bytes = reinterpret_cast<uint8_t *>(&source);
    for (size_t i = 0; i < sizeof(T); ++i) bytes[i] = static_cast<uint8_t>(i * 37 + 11);
    const T original_source = source;
    assert(write(source) == 0);
    assert(memcmp(&source, &original_source, sizeof(source)) == 0);
    assert(memcmp(LillaFram.memory.data() + address, &source, payload_bytes) == 0);
    uint32_t stored_crc = 0;
    memcpy(&stored_crc, LillaFram.memory.data() + address + payload_bytes, sizeof(stored_crc));
    assert(stored_crc == FRAM_Calculate_crc32(reinterpret_cast<const uint8_t *>(&source), payload_bytes));
    const auto baseline = LillaFram.memory;
    assert(read(destination) == 0 && memcmp(&source, &destination, payload_bytes) == 0 && destination.crc32 == stored_crc);
    for (uint32_t byte_index = 0; byte_index < sizeof(T); ++byte_index) {
        LillaFram.memory = baseline; LillaFram.clear_io();
        LillaFram.memory[address + byte_index] ^= 0x80;
        memset(&destination, 0xCC, sizeof(destination));
        const T previous = destination;
        assert(read(destination) == A::FRAM_ERROR_CRC);
        assert(memcmp(&destination, &previous, sizeof(destination)) == 0);
    }
    const int read_calls = static_cast<int>((sizeof(T) + 127) / 128);
    for (int failure_call = 1; failure_call <= read_calls; ++failure_call) {
        LillaFram.memory = baseline; LillaFram.clear_io(); LillaFram.fail_at = failure_call;
        const T previous = destination;
        assert(read(destination) == LillaFram.failure && memcmp(&destination, &previous, sizeof(destination)) == 0);
    }
    const int write_calls = static_cast<int>((payload_bytes + 127) / 128) + 1;
    for (int failure_call = 1; failure_call <= write_calls; ++failure_call) {
        LillaFram.memory = baseline; LillaFram.clear_io(); LillaFram.fail_at = failure_call;
        assert(write(source) == LillaFram.failure && LillaFram.calls == failure_call);
    }
}
void exercise_cc_settings() {
    A archive;
    A::FRAM_System_struct system{};
    system.optimization = 3;
    system.first_octave = -2;
    system.key_step = 2;
    memset(system.reserved, 0x5A, sizeof(system.reserved));
    LillaFram.reset();
    assert(archive.FRAM_Write_system(system) == 0);
    A::FRAM_CC_settings_struct source{}, destination{};
    for (size_t i = 0; i < sizeof(source); ++i) reinterpret_cast<uint8_t *>(&source)[i] = static_cast<uint8_t>(i * 19 + 7);
    assert(archive.FRAM_Write_CC_settings(source) == 0);
    assert(archive.FRAM_Read_CC_settings(destination) == 0 && memcmp(&source, &destination, sizeof(source)) == 0);
    A::FRAM_System_struct actual{};
    assert(archive.FRAM_Read_system(actual) == 0 && actual.optimization == 3 && actual.first_octave == -2 && actual.key_step == 2 && memcmp(actual.reserved, system.reserved, sizeof(system.reserved)) == 0);
    const auto baseline = LillaFram.memory;
    LillaFram.memory[0xFD00] ^= 1;
    const auto corrupt = LillaFram.memory;
    memset(&destination, 0xCC, sizeof(destination));
    const auto previous = destination;
    LillaFram.clear_io();
    assert(archive.FRAM_Read_CC_settings(destination) == A::FRAM_ERROR_CRC && memcmp(&destination, &previous, sizeof(destination)) == 0);
    assert(archive.FRAM_Write_CC_settings(source) == A::FRAM_ERROR_CRC && LillaFram.writes.empty() && LillaFram.memory == corrupt);
    LillaFram.memory = baseline;
}
void exercise_system_settings_backend() {
    A archive;
    A::FRAM_System_struct system{};
    system.first_octave = -2;
    LillaFram.reset();
    assert(archive.FRAM_Write_system(system) == 0);
    uint8_t sound_gain[INSTRUMENTS] = {11, 12, 13, 14, 15, 16, 17, 18};
    assert(archive.Save_optimization(2) == 0);
    assert(archive.Save_first_octave(-1) == 0);
    assert(archive.Save_key_step(3) == 0);
    assert(archive.Save_CC_settings(sound_gain, 74) == 0);
    uint8_t optimization = 0, key_step = 0, lowpass_filter = 0, actual_gain[INSTRUMENTS] = {};
    int8_t first_octave = 0;
    assert(archive.Read_optimization(optimization) == 0 && optimization == 2);
    assert(archive.Read_first_octave(first_octave) == 0 && first_octave == -1);
    assert(archive.Read_key_step(key_step) == 0 && key_step == 3);
    assert(archive.Read_CC_settings(actual_gain, lowpass_filter) == 0 && memcmp(actual_gain, sound_gain, sizeof(sound_gain)) == 0 && lowpass_filter == 74);
    assert(archive.Save_CC_Sound_gain(7, 99) == 0 && archive.Read_CC_Sound_gain(7, actual_gain[7]) == 0 && actual_gain[7] == 99);
    assert(archive.Save_CC_lowpass_filter(88) == 0 && archive.Read_CC_lowpass_filter(lowpass_filter) == 0 && lowpass_filter == 88);
    LillaFram.clear_io();
    assert(archive.Save_CC_Sound_gain(8, 1) == LillaFRAM_2x512::ERROR_11 && LillaFram.calls == 0);
    assert(archive.Read_CC_Sound_gain(8, actual_gain[0]) == LillaFRAM_2x512::ERROR_11 && LillaFram.calls == 0);
    LillaFram.memory[0xFD00] ^= 1;
    optimization = 77;
    assert(archive.Read_optimization(optimization) == A::FRAM_ERROR_CRC && optimization == 77);
    LillaFram.clear_io();
    assert(archive.Save_key_step(1) == A::FRAM_ERROR_CRC && LillaFram.writes.empty());
}
void exercise_recording_runtime_backend() {
    A archive;
    LillaFram.reset();
    Recording[5].first_packet = 123;
    Recording[5].packets = 17;
    Recording[5].bytes = 999;
    Recording[5].seconds = 1.5f;
    Recording[5].stereo = true;
    Recording[5].consistent = true;
    assert(archive.Save_DS_Recording(5) == 0);
    Recording[5] = {};
    Recording[5].bytes = 77;
    Recording[5].seconds = 2.5f;
    assert(archive.Read_DS_Recording(5) == 0);
    assert(Recording[5].first_packet == 123 && Recording[5].packets == 17 && Recording[5].stereo && Recording[5].consistent);
    assert(Recording[5].bytes == 77 && Recording[5].seconds == 2.5f);
    assert(archive.Save_DS_Recording(-1) == LillaFRAM_2x512::ERROR_11 && archive.Read_DS_Recording(30) == LillaFRAM_2x512::ERROR_11);
    Recording[5].first_packet = -1;
    assert(archive.Save_DS_Recording(5) == A::FRAM_ERROR_SOURCE);
    Recording[5].first_packet = 123;
    const VFS_Recording previous = Recording[5];
    LillaFram.memory[0xFB00 + 5 * 12] ^= 1;
    assert(archive.Read_DS_Recording(5) == A::FRAM_ERROR_CRC && memcmp(&Recording[5], &previous, sizeof(previous)) == 0);
}
int main() {
    {
        using A = ArchivingManager;
        A archive;
        auto seed_archive = [&]()
        {
            LillaFram.reset();
            A::FRAM_Patch_struct empty_patch{};
            A::FRAM_Sound_struct empty_sound{};
            A::FRAM_Recording_struct empty_recording{};
            empty_recording.consistent = 1;
            for (int id = 0; id < 200; ++id)
            {
                assert(archive.FRAM_Write_patch(id, empty_patch) == 0);
            }
            for (int id = 0; id < 800; ++id)
            {
                assert(archive.FRAM_Write_sound(id, empty_sound) == 0);
            }
            for (int id = 0; id < 30; ++id)
            {
                assert(archive.FRAM_Write_recording(id, empty_recording) == 0);
            }
            A::FRAM_System_struct settings{};
            empty_recording.first_packet = 123;
            empty_recording.packets = 7;
            assert(archive.FRAM_Write_recording(0, empty_recording) == 0);
            assert(archive.FRAM_Write_recording(29, empty_recording) == 0);
            settings.optimization = 2;
            settings.first_octave = -2;
            assert(archive.FRAM_Write_system(settings) == 0);
            empty_patch.used = 1;
            empty_patch.instruments = 1;
            empty_patch.Instrument[0].used = 1;
            empty_patch.Instrument[0].sound_id = 84;
            assert(archive.FRAM_Write_patch(23, empty_patch) == 0);
            empty_sound.used = 1;
            empty_sound.A = 0x12345678;
            empty_sound.pitch = -12;
            empty_sound.midi_channel = 15;
            empty_sound.attack_type = 1;
            assert(archive.FRAM_Write_sound(84, empty_sound) == 0);
        };
        seed_archive();
        A::FRAM_Patch_struct patch{};
        A::FRAM_Sound_struct value{};
        A::FRAM_Recording_struct recording{};
        A::FRAM_System_struct system{};
        // Recording repair assesses the complete bank before writing and can resume after interruption.
        LillaFram.memory[0xFB00] ^= 1;
        LillaFram.memory[0xFB00 + 29 * 12 + 8] ^= 1;
        const auto damaged_recordings = LillaFram.memory;
        A::FRAM_Recording_repair_report recording_report;
        LillaFram.clear_io(); LillaFram.fail_at = 2;
        assert(archive.Repair_Recordings_in_FRAM(recording_report) == LillaFram.failure && LillaFram.writes.empty() && LillaFram.memory == damaged_recordings);
        LillaFram.clear_io(); LillaFram.stop_after = 5;
        assert(archive.Repair_Recordings_in_FRAM(recording_report) == LillaFram.failure && recording_report.failed_id == 0);
        LillaFram.clear_io();
        assert(archive.Repair_Recordings_in_FRAM(recording_report) == 0 && recording_report.cleared_recordings == 2 && recording_report.failed_id == UINT8_MAX);
        for (uint8_t id : {0, 29}) {
            assert(archive.FRAM_Read_recording(id, recording) == 0 && recording.first_packet == 0 && recording.packets == 0 && recording.stereo == 0 && recording.consistent == 1);
        }
        LillaFram.clear_io();
        assert(archive.Repair_Recordings_in_FRAM(recording_report) == 0 && recording_report.cleared_recordings == 0 && LillaFram.writes.empty());
        // System repair defaults only CRC corruption, never an I/O failure, and is retryable.
        LillaFram.memory[0xFD00] ^= 1;
        const auto damaged_system = LillaFram.memory;
        A::FRAM_System_repair_report system_report;
        LillaFram.clear_io(); LillaFram.fail_at = 1;
        assert(archive.Repair_System_in_FRAM(system_report) == LillaFram.failure && !system_report.defaulted && LillaFram.writes.empty() && LillaFram.memory == damaged_system);
        LillaFram.clear_io(); LillaFram.stop_after = 100;
        assert(archive.Repair_System_in_FRAM(system_report) == LillaFram.failure && !system_report.defaulted);
        LillaFram.clear_io();
        assert(archive.Repair_System_in_FRAM(system_report) == 0 && system_report.defaulted);
        assert(archive.FRAM_Read_system(system) == 0 && system.optimization == 0 && system.first_octave == -2 && system.key_step == 0);
        const uint8_t *system_cc = reinterpret_cast<const uint8_t *>(&system.CC_settings);
        assert(std::all_of(system_cc, system_cc + sizeof(system.CC_settings), [](uint8_t value) { return value == 0; }));
        LillaFram.clear_io();
        assert(archive.Repair_System_in_FRAM(system_report) == 0 && !system_report.defaulted && LillaFram.writes.empty());
        seed_archive();
        // Load production runtime structures, including high Sound IDs and the reserved sampler slots.
        A::FRAM_Sound_struct high_sound{};
        assert(archive.FRAM_Read_sound(84, high_sound) == 0);
        assert(archive.FRAM_Write_sound(799, high_sound) == 0);
        A::FRAM_Patch_struct high_patch{};
        assert(archive.FRAM_Read_patch(23, high_patch) == 0);
        high_patch.Instrument[0].sound_id = 799;
        assert(archive.FRAM_Write_patch(199, high_patch) == 0);
        uint16_t failed_id = 0;
        bool failed_sound = false;
        memset(Patch, 0xA5, sizeof(Patch));
        memset(Sound, 0xA5, sizeof(Sound));
        LillaFram.clear_io();
        assert(archive.Load_Patch_Sound_from_FRAM(failed_id, failed_sound) == 0);
        assert(LillaFram.writes.empty() && failed_id == UINT16_MAX);
        assert(Patch[199].used && Patch[199].Instrument[0].sound_id == 799 && Sound[799].used);
        assert(Sound[799].A == 0x12345678 && Sound[799].pitch == -12 && Sound[799].data == 31);
        assert(!Patch[0].used && !Patch[200].used && !Sound[800].used && !Sound[801].used);
        LillaFram.memory[0x9700 + 799 * 32] ^= 1;
        assert(archive.Load_Patch_Sound_from_FRAM(failed_id, failed_sound) == A::FRAM_ERROR_CRC && failed_id == 799 && failed_sound);
        assert(!Patch[199].used && !Sound[799].used);
        assert(archive.FRAM_Write_sound(799, high_sound) == 0);
        high_patch.Instrument[0].sound_id = 800;
        assert(archive.FRAM_Write_patch(199, high_patch) == 0);
        assert(archive.Load_Patch_Sound_from_FRAM(failed_id, failed_sound) == A::FRAM_ERROR_SOURCE && failed_id == 199 && !failed_sound);
        assert(!Patch[23].used && !Sound[84].used);
        // Repair only FRAM: corrupt Patch links cannot delete a Sound referenced by a valid Patch.
        seed_archive();
        high_patch.Instrument[0].sound_id = 799;
        assert(archive.FRAM_Write_sound(799, high_sound) == 0);
        assert(archive.FRAM_Write_patch(199, high_patch) == 0);
        assert(archive.FRAM_Write_sound(700, high_sound) == 0); // Existing orphan.
        LillaFram.memory[0x100 + 23 * 192] ^= 1;
        LillaFram.memory[0x100 + 23 * 192 + 34] = 31; // Damaged link now points to Sound 799.
        LillaFram.memory[0x100 + 23 * 192 + 35] = 3;
        LillaFram.memory[0x9700 + 799 * 32] ^= 1;
        LillaFram.memory[0x9700 + 699 * 32] ^= 1; // Corrupt, unreferenced Sound.
        const auto damaged_fram = LillaFram.memory;
        uint8_t patch_before[sizeof(Patch)], sound_before[sizeof(Sound)];
        memcpy(patch_before, Patch, sizeof(Patch));
        memcpy(sound_before, Sound, sizeof(Sound));
        A::FRAM_Repair_report report;
        LillaFram.clear_io(); LillaFram.fail_at = 401; // First Sound read, after scanning all Patches.
        assert(archive.Repair_Patch_Sound_in_FRAM(report) == LillaFram.failure && LillaFram.writes.empty());
        assert(LillaFram.memory == damaged_fram);
        LillaFram.clear_io(); LillaFram.stop_after = 17;
        assert(archive.Repair_Patch_Sound_in_FRAM(report) == LillaFram.failure);
        assert(!report.failed_sound && report.failed_id == 23);
        LillaFram.clear_io();
        assert(archive.Repair_Patch_Sound_in_FRAM(report) == 0);
        assert(report.cleared_patches == 1 && report.cleared_sounds == 3 && report.defaulted_sounds == 1);
        assert(memcmp(patch_before, Patch, sizeof(Patch)) == 0 && memcmp(sound_before, Sound, sizeof(Sound)) == 0);
        assert(archive.FRAM_Read_patch(23, patch) == 0 && patch.used == 0);
        for (int id : {84, 699, 700}) {
            assert(archive.FRAM_Read_sound(id, value) == 0);
            const auto bytes = reinterpret_cast<const uint8_t *>(&value);
            assert(std::all_of(bytes, bytes + 28, [](uint8_t b) { return b == 0; }));
        }
        A::FRAM_Sound_struct defaults{};
        defaults.used = 1; defaults.B = 1000; defaults.decay = 50; defaults.sustain = 50; defaults.release = 10; defaults.gain = 12;
        assert(archive.FRAM_Read_sound(799, value) == 0 && memcmp(&value, &defaults, 28) == 0);
        assert(archive.FRAM_Read_patch(199, patch) == 0 && patch.used == 1 && patch.Instrument[0].sound_id == 799);
        LillaFram.clear_io();
        assert(archive.Repair_Patch_Sound_in_FRAM(report) == 0 && LillaFram.writes.empty());
        assert(report.cleared_patches == 0 && report.cleared_sounds == 0 && report.defaulted_sounds == 0);
        assert(archive.Load_Patch_Sound_from_FRAM(failed_id, failed_sound) == 0 && Sound[799].B == 1000 && Sound[799].gain == 12);
        Serial.println(F("PASS: FRAM-only repair, orphan cleanup, exact defaults, untrusted links, read failure, interrupted write and retry"));
        Serial.println(F("PASS: FRAM Recording repair, read failure, interrupted write and retry"));
        Serial.println(F("PASS: FRAM System repair, exact defaults, read failure, interrupted write and retry"));
        Serial.println(F("PASS: FRAM runtime load, high IDs, CRC failure, invalid links, zeroed sampler slots and no persistent writes"));
    }
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
    for (uint8_t recording : {0, 29}) exercise_crc_record<A::FRAM_Recording_struct>(0xFB00 + recording * 12, 8, [&](const auto &v) { return archive.FRAM_Write_recording(recording, v); }, [&](auto &v) { return archive.FRAM_Read_recording(recording, v); });
    exercise_crc_record<A::FRAM_System_struct>(0xFD00, 252, [&](const auto &v) { return archive.FRAM_Write_system(v); }, [&](auto &v) { return archive.FRAM_Read_system(v); });
    exercise_cc_settings();
    exercise_system_settings_backend();
    exercise_recording_runtime_backend();
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
    Serial.println(F("PASS: all 16 FRAM operations, physical offsets, first/last IDs, Sound IDs above 255, region isolation, buffer limits, invalid IDs and injected I/O failures"));
}
"""
tests = tests.replace("EXPECTED_PATTERN_CRC", hex(zlib.crc32(bytes((i * 37 + 11) & 255 for i in range(188)))))
tests = tests.replace("EXPECTED_SOUND_CRC", hex(zlib.crc32(bytes((i * 37 + 11) & 255 for i in range(28)))))
compiler = shutil.which("g++") or r"C:\msys64\ucrt64\bin\g++.exe"
with tempfile.TemporaryDirectory(prefix="lilla-fram-") as directory:
    cpp = Path(directory) / "fram.cpp"
    exe = Path(directory) / ("fram.exe" if os.name == "nt" else "fram")
    cpp.write_text(prefix + runtime + fixture + methods + tests, encoding="utf-8", newline="\r\n")
    subprocess.run([compiler, "-std=c++20", "-Wall", "-Wextra", "-Werror", str(cpp), "-o", str(exe)], check=True)
    environment = os.environ.copy()
    environment["PATH"] = str(Path(compiler).parent) + os.pathsep + environment.get("PATH", "")
    subprocess.run([str(exe)], check=True, env=environment)
