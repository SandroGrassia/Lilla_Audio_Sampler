# Host regression tests

From the repository root, run:

```powershell
python -B tests/run_regressions.py
```

The runner discovers `*_regression.py`, executes each script in a separate process, continues after failures, and exits with status 1 if any test fails. Helper modules are not executed as tests. Python bytecode caches are disabled for the runner's child processes.

Requirements:

- Python 3.10 or newer; no additional Python packages are required.
- GCC `g++` with C++20 support on `PATH`. On Windows the runner also detects `C:/msys64/ucrt64/bin/g++.exe` and adds its directory to the child processes' `PATH` for runtime DLLs.
- The project's PlatformIO Teensy framework and dependencies must already be installed. The MIDI tests read the framework under `%USERPROFILE%/.platformio/packages/framework-arduinoteensy`; import tests use `dr_libs` under `.pio/libdeps/teensy41`. A successful `platformio run -e teensy41` prepares these dependencies.
- Tests create and compile temporary C++ fixtures in the system temporary directory. They do not upload firmware or require a connected board.

Useful selections:

```powershell
python -B tests/run_regressions.py --list
python -B tests/run_regressions.py --match loop
python -B tests/run_regressions.py --mp3
```

`--mp3` adds the longer MP3 import/resampling cases. These require FFmpeg with `libmp3lame`, either on `PATH` or installed below `.pio/test-tools`. The normal suite still compiles the MP3 import code, but skips those generated audio fixtures.

Individual scripts can also be run with `python -B tests/<name>_regression.py`. Host tests validate logic using hardware stubs; firmware compilation and listening/timing tests on Teensy remain separate checks.
