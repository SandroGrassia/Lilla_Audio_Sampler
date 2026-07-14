/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#pragma once

#include <Arduino.h> 

// Firmware version
constexpr char FIRMWARE_VERSION[] = "7.0.0 beta 15/07/2026";

// Hardware versions
/*
Lilla PCB2026_R1
This model includes 5 Shift Registers (Shifters) MCP23S17 (SPI communication)
- each Shifter address matches with its id (0 to 4)
- Shiters' channels are configured with INPUT_PULLUP
- each Encoder (DT,CLK,PB) is connected to a single Shifter
*/

// Bus SPI1 pins
// Communication with Display and Shift register chips
static constexpr int SPI1_SCLK = 27;        // SCK
static constexpr int SPI1_MOSI = 26;        // SDA
static constexpr int SPI1_MISO = 39;        // only Shift Registers
static constexpr int SPI1_DC = 29;          // A0
static constexpr int SPI1_RST = 30;         // RESET
static constexpr int SPI1_DISPLAY_CS = 38;  // Display
static constexpr int SPI1_SHIFTERS_CS = 37; // Shift Registers

// GateIn and GateOut pins
static constexpr int GATE_IN_pin = 31;
static constexpr int GATE_OUT_pin = 22;

// Configure GateIn and GateOut GPIO pins
void Setup_GATE_pins(void);

// FRAM chips su I2C n.2
// SCL2 pin 24
// SDA2 pin 25

// MAIN CONSTANTS
static constexpr int PLAYERS = 16;
static constexpr int INSTRUMENTS = 8;   // mux number of Instruments per Patch
static constexpr int SAMPLES_VOLUME = 5000; // rampa per cambio gain - deve essere pari
static constexpr int BLOCK_MIN = 674;       // (at least AUDIO_BLOCK_SAMPLES * MAX_PITCH_FLASH) ; below this lenght, samples are copied from flash to RAM and tune is tracked with inner_tune
static constexpr int NOCLICK_DIM = 300;     // max number of samples included in cross-fade time in NoClick array creation
static constexpr int PATCHES_MAX = 24;      // max number of Patchs stored in EEPROM
static constexpr int SOUNDS_MAX = 85;       // max number of Sounds stored in EEPROM
static constexpr int NOTE_NUMBERS = 128;

// POLYPHONY AND MAX-PITCH
static constexpr double MIN_PITCH = 0.01;                     // minimum value for pitch
static constexpr int POLYPHONY_FLASH[4] = {16, 12, 8, 4};     // [optimization]
static constexpr float MAX_PITCH_FLASH[4] = {1.65, 3, 4, 10}; // [optimization]
static constexpr float MAX_PITCH_WAVETABLE = 24.0;            // maximum value for pitch when playing from RAM
static constexpr float MAX_PITCH_PSRAM = 12.0;                // maximum value for pitch when playing from PSRAM

// FILES
static constexpr int NAME_FILE_SIZE = 10;
static constexpr int RAW_FILES = 323; // nomi dei file audio (n.raw, m.rec, x.liv) esclusi i packet (Px.raw)
static constexpr int FIRST_RECORDING_FILE = 260;

// MIDI_LOOP
static constexpr int TRACKS = 4; // MIDI Loop encoders and pushbuttons 