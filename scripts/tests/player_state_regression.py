"""Verify named voice states, final-block cleanup and queued NoteOff transitions."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile
from player_state_fixture import with_player_state

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "lib/AudioPlayer/AudioPlayer.cpp").read_text(encoding="utf-8")


def method(name):
    return re.search(r"^void AudioPlayer::" + name + r"\([^\n]*\n\{.*?^\}", source, re.M | re.S).group(0)


code = r"""
#include <cassert>
#include <cstdint>
#include <cstdio>
uint32_t millis() { return 123; }
struct Envelope
{
    int releases = 0, stops = 0;
    void Release_note() { ++releases; }
    void Fast_stop() { ++stops; }
};
struct AudioPlayer
{
    PLAYER_STATE_MEMBERS
    Envelope envelope;
    Envelope *ADSR = &envelope;
    bool warmup_for_play_again_flag = false, restart_flag = false, pending_note_released = false;
    uint32_t time_stamp = 0;
    int led_off = 0;
    void My_LED(bool on) { led_off += !on; }
    bool Has_pending_note() const { return state != PlayerState::Idle && (warmup_for_play_again_flag || restart_flag); }
    void Release_note();
    void Fast_stop();
    void Panic_stop();
};
"""
code += "\n".join(method(name) for name in ("Release_note", "Fast_stop", "Panic_stop"))
code += r"""
int main()
{
    using State = AudioPlayer::PlayerState;
    struct Expected
    {
        State state;
        bool playing, powered, finishing;
    };
    const Expected cases[] = {
        {State::Idle, false, false, false},
        {State::Playing, true, true, false},
        {State::Released, true, false, false},
        {State::Finishing, true, true, true},
        {State::FinishingReleased, true, false, true},
        {State::Finished, false, false, true},
        {State::FinishedWithPendingNote, false, true, true}
    };
    for (const auto &expected : cases)
    {
        AudioPlayer player;
        player.state = expected.state;
        assert(player.State() == expected.state);
        assert(player.isPlaying() == expected.playing);
        assert(player.isPoweredOn() == expected.powered);
        assert(player.Is_finishing() == expected.finishing);
        player.Finish_block();
        assert(player.isPlaying() == expected.playing && player.isPoweredOn() == expected.powered);
        assert(player.Is_finishing() == (expected.state != State::Idle));
        const auto completed = player.State();
        player.Finish_block();
        assert(player.State() == completed);
    }

    AudioPlayer voice;
    voice.Release_note();
    assert(voice.State() == State::Idle && voice.envelope.releases == 0);
    voice.Set_note_on(true);
    assert(voice.State() == State::Playing);
    voice.Release_note();
    assert(voice.State() == State::Released && voice.isPlaying() && voice.envelope.releases == 1);

    voice.Set_note_on(true);
    voice.warmup_for_play_again_flag = true;
    voice.Release_note();
    assert(voice.pending_note_released && !voice.isPoweredOn() && voice.isPlaying());
    assert(voice.envelope.releases == 1); // A queued NoteOff must not release the outgoing envelope.
    voice.Fast_stop();
    assert(voice.State() == State::FinishingReleased && voice.envelope.stops == 1);
    voice.Set_note_on(true);
    assert(voice.State() == State::Finishing); // A new gate must retain final-block cleanup.
    voice.Panic_stop();
    assert(!voice.Has_pending_note() && !voice.pending_note_released && voice.Is_finishing());

    voice.state = State::Finished;
    voice.Set_note_on(true);
    assert(voice.State() == State::FinishedWithPendingNote && !voice.isPlaying());
    voice.warmup_for_play_again_flag = true;
    voice.Release_note();
    assert(voice.State() == State::Finished && voice.pending_note_released);
    voice.Finish_block();
    assert(voice.State() == State::Finished); // Never make an already completed source busy again.
    voice.state = State::Idle; // Same final cleanup performed by update().
    assert(!voice.isPlaying() && !voice.isPoweredOn());
    std::puts("PASS: all seven state contracts, final-block idempotence, gate changes, queued NoteOff and Panic");
}
"""
compiler = shutil.which("g++") or "C:/msys64/ucrt64/bin/g++.exe"
env = os.environ.copy()
env["PATH"] = str(Path(compiler).parent) + os.pathsep + env.get("PATH", "")
with tempfile.TemporaryDirectory(prefix="lilla-player-state-") as directory:
    cpp = Path(directory) / "state.cpp"
    exe = Path(directory) / "state.exe"
    cpp.write_text(with_player_state(code, "Idle"), encoding="utf-8", newline="\r\n")
    subprocess.run([compiler, "-std=c++20", "-Wall", "-Wextra", "-Werror", str(cpp), "-o", str(exe)], check=True, env=env)
    subprocess.run([str(exe)], check=True, env=env)
