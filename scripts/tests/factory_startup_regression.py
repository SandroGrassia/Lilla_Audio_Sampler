"""Verify the production factory setup and a cold metadata reload using simulated FRAM."""
from pathlib import Path
import os
import re
import runpy
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
runtime = runpy.run_path(str(Path(__file__).with_name('fram_runtime_regression.py')))
base = runtime['base']
main = (ROOT / 'src/main.cpp').read_text(encoding='utf-8')

def method(signature):
    return re.search(r'^' + re.escape(signature) + r'\n\{.*?^\}', main, re.M | re.S).group(0) + '\n'

fixture = runtime['fixture']
prefix = base['prefix'].replace('struct SerialStub {', 'struct SerialStub {\n    void println() {}\n    void println(int) {}')
production = runtime['methods'] + r'''
bool ArchivingManager::Copy_Patch_from_RAM_to_SD(const int) { return false; } // SD absent.
ArchivingManager Archive;
int Patch_id = 99, Sound_id = 99;
Delay_data_struct Delay_data{};
int erases = 0;
void Require_FRAM(byte result) { assert(result == 0); }
void VFS_Erase_all_packets()
{
    assert(Archive.Check_FRAM_archive() != 0); // Do not publish READY before packet cleanup.
    ++erases;
}
'''
for signature in ['void P_Delete_all_Patches_and_Sounds(void)', 'void DS_seed_all_Recordings(void)', 'void Factory_setup_FRAM(void)', 'uint8_t P_Get_first_Patch_id_existing(void)']:
    production += method(signature)

checks = r'''
int main()
{
    LillaFram.reset();
    for (int pass = 0; pass < 2; ++pass)
    {
        Factory_setup_FRAM();
        assert(Archive.Check_FRAM_archive() == 0);
        assert(erases == pass + 1);
        // Discard volatile objects, keeping only persistent FRAM, as on a power cycle.
        P_Delete_all_Patches_and_Sounds();
        Delay_data = {};
        FileNameRegistry::Clear();
        assert(Archive.Check_FRAM_archive() == 0);
        char filename[FileNameRegistry::FILENAME_BYTES];
        assert(std::strcmp(FileNameRegistry::Filename(0, filename), "0.raw") == 0);
        ArchivingManager::FRAM_Repair_report patches{};
        ArchivingManager::FRAM_Recording_repair_report recordings{};
        ArchivingManager::FRAM_System_repair_report system{};
        assert(Archive.Repair_System_in_FRAM(system) == 0 && !system.defaulted);
        assert(Archive.Repair_Recordings_in_FRAM(recordings) == 0 && recordings.cleared_recordings == 0);
        assert(Archive.Repair_Patch_Sound_in_FRAM(patches) == 0);
        assert(patches.cleared_patches == 0 && patches.cleared_sounds == 0 && patches.defaulted_sounds == 0);
        uint16_t failed_id = 0;
        bool failed_sound = false;
        assert(Archive.Load_Patch_Sound_from_FRAM(failed_id, failed_sound) == 0);
        assert(P_Get_first_Patch_id_existing() == 0);
        assert(Patch[0].used && Patch[0].instruments == 1);
        const auto &instrument = Patch[0].Instrument[0];
        assert(instrument.used && instrument.sound_id == 0 && instrument.root_key == 60);
        assert(instrument.from_note == 0 && instrument.to_note == 127 && !instrument.Filter.use);
        assert(Sound[0].used && Sound[0].file == 0 && Sound[0].A == 0 && Sound[0].B == 40000);
        assert(Sound[0].gain > 0 && Sound[0].sustain > 0);
        assert(Sound[0].B < 43996); // Built-in 0.raw bounds (also checked against its production declaration below).
        for (int id = 1; id < PATCHES_MAX; ++id)
        {
            assert(!Patch[id].used);
        }
        for (int id = 1; id < SOUNDS_MAX; ++id)
        {
            assert(!Sound[id].used);
        }
        for (int id = 0; id < RECORDINGS; ++id)
        {
            assert(Archive.Read_DS_Recording(id) == 0);
            assert(Recording[id].consistent && Recording[id].packets == 0);
        }
        int8_t octave = 0;
        uint8_t step = 255;
        assert(Archive.Read_first_octave(octave) == 0 && octave == -2);
        assert(Archive.Read_key_step(step) == 0 && step == 0);
        assert(Archive.Read_Delay(0, Delay_data) == 0);
        assert(Delay_data.samples == 24 && Delay_data.loop_gain == 65 && Delay_data.instrument_route == 0 && Delay_data.lpf_cutoff == LPF_MAX);
    }
    std::puts("PASS: factory setup, repeat reset, cold metadata reload, CRCs, Patch 0/Sound 0, default audio bounds, empty recordings and system/delay defaults without SD");
}
'''
assert 'zeroraw[43996]' in (ROOT / 'include/ZeroRaw.h').read_text()
compiler = runtime['compiler']
with tempfile.TemporaryDirectory(prefix='lilla-factory-startup-') as directory:
    cpp = Path(directory) / 'factory.cpp'
    exe = Path(directory) / 'factory.exe'
    cpp.write_text(prefix + runtime['mock'] + base['runtime'] + fixture + production + checks, encoding='utf-8')
    subprocess.run([compiler, '-include', str(Path(__file__).with_name('sd_sound_source_stub.h').resolve()), '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    environment = os.environ.copy()
    environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
    subprocess.run([str(exe)], check=True, env=environment)
