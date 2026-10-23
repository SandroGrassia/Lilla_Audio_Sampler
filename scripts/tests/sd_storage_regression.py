"""Verify SD directory migration and FAT/exFAT Hidden/System metadata updates."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = (root / "lib/SdStorage/SdStorage.cpp").read_text()
source = re.sub(r'^#include[^\n]*', '', source, flags=re.M)
fixture = r'''

#include <cassert>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#define FLASHMEM
constexpr int BUILTIN_SDCARD = 1, O_RDONLY = 0, FAT_TYPE_FAT16 = 16, FAT_TYPE_EXFAT = 64;
struct fspos_t
{
    uint64_t position;
    uint32_t cluster;
};
int format = 32, index_value = 15, writes = 0, mounts = 0;
bool mounted = true, fail_write = false, fail_rename = false, fail_mount = false, hidden = false, legacy = false;
uint8_t root_data[2048];
std::map<uint32_t, std::vector<uint8_t>> disk;
std::map<std::string, std::string> files;
uint32_t sector_for(size_t offset)
{
    return format == 16 ? 50 + offset / 512 : 100 + (offset / 1024) * 8 + (offset / 512) % 2;
}
struct Card
{
    bool readSector(uint32_t sector, uint8_t *out)
    {
        if (!disk.count(sector))
        {
            return false;
        }
        memcpy(out, disk[sector].data(), 512);
        return true;
    }
    bool writeSector(uint32_t sector, const uint8_t *in)
    {
        ++writes;
        if (fail_write)
        {
            return false;
        }
        disk[sector].assign(in, in + 512);
        return true;
    }
    bool syncDevice()
    {
        return true;
    }
} card;
struct FsFile
{
    bool valid = false, root = false;
    size_t pos = 0;
    operator bool() const
    {
        return valid;
    }
    bool isDir() const
    {
        return valid;
    }
    uint32_t dirIndex() const
    {
        return index_value;
    }
    void close()
    {
        valid = false;
    }
    bool seekSet(size_t offset)
    {
        pos = offset;
        return pos < sizeof(root_data);
    }
    int read(void *out, size_t count)
    {
        if (pos + count > sizeof(root_data))
        {
            return -1;
        }
        memcpy(out, root_data + pos, count);
        pos += count;
        return count;
    }
    void fgetpos(fspos_t *out)
    {
        *out = {pos, static_cast<uint32_t>(2 + ((pos - 1) / 1024) * 4)};
    }
};
struct Volume
{
    void end()
    {
        mounted = false;
    }
} volume;
struct Sdfs
{
    FsFile open(const char *path, int)
    {
        return {std::string(path) == "/" || hidden, std::string(path) == "/"};
    }
    int fatType()
    {
        return format;
    }
    uint32_t sectorsPerCluster()
    {
        return 2;
    }
    uint32_t dataStartSector()
    {
        return 100;
    }
    Card *card()
    {
        return &::card;
    }
    Volume *vol()
    {
        return &volume;
    }
};
struct FatVolume
{
    bool begin(Card *, bool, int)
    {
        return true;
    }
    uint32_t rootDirStart()
    {
        return 50;
    }
};
struct Sd
{
    Sdfs sdfs;
    bool begin(int)
    {
        ++mounts;
        mounted = !fail_mount;
        for (size_t i = 0; i < sizeof(root_data); i += 512)
        {
            if (disk.count(sector_for(i)))
            {
                memcpy(root_data + i, disk[sector_for(i)].data(), 512);
            }
        }
        return mounted;
    }
    bool exists(const char *path)
    {
        return std::string(path) == "/LILLA_PRIVATE" ? hidden : (std::string(path) == "/LILLA_HIDDEN" ? legacy : files.count(path) != 0);
    }
    bool rename(const char *from, const char *to)
    {
        if (fail_rename)
        {
            return false;
        }
        if (std::string(from) == "/LILLA_HIDDEN")
        {
            hidden = true;
            legacy = false;
        }
        else
        {
            std::vector<std::pair<std::string, std::string>> moved;
            for (const auto &[path, data] : files)
            {
                if (path == from || path.starts_with(std::string(from) + "/"))
                {
                    moved.emplace_back(path, data);
                }
            }
            for (const auto &[path, data] : moved)
            {
                files[std::string(to) + path.substr(strlen(from))] = data;
                files.erase(path);
            }
        }
        return true;
    }
    bool mkdir(const char *)
    {
        hidden = true;
        return true;
    }
} SD;
class SdStorage
{
  public:
    static bool Begin();
    static bool Ensure();
    static bool Hide();
    static bool Hide_entry(uint8_t *, size_t, bool);
};
void checksum(uint8_t *entry, size_t bytes)
{
    uint16_t sum = 0;
    for (size_t i = 0; i < bytes; ++i)
    {
        if (i != 2 && i != 3)
        {
            sum = static_cast<uint16_t>((sum >> 1) | ((sum & 1) << 15));
            sum += entry[i];
        }
    }
    entry[2] = sum & 255;
    entry[3] = sum >> 8;
}
void reset(int fs, int index)
{
    format = fs;
    index_value = index;
    writes = mounts = 0;
    mounted = true;
    hidden = true;
    legacy = false;
    fail_write = fail_rename = fail_mount = false;
    memset(root_data, 0, sizeof(root_data));
    disk.clear();
    files.clear();
    auto *entry = root_data + index * 32;
    if (fs == 64)
    {
        entry[0] = 0x85;
        entry[1] = 2;
        entry[4] = 0x10;
        entry[32] = 0xC0;
        entry[64] = 0xC1;
        checksum(entry, 96);
    }
    else
    {
        entry[0] = 'L';
        entry[11] = 0x30;
    }
    for (size_t i = 0; i < sizeof(root_data); i += 512)
    {
        disk[sector_for(i)] = {root_data + i, root_data + i + 512};
    }
}
'''
checks = r'''

int main()
{
    for (int fs : {16, 32, 64})
    {
        for (int index : {0, 15, 31, 32})
        {
            reset(fs, index);
            const auto before = disk;
            assert(SdStorage::Hide());
            assert(mounted && writes == 1);
            const auto *entry = root_data + index * 32;
            assert((entry[fs == 64 ? 4 : 11] & 6) == 6);
            for (const auto &[sector, bytes] : disk)
            {
                for (size_t i = 0; i < 512; ++i)
                {
                    const size_t base = (index * 32) % 512;
                    const bool changed = sector == sector_for(index * 32) && (i == base + (fs == 64 ? 4 : 11) || (fs == 64 && (i == base + 2 || i == base + 3)));
                    if (!changed)
                    {
                        assert(bytes[i] == before.at(sector)[i]);
                    }
                }
            }
            assert(SdStorage::Hide());
            assert(writes == 1);
        }
    }
    reset(64, 15);
    root_data[15 * 32 + 2] ^= 1;
    assert(!SdStorage::Hide() && writes == 0);
    reset(32, 15);
    disk[sector_for(15 * 32)][(15 * 32) % 512] ^= 1;
    assert(!SdStorage::Hide() && writes == 0);
    reset(32, 15);
    fail_write = true;
    assert(!SdStorage::Hide() && mounted);
    reset(32, 15);
    hidden = false;
    legacy = true;
    assert(SdStorage::Begin() && hidden && !legacy);
    reset(32, 15);
    legacy = true;
    assert(SdStorage::Begin() && hidden && legacy);
    reset(32, 15);
    hidden = false;
    legacy = true;
    fail_rename = true;
    assert(!SdStorage::Begin() && !hidden && legacy && writes == 0);
    reset(32, 15);
    hidden = false;
    assert(SdStorage::Begin() && !hidden && writes == 0);
    assert(SdStorage::Ensure() && hidden && writes == 1);
    reset(32, 15);
    files = {{"/LILLA_PRIVATE/AUDIO_FILES", "directory"}, {"/LILLA_PRIVATE/AUDIO_FILES/Piano.raw", "pcm"}, {"/LILLA_PRIVATE/AUDIO_FILES/.lilla-index", "index"}, {"/LILLA_PRIVATE/AUDIO_FILES/.lilla-sources", "references"}, {"/FLASH_AUDIO_FILES", "upload"}};
    assert(SdStorage::Begin());
    assert(files["/AUDIO_FILES/Piano.raw"] == "pcm" && files["/LILLA_PRIVATE/.lilla-index"] == "index" && files["/LILLA_PRIVATE/.lilla-sources"] == "references" && files["/UPLOAD_AUDIO_FILES"] == "upload");
    assert(!files.count("/AUDIO_FILES/.lilla-index") && !files.count("/AUDIO_FILES/.lilla-sources"));
    reset(32, 15);
    files = {{"/AUDIO_FILES", "new directory"}, {"/SD_AUDIO_FILES", "legacy directory"}, {"/UPLOAD_AUDIO_FILES", "new upload"}, {"/FLASH_AUDIO_FILES", "old upload"}};
    assert(SdStorage::Begin());
    assert(files["/AUDIO_FILES"] == "new directory" && files["/SD_AUDIO_FILES"] == "legacy directory" && files["/UPLOAD_AUDIO_FILES"] == "new upload" && files["/FLASH_AUDIO_FILES"] == "old upload");
}
'''
compiler = shutil.which("g++") or r"C:\msys64\ucrt64\bin\g++.exe"
with tempfile.TemporaryDirectory(prefix="lilla-sd-storage-") as directory:
    cpp=Path(directory)/"storage.cpp"
    exe=Path(directory)/"storage.exe"
    cpp.write_text(fixture+source+checks)
    subprocess.run([compiler,"-std=c++20","-Wall","-Wextra",str(cpp),"-o",str(exe)],check=True)
    environment=os.environ.copy()
    environment["PATH"]=str(Path(compiler).parent)+os.pathsep+environment.get("PATH","")
    subprocess.run([str(exe)],check=True,env=environment)
print("PASS: FAT16/FAT32/exFAT attributes, fragmented root and sector boundaries, checksum, unchanged metadata, idempotence, I/O failure and safe migration")
