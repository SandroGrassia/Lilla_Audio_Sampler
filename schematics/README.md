# LILLA Audio Sampler - Schematics

Hardware overview of the LILLA Audio Sampler, with the main I/O sections, components and memory devices.

## Drawings

| File | Contents | Drawing revision |
| --- | --- | --- |
| [Mother.pdf](Mother.pdf) | Motherboard: Teensy, audio interfaces, MIDI, Gate, USB-C, FRAM and module connections. | PCB 2026 R2, 6 August 2026 |
| [Top.pdf](Top.pdf) | Control panel: display, I/O expanders, encoders, pushbuttons, selectors and input-level potentiometer. | PCB 2026 R2, 6 August 2026 |

Reference designators are local to each drawing: for example, motherboard U1 and control-panel U1 are different components. The PDFs show the board connections; module internals and some memory devices are not drawn individually. Part numbers and capacities below also use the current firmware's hardware declarations where the drawings use generic symbols.

## External I/O

| Section | Motherboard reference | Signal path and relevant components |
| --- | --- | --- |
| Stereo line / dynamic microphone input | J6, LINE IN | 3.5 mm input jack. The signal passes through the control-panel dual 10 kOhm input-level potentiometer (Top POT1), then the AD828 microphone amplifier module (MODULE2), into the Teensy Audio Adaptor (MODULE1). |
| Stereo line output | J5, LINE OUT | 3.5 mm jack connected to the Audio Adaptor's left/right line outputs and audio ground. The project specifies a 3.1 Vpp line output. |
| Main headphones | J8, PHONES LINE | 3.5 mm main headphone connection. The drawing brings its three contacts to H0-H2 for wiring to the headphone output. |
| Pre-listen headphones | J7, PHONES PRE-LISTEN | Separate stereo output driven by U4, TPA6132A2. Teensy PWM_L/PWM_R signals pass through RC filtering and AC coupling before the amplifier. SW2 is a dual DIP switch connected to its G0/G1 gain-selection inputs. |
| MIDI input | J1, MIDI IN | 3.5 mm jack feeding U1, 6N137S-TA1-L optocoupler, with a 220 Ohm input resistor and D0 protection diode. The receiver output connects to Teensy pin 0 (RX1). |
| MIDI output | J2, MIDI OUT | 3.5 mm jack driven from Teensy pin 1 (TX1), through Q0, LBSS138LT1G, and U2, SN74LVC2G132DCUR, with 220 Ohm series resistors. |
| Gate input | J4, GATE IN | +5 V Gate interface with resistor network, D2 BZV55B4V7 clamp and U3 SN74LVC2G132DCUR signal conditioning. Connects to Teensy pin 31; LED1 indicates the conditioned signal. |
| Gate output | J3, GATE OUT | +5 V Gate interface driven from Teensy pin 22 through Q1 LBSS138LT1G and U2. Includes series resistors, D1 LBZT52C6V2T1G clamp and LED0. |
| Power and programming | USB0, SW1, SW0 | USB-C_MC-107DMS connector for +5 V power and USB data, with 5.1 kOhm CC resistors. SW1 is the latching power switch; SW0 pulls PROGRAM to ground for firmware upload mode. |
| Removable storage | Teensy 4.1 built-in microSD slot | Audio files, exports, backups and saved MIDI loops; see the memory section below. |

