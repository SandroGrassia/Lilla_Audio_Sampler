"""Verify the production player update prefix skips idle work but preserves release/restart paths."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / 'lib/AudioPlayer/AudioPlayer.cpp').read_text(encoding='utf-8')
start = source.index('void AudioPlayer::update(void)\n{')
end = source.index('    audio_block_t *block_L, *block_R;', start)
production = source[start:end] + '    ++active_entries;\n}\n'
production += re.search(r'void AudioPlayer::Enforce_cycle_deadline\(void\)\n\{.*?^\}', source, re.M | re.S).group(0)
prefix = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
constexpr int IDLE = 0, PLAYING = 1, AUDIO_BLOCK_SAMPLES = 128;
constexpr uint32_t QUICK_RELEASE_TIME = 10, PATCH_RELEASE_BLOCK_MS = 3, AUDIO_PLAYER_DEADLINE_US = 2700;
uint32_t now_ms = 0, audio_update_cycle = 42, audio_update_time_micros = 0, audio_player_emergency_stops = 0;
uint32_t millis() { return now_ms; }
struct PlayerReadDiagnostics
{
    enum { Transition = 1, RestartExecuted = 2 };
    uint32_t flags = 0, cycle = 0;
};
struct audio_block_t { int16_t data[AUDIO_BLOCK_SAMPLES] = {}; };
struct AudioPlayer
{
    int state = IDLE;
    bool read_diagnostics_enabled = true, budget_tail_pending = false, patch_release_pending = false;
    bool warmup_for_play_again_flag = false, restart_flag = false, main_settings_editing_flag = false;
    bool idle = true, power_on = false, pending_note_released = false;
    uint32_t patch_release_started = 0;
    float budget_tail_sample = 100, budget_tail_pan_L = 1, budget_tail_pan_R = 1;
    PlayerReadDiagnostics read_diagnostics;
    audio_block_t output[2];
    int allocations = 0, releases = 0, transmissions = 0, active_entries = 0, closed = 0, led_off = 0, starts = 0;
    int fail_allocation = -1;
    audio_block_t *allocate()
    {
        const int index = allocations++;
        return index == fail_allocation ? nullptr : &output[index % 2];
    }
    void release(audio_block_t *) { ++releases; }
    void transmit(audio_block_t *, int) { ++transmissions; }
    void Close_source() { ++closed; }
    void My_LED(bool on) { assert(!on); ++led_off; }
    void Start_playing() { ++starts; state = PLAYING; idle = false; }
    void update();
    void Enforce_cycle_deadline();
};
'''
checks = r'''
int main()
{
    AudioPlayer idle;
    idle.read_diagnostics.flags = 255;
    idle.update();
    assert(idle.allocations == 0 && idle.transmissions == 0 && idle.active_entries == 0);
    assert(idle.read_diagnostics.cycle == audio_update_cycle && idle.read_diagnostics.flags == 0);
    AudioPlayer active;
    active.state = PLAYING;
    active.update();
    assert(active.active_entries == 1);
    AudioPlayer release;
    release.state = PLAYING;
    release.patch_release_pending = true;
    now_ms = QUICK_RELEASE_TIME;
    release.update();
    assert(release.state == IDLE && release.closed == 1 && release.led_off == 1 && release.allocations == 0 && release.active_entries == 0);
    AudioPlayer restart;
    restart.patch_release_pending = true;
    restart.restart_flag = true;
    restart.update();
    assert(restart.starts == 1 && restart.state == PLAYING && restart.active_entries == 1);
    assert(!restart.restart_flag && (restart.read_diagnostics.flags & PlayerReadDiagnostics::RestartExecuted));
    AudioPlayer deadline;
    deadline.state = PLAYING;
    audio_update_time_micros = AUDIO_PLAYER_DEADLINE_US;
    deadline.update();
    assert(deadline.state == IDLE && deadline.closed == 1 && deadline.led_off == 1 && deadline.allocations == 0 && deadline.active_entries == 0);
    audio_update_time_micros = 0;
    AudioPlayer tail;
    tail.budget_tail_pending = true;
    tail.update();
    assert(tail.allocations == 2 && tail.releases == 2 && tail.transmissions == 2 && !tail.budget_tail_pending);
    assert(tail.output[0].data[0] == 100 && tail.output[0].data[AUDIO_BLOCK_SAMPLES - 1] == 0);
    tail.update();
    assert(tail.allocations == 2 && tail.transmissions == 2 && tail.active_entries == 0);
    for (int failed = 0; failed < 2; ++failed)
    {
        AudioPlayer retry;
        retry.budget_tail_pending = true;
        retry.fail_allocation = failed;
        retry.update();
        assert(retry.budget_tail_pending && retry.releases == 1 && retry.transmissions == 0);
        retry.fail_allocation = -1;
        retry.update();
        assert(!retry.budget_tail_pending && retry.transmissions == 2);
    }
    std::puts("PASS: idle without allocations, diagnostics, release, queued restart, deadline, budget tail and allocation retry");
}
'''
compiler = shutil.which('g++') or 'C:/msys64/ucrt64/bin/g++.exe'
with tempfile.TemporaryDirectory(prefix='lilla-idle-') as directory:
    cpp = Path(directory) / 'idle.cpp'
    exe = Path(directory) / 'idle.exe'
    cpp.write_text(prefix + production + checks, encoding='utf-8')
    subprocess.run([compiler, '-std=c++17', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    environment = os.environ.copy()
    environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
    subprocess.run([str(exe)], check=True, env=environment)
