"""Check upper voice bits and audio graph connections."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
main = (ROOT / "src/main.cpp").read_text(encoding="utf-8")
manager = (ROOT / "lib/PlayersManager/PlayersManager.cpp").read_text(encoding="utf-8")
def method(source, name):
    return re.search(r"^(?:void|uint16_t) " + re.escape(name) + r"[^\n]*\n\{.*?^\}", source, re.M | re.S).group(0)

for voice in (14, 15):
    for output, side in ((0, "L"), (1, "R")):
        assert f"Player[{voice}], {output}, Router_{side}, {voice}" in main
code = r"""
#include <cassert>
#include <cstdint>
#include <cstdio>
constexpr int PLAYERS = 16;
uint32_t millis() { return 1000; }
struct Voice
{
    bool stop = false;
    bool Fast_stop_using_tables(uint8_t) { return stop; }
    bool Fast_stop_using_cache(uint16_t) { return stop; }
    void Write_time_stamp(uint16_t) {}
} Player[PLAYERS];
struct PlayersManager
{
    Voice *Player_ptr = Player;
    uint16_t Fast_stop_players_using_tables(uint8_t);
    uint16_t Fast_stop_players_using_cache(uint16_t);
};
"""
for name in ("Fast_stop_players_using_tables", "Fast_stop_players_using_cache"):
    code += method(manager, "PlayersManager::" + name)
code += r"""
int main()
{
    PlayersManager manager;
    Player[14].stop = Player[15].stop = true;
    assert(manager.Fast_stop_players_using_tables(1) == 0xc000u);
    assert(manager.Fast_stop_players_using_cache(1) == 0xc000u);
    puts("PASS: voices 14/15 mask bits and graph connections");
}
"""
with tempfile.TemporaryDirectory(prefix="lilla-capacity-") as directory:
    cpp = Path(directory) / "test.cpp"
    exe = Path(directory) / "test.exe"
    cpp.write_text(code, encoding="utf-8")
    compiler = shutil.which("g++")
    subprocess.run([compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror", str(cpp), "-o", str(exe)], check=True)
    env = os.environ.copy()
    env["PATH"] = str(Path(compiler).parent) + os.pathsep + env.get("PATH", "")
    subprocess.run([str(exe)], check=True, env=env)
