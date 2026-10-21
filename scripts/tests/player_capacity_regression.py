"""Check upper voice bits and the live CPU/memory diagnostic snapshot."""
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
#include <cstdarg>
#include <string>
constexpr int PLAYERS = 16, AUDIO_BLOCK_SAMPLES = 128, AUDIO_POOL_BLOCKS = 96, IRQ_SOFTWARE = 1;
constexpr float AUDIO_SAMPLE_RATE_EXACT = 44100;
bool enabled = true;
bool NVIC_IS_ENABLED(int) { return enabled; }
void AudioNoInterrupts() { enabled = false; }
void AudioInterrupts() { enabled = true; }
uint32_t now = 1000, audio_cost_started_ms = 0;
uint32_t millis() { return now; }
float cpu = 20, peak = 75;
unsigned blocks = 12, peak_blocks = 80;
float AudioProcessorUsage() { return cpu; }
float AudioProcessorUsageMax() { return peak; }
void AudioProcessorUsageMaxReset() { peak = cpu; }
unsigned AudioMemoryUsage() { return blocks; }
unsigned AudioMemoryUsageMax() { return peak_blocks; }
void AudioMemoryUsageMaxReset() { peak_blocks = blocks; }
struct Voice
{
    bool stop = false;
    float current = 1, maximum = 4;
    bool Fast_stop_using_tables(uint8_t) { return stop; }
    bool Fast_stop_using_cache(uint16_t) { return stop; }
    void Write_time_stamp(uint16_t) {}
    bool isPlaying() { return stop; }
    float processorUsage() { return current; }
    float processorUsageMax() { return maximum; }
    void processorUsageMaxReset() { maximum = current; }
} Player[PLAYERS];
struct PlayersManager
{
    Voice *Player_ptr = Player;
    uint16_t Fast_stop_players_using_tables(uint8_t);
    uint16_t Fast_stop_players_using_cache(uint16_t);
};
#define F(x) x
struct SerialMock
{
    std::string log;
    void printf(const char *format, ...)
    {
        char line[1024];
        va_list args;
        va_start(args, format);
        vsnprintf(line, sizeof(line), format, args);
        va_end(args);
        log += line;
    }
    void println(const char *s) { log += s; log += "\n"; }
} Serial;
"""
for name in ("Fast_stop_players_using_tables", "Fast_stop_players_using_cache"):
    code += method(manager, "PlayersManager::" + name)
for name in ("Reset_audio_cost_diagnostics", "Print_audio_cost_diagnostics"):
    code += method(main, name)
code += r"""
int main()
{
    PlayersManager manager;
    Player[14].stop = Player[15].stop = true;
    assert(manager.Fast_stop_players_using_tables(1) == 0xc000u);
    assert(manager.Fast_stop_players_using_cache(1) == 0xc000u);
    Print_audio_cost_diagnostics();
    assert(enabled);
    assert(Serial.log.find("voices=16,active=2") != std::string::npos);
    assert(Serial.log.find("peak_pct=75.000") != std::string::npos);
    assert(Serial.log.find("peak_blocks=80,pool_blocks=96") != std::string::npos);
    assert(Serial.log.find("AUDIO_PLAYER,15,") != std::string::npos);
    Reset_audio_cost_diagnostics();
    assert(peak == cpu && peak_blocks == blocks && audio_cost_started_ms == 1000);
    assert(Player[15].maximum == Player[15].current);
    now = 2500;
    Serial.log.clear();
    Print_audio_cost_diagnostics();
    assert(Serial.log.find("window_ms=1500") != std::string::npos);
    puts("PASS: voices 14/15 mask bits, graph connections, CPU/memory peaks and reset");
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