MIDI OUT and Gate IN/OUT are present in the hardware, but the current user-facing firmware features do not use them. See the [main README](../README.md#connections-and-buttons) and [User Guide](../User%20Guide.md) for operating instructions.

## Main components and control panel

| Component | Reference | Role |
| --- | --- | --- |
| Teensy 4.1 | Mother MODULE0 | Main controller for audio processing, storage, MIDI and the user interface; the firmware targets 600 MHz. |
| Teensy Audio Adaptor Rev D / SGTL5000 | Mother MODULE1 | Stereo audio conversion, main line output and main headphone path. Digital audio uses I2S; codec control uses I2C. |
| AD828 amplifier module | Mother MODULE2 | Stereo analog input amplification ahead of the codec. |
| TPA6132A2 | Mother U4 | Dedicated stereo pre-listen headphone amplifier. |
| 6N137S-TA1-L | Mother U1 | Optically isolated MIDI input receiver. |
| SN74LVC2G132DCUR and LBSS138LT1G | Mother U2/U3 and Q0/Q1 | MIDI/Gate logic conditioning and output-drive stages. |
| ILI9341 display | Top DISPLAY0 | Main graphical interface, controlled over SPI1. The drawing uses a generic display-module symbol; the controller is identified by the firmware. |
| Five MCP23S17 I/O expanders | Top U0-U4, SHIFTER_0-SHIFTER_4 | Read panel controls and drive button LEDs over SPI1. The firmware calls these devices “shifters”; they provide 16 GPIOs each. |
| 17 rotary encoders with pushbuttons | Top ENC0-ENC16 | Parameter entry and push actions. Each encoder provides CLK, DT and pushbutton signals. |
| Eight D6R90 pushbuttons | Top P0-P7 | Dedicated panel buttons. |
| Five TR2-21-L5 illuminated pushbuttons | Top P8-P12 | Panel buttons with separately controlled LEDs and 680 Ohm series resistors. |
| Two SRBV141201 rotary selectors | Top SW0/SW1 | Four-position function and operating-mode selection. |
| Dual 10 kOhm potentiometer | Top POT1 | Analog left/right input-level control. |

The panel's digital connection is **Mother FFC2 to Top FFC0** (12 contacts), carrying power, SPI and display-control signals. The analog input-level connection is **Mother FFC1 to Top FFC1** (8 contacts), carrying left/right inputs, potentiometer outputs and analog ground. The `LINE_L_OUT` / `LINE_R_OUT` labels on this analog interconnect refer to the potentiometer outputs feeding the input amplifier, not the rear LINE OUT jack.

## Memory devices

The firmware configuration uses **64 MiB Flash, 32 MiB PSRAM and 128 KiB FRAM**, plus removable microSD storage. These correspond to the 64 MB, 32 MB and 128 KB capacities used elsewhere in the project documentation.

| Memory | Device and capacity | Interface / location | Firmware use | Retained without power |
| --- | --- | --- | --- | --- |
| Teensy program Flash | 8 MiB on the Teensy 4.1 module | Part of MODULE0 | Firmware storage, separate from the sample Flash module. | Yes |
| Teensy RAM | 1 MiB, split into 512 KiB RAM1 and 512 KiB RAM2 | Internal controller memory, MODULE0 | Runtime code/data, audio blocks and working buffers. | No |
| Sample Flash | One W25Q512JVFIM, 512 Mbit / 64 MiB | SPI Flash module, Mother MODULE3; CS pin 6, MOSI pin 11, MISO pin 12, SCK pin 13 | Imported audio library and Sampler recordings. | Yes |
| PSRAM | Two IS66WVS16M8FBLL-104NLI, 16 MiB each; 32 MiB total | Quad-SPI; specified in the firmware hardware configuration, not individually shown in these PDFs | Playback sample caches, delay buffers and the Live Sampler circular buffer. | No |
| FRAM | Two MB85RC512V, 512 Kbit / 64 KiB each; 128 KiB total used | I2C bus `Wire2`; SCL2 pin 24, SDA2 pin 25; device addresses `0x50` and `0x51` | Patch, sound and system settings, recording metadata and saved source-file references. | Yes |
| microSD | Removable card; capacity depends on the installed card | Teensy 4.1 built-in card interface | SD audio library and playback sources, Flash imports, WAV exports, backups and MIDI-loop files. | Yes |

**FRAM drawing versus firmware:** Mother.pdf shows four generic FRAM positions, U5-U8. The current `LillaFRAM_2x512` driver manages only two 64 KiB devices at `0x50` and `0x51`, exposing a linear address range of `0x00000` to `0x1FFFF`. Addresses `0x52` and `0x53` are explicitly unused; the four drawn positions must not be read as four active devices in the current firmware.

PSRAM is working audio memory: saving a captured Live Sampler sound writes its audio to Flash. FRAM stores the associated persistent settings and metadata. microSD provides removable file storage rather than increasing the physical capacity of Flash or PSRAM. Card filesystem support and folder purposes are documented in [SD_FILES.md](../SD_FILES.md).

## Main digital connections

| Interface | Teensy pins | Connected section |
| --- | --- | --- |
| I2S audio | 7 (codec DIN), 8 (codec DOUT), 20 (LRCLK), 21 (BCLK), 23 (MCLK) | Audio Adaptor, MODULE1. |
| Codec I2C | 18 (SDA), 19 (SCL) | Audio Adaptor control. |
| SPI Flash | 6 (CS), 11 (MOSI), 12 (MISO), 13 (SCK) | Flash module, MODULE3. |
| Panel SPI1 | 26 (MOSI), 27 (SCK), 39 (MISO), 37 (expander CS), 38 (display CS) | MCP23S17 expanders and ILI9341 display. MISO is used by the expanders; the display connection is write-only. |
| Display control | 29 (DC), 30 (RESET) | Control-panel display. |
| FRAM I2C2 | 24 (SCL2), 25 (SDA2) | Persistent metadata memory. |
| Pre-listen PWM | 3 (PWM_L), 4 (PWM_R) | Filter and TPA6132A2 headphone amplifier. |

## Firmware references

- [Hardware declarations and audio setup](../src/main.cpp)
- [Panel SPI and Gate pin assignments](../lib/config/config.h)
- [Panel I/O management](../lib/ShiftRegisters/ShiftRegisters.h)
- [FRAM devices and address map](../lib/LillaFRAM_2x512/LillaFRAM_2x512.h)
- [PSRAM devices and buffer allocation](../lib/SharedElements/SharedElements.h)
- [Firmware architecture](../FIRMWARE_ARCHITECTURE.md)
