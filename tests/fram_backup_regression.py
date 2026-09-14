"""Exercise FRAM metadata codecs, audio manifests and restore markers with injected failures."""
from pathlib import Path
import os
import runpy
import subprocess
import tempfile

base = runpy.run_path(str(Path(__file__).with_name("fram_archiving_regression.py")))
header = base["header"]
implementation = base["implementation"]
extra = header[header.index("    byte Factory_reset_FRAM"):header.index("    struct FRAM_Repair_report")]
fixture = base["fixture"].replace("class ArchivingManager {\npublic:\n", "class ArchivingManager {\npublic:\n" + extra)
methods = implementation[implementation.index("namespace\n{"):implementation.index("uint32_t ArchivingManager::FRAM_Get_patch_address")]
methods += base["methods"][base["methods"].index("uint32_t ArchivingManager::FRAM_Get_patch_address"):]
mock = r'''
#include <map>
#include <memory>
#include <string>
constexpr int PACKET_DIM = 65536;
constexpr int FILE_WRITE = 1;
struct File {
    std::shared_ptr<std::vector<uint8_t>> data;
    size_t position = 0;
    static inline bool fail_write = false;
    explicit operator bool() const { return bool(data); }
    size_t size() const { return data ? data->size() : 0; }
    bool seek(size_t p) { position = p; return data && p <= data->size(); }
    size_t read(uint8_t *out, size_t count) {
        if (!data || position > data->size())
        {
            return 0;
        }
        count = std::min(count, data->size() - position);
        memcpy(out, data->data() + position, count);
        position += count;
        return count;
    }
    size_t write(const uint8_t *in, size_t count) {
        if (!data || fail_write)
        {
            return 0;
        }
        data->resize(std::max(data->size(), position + count));
        memcpy(data->data() + position, in, count);
        position += count;
        return count;
    }
    void close() { data.reset(); }
};
struct SDMock {
    std::map<std::string, std::shared_ptr<std::vector<uint8_t>>> files;
    int rename_calls = 0, fail_rename = 0;
    bool exists(const char *p) { return files.count(p) != 0; }
    bool remove(const char *p) { return files.erase(p) != 0; }
    bool rename(const char *a, const char *b) {
        if (++rename_calls == fail_rename || !exists(a) || exists(b))
        {
            return false;
        }
        files[b] = files[a];
        files.erase(a);
        return true;
    }
    File open(const char *p, int mode = 0) {
        if (!exists(p) && mode == FILE_WRITE)
        {
            files[p] = std::make_shared<std::vector<uint8_t>>();
        }
        return exists(p) ? File{files[p], mode == FILE_WRITE ? files[p]->size() : 0} : File{};
    }
} SD;
'''
tests = r'''
int main() {
    using A = ArchivingManager;
    A archive;
    LillaFram.reset();
    assert(archive.Factory_reset_FRAM() == 0);
    A::FRAM_Recording_struct recording{};
    recording.first_packet = 10;
    recording.packets = 7;
    recording.consistent = 1;
    assert(archive.FRAM_Write_recording(0, recording) == 0);
    const auto baseline = LillaFram.memory;
    std::fill_n(LillaFram.memory.begin(), 16, 0);
    assert(archive.Check_FRAM_archive() == 0);
    assert(std::equal(baseline.begin() + 256, baseline.end(), LillaFram.memory.begin() + 256));
    File backup = SD.open("/metadata.test", FILE_WRITE);
    assert(archive.Save_FRAM_backup(backup));
    const auto good = *backup.data;
    assert(good.size() == 20 + 0xFE00 - 256);
    assert(archive.Verify_FRAM_backup(backup));
    (*backup.data)[25] ^= 1;
    LillaFram.clear_io();
    assert(!archive.Restore_FRAM_backup(backup) && LillaFram.writes.empty());
    *backup.data = good;
    backup.data->push_back(0);
    assert(!archive.Restore_FRAM_backup(backup) && LillaFram.writes.empty());
    *backup.data = good;
    // Interrupt the state marker, payload, and final READY marker at byte boundaries.
    for (int limit : {1, 8, 12, 15, 16, 17, 300, 64784, 64790, 64799})
    {
        LillaFram.memory = baseline;
        LillaFram.clear_io();
        LillaFram.stop_after = limit;
        assert(!archive.Restore_FRAM_backup(backup));
        LillaFram.clear_io();
        if (limit >= 12)
        {
            assert(archive.Check_FRAM_archive() != 0);
        }
        assert(archive.Restore_FRAM_backup(backup));
        assert(archive.Check_FRAM_archive() == 0);
        assert(std::equal(baseline.begin() + 256, baseline.begin() + 0xFE00, LillaFram.memory.begin() + 256));
        A::FRAM_Recording_struct actual{};
        assert(archive.FRAM_Read_recording(0, actual) == 0 && actual.packets == 7);
    }
    File::fail_write = true;
    LillaFram.clear_io();
    LillaFram.fail_at = 5; // Payload readback, after the in-progress marker and first write.
    assert(!archive.Restore_FRAM_backup(backup));
    LillaFram.clear_io();
    assert(archive.Check_FRAM_archive() != 0);
    assert(archive.Restore_FRAM_backup(backup));
    assert(!archive.Save_FRAM_backup(backup));
    assert(*backup.data == good);
    File::fail_write = false;
    A::Recording_backup_audio audio[RECORDINGS]{};
    VFS_Recording entries[RECORDINGS]{};
    assert(!archive.Read_backup_audio(backup, audio, entries)); // V2 has no audio identity.
    audio[0].bytes[0] = 7 * PACKET_DIM;
    audio[0].crc32[0] = 12345;
    File full{std::make_shared<std::vector<uint8_t>>()};
    assert(archive.Save_FRAM_backup(full, audio));
    assert(archive.Read_backup_audio(full, audio, entries));
    assert(entries[0].first_packet == 10 && entries[0].packets == 7 && audio[0].crc32[0] == 12345);
    LillaFram.clear_io();
    assert(!archive.Restore_FRAM_backup(full) && LillaFram.writes.empty()); // Cannot publish metadata-only V3 restore.
    assert(archive.Restore_FRAM_backup(full, false) && archive.Check_FRAM_archive() != 0);
    assert(archive.Set_FRAM_archive_state(A::ARCHIVE_READY) == 0);
    audio[0].bytes[0] -= 2;
    File mismatch{std::make_shared<std::vector<uint8_t>>()};
    assert(archive.Save_FRAM_backup(mismatch, audio));
    assert(archive.Verify_FRAM_backup(mismatch) && !archive.Read_backup_audio(mismatch, audio, entries));
    // Version 1 includes the old header: validate it, but never copy it to FRAM.
    A::FRAM_Backup_header_struct old{};
    memcpy(old.magic, "LILLAFRM", 8);
    old.version = 1;
    old.header_bytes = sizeof(old);
    old.payload_bytes = 0xFE00;
    auto legacy = baseline;
    std::fill_n(legacy.begin(), 256, 0xCC);
    old.payload_crc32 = FRAM_Calculate_crc32(legacy.data(), old.payload_bytes);
    auto bytes = std::make_shared<std::vector<uint8_t>>(sizeof(old) + old.payload_bytes);
    memcpy(bytes->data(), &old, sizeof(old));
    memcpy(bytes->data() + sizeof(old), legacy.data(), old.payload_bytes);
    File v1{bytes};
    assert(archive.Restore_FRAM_backup(v1));
    assert(archive.Check_FRAM_archive() == 0);
    assert(LillaFram.memory[100] == baseline[100]);
    assert(archive.Factory_reset_FRAM(false) == 0);
    assert(archive.Check_FRAM_archive() != 0);
    assert(archive.Set_FRAM_archive_state(A::ARCHIVE_READY) == 0);
    assert(archive.Check_FRAM_archive() == 0);
    std::puts("PASS: header initialization, V1/V2 codecs, V3 audio manifest, deferred READY, interruption/retry and CRC rejection");
}
'''
compiler = base["compiler"]
with tempfile.TemporaryDirectory(prefix="lilla-backup-") as directory:
    source = Path(directory) / "backup.cpp"
    executable = Path(directory) / "backup.exe"
    source.write_text(base["prefix"] + mock + base["runtime"] + fixture + methods + tests, encoding="utf-8", newline="\r\n")
    subprocess.run([compiler, "-std=c++20", "-Wall", "-Wextra", "-Werror", str(source), "-o", str(executable)], check=True)
    environment = os.environ.copy()
    environment["PATH"] = str(Path(compiler).parent) + os.pathsep + environment.get("PATH", "")
    subprocess.run([str(executable)], env=environment, check=True)
