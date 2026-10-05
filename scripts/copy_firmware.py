"""Copy the compiled HEX into the project directory using the firmware version."""
from pathlib import Path
import re
import shutil

Import("env")


def copy_firmware(source, target, env):
    project_dir = Path(env.subst("$PROJECT_DIR"))
    config_text = (project_dir / "lib" / "config" / "config.h").read_text(encoding="utf-8")
    version = re.search(r'FIRMWARE_VERSION\[\]\s*=\s*"(\d+\.\d+\.\d+(?:\.\d+)?)(?:\s|\")', config_text).group(1)
    version_parts = version.split(".")
    if len(version_parts) == 3:
        version_parts.append("0")
    firmware_path = Path(env.subst("$BUILD_DIR/${PROGNAME}.hex"))
    destination = project_dir / ("Lilla_v" + "_".join(version_parts) + ".hex")
    shutil.copyfile(firmware_path, destination)
    print("Firmware copy: " + str(destination))


env.AddPostAction("buildprog", copy_firmware)
env.AlwaysBuild("buildprog")
