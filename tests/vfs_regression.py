"""Run the production VFS compactor/cleanup with Flash and FRAM fault injection."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
main = (ROOT / 'src/main.cpp').read_text(encoding='utf-8')
shared = (ROOT / 'lib/SharedVFS/SharedVFS.h').read_text(encoding='utf-8')
recording = shared[shared.index('struct VFS_Recording'):]
production = main[main.index('// VFS operations run without audio callbacks'):main.index('void VFS_Print_FAT(void)\n{')]

prefix = r'''
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>
#define FLASHMEM
#define F(x) x
#define NVIC_IS_ENABLED(x) audio_enabled
#define IRQ_SOFTWARE 0
using byte = uint8_t;
constexpr int RECORDINGS = 30, PACKET_DIM = 65536, VFS_PACKETS_MAX = 512, VFS_PACKETS_DS = 512;
int DS_First_packet = 0, DS_VFS_packets = 24, DS_Last_packet = 23;
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
void AudioNoInterrupts() { audio_enabled = false; }
void AudioInterrupts() { audio_enabled = true; }
uint32_t clock_ms = 0;
uint32_t millis() { return clock_ms; }
void delay(uint32_t ms) { clock_ms += ms; }
struct Stub
{
    template<class T> void print(T) {}
    template<class T> void println(T) {}
    void println() {}
    void Stop() {}
} Serial, Trigger, Midi_reader;
namespace LillaFRAM_2x512 { constexpr byte ERROR_0 = 0; }
struct Meta
{
    uint16_t first_packet = 0, packets = 0;
    uint8_t stereo = 0, consistent = 1;
};
std::array<Meta, RECORDINGS> fram;
int mutation = 0, cut_after = -1, erases = 0, writes = 0, saves = 0, fram_reads = 0;
uint32_t written_bytes = 0;
bool stuck = false, corrupt_write = false, fail_erase = false, short_read = false, short_write = false;
int fail_save = -1, fail_fram_read = -1, missing = -1, wrong_size = -1;
void changed()
{
    if (++mutation == cut_after)
    {
        throw std::runtime_error("power loss");
    }
}
void check_unowned(int packet)
{
    for (const auto &entry : fram)
    {
        assert(!entry.consistent || entry.packets == 0 || packet < entry.first_packet || packet >= entry.first_packet + entry.packets * (entry.stereo ? 2 : 1));
    }
}
std::vector<std::vector<uint8_t>> flash;
struct SerialFlashFile
{
    int id = -1;
    uint32_t offset = 0;
    explicit operator bool() const { return id >= 0; }
    uint32_t size() const { return id == wrong_size ? 256 : PACKET_DIM; }
    uint32_t getFlashAddress() const { return (id + 1) * PACKET_DIM; }
    void seek(uint32_t value) { offset = value; }
    void close() {}
    uint32_t read(void *buffer, uint32_t size)
    {
        if (short_read)
        {
            return 0;
        }
        assert(id >= 0 && offset + size <= PACKET_DIM);
        memcpy(buffer, flash[id].data() + offset, size);
        offset += size;
        return size;
    }
    uint32_t write(const void *buffer, uint32_t size)
    {
        assert(id >= 0 && offset + size <= PACKET_DIM);
        check_unowned(id);
        if (short_write)
        {
            return 0;
        }
        memcpy(flash[id].data() + offset, buffer, size);
        if (corrupt_write)
        {
            flash[id][offset] ^= 1;
        }
        offset += size;
        written_bytes += size;
        ++writes;
        changed();
        return size;
    }
};
struct FlashStub
{
    bool ready() const { return !stuck; }
    uint32_t blockSize() const { return PACKET_DIM; }
    bool exists(const char *name) const { return atoi(name + 1) != missing; }
    SerialFlashFile open(const char *name)
    {
        const int id = atoi(name + 1);
        return id == missing ? SerialFlashFile{} : SerialFlashFile{id};
    }
    void eraseBlock(uint32_t address)
    {
        const int id = address / PACKET_DIM - 1;
        check_unowned(id);
        if (!fail_erase)
        {
            std::fill(flash[id].begin(), flash[id].end(), 255);
        }
        ++erases;
        changed();
    }
} SerialFlash;
'''

archive = r'''
VFS_Recording Recording[RECORDINGS];
struct ArchivingManager
{
    using FRAM_Recording_struct = Meta;
    byte Save_DS_Recording(int id)
    {
        if (++saves == fail_save)
        {
            return 1;
        }
        const auto &r = Recording[id];
        fram[id] = {uint16_t(r.first_packet), uint16_t(r.packets), uint8_t(r.stereo), uint8_t(r.consistent)};
        changed();
        return 0;
    }
    byte Read_DS_Recording(int id)
    {
        if (++fram_reads == fail_fram_read)
        {
            return 1;
        }
        const auto &entry = fram[id];
        Recording[id].first_packet = entry.first_packet;
        Recording[id].packets = entry.packets;
        Recording[id].stereo = entry.stereo;
        Recording[id].consistent = entry.consistent;
        return 0;
    }
} Archive;
void VFS_Reset_FAT_table(void);
bool VFS_Erase_packet(int);
bool VFS_Clean_up_orphan_packets(void);
bool VFS_Shift_file(int, int);
'''

tests = r'''
void reset()
{
    flash.assign(24, std::vector<uint8_t>(PACKET_DIM, 255));
    for (int i = 0; i < VFS_PACKETS_MAX; ++i)
    {
        snprintf(name_packet[i], sizeof(name_packet[i]), "P%d.raw", i);
    }
    for (int id = 0; id < RECORDINGS; ++id)
    {
        Recording[id] = {};
        Recording[id].consistent = true;
        fram[id] = {};
    }
    mutation = erases = writes = saves = fram_reads = 0;
    written_bytes = clock_ms = 0;
    cut_after = fail_save = fail_fram_read = missing = wrong_size = -1;
    stuck = corrupt_write = fail_erase = short_read = short_write = false;
    audio_enabled = true;
    DS_First_packet = 0;
    DS_VFS_packets = 24;
}
void add(int id, int first, int packets, bool stereo)
{
    Recording[id] = {first, packets, 0, 0.0f, stereo, true}; // Bytes intentionally unset: do not trust runtime tail estimates.
    fram[id] = {uint16_t(first), uint16_t(packets), uint8_t(stereo), 1};
    const int channels = stereo ? 2 : 1;
    for (int i = 0; i < packets * channels; ++i)
    {
        const int used = i / channels == packets - 1 ? 258 + 42 * (i % channels) : PACKET_DIM;
        for (int byte = 0; byte < used; ++byte)
        {
            flash[first + i][byte] = (id * 31 + i * 3 + byte) % 254;
        }
    }
}
auto contents(int id)
{
    const auto &r = Recording[id];
    return std::vector<std::vector<uint8_t>>(flash.begin() + r.first_packet, flash.begin() + r.first_packet + r.packets * (r.stereo ? 2 : 1));
}
void reboot()
{
    for (int id = 0; id < RECORDINGS; ++id)
    {
        const auto &r = fram[id];
        Recording[id] = {r.first_packet, r.packets, 0, 0.0f, r.stereo != 0, r.consistent != 0};
    }
    cut_after = -1;
    stuck = corrupt_write = fail_erase = short_read = short_write = false;
    fail_save = fail_fram_read = missing = wrong_size = -1;
}
bool blank(int packet)
{
    return std::all_of(flash[packet].begin(), flash[packet].end(), [](uint8_t value) { return value == 255; });
}
int main()
{
    reset();
    add(0, 0, 1, false);
    add(1, 3, 4, false);
    add(2, 9, 2, true);
    const auto a = contents(0), b = contents(1), c = contents(2);
    assert(VFS_Defragment());
    assert(contents(0) == a && contents(1) == b && contents(2) == c);
    assert(Recording[1].first_packet == 1 && Recording[2].first_packet == 5);
    assert(fram[1].first_packet == 1 && fram[2].first_packet == 5 && fram[1].consistent && fram[2].consistent);
    assert(erases == 14 && saves == 4 && audio_enabled);
    assert(written_bytes == 5 * PACKET_DIM + 258 + 258 + 300);
    const int previous_erases = erases, previous_writes = writes, previous_saves = saves;
    assert(VFS_Defragment() && erases == previous_erases && writes == previous_writes && saves == previous_saves);
    for (int packet = 9; packet < 24; ++packet)
    {
        assert(blank(packet));
    }
    reset();
    add(0, 1, 10, false);
    const auto ten = contents(0);
    assert(VFS_Defragment() && erases == 11 && contents(0) == ten);
    // Missing files / incorrect geometry / FRAM failure must not start any Flash mutation.
    for (int fault = 0; fault < 4; ++fault)
    {
        reset();
        add(0, 2, 2, false);
        missing = fault == 0 ? 2 : -1;
        wrong_size = fault == 1 ? 0 : -1;
        fail_save = fault == 2 ? 1 : -1;
        fail_fram_read = fault == 3 ? 1 : -1;
        assert(!VFS_Defragment() && erases == 0 && writes == 0);
    }
    // Silent program/erase failure and short I/O must never publish consistent=true.
    for (int fault = 0; fault < 4; ++fault)
    {
        reset();
        add(0, 2, 3, false);
        flash[0][0] = 0; // Ensure a failed destination erase is detectable.
        corrupt_write = fault == 0;
        fail_erase = fault == 1;
        short_read = fault == 2;
        short_write = fault == 3;
        assert(!VFS_Defragment() && !fram[0].consistent && !Recording[0].consistent);
        reboot();
        assert(VFS_Clean_up_VFS() && fram[0].consistent && fram[0].packets == 0);
        for (int packet = 0; packet < 24; ++packet)
        {
            assert(blank(packet));
        }
    }
    reset();
    add(0, 2, 2, false);
    fail_save = 2; // Completed Flash move, failed final metadata commit.
    assert(!VFS_Defragment() && !fram[0].consistent && fram[0].first_packet == 2);
    reboot();
    assert(VFS_Clean_up_VFS() && fram[0].packets == 0);
    reset();
    add(0, 2, 1, false);
    stuck = true;
    assert(!VFS_Defragment() && erases == 0 && clock_ms == VFS_FLASH_TIMEOUT_MS && !audio_enabled);
    // Simulate power loss at metadata, copy, overlapping erase, tail erase and final commit boundaries.
    reset();
    add(0, 1, 3, false);
    assert(VFS_Defragment());
    const int operations = mutation;
    for (int cut : {1, 2, 3, 130, 259, 260, operations - 2, operations - 1, operations})
    {
        reset();
        add(0, 1, 3, false);
        add(1, 10, 1, false);
        const auto expected = contents(0), survivor = contents(1);
        cut_after = cut;
        try
        {
            VFS_Defragment();
            assert(false);
        }
        catch (const std::runtime_error &) {}
        reboot();
        assert(VFS_Clean_up_VFS());
        assert(contents(1) == survivor);
        if (fram[0].packets)
        {
            assert(contents(0) == expected);
        }
        assert(VFS_Compile_FAT_table());
        for (int packet = 0; packet < 24; ++packet)
        {
            assert(VFS_FAT_table[packet] >= 0 || blank(packet));
        }
    }
    // Power loss DURING orphan cleanup: marker is kept until every free packet is blank.
    reset();
    add(0, 6, 1, false);
    const auto survivor = contents(0);
    fram[1].consistent = 0; // Empty durable marker produced by CRC repair.
    Recording[1].consistent = false;
    flash[1][0] = flash[20][0] = 0;
    cut_after = 2; // Persist marker then erase the first orphan.
    try
    {
        VFS_Clean_up_VFS();
        assert(false);
    }
    catch (const std::runtime_error &) {}
    assert(!fram[1].consistent && !blank(20));
    reboot();
    assert(VFS_Clean_up_VFS() && blank(1) && blank(20) && fram[1].consistent && contents(0) == survivor);
    // Bad spans and overlaps are rejected without writes.
    reset();
    add(0, 2, 2, false);
    add(1, 3, 1, false);
    assert(!VFS_Defragment() && mutation == 0);
    reset();
    Recording[0].first_packet = 23;
    Recording[0].packets = 2;
    assert(!VFS_Defragment() && mutation == 0);
    std::cout << "PASS: linear VFS compaction, 11 vs 20 erases, mono/stereo tails, FRAM ordering, Flash verification/timeouts, power-loss cleanup and survivor preservation\n";
}
'''

compiler = shutil.which('g++') or r'C:\msys64\ucrt64\bin\g++.exe'
with tempfile.TemporaryDirectory(prefix='lilla-vfs-') as directory:
    cpp = Path(directory) / 'vfs.cpp'
    exe = Path(directory) / ('vfs.exe' if os.name == 'nt' else 'vfs')
    cpp.write_text(prefix + recording + archive + production + tests, encoding='utf-8', newline='\r\n')
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    environment = os.environ.copy()
    environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
    subprocess.run([str(exe)], check=True, env=environment)
