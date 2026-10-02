"""Exercise numbered backups and complete restores using production FRAM, VFS and SD code."""
from pathlib import Path
import os
import re
import runpy
import subprocess
import tempfile

backup = runpy.run_path(str(Path(__file__).with_name('fram_backup_regression.py')))
base = backup['base']
main = (base['ROOT'] / 'src/main.cpp').read_text(encoding='utf-8')
prefix = re.sub(r'struct SerialStub \{.*?\} Serial;', 'struct SerialStub { template<class T> void print(T) {} template<class T> void println(T) {} void println() {} void Stop() {} } Serial, Trigger, Midi_reader;', base['prefix'], flags=re.S)
prefix = re.sub(r'struct FlashMock \{.*?\} SerialFlash;\n', '', prefix)

mock = r'''
#include <map>
#include <memory>
#include <string>
#include <stdexcept>
#define FLASHMEM
#define IRQ_SOFTWARE 0
#define NVIC_IS_ENABLED(x) audio_enabled
constexpr int PACKET_DIM = 65536, VFS_PACKETS_DS = 512, BUILTIN_SDCARD = 0;
constexpr int FILE_WRITE = 1, O_RDONLY = 0, O_WRONLY = 2, O_CREAT = 4, O_EXCL = 8;
int DS_First_packet = 0, DS_VFS_packets = 12, DS_Last_packet = 11, VFS_packets = 12;
int VFS_FAT_table[VFS_PACKETS_DS];
char name_packet[VFS_PACKETS_MAX][16];
#include <cstdio>
#include <cstdint>
constexpr int NAME_PACKET_SIZE = 10;
const char *Get_packet_name(uint16_t id, char (&name)[NAME_PACKET_SIZE])
{
    snprintf(name, sizeof(name), "%s", name_packet[id]);
    return name;
}

bool audio_enabled = true;
uint32_t clock_ms = 0;
void AudioNoInterrupts() { audio_enabled = false; }
void AudioInterrupts() { audio_enabled = true; }
uint32_t millis() { return clock_ms; }
void delay(uint32_t ms) { clock_ms += ms; }
bool sd_missing = false, fail_sync = false, fail_close = false, fail_rename = false;
int sd_write_budget = -1, flash_write_budget = -1, flash_writes = 0, flash_erases = 0, physical_capacity = 12;
int cut_after = -1, flash_mutations = 0;
bool corrupt_flash_write = false, flash_stuck = false;
std::string read_failure_path;
int read_budget = -1;
struct Node { bool directory = false; std::vector<uint8_t> bytes; };
std::map<std::string, std::shared_ptr<Node>> nodes;
struct File
{
    std::shared_ptr<Node> node;
    std::string path;
    size_t position = 0, child = 0;
    bool error = false;
    explicit operator bool() const { return bool(node); }
    bool isDirectory() const { return node && node->directory; }
    const char *name() const { return path.c_str(); }
    size_t size() const { return node ? node->bytes.size() : 0; }
    bool seek(size_t offset) { position = offset; return node && offset <= size(); }
    size_t read(uint8_t *data, size_t count)
    {
        if (!node || position > size())
        {
            return 0;
        }
        count = std::min(count, size() - position);
        if (path == read_failure_path && read_budget >= 0)
        {
            count = std::min(count, static_cast<size_t>(read_budget));
            read_budget -= count;
        }
        memcpy(data, node->bytes.data() + position, count);
        position += count;
        return count;
    }
    size_t write(const uint8_t *data, size_t count)
    {
        if (!node)
        {
            return 0;
        }
        if (sd_write_budget >= 0)
        {
            count = std::min(count, static_cast<size_t>(sd_write_budget));
            sd_write_budget -= count;
        }
        node->bytes.resize(std::max(size(), position + count));
        memcpy(node->bytes.data() + position, data, count);
        position += count;
        return count;
    }
    void close() { node.reset(); }
    File openNextFile()
    {
        size_t index = 0;
        const std::string start = path + "/";
        for (const auto &[name, value] : nodes)
        {
            if (name.rfind(start, 0) == 0 && name.find('/', start.size()) == std::string::npos)
            {
                if (index++ == child)
                {
                    ++child;
                    return {value, name};
                }
            }
        }
        return {};
    }
};
struct FsFile : File
{
    FsFile() = default;
    explicit FsFile(File file) : File(file) {}
    int read(uint8_t *data, size_t count) { return static_cast<int>(File::read(data, count)); }
    size_t fileSize() const { return size(); }
    bool getError() const { return error; }
    bool sync() const { return !fail_sync; }
    bool close() { File::close(); return !fail_close; }
};
File open_file(const char *path, bool write, bool exclusive = false)
{
    const bool exists = nodes.count(path);
    if ((exists && exclusive) || (!exists && !write))
    {
        return {};
    }
    if (!exists)
    {
        nodes[path] = std::make_shared<Node>();
    }
    return {nodes[path], path, write ? nodes[path]->bytes.size() : 0};
}
struct SDStub
{
    struct Backend
    {
        FsFile open(const char *path, int mode) { return FsFile(open_file(path, mode & O_WRONLY, mode & O_EXCL)); }
    } sdfs;
    bool begin(int) const { return !sd_missing; }
    bool exists(const char *path) const { return nodes.count(path); }
    bool mkdir(const char *path)
    {
        if (exists(path))
        {
            return false;
        }
        nodes[path] = std::make_shared<Node>(Node{true, {}});
        return true;
    }
    File open(const char *path, int mode = 0) { return open_file(path, mode == FILE_WRITE); }
    bool rename(const char *from, const char *to)
    {
        if (fail_rename || !exists(from) || exists(to))
        {
            return false;
        }
        const std::string prefix = std::string(from) + "/";
        std::map<std::string, std::shared_ptr<Node>> moved;
        for (auto it = nodes.begin(); it != nodes.end();)
        {
            if (it->first == from || it->first.rfind(prefix, 0) == 0)
            {
                moved[std::string(to) + it->first.substr(strlen(from))] = it->second;
                it = nodes.erase(it);
            }
            else
            {
                ++it;
            }
        }
        nodes.insert(moved.begin(), moved.end());
        return true;
    }
} SD;
std::vector<std::vector<uint8_t>> flash(12, std::vector<uint8_t>(PACKET_DIM, 255));
void flash_changed()
{
    uint32_t state = 0;
    memcpy(&state, LillaFram.memory.data() + 8, sizeof(state));
    assert(state == 0x52535452); // Must be durably in-progress before ANY Flash mutation.
    if (++flash_mutations == cut_after)
    {
        throw std::runtime_error("power loss");
    }
}
struct SerialFlashFile
{
    int id = -1;
    size_t offset = 0;
    explicit operator bool() const { return id >= 0; }
    uint32_t size() const { return PACKET_DIM; }
    uint32_t getFlashAddress() const { return (id + 1) * PACKET_DIM; }
    void close() {}
    void seek(uint32_t value) { offset = value; }
    uint32_t read(void *data, uint32_t count)
    {
        assert(id >= 0 && offset + count <= PACKET_DIM);
        memcpy(data, flash[id].data() + offset, count);
        offset += count;
        return count;
    }
    uint32_t write(const void *data, uint32_t count)
    {
        if (flash_write_budget == 0)
        {
            return 0;
        }
        assert(id >= 0 && offset + count <= PACKET_DIM);
        if (flash_write_budget > 0)
        {
            --flash_write_budget;
        }
        memcpy(flash[id].data() + offset, data, count);
        if (corrupt_flash_write)
        {
            flash[id][offset] ^= 1;
        }
        offset += count;
        ++flash_writes;
        flash_changed();
        return count;
    }
};
struct FlashStub
{
    bool ready() const { return !flash_stuck; }
    uint32_t blockSize() const { return PACKET_DIM; }
    bool exists(const char *name) const { return atoi(name + 1) < physical_capacity; }
    SerialFlashFile open(const char *name) { return exists(name) ? SerialFlashFile{atoi(name + 1)} : SerialFlashFile{}; }
    void eraseBlock(uint32_t address)
    {
        const int id = address / PACKET_DIM - 1;
        std::fill(flash[id].begin(), flash[id].end(), 255);
        ++flash_erases;
        flash_changed();
    }
} SerialFlash;
void VFS_Reset_FAT_table(void);
bool VFS_Erase_packet(int);
bool VFS_Shift_file(int, int);
bool VFS_Clean_up_orphan_packets(void);
'''

