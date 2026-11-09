"""Share the production player state enum and queries across host fixtures."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]


def with_player_state(code, initial="Playing"):
    """Inject actual state logic instead of duplicating it in each audio mock."""
    header = (ROOT / "lib/AudioPlayer/AudioPlayer.h").read_text(encoding="utf-8")
    source = (ROOT / "lib/AudioPlayer/AudioPlayer.cpp").read_text(encoding="utf-8")
    enum = re.search(r"    enum class PlayerState : uint8_t\n    \{.*?\n    \};", header, re.S).group(0)
    members = enum + "\n    volatile PlayerState state = PlayerState::" + initial + ";\n"
    for name in ("Is_finishing", "Finish_block", "Set_note_on", "isPlaying", "isPoweredOn", "State"):
        definition = re.search(r"^[^\n]+ AudioPlayer::" + name + r"\([^\n]*\n\{.*?^\}", source, re.M | re.S).group(0)
        members += definition.replace("AudioPlayer::", "") + "\n"
    assert code.count("PLAYER_STATE_MEMBERS") == 1
    return code.replace("PLAYER_STATE_MEMBERS", members)
