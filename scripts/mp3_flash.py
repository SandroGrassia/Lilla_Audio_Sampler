"""Keep the offline MP3 decoder code and constants out of Teensy's audio RAM."""
from pathlib import Path

Import("env")

original = Path(env.subst("$LDSCRIPT_PATH"))
if not original.is_file():
    original = Path(env.PioPlatform().get_package_dir("framework-arduinoteensy")) / "cores" / "teensy4" / original.name
contents = original.read_text(encoding="utf-8")
anchor = "*(.flashmem*)"
if contents.count(anchor) != 1:
    raise RuntimeError("Cannot locate the Teensy Flash section for the MP3 decoder")
contents = contents.replace(anchor, anchor + "\n\t\t*Mp3Import.cpp.o(.text* .rodata*)")
generated = Path(env.subst("$BUILD_DIR")) / "mp3_flash.ld"
generated.parent.mkdir(parents=True, exist_ok=True)
generated.write_text(contents, encoding="utf-8", newline="\r\n")
env.Replace(LDSCRIPT_PATH=str(generated))
link_flags = list(env["LINKFLAGS"])
link_flags[link_flags.index("-T") + 1] = str(generated)
env.Replace(LINKFLAGS=link_flags)
