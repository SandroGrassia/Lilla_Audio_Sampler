"""Keep Sound Edit trim coordinates across file changes, adapting only to source bounds."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "src/main.cpp").read_text(encoding="utf-8")
body = re.search(r"        auto preserve_trim = .*?^        };", source, re.M | re.S).group(0)
assert source.count("preserve_trim();") == 3
program = r"""
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
using std::min;
#define FLASHMEM
constexpr uint32_t PATCH_CACHE_ARRAY_SAMPLES = 1000;
struct __attribute__((packed)) SoundData { uint32_t A, B; int file; } Sound[1];
int Sound_id = 0;
uint32_t source_samples = 0;
uint32_t Get_samples_in_raw_file(int) { return source_samples; }
int main()
{
""" + body + r"""
    const auto check = [&](uint32_t a, uint32_t b, uint32_t samples, uint32_t expected_a, uint32_t expected_b)
    {
        Sound[0] = {a, b, 0};
        source_samples = samples;
        preserve_trim();
        assert(Sound[0].A == expected_a && Sound[0].B == expected_b);
    };
    check(200, 599, 2000, 200, 599); // Compatible coordinates stay identical.
    check(200, 599, 600, 200, 599); // Exact endpoint.
    check(200, 599, 500, 100, 499); // Shift back, preserve duration.
    check(1500, 1899, 1000, 600, 999); // Entire old window outside the new file.
    check(200, 599, 100, 0, 99); // Only a shorter file reduces duration.
    check(5000, 5999, 10000, 5000, 5999); // Absolute offsets beyond PSRAM capacity remain valid.
    check(200, 1599, 2000, 200, 1199); // Keep the PSRAM capacity invariant.
    check(200, 599, 1, 0, 0);
    check(200, 599, 0, 0, 0);
    puts("PASS: preserved trim, endpoint shifts, shorter/empty sources and PSRAM bounds");
}
"""
with tempfile.TemporaryDirectory(prefix="lilla-trim-change-") as directory:
    cpp = Path(directory) / "trim.cpp"
    exe = Path(directory) / "trim.exe"
    cpp.write_text(program, encoding="utf-8")
    compiler = shutil.which("g++")
    subprocess.run([compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror", str(cpp), "-o", str(exe)], check=True)
    env = os.environ.copy()
    env["PATH"] = str(Path(compiler).parent) + os.pathsep + env.get("PATH", "")
    subprocess.run([str(exe)], check=True, env=env)
