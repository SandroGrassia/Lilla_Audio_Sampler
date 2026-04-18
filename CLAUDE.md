# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

This project uses **PlatformIO** targeting Teensy 4.1.

```bash
pio run                  # Build firmware
pio run -t upload        # Build and upload to device
pio run -v               # Verbose build (shows compiler commands)
```

There are no automated tests — validation is hardware-based on the physical device.

## Project Overview

LILLA is polyphonic, multitimbral audio sampler firmware running on Teensy 4.1 (ARM Cortex-M7, 600 MHz). It supports 16 simultaneous sample players, multi-mode UI, MIDI in/out, and recording to three storage backends: external SPI flash (64 MB), PSRAM (16 MB QSPI), and SD card.

Audio codec: SGTL5000 on Teensy Audio Adaptor Rev D. Audio format: 16-bit signed PCM at 44.1 kHz.

## Architecture

### Entry Point

[src/main.cpp](src/main.cpp) (~12 500 lines) is the single orchestration file. It:
- Instantiates all AudioStream objects and wires them with AudioConnection patch cords (~76 total)
- Holds all global state variables (patches, instruments, sounds, MIDI state)
- Contains the main event loop, MIDI dispatcher, and UI state machine (13 context types)

### Audio Graph

```
AudioInputI2S → AudioPlayer[16] → AudioVCF[16] → AudioADSR[16]
                                                        ↓
                                               Router_16x3 (L/R)
                                           ↙          ↓         ↘
                                     MainMix      FeedbackMix   PWMMix
                                        ↓               ↓
                                  StereoDelay ←── StereoGain
                                        ↓
                                  FilterBiquad (L/R)
                                        ↓
                                  AudioOutputI2S + AudioOutputNoiseShapedPWM
```

NoclickCrossmix[8] handles seamless sound switching between instruments.

### Module Layout (lib/)

**Shared state** — `Shared*/` headers define the global structs passed everywhere:
- `SharedElements.h` (in `include/`) — core constants: 16 players, 8 instruments, 24 patches, 85 sounds
- `SharedPerformance.h`, `SharedSound.h`, `SharedVCF.h`, `SharedSampler.h`, `SharedLiveSampler.h`, `SharedDelay.h`, `SharedMixer.h`, `SharedLoop.h`, `SharedMM.h`, `SharedVFS.h`

**UI contexts** — 13 enum values (Performance, Sound_edit, Sampler, Live_Sampler, Mixer, Delay, Loop, Setup, ...) gate all display and encoder/button behavior.

**Pointer classes** (`Pointer*/`) — one per UI context; each manages navigation state (selected row, column, field) for its screen. They do not own audio objects.

**Display classes** (`Display*/`) — one per UI context; each reads from its Pointer and the relevant Shared struct to render to the ILI9341 display over SPI1.

**PlayersManager** — orchestrates the 16 AudioPlayer instances, handles MIDI note allocation and voice stealing, and drives sound switching via NoclickCrossmix.

**ArchivingManager** — VFS abstraction over LillaSerialFlash; manages packet-based recording/playback for StereoSampler (direct-to-flash) and StereoLiveSampler (PSRAM buffer).

**Hardware abstraction**:
- `ShiftRegisters/` — 6 x MCP23S17 I/O expanders over SPI1 (36 pushbuttons + LEDs)
- `Encoders/` — 26 rotary encoders
- `LillaFRAM_MB85RC_I2C/` — FRAM (I2C1, pins 16/17) for persistent settings
- `config/` — pin definitions, context enum, GPIO setup

### Key Design Patterns

- **AudioStream inheritance** — all audio processors subclass Teensy's `AudioStream` and implement `update()` called at 44.1 kHz / block-size interrupt.
- **Pointer -> Display split** — navigation state lives in `Pointer*` classes; rendering logic lives in `Display*` classes. Both read the same `Shared*` struct.
- **Global singletons** — `GlobalDisplay.h`, `GlobalFRAM.h`, `GlobalInfoMaster.h` expose shared object pointers for cross-module access without dependency injection.
- **Context enum dispatch** — `main.cpp` switches on the active context to route encoder deltas, button presses, and display refresh to the correct Pointer/Display pair.

## Compiler Settings

- C++ standard: `gnu++20`
- Optimization: `-DTEENSY_OPT_FASTEST`
- Warnings: `-Wall`
- Include paths: `include/`, `lib/`, `src/`

## Hardware Quick Reference

| Bus | Pins | Connected to |
|-----|------|--------------|
| SPI1 SCK/MOSI/MISO | 27 / 26 / 39 | ILI9341 display + MCP23S17 shift registers |
| Display CS / DC / RST | 38 / 29 / 30 | ILI9341 |
| Shifter CS | 37 | MCP23S17 chain |
| I2C1 SCL/SDA | 16 / 17 | FRAM (0x50-0x57) |
| Serial1 | 31250 baud | MIDI in/out |
| Gate In / Gate Out | 31 / 22 | CV triggering |
| SD | BUILTIN_SDCARD | SD card |
