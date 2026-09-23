"""Check the production reload prefix before delay buffers are initialized."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/main.cpp').read_text(encoding='utf-8')
start = source.index('void Reload_system_state(void)\n{')
end = source.index('    if (Archive.Check_FRAM_archive()', start)
reload_prefix = source[start:end] + '}\n'

fixture = r'''
#include <cassert>
#include <iostream>
#define F(value) value
constexpr int IRQ_SOFTWARE = 0;
bool irq_enabled = false, buffers_ready = false, quiesce_ok = true;
int enable_calls = 0, cache_resets = 0;
int Capture_new_patch = 3, Capture_target = 3, Capture_learn_note = 60;
bool Capture_learn_key = true;
struct Source { int value = 0; } Capture_sources[8];
struct SerialStub { void println(const char *) {} } Serial;
bool NVIC_IS_ENABLED(int) { return irq_enabled; }
void AudioNoInterrupts() { irq_enabled = false; }
void AudioInterrupts()
{
    assert(buffers_ready); // An early audio callback would access uninitialized delay storage.
    irq_enabled = true;
    ++enable_calls;
}
bool P_Quiesce_audio_players() { return quiesce_ok; }
struct Cache
{
    void Begin()
    {
        assert(!irq_enabled);
        ++cache_resets;
    }
} PatchCache_Manager;
'''
checks = r'''
int main()
{
    Capture_sources[0].value = 7;
    Reload_system_state();
    assert(!irq_enabled && enable_calls == 0 && cache_resets == 1);
    assert(Capture_new_patch == -1 && Capture_target == -1 && Capture_learn_note == -1 && !Capture_learn_key);
    assert(Capture_sources[0].value == 0);
    buffers_ready = true;
    irq_enabled = true;
    Reload_system_state();
    assert(irq_enabled && enable_calls == 1 && cache_resets == 2);
    irq_enabled = false;
    Reload_system_state();
    assert(!irq_enabled && enable_calls == 1 && cache_resets == 3);
    quiesce_ok = false;
    Capture_sources[0].value = 9;
    Reload_system_state();
    assert(!irq_enabled && cache_resets == 3 && Capture_sources[0].value == 9);
    std::cout << "PASS: cold startup, running reload, disabled IRQ and failed quiesce\n";
}
'''
with tempfile.TemporaryDirectory(prefix='lilla-capture-startup-') as directory:
    cpp = Path(directory) / 'startup.cpp'
    exe = Path(directory) / 'startup.exe'
    cpp.write_text(fixture + reload_prefix + checks, encoding='utf-8')
    compiler = shutil.which('g++')
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    environment = os.environ.copy()
    environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
    subprocess.run([str(exe)], check=True, env=environment)
