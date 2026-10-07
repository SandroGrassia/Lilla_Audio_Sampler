"""Print the native Teensy memory report at the end of every firmware build."""
from pathlib import Path
import subprocess

Import("env")


def report_memory_usage(source, target, env):
    firmware = Path(env.subst("$BUILD_DIR/${PROGNAME}.elf"))
    size_tool = env.WhereIs("teensy_size")
    result = subprocess.run([size_tool, str(firmware)], capture_output=True, text=True)
    print("\nMemory usage - Flash, RAM1, RAM2:")
    if result.stdout:
        print(result.stdout, end="")
    if result.stderr:
        print(result.stderr, end="")
    return result.returncode


env.AddPostAction("buildprog", report_memory_usage)
env.AlwaysBuild("buildprog")
