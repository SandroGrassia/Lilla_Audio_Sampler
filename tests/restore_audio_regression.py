"""Exercise production restore-menu recovery and final audio/control publication."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/main.cpp').read_text(encoding='utf-8')
menu = source.split('            case 4: // Restore configuration', 1)[1].split('            case 5:', 1)[0]
menu = 'case 4: //' + menu
reload = source.split('void Reload_system_state(void)\n{', 1)[1].split('\n}\n', 1)[0]
tail = reload[reload.index('    // ****************    DEFINE STARTUP MODE'):]

prefix = r'''
#include <cassert>
#include <iostream>
#define F(x) x
constexpr int BUILTIN_SDCARD = 0, IRQ_SOFTWARE = 0, SET_menu = 0, LED_Tools = 0;
bool irq_enabled = true, tables_ready = true, prepare_ok = true, quiesce_ok = true;
bool restore_ok = false, invalid_config = true, changed = false, sd_present = true, config_present = true;
bool TOOLS_pushbutton = true;
int result = 1, prepares = 0, reloads = 0, quiesces = 0, enables = 0;
struct Controls {
    bool running = true;
    bool Is_running() { return running; }
    void Stop() { running = false; }
    void Begin() { assert(!irq_enabled); }
    void Start() { assert(!irq_enabled && tables_ready); running = true; }
} Midi_reader, Trigger;
struct SDStub {
    bool begin(int) { return sd_present; }
    bool exists(const char*) { return config_present; }
} SD;
struct DisplayStub {
    void Confirm_config_import_popup() {}
    void Confirm_config_import_frame(int) {}
    void SETUP_show_SETUP_page() {}
    void SETUP_show_frame(int) {}
    void Config_import_FILE_error_popup() {}
} Display_Storage, Display_Setup;
struct SerialStub { void println(const char*) {} } Serial;
struct ShiftersStub { void Switch_led(int, bool) {} } Shifters_manager;
void SET_Ask_if_IMPORT_EXPORT_setup() {}
void delay(int) {}
bool NVIC_IS_ENABLED(int) { return irq_enabled; }
void AudioNoInterrupts() { irq_enabled = false; }
void AudioInterrupts() { irq_enabled = true; ++enables; }
bool P_Quiesce_audio_players()
{
    ++quiesces;
    Trigger.Stop();
    Midi_reader.Stop();
    tables_ready = false;
    return quiesce_ok;
}
bool S_Fill_all_tables()
{
    assert(!irq_enabled && !Trigger.running && !Midi_reader.running);
    ++prepares;
    tables_ready = prepare_ok;
    return prepare_ok;
}
bool Startup_mode()
{
    assert(!Trigger.running && !Midi_reader.running);
    tables_ready = prepare_ok;
    return prepare_ok;
}
bool BACKUP_Restore(bool* config_error)
{
    *config_error = invalid_config;
    if (changed)
    {
        irq_enabled = false;
    }
    return restore_ok;
}
'''

checks = r'''
void reset()
{
    irq_enabled = tables_ready = prepare_ok = quiesce_ok = sd_present = config_present = true;
    Trigger.running = Midi_reader.running = true;
    restore_ok = changed = false;
    invalid_config = true;
    result = 1;
    prepares = reloads = quiesces = enables = 0;
}
int main()
{
    reset();
    run_menu();
    assert(irq_enabled && Trigger.running && Midi_reader.running && tables_ready && prepares == 1 && reloads == 0);
    reset();
    prepare_ok = false;
    run_menu();
    assert(!Trigger.running && !Midi_reader.running && prepares == 1);
    reset();
    Trigger.running = Midi_reader.running = false;
    irq_enabled = false;
    run_menu();
    assert(!irq_enabled && !Trigger.running && !Midi_reader.running && prepares == 0);
    for (int failure : {0, 1, 2})
    {
        reset();
        result = failure == 0 ? 0 : 1;
        sd_present = failure != 1;
        config_present = failure != 2;
        run_menu();
        assert(irq_enabled && Trigger.running && Midi_reader.running && quiesces == 0);
    }
    reset();
    changed = true;
    invalid_config = false;
    run_menu(); // The reload completes recovery with its audio IRQ initially disabled.
    assert(reloads == 1 && irq_enabled && tables_ready && Trigger.running && Midi_reader.running && enables == 1);
    reset();
    restore_ok = true;
    invalid_config = false;
    run_menu();
    assert(reloads == 1 && irq_enabled && Trigger.running && Midi_reader.running);
    reset();
    irq_enabled = false;
    Trigger.running = Midi_reader.running = false;
    prepare_ok = false;
    Reload_system_state();
    assert(!irq_enabled && !Trigger.running && !Midi_reader.running && enables == 0);
    prepare_ok = true;
    Reload_system_state();
    assert(irq_enabled && Trigger.running && Midi_reader.running && enables == 1);
    std::cout << "PASS: invalid backup resume, preparation failure, stopped controls, cancellation/missing SD, restored IRQ and publication after successful reload only\n";
}
'''
production = '\nvoid Reload_system_state()\n{\n++reloads;\n' + tail + '\n}\n'
production += '\nvoid run_menu()\n{\nswitch (4)\n{\n' + menu + '\n}\n}\n'
compiler = shutil.which('g++') or r'C:\msys64\ucrt64\bin\g++.exe'
environment = os.environ.copy()
environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
with tempfile.TemporaryDirectory(prefix='lilla-restore-audio-') as directory:
    cpp = Path(directory) / 'test.cpp'
    exe = Path(directory) / 'test.exe'
    cpp.write_text(prefix + production + checks, encoding='utf-8', newline='\r\n')
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True, env=environment)
    subprocess.run([str(exe)], check=True, env=environment)