production = 'ArchivingManager Archive;\nbool BACKUP_Restore(bool *config_error = nullptr);\n'
production += main[main.index('// VFS operations run without audio callbacks'):main.index('void VFS_Print_FAT(void)\n{')]
production += main[main.index('// Complete SD backups:'):main.index('// End complete SD backups.')]

tests = r'''
using A = ArchivingManager;
void seed()
{
    LillaFram.reset();
    assert(Archive.Factory_reset_FRAM() == 0);
    for (int id = 0; id < RECORDINGS; ++id)
    {
        assert(Archive.Read_DS_Recording(id) == 0);
    }
    Recording[3] = {5, 2, 0, 0, true, true};
    Recording[29] = {1, 1, 0, 0, false, true};
    assert(Archive.Save_DS_Recording(3) == 0 && Archive.Save_DS_Recording(29) == 0);
    for (int packet = 0; packet < 12; ++packet)
    {
        for (int i = 0; i < PACKET_DIM; ++i)
        {
            flash[packet][i] = uint8_t((i + packet * 19) % 256);
        }
    }
    for (int packet = 0; packet < VFS_PACKETS_MAX; ++packet)
    {
        snprintf(name_packet[packet], sizeof(name_packet[packet]), "P%d.raw", packet);
    }
}
void select_backup(const std::string &folder)
{
    std::map<std::string, std::shared_ptr<Node>> selected;
    for (const auto &[name, node] : nodes)
    {
        if (name.rfind(folder + "/", 0) == 0)
        {
            selected[std::string(BACKUP_ROOT) + name.substr(folder.size())] = std::make_shared<Node>(*node);
        }
    }
    for (const auto &[name, node] : selected)
    {
        nodes[name] = node;
    }
}
void clear_faults()
{
    LillaFram.clear_io();
    sd_missing = fail_sync = fail_close = fail_rename = corrupt_flash_write = flash_stuck = false;
    sd_write_budget = flash_write_budget = cut_after = read_budget = -1;
    read_failure_path.clear();
    flash_mutations = flash_writes = flash_erases = 0;
}
void rejected_without_changes()
{
    LillaFram.clear_io();
    const auto before_fram = LillaFram.memory;
    const auto before_flash = flash;
    assert(!BACKUP_Restore());
    assert(LillaFram.writes.empty() && LillaFram.memory == before_fram && flash == before_flash);
}
void restored_without_stereo(const std::vector<uint8_t> &mono)
{
    assert(BACKUP_Restore() && Archive.Check_FRAM_archive() == 0);
    assert(Archive.Read_DS_Recording(3) == 0);
    assert(Recording[3].first_packet == 0 && Recording[3].packets == 0 && Recording[3].bytes == 0 && Recording[3].seconds == 0 && !Recording[3].stereo && Recording[3].consistent);
    assert(Archive.Read_DS_Recording(29) == 0 && Recording[29].first_packet == 0 && Recording[29].packets == 1);
    assert(flash[0] == mono);
    assert(std::all_of(flash[1].begin(), flash[1].end(), [](uint8_t value) { return value == 0xFF; }));
}
int main()
{
    seed();
    const auto original = flash;
    const auto config = LillaFram.memory;
    assert(BACKUP_Export());
    const std::string first = "/LILLABACKUP/000001", second = "/LILLABACKUP/000002";
    assert(SD.exists((first + "/LILLA_CONFIG.fram").c_str()));
    assert(SD.exists((first + "/REC_03_L.raw").c_str()) && SD.exists((first + "/REC_03_R.raw").c_str()));
    assert(SD.exists((first + "/REC_29_L.raw").c_str()) && !SD.exists((first + "/REC_29_R.raw").c_str()));
    assert(!SD.exists("/LILLABACKUP/000001.tmp") && flash == original && LillaFram.memory == config);
    const auto first_config = nodes.at(first + "/LILLA_CONFIG.fram")->bytes;
    assert(BACKUP_Export());
    assert(SD.exists(second.c_str()) && nodes.at(first + "/LILLA_CONFIG.fram")->bytes == first_config);
    // Subdirectories are not imported automatically; the user must select files in the root.
    rejected_without_changes();
    sd_write_budget = 300;
    assert(!BACKUP_Export() && !SD.exists("/LILLABACKUP/000003"));
    assert(SD.exists("/LILLABACKUP/000003.tmp") && nodes.at(first + "/LILLA_CONFIG.fram")->bytes == first_config);
    clear_faults();
    fail_sync = true;
    assert(!BACKUP_Export() && !SD.exists("/LILLABACKUP/000004"));
    clear_faults();
    fail_rename = true;
    assert(!BACKUP_Export() && !SD.exists("/LILLABACKUP/000005"));
    clear_faults();
    assert(BACKUP_Export() && SD.exists("/LILLABACKUP/000006")); // Never reuse incomplete numbers.
    select_backup(first);
    const std::string root_config = "/LILLABACKUP/LILLA_CONFIG.fram", right = "/LILLABACKUP/REC_03_R.raw";
    auto right_file = nodes.at(right);
    nodes.erase(right);
    restored_without_stereo(original[1]);
    nodes[right] = right_file;
    right_file->bytes[9] ^= 1;
    restored_without_stereo(original[1]);
    right_file->bytes[9] ^= 1;
    right_file->bytes.pop_back();
    restored_without_stereo(original[1]);
    // Capacity counts only surviving Recordings; a bad left channel also discards the right.
    select_backup(first);
    nodes.erase("/LILLABACKUP/REC_03_L.raw");
    physical_capacity = 2;
    restored_without_stereo(original[1]);
    physical_capacity = 12;
    select_backup(first);
    nodes[root_config]->bytes[30] ^= 1;
    rejected_without_changes();
    bool config_error = false;
    assert(!BACKUP_Restore(&config_error) && config_error);
    nodes.erase(root_config);
    config_error = false;
    assert(!BACKUP_Restore(&config_error) && config_error);
    sd_missing = true;
    config_error = false;
    assert(!BACKUP_Restore(&config_error) && config_error);
    clear_faults();
    select_backup(first);
    physical_capacity = 4; // Five packets are required.
    rejected_without_changes();
    config_error = true;
    assert(!BACKUP_Restore(&config_error) && !config_error);
    physical_capacity = 5; // Original addresses reach packet 8; restoration must relocate them.
    for (auto &packet : flash)
    {
        std::fill(packet.begin(), packet.end(), 0x33);
    }
    assert(BACKUP_Restore() && Archive.Check_FRAM_archive() == 0);
    assert(Recording[3].first_packet == 0 && Recording[29].first_packet == 4);
    assert(flash[0] == original[5] && flash[1] == original[6] && flash[2] == original[7] && flash[3] == original[8] && flash[4] == original[1]);
    assert(std::equal(config.begin() + 256, config.begin() + 0xFB00, LillaFram.memory.begin() + 256));
    // Fail after prevalidation while reading audio: READY must never be published.
    clear_faults();
    read_failure_path = "/LILLABACKUP/REC_03_L.raw";
    read_budget = 2 * PACKET_DIM + 256; // One validation pass, then one restored page.
    assert(!BACKUP_Restore() && Archive.Check_FRAM_archive() != 0 && !audio_enabled);
    clear_faults();
    assert(BACKUP_Restore() && Archive.Check_FRAM_archive() == 0);
    corrupt_flash_write = true;
    assert(!BACKUP_Restore() && Archive.Check_FRAM_archive() != 0);
    clear_faults();
    assert(BACKUP_Restore());
    // A power cut during erase or audio programming remains recoverable from the untouched SD bundle.
    for (int cut : {1, 5, 6, 270})
    {
        clear_faults();
        cut_after = cut;
        try
        {
            BACKUP_Restore();
            assert(false);
        }
        catch (const std::runtime_error &) {}
        clear_faults();
        assert(Archive.Check_FRAM_archive() != 0);
        DS_VFS_packets = VFS_packets = 0; // Cold-start recovery has not initialized VFS runtime globals.
        assert(BACKUP_Restore() && Archive.Check_FRAM_archive() == 0 && DS_VFS_packets == 5);
        assert(flash[0] == original[5] && flash[4] == original[1]);
    }
    assert(nodes.at(first + "/LILLA_CONFIG.fram")->bytes == first_config);
    std::puts("PASS: numbered immutable backups, invalid audio discarded per Recording, root-only restore, survivor capacity checks, relocation, Flash faults and power-loss retry");
}
'''

compiler = base['compiler']
with tempfile.TemporaryDirectory(prefix='lilla-full-backup-') as directory:
    cpp = Path(directory) / 'full_backup.cpp'
    exe = Path(directory) / ('full_backup.exe' if os.name == 'nt' else 'full_backup')
    cpp.write_text(prefix + mock + base['runtime'] + backup['fixture'] + backup['methods'] + production + tests, encoding='utf-8', newline='\r\n')
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    environment = os.environ.copy()
    environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
    subprocess.run([str(exe)], check=True, env=environment)
