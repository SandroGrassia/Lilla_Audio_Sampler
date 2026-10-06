/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#pragma once

#include <Arduino.h> 
#include "PlaybackProfile.h"

// Firmware version
constexpr char FIRMWARE_VERSION[] = "7.0.0.1 06/10/2026";

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

// MAIN CONSTANTS
static constexpr int AUDIO_CYCLE_BUDGET_US = 2900; // Conservative block budget for 128 samples at 44.1 kHz.
static constexpr int AUDIO_POST_PLAYER_RESERVE_US = 200; // Initial downstream processing margin; validate under worst-case hardware load.
static constexpr int AUDIO_PLAYER_DEADLINE_US = AUDIO_CYCLE_BUDGET_US - AUDIO_POST_PLAYER_RESERVE_US; // Emergency cutoff measured from Trigger, including MIDI preparation.
static constexpr int PLAYERS = 16;
static constexpr int INSTRUMENTS = 8;   // mux number of Instruments per Patch
static constexpr int SAMPLES_VOLUME = 5000; // rampa per cambio gain - deve essere pari
static constexpr int BLOCK_MIN = 674;       // Maximum span stored in AudioTables as a wavetable; longer loops are read in bounded segments.
static constexpr int NOCLICK_DIM = 300;     // max number of samples included in cross-fade time in NoClick array creation
static constexpr int PATCHES_MAX = 200;     // FRAM Patch capacity
static constexpr int SOUNDS_MAX = 800;      // FRAM Sound capacity
static constexpr int NOTE_NUMBERS = 128;

// POLYPHONY AND MAX-PITCH
static constexpr double MIN_PITCH = 0.01;                     // minimum value for pitch

// MIDI_LOOP
static constexpr int TRACKS = 4; // MIDI Loop encoders and pushbuttons 
