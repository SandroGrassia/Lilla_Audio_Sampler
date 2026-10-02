"""Exercise the production FRAM name registry, including torn commits and missing identities."""
from pathlib import Path
import os
import runpy
import subprocess
import tempfile

base = runpy.run_path(str(Path(__file__).with_name("fram_archiving_regression.py")))
tests = r'''
int main()
{
    using R = FileNameRegistry;
    LillaFram.reset();
    assert(!R::Load());
    R::Clear();
    assert(R::Bind_numeric(7));
    assert(R::Save());
    assert(R::Find("0.raw") == 0 && R::Find("7.raw") == 7);
    const int kick = R::Add("kick.raw");
    assert(kick == 1 && R::Save());
    assert(R::Load() && R::Add("kick.raw") == kick);
    assert(!R::Numeric_available(kick) && !R::Bind_numeric(kick));
    assert(R::Add("snare.raw") == 2 && R::Save());
    assert(R::Load() && R::Find("kick.raw") == kick); // Absent audio never frees an identity.
    char name[R::FILENAME_BYTES];
    assert(strcmp(R::Filename(7, name), "7.raw") == 0);
    assert(strcmp(R::Filename(kick, name), "kick.raw") == 0);
    assert(!R::Valid_name("P10.raw") && !R::Valid_name("../kick.raw"));
    assert(!R::Valid_name(".raw") && !R::Valid_name("kick.wav"));
    assert(R::Add("1234567890123456789012345678901.raw") == 3);
    assert(R::Add("12345678901234567890123456789012.raw") == -1);
    assert(R::Save());
    const auto before = LillaFram.memory;
    // Interrupt each transferred block, and the final commit marker, in both bank directions.
    for (int direction = 0; direction < 2; ++direction)
    {
        LillaFram.memory = before;
        LillaFram.clear_io();
        assert(R::Load());
        if (direction == 1)
        {
            assert(R::Save());
        }
        const auto baseline = LillaFram.memory;
        for (int limit = 0; limit < static_cast<int>(R::BANK_BYTES + 4); limit += 31)
        {
            LillaFram.memory = baseline;
            LillaFram.clear_io();
            assert(R::Load() && R::Add("new.raw") == 4);
            LillaFram.stop_after = limit;
            assert(!R::Save());
            LillaFram.clear_io();
            assert(R::Load());
            assert(R::Find("kick.raw") == kick && R::Find("7.raw") == 7);
            assert(R::Find("new.raw") == -1);
        }
    }
    LillaFram.memory = before;
    LillaFram.clear_io();
    assert(R::Load());
    for (int i = 0; i < 260; ++i)
    {
        snprintf(name, sizeof(name), "sample%d.raw", i);
        R::Add(name);
    }
    assert(R::Add("overflow.raw") == -1 && R::Save());
    assert(R::Load() && R::Add("kick.raw") == kick);
    // Reject a corrupted registry instead of inventing identities.
    LillaFram.memory[R::ADDRESS] = 0;
    LillaFram.memory[R::ADDRESS + R::BANK_BYTES] = 0;
    assert(!R::Load());
    puts("PASS: stable names, numeric migration, absent files, bounds, capacity and torn dual-bank writes");
}
'''
compiler = base["compiler"]
with tempfile.TemporaryDirectory(prefix="lilla-registry-") as directory:
    source = Path(directory) / "registry.cpp"
    executable = Path(directory) / "registry.exe"
    source.write_text(base["prefix"] + tests, encoding="utf-8", newline="\r\n")
    subprocess.run([compiler, "-std=c++20", "-O2", "-Wall", "-Wextra", "-Werror", str(source), "-o", str(executable)], check=True)
    environment = os.environ.copy()
    environment["PATH"] = str(Path(compiler).parent) + os.pathsep + environment.get("PATH", "")
    subprocess.run([str(executable)], env=environment, check=True)
