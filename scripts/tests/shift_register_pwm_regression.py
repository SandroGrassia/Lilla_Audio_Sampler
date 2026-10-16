"""Exercise production physical LED PWM timing without Teensy hardware."""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
header = (ROOT / "lib/ShiftRegisters/ShiftRegisters.h").read_text()
source = (ROOT / "lib/ShiftRegisters/ShiftRegisters.cpp").read_text()
fields = header[header.index("    static constexpr int PWM_LEDS"):header.index("    static constexpr int STATES")]
methods = "\n".join(re.search(r"void ShiftRegisters::" + name + r"\([^\n]*\)\n\{.*?^\}", source, re.M | re.S).group(0) for name in ("Switch_led", "Update_led_pwm"))
table = re.search(r"const uint16_t ShiftRegisters::LED_BREATH_ON_US.*?;", source).group(0)
program = r'''
#include <cassert>
#include <cstdint>
#define F(x) x
constexpr int LOW = 0, HIGH = 1, UI_LEDS = 5;
enum LedNames { LED_Rec_1, LED_Rec_2, LED_Rec_3, LED_Rec_4, LED_Tools };
uint32_t clock_us = 0;
uint32_t micros() { return clock_us; }
struct { void println(const char *) {} } Serial;
struct Mapping { int shifter_id; int shifter_channel; };
constexpr Mapping UI_leds[] = {{2, 1}, {2, 15}, {4, 3}, {4, 12}, {1, 5}};
struct Device
{
    int writes = 0;
    int levels[16] = {};
    void digitalWrite(int pin, int level) { ++writes; levels[pin] = level; }
};
class ShiftRegisters
{
public:
    Device Shifter[5];
''' + fields + "    void Switch_led(LedNames led, bool on);\n};\n" + table + "\n" + methods + r'''
int main()
{
    ShiftRegisters leds;
    leds.Switch_led(LED_Rec_1, true);
    assert(leds.Shifter[2].levels[1] == LOW);
    for (uint32_t t = 0; t < 4000000; t += 100)
    {
        clock_us = t;
        leds.Update_led_pwm();
        if (t == 2000000 || t == 2003900)
        {
            assert(leds.Shifter[2].levels[1] == LOW);
        }
        if (t == 2004000)
        {
            assert(leds.Shifter[2].levels[1] == HIGH);
        }
        if (t == 500000)
        {
            leds.Switch_led(LED_Rec_1, true);
            assert(leds.led_cycle_start_us[0] == 0);
        }
    }
    clock_us = 4000000;
    leds.Update_led_pwm();
    assert(leds.led_cycle_start_us[0] == 4000000);
    leds.Switch_led(LED_Rec_1, false);
    assert(leds.Shifter[2].levels[1] == HIGH);
    const int writes = leds.Shifter[2].writes;
    clock_us += 5000;
    leds.Update_led_pwm();
    assert(leds.Shifter[2].writes == writes);
    leds.Switch_led(LED_Tools, true);
    leds.Update_led_pwm();
    assert(leds.Shifter[1].levels[5] == LOW);
    clock_us = UINT32_MAX - 500000;
    leds.Switch_led(LED_Rec_2, true);
    clock_us += 2004000;
    leds.Update_led_pwm();
    assert(leds.Shifter[2].levels[15] == HIGH);
    leds.Switch_led(LED_Rec_2, false);
    leds.Switch_led(LED_Rec_2, true);
    assert(leds.Shifter[2].levels[15] == LOW);
    assert(leds.led_cycle_start_us[1] == clock_us);
}
'''
with tempfile.TemporaryDirectory() as directory:
    cpp = Path(directory) / "pwm.cpp"
    exe = Path(directory) / "pwm.exe"
    cpp.write_text(program)
    subprocess.run(["g++", "-std=c++17", "-Wall", "-Wextra", str(cpp), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print("Physical LED PWM regression passed")
