# Host regression tests

From the repository root, run:

```powershell
python -B scripts/tests/run_regressions.py
```

The runner discovers `*_regression.py`, executes each script in a separate process, continues after failures, and exits with status 1 if any test fails. Helper modules are not executed as tests. Python bytecode caches are disabled for the runner's child processes.

Requirements:

- Python 3.10 or newer; no additional Python packages are required.
- GCC `g++` with C++20 support on `PATH`. On Windows the runner also detects `C:/msys64/ucrt64/bin/g++.exe` and adds its directory to the child processes' `PATH` for runtime DLLs.
- The project's PlatformIO Teensy framework and dependencies must already be installed. The MIDI tests read the framework under `%USERPROFILE%/.platformio/packages/framework-arduinoteensy`; import tests use `dr_libs` under `.pio/libdeps/teensy41`. A successful `platformio run -e teensy41` prepares these dependencies.
- Tests create and compile temporary C++ fixtures in the system temporary directory. They do not upload firmware or require a connected board.

Useful selections:

```powershell
python -B scripts/tests/run_regressions.py --list
python -B scripts/tests/run_regressions.py --match loop
python -B scripts/tests/run_regressions.py --mp3
```

`--mp3` adds the longer MP3 import/resampling cases. These require FFmpeg with `libmp3lame`, either on `PATH` or installed below `.pio/test-tools`. The normal suite still compiles the MP3 import code, but skips those generated audio fixtures.

Individual scripts can also be run with `python -B scripts/tests/<name>_regression.py`. Host tests validate logic using hardware stubs; firmware compilation and listening/timing tests on Teensy remain separate checks.

The SLIDE regressions cover signed window motion, end-of-source clamping, NoClick between consecutive windows, fractional pitch, interpolation history, shared-buffer reuse and note-start reset (`player_slide_regression.py`), plus conditional SOUND cursor navigation and signed display layout (`sound_slide_controls_regression.py`). FRAM runtime and cache regressions also cover persistence and complete-source selection for sliding RAW loops.

`player_slide_edit_regression.py` checks continuous step edits through the production voice edit path: a started or prefetched junction retains its destination, zero freezes the reached window, reversal starts from the reached file edge, repeated edits coalesce, and only the next Note On resets the original window. A bounded history of step changes retains interpolation context without allocating audio buffers; a sudden pitch increase limits filter history to the retained trajectory.
