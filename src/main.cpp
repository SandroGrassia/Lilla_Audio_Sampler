/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

// **********************************************************
// **************       VERSIONE LILLA         **************
// **********************************************************
/*
    PCB: LILLA_2026_R2 - august 2026

    Hardware
    - Teensy 4.1 (ARM Cortex-M7; 1MB RAM; 8MB Flash memory; external FRAM: 128 KiB)
    - Audio Adaptor Rev.D
    - display: SPI ILI9341 240x320
    - n.1 Mic amplifier (AD828A) module
    - n.1 SPI Flash memory chip 64MB (W25Q512JVFIM)
    - n.2 QSPI PSRAM chips 16MB (IS66WVS16M8FBLL-104NLI) tot: 32MB
    - n.2 FRAM chips 64Mbyte tot: 128MByte
    - n.5 Shift registers chips (MCP23S17)

    Microcontrolleer
    RAM1 (fast): 512KB (16 blocks x 32KB)
    RAM2 (4 times slower): 512KB

    Compiler options
    - clock: 600MHz
    - Option "Fastest"

    Notes
    - micro SD card: on Teensy
    - Audio files: 16bit signed PCM MONO, 44.1 kHz

    I/O
    - USB
    - MIDI in
    - MIDI out
    - LINE in
    - LINE out
    - PHONES MONITOR out
    - PHONES MAIN out
    - GATE in
    - GATE out

     (Direct) Sampler
    - Sampler stores audio files into the external 64MB Flash memory chip
    - Sampler exports audio files into the micro SD card

    Live Sampler
    - Live Sampler stores audio into the PSRAM chips

    Midi Loop
    - Midi Loop stores loops data into the micro SD card

    Notes from: https://gist.github.com/somebox/d969f8a97e5a4362af5049ed554a9e69
    - Fact (Voltage Levels): Teensy 4.1 operates at 3.3V logic levels. Its I/O pins are NOT 5V tolerant. Applying more than 3.3V to any general-purpose I/O pin will cause permanent damage.
    - Best Practice (External Power): The VIN pin accepts an external voltage, with an official maximum of 6V, though staying at 5V is safest.
    - Best Practice (Library Management): The Arduino IDE prioritizes libraries in Documents/Arduino/libraries. An older or incompatible library here can override the correct version bundled with Teensyduino, causing compilation errors.
      If you encounter unexpected library-related errors, check this folder for duplicates and remove them.
    - Critical Warning (VUSB/VIN Separation): If using an external power source on VIN, you must cut the trace between the VUSB and VIN pads on the bottom of the board. Failure to do so can back-feed voltage to your computer's USB port, causing damage.
    - Fact (USB Host Power): The Teensy 4.1's USB Host port (VHST pin) provides a software-controllable 5V rail with built-in current limiting (~850mA hardware limit, but practically limited by the main 0.5A fuse if USB-powered). This is a feature unique to the T4.1.

    - INPUT_PULLUP: Use pinMode(pin, INPUT_PULLUP) for connecting switches to ground. This activates an internal ~47kÃƒÅ½Ã‚Â© pull-up resistor, eliminating the need for external components.
    - Synchronization: Sequential digitalWriteFast() calls to multiple pins are not perfectly simultaneous. For true atomic, multi-pin state changes, direct port register manipulation is required.
    - Audio Library: The Audio library can conflict with other DMA-based libraries (like FastLED/ObjectFLED). This is often a low-level hardware resource contention, which can sometimes be mitigated by adjusting timing parameters in the conflicting library.

    - Known Issue (USB Serial Output): Initial data sent via Serial.println() may be lost or jumbled. This is often a host-side issue. Best Practice: Use while(!Serial && millis() < 5000) {} to wait for the connection to be established without blocking indefinitely.
      Adding a small extra delay(2) after this loop can also improve reliability.

    - External QSPI Flash: The pads on the bottom of the T4.1 can also be used for an external QSPI Flash chip (up to 256MB). This memory must be accessed via a filesystem like LittleFS. It cannot be used to extend the program memory.

    - Pin States at Boot: GPIO pins are not guaranteed to be tri-stated at power-up. The NXP ROM, which runs first, may drive certain pins (notably UART1 pins 24 and 25) to a HIGH state.
    - 15-Second Restore: If a Teensy becomes unresponsive, holding the PROGRAM button for ~15 seconds initiates a full flash erase and restores a factory blink sketch. This is a crucial recovery tool.
    - Blinking Red LED: A slowly blinking red LED in bootloader mode means the Teensy is not detecting any USB communication from the host. This is almost always due to a faulty or "charge-only" USB cable. A solid red LED indicates a successful connection.

    - Best Practice (Level Shifting): WS2812B LEDs require a 5V data signal. The Teensy's 3.3V output may work but is unreliable. Use a 3.3V-to-5V level shifter (e.g., 74AHCT125) for robust operation.

    - CrashReport: Teensy 4.x has a built-in CrashReport feature. Print this to Serial in setup() to see details about the last crash.
    - addr2line: Use the arm-none-eabi-addr2line utility (part of the Teensy toolchain) with the memory address from a CrashReport and the compiled .elf file to find the exact source file and line number that caused the crash.
    - Breadcrumbs: Use CrashReport.breadcrumb(num, value) to leave markers in your code, helping to trace the execution path leading up to a crash.

    - RTOS Integration (Zephyr)
      The Zephyr RTOS has official support for Teensy 4.x boards, offering advanced multi-threading and task management.
      It uses a complex Device Tree (.dts) and Kconfig system for hardware configuration, which presents a steep learning curve.
      It is a powerful option for complex, concurrent applications, but requires careful consideration of thread safety for existing code.
*/

// *************************************************************
// ****************         LIBRARIES          *****************
// *************************************************************

#include <Arduino.h>
#include <util/atomic.h>
#include <type_traits>
#include <strings.h>
#include <spi_interrupt.h>
#include "CaptureSources.h"
#include "ZeroRaw.h"
#include "Mp3Import.h"

#if defined(LILLA_READ_BENCHMARK)
#include "ReadBenchmark.h"
#endif

#include <control_sgtl5000.h>
#include <filter_biquad.h>
#include <input_i2s.h>
#include <mixer.h>
#include <output_i2s.h>
#include <analyze_fft1024.h>
#include <SD.h>
#include <SerialFlash.h> // accesso alla Flash memory SPI
#include <utility/dspinst.h>
#include <MIDI.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <ILI9341_t3n.h>
#include <array>
#include <new>

#include "output_noiseshaped_pwm.h"
#include "Gate.h"
#include "MidiReader.h"
#include "MidiOut.h"

#include "config.h"
#include "Functions.h"
#include "UserInterface.h"
#include "ShiftRegisters.h"
#include "Encoders.h"
#include "Pushbuttons.h"
#include "Switches.h"

#include "SharedElements.h"
#include "SharedPerformance.h"
#include "SharedSound.h"
#include "SharedVCF.h"
#include "SharedSampler.h"
#include "SharedLiveSampler.h"
#include "SharedLoop.h"
#include "SharedDelay.h"
#include "SharedVFS.h"
#include "SharedMixer.h"
#include "SharedMM.h"

#include "InfoMaster.h"
#include "PlayersManager.h"
#include "PlayersStatistics.h"
#include "AudioPlayer.h"
#include "AudioADSR.h"
#include "AudioVCF.h"

#include "NoclickCrossmix.h"
#include "WaveSine.h"
#include "WaveVibrato.h"
#include "WavetableManager.h"
#include "AudioTables.h"

#include "WaveLFO.h"
#include "FilterBiquadManager.h"
#include "LillaClock.h"
#include "PerformanceLedSet.h"
#include "DelayManager.h"
#include "StereoDelay.h"
#include "StereoGain.h"
#include "AudioGain.h"
#include "Router_16x3.h"
#include "Mixer_2x1.h"
#include "AmpliOutMuteIn.h"
#include "AudioPeakDetector.h"
#include "StereoSampler.h"
#include "StereoLiveSampler.h"
#include "AudioLiveCompressor.h"
#include "LoopLedSet.h"
#include "LoopMetronomo.h"
#include "CacheCycleFinalizer.h"

#include "ArchivingManager.h"
#include "LillaFRAM_2x512.h"

#include "GraphicElements.h"
#include "DisplayPrimitives.h"
#include "DisplayCommon.h"
#include "DisplaySetup.h"
#include "DisplayStorage.h"
#include "DisplayDiagnostics.h"
#include "DisplayStartup.h"
#include "DisplayPerformance.h"
#include "DisplaySound.h"
#include "DisplayVCF.h"
#include "DisplayMixer.h"
#include "DisplayDelay.h"
#include "DisplayLiveSampler.h"
#include "DisplaySampler.h"
#include "GlobalDisplaySampler.h"
#include "DisplayMidiLoop.h"
#include "GlobalDisplayMidiLoop.h"

#include "PointerPerformance.h"
#include "PointerSound.h"
#include "PointerVCF.h"
#include "PointerMixer.h"
#include "PointerDelay.h"
#include "PointerLiveSampler.h"
#include "PointerSampler.h"
#include "PointerMidiLoop.h"

#include "PatchCacheManager.h"

// *************************************************************
// ****************   AUDIOSTREAM OBJECTS      *****************
// *************************************************************

// Attenzione: la funzione update() e' chiamata nell'ordine in cui vengono dichiarati gli oggetti Audiostream
class AutoTuneInput : public AudioStream
{
private:
    const int16_t *samples = nullptr;
    volatile uint16_t position = 1024;

public:
    AutoTuneInput() : AudioStream(0, nullptr) {}
    bool Idle() const { return position == 1024; }
    void Begin(const int16_t *input) // Called with audio interrupts disabled and the previous window fully sent.
    {
        samples = input;
        position = 0;
    }
    void update() override
    {
        if (Idle())
        {
            return;
        }
        audio_block_t *block = allocate();
        if (block == nullptr)
        {
            return;
        }
        memcpy(block->data, samples + position, AUDIO_BLOCK_SAMPLES * sizeof(int16_t));
        transmit(block);
        release(block);
        position += AUDIO_BLOCK_SAMPLES;
    }
};

LillaClock Trigger;
AudioPlayer Player[PLAYERS];
Router_16x3 Router_L;
Router_16x3 Router_R;
CacheCycleFinalizer CacheCycle_finalizer;
AudioInputI2S InputDevice;
StereoGain LINE_IN_amplifier;
AudioPeakDetector PeakTracking_L;
AudioPeakDetector PeakTracking_R;
AudioLiveCompressor LS_Compressor;
StereoLiveSampler LiveSampler;
AutoTuneInput AutoTune_input;
AudioAnalyzeFFT1024 AutoTune_fft;

ArchivingManager Archive;
StereoSampler DirectSampler(Archive);

AudioGain D_gain_L_feedback;
Mixer_2x1 D_mixer_L_feedback;
StereoDelay Delay_L;

AudioGain D_gain_R_n;
Mixer_2x1 D_mixer_R_in;
StereoDelay Delay_R;

WaveSine Tone_generator;
AudioMixer4 mixer_L;
AudioMixer4 mixer_R;
AudioFilterBiquad biquad_L;
AudioFilterBiquad biquad_R;
AmpliOutMuteIn MAIN_mixer_out_L;
AmpliOutMuteIn MAIN_mixer_out_R;
AmpliOutMuteIn PWM_mixer_out_L;
AmpliOutMuteIn PWM_mixer_out_R;
AudioOutputNoiseShapedPWM PWM_L(3);
AudioOutputNoiseShapedPWM PWM_R(4);
AudioOutputI2S audio_out;

AudioConnection patchCord1(Player[0], 0, Router_L, 0);
AudioConnection patchCord2(Player[1], 0, Router_L, 1);
AudioConnection patchCord3(Player[2], 0, Router_L, 2);
AudioConnection patchCord4(Player[3], 0, Router_L, 3);
AudioConnection patchCord5(Player[4], 0, Router_L, 4);
AudioConnection patchCord6(Player[5], 0, Router_L, 5);
AudioConnection patchCord7(Player[6], 0, Router_L, 6);
AudioConnection patchCord8(Player[7], 0, Router_L, 7);
AudioConnection patchCord9(Player[8], 0, Router_L, 8);
AudioConnection patchCord10(Player[9], 0, Router_L, 9);
AudioConnection patchCord11(Player[10], 0, Router_L, 10);
AudioConnection patchCord12(Player[11], 0, Router_L, 11);
AudioConnection patchCord13(Player[12], 0, Router_L, 12);
AudioConnection patchCord14(Player[13], 0, Router_L, 13);
AudioConnection patchCord15(Player[14], 0, Router_L, 14);
AudioConnection patchCord16(Player[15], 0, Router_L, 15);

AudioConnection patchCord17(Player[0], 1, Router_R, 0);
AudioConnection patchCord18(Player[1], 1, Router_R, 1);
AudioConnection patchCord19(Player[2], 1, Router_R, 2);
AudioConnection patchCord20(Player[3], 1, Router_R, 3);
AudioConnection patchCord21(Player[4], 1, Router_R, 4);
AudioConnection patchCord22(Player[5], 1, Router_R, 5);
AudioConnection patchCord23(Player[6], 1, Router_R, 6);
AudioConnection patchCord24(Player[7], 1, Router_R, 7);
AudioConnection patchCord25(Player[8], 1, Router_R, 8);
AudioConnection patchCord26(Player[9], 1, Router_R, 9);
AudioConnection patchCord27(Player[10], 1, Router_R, 10);
AudioConnection patchCord28(Player[11], 1, Router_R, 11);
AudioConnection patchCord29(Player[12], 1, Router_R, 12);
AudioConnection patchCord30(Player[13], 1, Router_R, 13);
AudioConnection patchCord31(Player[14], 1, Router_R, 14);
AudioConnection patchCord32(Player[15], 1, Router_R, 15);

AudioConnection patchCord33(Router_L, 0, D_mixer_L_feedback, 0);
AudioConnection patchCord34(D_mixer_L_feedback, 0, Delay_L, 0);
AudioConnection patchCord35(D_mixer_L_feedback, 0, mixer_L, 0);
AudioConnection patchCord36(Delay_L, 0, D_gain_L_feedback, 0);
AudioConnection patchCord37(D_gain_L_feedback, 0, D_mixer_L_feedback, 1);
AudioConnection patchCord38(Router_L, 0, Delay_L, 1);

AudioConnection patchCord39(Router_R, 0, D_mixer_R_in, 0);
AudioConnection patchCord40(D_mixer_R_in, 0, Delay_R, 0);
AudioConnection patchCord41(D_mixer_R_in, 0, mixer_R, 0);
AudioConnection patchCord42(Delay_R, 0, D_gain_R_n, 0);
AudioConnection patchCord43(D_gain_R_n, 0, D_mixer_R_in, 1);
AudioConnection patchCord44(Router_R, 0, Delay_R, 1);

AudioConnection patchCord45(Router_L, 1, mixer_L, 1);
AudioConnection patchCord46(Tone_generator, 0, mixer_L, 2);

AudioConnection patchCord47(Router_R, 1, mixer_R, 1);
AudioConnection patchCord48(Tone_generator, 0, mixer_R, 2);

AudioConnection patchCord49(mixer_L, 0, biquad_L, 0);
AudioConnection patchCord50(mixer_R, 0, biquad_R, 0);

AudioConnection patchCord51(biquad_L, 0, MAIN_mixer_out_L, 0);
AudioConnection patchCord52(biquad_R, 0, MAIN_mixer_out_R, 0);

AudioConnection patchCord53(biquad_L, 0, LS_Compressor, 2);
AudioConnection patchCord54(biquad_R, 0, LS_Compressor, 3);

AudioConnection patchCord55(InputDevice, 0, LINE_IN_amplifier, 0);
AudioConnection patchCord56(InputDevice, 1, LINE_IN_amplifier, 1);

AudioConnection patchCord57(LINE_IN_amplifier, 0, PeakTracking_L, 0);
AudioConnection patchCord58(LINE_IN_amplifier, 1, PeakTracking_R, 0);
AudioConnection patchCord59(LINE_IN_amplifier, 0, MAIN_mixer_out_L, 1);
AudioConnection patchCord60(LINE_IN_amplifier, 1, MAIN_mixer_out_R, 1);

AudioConnection patchCord61(MAIN_mixer_out_L, 0, audio_out, 0);
AudioConnection patchCord62(MAIN_mixer_out_R, 0, audio_out, 1);

AudioConnection patchCord63(Router_L, 2, PWM_mixer_out_L, 0);
AudioConnection patchCord64(Router_R, 2, PWM_mixer_out_R, 0);

AudioConnection patchCord65(LINE_IN_amplifier, 0, PWM_mixer_out_L, 1);
AudioConnection patchCord66(LINE_IN_amplifier, 1, PWM_mixer_out_R, 1);

AudioConnection patchCord67(LINE_IN_amplifier, 0, LS_Compressor, 0);
AudioConnection patchCord68(LINE_IN_amplifier, 1, LS_Compressor, 1);

AudioConnection patchCord69(LS_Compressor, 0, LiveSampler, 0);
AudioConnection patchCord70(LS_Compressor, 1, LiveSampler, 1);

AudioConnection patchCord71(LINE_IN_amplifier, 0, DirectSampler, 0);
AudioConnection patchCord72(LINE_IN_amplifier, 1, DirectSampler, 1);

AudioConnection patchCord73(PWM_mixer_out_L, 0, PWM_L, 0);
AudioConnection patchCord74(PWM_mixer_out_R, 0, PWM_R, 0);
AudioConnection autoTuneCord(AutoTune_input, 0, AutoTune_fft, 0);

AudioControlSGTL5000 Audio_shield;

// *************************************************************
// ***************   NON AUDIOSTREAM OBJECTS   *****************
// *************************************************************

// 16-bit ('565') color settings http://www.barth-dev.de/online/rgb565-color-picker/ and https://ee-programming-notepad.blogspot.com/2016/10/16-bit-color-generator-picker.html
ILI9341_t3n tft(SPI1_DISPLAY_CS, SPI1_DC, SPI1_RST, SPI1_MOSI, SPI1_SCLK, 255); // Write-only display on SPI1; MISO 39 is reserved for the MCP23S17 devices.
GFXcanvas16 canvas = GFXcanvas16(WAVEBOARD_WIDTH, WAVEBOARD_HEIGHT);            // RGB565 off-screen waveform canvas; uses two bytes per pixel.

InfoMaster Info;                                     // Provide sample counts and waveform data for audio sources.
WaveVibrato Vibrato;                                 // Generate the shared MIDI vibrato waveform.
float *Vibrato_array_pointer;                        // Pointer to the shared vibrato waveform samples.
uint8_t *Vibrato_array_last_element;                 // Pointer to the shared vibrato waveform position indicator.
DMAMEM AudioTables Audio_tables;                     // Store and manage wavetable and click-suppression tables in RAM2.
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI); // Create the hardware MIDI interface on Serial1.

FilterBiquadManager Filter_Biquad_Manager; // Manage biquad filter configuration.
AudioVCF VCF[PLAYERS];                     // Per-voice voltage-controlled filter processing objects.
WaveLFO LFO_P0[PLAYERS];                   // Per-voice filter modulation oscillators.
WaveLFO LFO_D[2];                          // Modulation oscillators for the two delay channels.
PlayersStatistics Players_statistics;      // Track active voices and their instrument and loop-track assignments.
FlashFileRegisterParser File_scanner;      // Scan Flash files and cache their metadata.

DisplayCommon Display_Common; // Render shared UI elements.
DisplaySetup Display_Setup;
DisplayStorage Display_Storage;
DisplayDiagnostics Display_Diagnostics;
DisplayStartup Display_Startup;
DisplayPerformance Display_Performance; // Render the Performance page.
DisplaySound Display_Sound;             // Render the Sound editing page.
DisplayVCF Display_VCF;                 // Render the instrument filter page.
DisplayMixer Display_Mixer;             // Render the Mixer page.
DisplayDelay Display_Delay;             // Render the Delay settings page.
DisplayLiveSampler Display_LiveSampler; // Render the Live Sampler page and its notifications.
DisplaySampler Display_Sampler;         // Render the Direct Sampler page.
DisplayMidiLoop Display_MidiLoop;       // Render the MIDI Loop page and its notifications.

LoopLedSet Loop_led_set;                                                                              // Manage MIDI Loop track LEDs.
PerformanceLedSet Performance_led_set;                                                                // Manage instrument LEDs on Performance-related pages.
LoopMetronomo LOOP_metronomo(Display_MidiLoop);                                                       // Drive the MIDI Loop metronome and its display feedback.
PatchCacheManager PatchCache_Manager;                                                                 // Allocate and load PSRAM caches for patch audio sources.
PlayersManager Players_Manager(&Player[0], &Router_L, &Router_R, &Audio_tables, &PatchCache_Manager); // Allocate voices and publish their presets, routing and cache sources.
MidiReader Midi_reader(LOOP_metronomo);                                                               // Process incoming MIDI and coordinate it with loop playback.
DelayManager Delay_manager;                                                                           // Own and publish delay parameter changes.
AudioADSR ADSR[PLAYERS];                                                                              // Per-voice amplitude envelope generators.

// Midi out
MidiOut Midi_out; // Transmit outgoing MIDI messages.

// Gate
GateIn Gate_in;   // Read the external gate input.
GateOut Gate_out; // Drive the external gate output.

// Encoders
Encoders Encoders_manager; // Track encoder rotation events.

// Pushbuttons
Pushbuttons Pushbuttons_manager; // Track pushbutton states and press events.

// Switches
Switches Switches_manager; // Track hardware selector states and changes.

// Shift register chips
ShiftRegisters Shifters_manager(Encoders_manager, Pushbuttons_manager, Switches_manager); // Read the shared control hardware and update encoders, buttons and switches.

// FRAM
LillaFRAM_2x512 LillaFram; // Access the two FRAM devices used for persistent data.

// Pointers
PointerPerformance Pointer_Performance; // Manage selection and navigation on the Performance page.
PointerSound Pointer_Sound;             // Manage selection and navigation on the Sound page.
PointerVCF Pointer_VCF;                 // Manage selection and navigation on the filter page.
PointerMixer Pointer_Mixer;             // Manage selection and navigation on the Mixer page.
PointerDelay Pointer_Delay;             // Manage selection and navigation on the Delay page.
PointerLiveSampler Pointer_LiveSampler; // Manage selection and navigation on the Live Sampler page.
PointerSampler Pointer_Sampler;         // Manage selection and navigation on the Direct Sampler page.
PointerMidiLoop Pointer_MidiLoop;       // Manage selection and navigation on the MIDI Loop page.

// *************************************************************
// ****************    VARIABLES AND ARRAYS     ****************
// *************************************************************

#define AudioNoInterrupts() (NVIC_DISABLE_IRQ(IRQ_SOFTWARE))
#define AudioInterrupts() (NVIC_ENABLE_IRQ(IRQ_SOFTWARE))

// >>>>>>>>>>>>>>>>>>>>>>> Polyphony/Max Pitch
int first_octave_cache; // Previous first-octave setting, used to detect changes that need saving.

// >>>>>>>>>>>>>>>>>>>>>>> Audio files
uint32_t samples_in_file; // Sample count of the audio source currently being edited.

// >>>>>>>>>>>>>>>>>>>>>>> Downsampling and resolution
int resolution_cache;    // Resolution setting saved before temporarily resetting the effect.
int downsampling_cache;  // Downsampling setting saved before temporarily resetting the effect.
bool resolution_reset;   // Whether the resolution effect is temporarily reset.
bool downsampling_reset; // Whether the downsampling effect is temporarily reset.

// >>>>>>>>>>>>>>>>>>>>>>> Tools or Modes
bool TOOLS_pushbutton; // Track activation of the Tools control.

// >>>>>>>>>>>>>>>>>>>>>>>  PERFORMANCE
// Menu
int P_menu_max;                    // Upper navigation bound for the available Performance menu entries.
void P_Select_menu_elements(void); // Enable Performance menu entries according to the current patch state.

// Pointer
P_field_description_struct P_pointer; // Current Performance menu or instrument-field selection.

// Patch
Patch_struct Patch_cache_P;                          // Reference patch metadata used to detect edits and restore discarded changes.
uint8_t Patch_id_old;                                // Performance patch to restore when leaving a temporary operating mode.
bool patch_original;                                 // Whether the current patch still matches its reference metadata.
bool patch_original_0;                               // Previous patch comparison result, used to detect menu changes.
uint8_t patches_number;                              // Number of patch slots currently in use.
int S_Get_Patch_id_free(void);                       // Return an unused patch slot, or -1 if all slots are occupied.
void P_Delete_all_Patches_and_Sounds(void);          // Delete all patch and Sound metadata.
void P_Read_all_Patches(void);                       // Load patch metadata from FRAM.
void P_Update_Patches_number(void);                  // Recount the patch slots currently in use.
uint8_t P_Get_first_Patch_id_existing(void);         // Find the first patch slot in use.
uint8_t P_Get_next_Patch_id_existing(void);          // Find the next existing patch relative to the current selection.
uint8_t P_Get_previous_Patch_id_existing(void);      // Find the previous existing patch relative to the current selection.
int P_Ask_if_change_Patch(void);                     // Show the patch-change dialog and return the selected action.
bool P_Ask_if_delete_this_Patch(void);               // Ask whether to delete the current patch; return true if confirmed.
bool P_Verify_is_Patch_original(const int patch_id); // Compare a patch and its used instruments with the reference metadata.

// >>>>>>>>>>>>>>>>>>>>>>>  INSTRUMENT EDIT
uint8_t Instrument_id; // Instrument currently selected for editing.

// Variables
uint8_t midi_channel_change; // Working MIDI-channel value during instrument editing.
uint8_t from_key_change;     // Working lower key limit during instrument editing.
uint8_t to_key_change;       // Working upper key limit during instrument editing.
uint8_t patch_change;        // Working patch selection during navigation.

// Functions
void Update_instruments_leds(void);                                                              // Refresh instrument LEDs for the current operating mode.
void P_Update_line_of_all_instruments(void);                                                     // Recalculate the display row assigned to each instrument.
bool P_Verify_if_Instrument_original(const int patch_id, const int instrument_id);               // Compare one instrument and its Sound with the reference metadata.
void P_Macro_Instrument_editing(const int patch_id, const int instrument_id, const int element); // Apply an instrument edit and publish the related playback changes.
void P_Update_all_maps_Instrument_for_notes(void);                                               // Rebuild the mapping from MIDI channel and note to patch instruments.
void P_Reset_all_maps_Instrument_for_notes(void);                                                // Clear all instrument mappings for MIDI channels and notes.
void P_Reset_map_Instrument_for_notes(const int instrument_id);                                  // Clear one instrument's note mappings before rebuilding its range.
void P_Delete_one_map_Instrument_for_notes(const int instrument_id);                             // Remove one instrument from the MIDI channel and note mappings.

// >>>>>>>>>>>>>>>>>>>>>>>  INSTRUMENT VCF
void Macro_VCF_filter_on_none(void);  // Toggle the selected instrument's filter, updating both Live Sampler channels when needed.
void Macro_VCF_modulation_none(void); // Disable filter modulation, updating both Live Sampler channels when needed.

// >>>>>>>>>>>>>>>>>>>>>>>  SOUND_EDIT
// Menu
int S_menu_max;                    // Upper navigation bound for the available Sound menu entries.
void S_Select_menu_elements(void); // Enable Sound menu entries according to the selected Sound and editing state.

// Variables
DMAMEM Sound_struct S_Sound_cache_P[SOUNDS_MAX]; // Reference metadata in RAM2.

uint16_t Sound_id;               // Sound currently selected for editing.
bool S_sound_original = true;    // Whether the selected Sound matches its reference metadata.
uint32_t S_trim_step;            // Number of samples moved by one trimming step.
int S_slicing_window;            // Selected waveform slicing window.
constexpr int MIN_SNIPPET = 100; // Minimum playback snippet length in samples.

// Pointer
S_field_description_struct S_pointer; // Current Sound menu or parameter selection.

// Functions
void S_Map_one_Instrument_for_all_notes(const int instrument_id);                                                                                              // Map the selected instrument across the note range for Sound editing.
void S_Drop_Instrument(const int instrument_id);                                                                                                               // Drop an instrument and release its cache pin while preserving playing tails.
struct PatchEditSnapshot;                                                                                                                                      // Forward declaration of the state snapshot used for reversible patch edits.
bool S_Clone_Instrument(const int instrument_id, int &new_instrument, PatchEditSnapshot &snapshot);                                                            // Insert a clone below the selected instrument; use the snapshot to preserve edit state.
bool S_Verify_is_Sound_original(int sound_id);                                                                                                                 // Compare a Sound with its reference metadata.
FLASHMEM const char *S_Auto_tune_pitch(int sound_id);                                                                                                          // Tune the selected loop; return null on success or the reason it could not be tuned.
void S_Refresh_source_limits(bool force);                                                                                                                      // Refresh Sound pitch/polyphony limits every 20 ms; force the first redraw when entering the page.
void S_Copy_all_Sound_to_Sound_cache_P(void);                                                                                                                  // Save current Sound metadata as the reference for editing and discard.
bool S_Pull_all_Sound_from_Sound_cache_P(PatchEditSnapshot *snapshot = nullptr);                                                                               // Restore Sound metadata from the reference, optionally using an edit snapshot.
uint16_t S_Get_sounds_free(void);                                                                                                                              // Count unused Sound slots.
bool S_Read_all_Sounds(PatchEditSnapshot *snapshot = nullptr);                                                                                                 // Load Sound metadata from FRAM, optionally preserving an edit snapshot.
bool S_Save_all_Sounds_changed(void);                                                                                                                          // Save modified Sound metadata to FRAM; report whether the operation succeeded.
int S_Get_sound_free(void);                                                                                                                                    // Return an unused Sound slot, or -1 when none is available.
uint32_t S_Calc_trim_step(int value);                                                                                                                          // Calculate the sample increment for the selected trimming speed.
uint8_t S_Get_midi_channel_from_Sound(int sound_id);                                                                                                           // Decode the MIDI channel stored in a Sound's packed metadata.
void S_Set_midi_channel_for_Sound(int sound_id, int midi_channel);                                                                                             // Update the MIDI channel bits in a Sound's packed metadata.
void S_Set_Sound_SOLO_OFF(void);                                                                                                                               // Disable Sound solo mode and restore normal instrument note routing.
bool P_Prepare_audio_tables(int patch_id, float patch_volume, Preset_struct (&presets)[INSTRUMENTS], uint16_t &tables_mask, bool allow_retiring_fade = false); // Main only. Preserve the audio IRQ state; activate or cancel a successfully prepared bank. Retiring-bank fading requires global interrupts to be enabled by the caller.

// >>>>>>>>>>>>>>>>>>>>>>> AudioTables (Wavetable e NoClickCrossmix)
// AudioTables publication is coordinated here, outside the audio objects.
uint16_t S_Calc_Noclick_max(bool use_Wavetable);                      // Return the maximum click-suppression setting for the current source type.
bool S_Fill_tables(uint8_t instrument_id);                            // Prepare a Sound edit from the model and publish matching presets with audio interrupts disabled.
bool S_Fill_all_tables(void);                                         // Prepare all used instruments from the model with audio interrupts disabled.
bool S_Rebuild_audio_tables(uint8_t edited_instrument = INSTRUMENTS); // Publish a complete table bank while preserving previous presets if preparation fails.
bool P_Quiesce_audio_players(void);                                   // Stop control callbacks and drain players before replacing file or patch metadata.
void Print_player_read_diagnostics(void);                             // Serial p: last, peak estimate, restart and largest positive gap; d enables/resets, D disables.
void P_Service_patch_cache(void);                                     // Copy one bounded chunk between audio updates and publish only completed files.
void P_Invalidate_file_cache(int file_id);                            // Invalidate replaced audio while retaining buffers still referenced by players.
void P_Invalidate_recording_cache(int recording_id);                  // Retire both cached channels before recording data is deleted or replaced.
bool audio_tables_error_pending = false;                              // Defer an audio-table preparation failure notification to the UI.

bool S_Fill_tables(uint8_t instrument_id); // Prepare a Sound edit from the model and publish matching presets with audio interrupts disabled.
bool S_Fill_all_tables(void);              // Prepare all used instruments from the model with audio interrupts disabled.
bool S_Rebuild_audio_tables(uint8_t);      // Publish a complete table bank while preserving previous presets if preparation fails.

// >>>>>>> FILE COPY TO PSRAM
EXTMEM int16_t patch_cache_array[PATCH_CACHE_ARRAY_COUNT][PATCH_CACHE_ARRAY_SAMPLES]; // PSRAM backing arrays for the patch audio caches.

// >>>>>>> SETTINGS
int8_t SET_menu;                                // Current Setup menu selection.
void Calc_pitch_from_note(const int &key_step); // Recalculate the note pitch multipliers for the selected keyboard scale.
uint8_t Line_in_gain = 8;                       // Shared hardware input gain; displayed as 1..16.
uint8_t Line_out_level;                         // Hardware line-output level setting.

// functions
bool SET_Copy_audio_files_from_SD_to_Flash(bool &flash_changed); // Import audio files from SD and report whether Flash contents changed.
float SET_eraseBytesPerSecond(const unsigned char *id);          // Estimate the Flash erase rate from the chip identification bytes.
void SET_Ask_if_IMPORT_EXPORT_setup(void);                       // Present the setup import/export choices and handle the selected operation.
void SET_Ask_if_FACTORY_RESET(void);                             // Request confirmation before restoring factory settings.

// CC Control Change
uint8_t CC_Sound_gain_cache[INSTRUMENTS]; // Cached per-instrument Sound gains used by the Control Change page.
uint8_t CC_lowpass_filter_cache;          // Cached low-pass filter control value.
int8_t CC_menu;                           // Current Control Change menu selection.
int CC_number;                            // Selected MIDI Control Change number.

// functions
byte CC_Save_settings(void);       // Persist Control Change settings and return the FRAM status code.
byte CC_Read_all_Sound_gain(void); // Load instrument Sound gains for the Control Change page and return the FRAM status.

// >>>>>>> DELAY
EXTMEM int16_t DELAY_fifo_L[DELAY_CACHE_CHANNEL_SAMPLES]; // PSRAM circular storage for the left delay channel.
EXTMEM int16_t DELAY_fifo_R[DELAY_CACHE_CHANNEL_SAMPLES]; // PSRAM circular storage for the right delay channel.
uint8_t delay_instrument_routing;                         // Delay routing target: instrument 0..7, or 8 for the stereo instrument pair 0 and 1.

// pointer
DELAY_element_name DELAY_local_pointer; // Current Delay menu or parameter selection.

void D_Set_value(int item, int value); // Publish one UI request through the same parameter owner used by patch changes.
bool D_Read_value(int item);           // Read a delay parameter edit into a local value and publish it through the parameter owner.

// >>>>>>> DIRECT_SAMPLING
// menu
DS_pointer_struct DS_local_pointer; // Current Direct Sampler menu or parameter selection.

// variables
const int myInput = AUDIO_INPUT_LINEIN; // Audio shield input selection: line input rather than microphone input.
int DS_export;                          // Direct Sampler export mode, selecting mono or stereo output.

enum DS_state_name
{
    DS_waiting_state,
    DS_pause_state,
    DS_recording_state,
    DS_convert_state,
    DS_export_SD_state
};
DS_state_name DS_state;                 // Current Direct Sampler state: waiting, paused, recording, converting or exporting.
elapsedMillis DS_recording_time;        // Elapsed recording duration in milliseconds.
elapsedMillis DS_recording_time_update; // Timer used to throttle recording-time display updates.
int DS_recording_change;                // Working recording selection during Direct Sampler navigation.

// PACKETS
int DS_packets_free;     // Number of available recording packets.
int DS_VFS_packets = 0;  // Number of Flash packets reserved for Direct Sampling.
int DS_First_packet = 0; // First packet in the Direct Sampling Flash area.
int DS_Last_packet = 0;  // Last packet in the Direct Sampling Flash area.

// functions
bool DS_setup_DIRECT_SAMPLING_Patch_and_Preset(void); // Prepare the Direct Sampler model and tables with audio interrupts disabled.
bool DS_export_wav_to_SD(void);                       // Export the selected Direct Sampler recording to one mono or stereo PCM WAV on SD.
void DS_ask_if_EXIT_from_DS(void);                    // Ask whether to stop recording and leave Direct Sampler; store the choice in action.
bool DS_back_to_first_DS_Recording(void);             // Prepare the first remaining recording before restoring the Direct Sampler page.
void DS_convert_file_L(int file_L_RAW, int bytes);    // Convert the left recording channel and invalidate its previous RAW cache.
void DS_convert_file_R(int file_R_RAW, int bytes);    // Convert the right recording channel and invalidate its previous RAW cache.
void DS_seed_all_Recordings(void);                    // Initialize empty recording metadata and save it to FRAM.
void DS_update_recordings(void);                      // Recount valid Direct Sampler recordings.
byte DS_read_all_Recordings(void);                    // Load all recording metadata and stop at the first FRAM error.
byte DS_read_Recording(int value);                    // Load one recording and derive its byte count and duration; return the FRAM status.
float DS_get_Recording_seconds(int value);            // Calculate a recording's duration in seconds from its sample count.
int DS_find_Recording_free(void);                     // Return a recording slot with no allocated packets, or -1 if none exists.
int DS_get_next_Recording(int value);                 // Find the next valid recording, retaining the supplied selection if none follows.
int DS_get_last_Recording(void);                      // Return the last valid recording, or -1 when none exists.
int DS_get_previous_Recording(int value);             // Find the previous valid recording relative to the supplied selection.
bool DS_check_conversion(void);                       // Check available Flash space and a free RAW file slot before conversion.
void DS_define_menu(void);                            // Enable Direct Sampler menu entries for the current recording state.
void DS_set_DS_Sampling_Patch(void);                  // Configure the temporary patch and Sounds used to play Direct Sampler recordings.
int DS_get_samples_in_Recording(int value);           // Return the sample count for a Direct Sampler recording.
void P_Recording(int value);                          // Print one recording's metadata to Serial.

// VFS VIRTUAL FILE SYSTEM
void VFS_Make_VFS(void);                              // Ask for the recording-area size and create the erasable Flash packet files.
int VFS_Get_packets(void);                            // Count the VFS packet files present in Flash.
void VFS_Print_allocation(void);                      // Print the VFS and Direct Sampler packet allocation to Serial.
bool VFS_Compile_FAT_table(void);                     // Build the packet ownership table from recording metadata and validate it.
void VFS_Reset_FAT_table(void);                       // Clear the packet ownership entries in the configured Direct Sampling range.
int VFS_Get_first_packet_free(void);                  // Find the first available recording packet.
void VFS_Erase_all_packets(void);                     // Erase all VFS packet files.
void VFS_Erase_all_packets_for_DS(void);              // Erase packets reserved for Direct Sampling.
bool VFS_Erase_packet(int value);                     // Erase one packet and report whether the operation succeeded.
bool VFS_Clean_up_VFS(void);                          // Clean invalid recording data and reclaim unused VFS storage.
bool VFS_Clean_up_orphan_packets(void);               // Erase packets that are no longer referenced by valid recordings.
bool VFS_Defragment(void);                            // Compact recording packets to consolidate free VFS storage.
bool VFS_Shift_file(int to_packet, int recording_id); // Move a recording to the requested packet position and update its metadata.
void Require_VFS(bool result);                        // Stop further processing and report a failed VFS operation.
void VFS_Print_FAT(void);                             // Print the packet ownership table to Serial.
bool BACKUP_Export(void);                             // Export configuration and recording audio to an SD backup.
bool BACKUP_Restore(bool *config_error = nullptr);    // Restore an SD backup and optionally distinguish configuration errors from other failures.

// STANDARD FILE SYSTEM
int Get_next_raw_file_in_flash(int file);          // Find the next available RAW audio source after the supplied file ID.
int Get_previous_raw_file_in_flash(int file);      // Find the previous available RAW audio source before the supplied file ID.
int Get_samples_in_raw_file(int value);            // Return the sample count for a RAW file or Direct Sampler channel.
bool Verify_space_on_flash(int value);             // Check whether total capacity minus file sizes covers the requested byte count.
int Get_first_raw_file_available(int start_value); // Find an unused RAW file ID at or above the starting ID, excluding pending captures.
void Print_flash_file_list(void);                  // Print Flash filenames, file sizes and storage totals to Serial.
int Get_flashchip_size(void);                      // Read and print Flash chip identification and return its capacity in bytes.
int Get_raw_files(void);                           // Count stored RAW files in the ordinary audio-file ID range.
int Get_raw_files_volume(void);                    // Sum the sizes of stored ordinary RAW files, in bytes.
const char *id2chip(const unsigned char *id);      // Translate Flash identification bytes into a chip model name.

// >>>>>>> LIVE_SAMPLING
// pointer
LS_pointer_struct LS_local_pointer; // Current Live Sampler menu or parameter selection.

// variables
int LS_sound_id;                                     // Live Sound whose waveform is displayed; selects left or right in stereo mode.
int LS_instrument;                                   // Selected Live Sampler instrument: 0 for mono/left, 1 for right.
int LS_COMB = 64;                                    // Divisor used to derive the play-point step from the waveform window width.
int LS_window_step;                                  // Step used when changing the waveform window width.
const int LS_XY_DELTA_MIN = 4 * AUDIO_BLOCK_SAMPLES; // Minimum Live Sampler playback interval, equal to four audio blocks.

EXTMEM int16_t LS_buffer_storage[LS_CACHE_TOTAL_SAMPLES];                      // Shared PSRAM storage for the mono or stereo Live Sampler circular buffer.
int16_t *const LS_buffer_mono_ptr = LS_buffer_storage;                         // Mono view spanning the shared Live Sampler storage.
int16_t *const LS_buffer_L_ptr = LS_buffer_storage;                            // Left-channel view at the start of the shared Live Sampler storage.
int16_t *const LS_buffer_R_ptr = LS_buffer_storage + LS_CACHE_CHANNEL_SAMPLES; // Right-channel view after the left channel's reserved sample area.
const int LS_REFRESH = 200;                                                    // Minimum interval between recording waveform refreshes, in milliseconds.
elapsedMillis LS_wave_refresh_timer;                                           // Elapsed time since the last recording waveform refresh.

static int Capture_target = -1;                                             // Destination patch for live captures, or -1 when no destination is selected.
static uint8_t Capture_return_patch = 0;                                    // Original patch to restore when a newly created capture patch is discarded.
static int8_t Capture_pair[INSTRUMENTS] = {-1, -1, -1, -1, -1, -1, -1, -1}; // Stereo partner instrument for each capture slot, or -1 for an unpaired slot.

// functions
void LS_refresh_LS_page(void);                                        // Redraw Live Sampler, restore its controls and discard notices from the previous page.
bool LS_ask_if_exit_from_LS(void);                                    // Ask whether to stop Live Sampler recording and leave the page.
void LS_update_menu_elements(void);                                   // Enable Live Sampler menu entries for the empty, recording or playback state.
int LS_constrain_position(int value);                                 // Wrap a sample position into the Live Sampler circular buffer.
void LS_lock_X_sample(void);                                          // Capture the current play point from the write position and lock it.
void LS_update_both_X_Y_samples(void);                                // Update the write position and both playback boundaries with audio interrupts disabled.
void LS_update_Q_sample(void);                                        // Snapshot the current recording write position with audio interrupts disabled.
void LS_Reset_buffer(void);                                           // Clear the Live Sampler storage, reconnect its views and reset recording positions.
void LS_setup_LS_Patch(bool stereo);                                  // Configure the temporary Live Sampler patch and its mono or stereo Sounds.
FLASHMEM void LS_Capture_notice(const char *message, const char *second_line = "");                 // Show a centered notice for two seconds and discard pending UI events.
FLASHMEM bool LS_Capture_confirm(void);                               // Ask whether to replace an existing capture; return true if confirmed.
FLASHMEM bool LS_Capture_root(uint8_t &root);                         // Learn the capture root note from a key press; return false if cancelled.
FLASHMEM bool LS_Capture_drain(void);                                 // Stop MIDI input and wait up to 100 ms for players to stop; leave MIDI stopped on success.
FLASHMEM bool LS_Capture_referenced(int file, int except_sound = -1); // Check current Sounds and saved snapshots for file references, excluding only the specified current Sound.
FLASHMEM void LS_Capture_collect(void);                               // Release unreferenced capture sources and caches while preserving references held by players.
FLASHMEM void LS_Capture_sound(int selected);                         // Capture the selected live loop into PSRAM and assign it to the target instrument or stereo pair.
FLASHMEM bool LS_Capture_write(CaptureSource &source);                // Write and verify one capture as a RAW file in Flash; return false on failure.
FLASHMEM bool LS_Capture_materialize(void);                           // Write pending captures referenced by used Sounds to RAW files before saving the patch.
FLASHMEM void LS_Capture_finish_save(void);                           // Clear successfully saved capture sources and release caches no longer needed after saving.

// >>>>>>>>>>>>>>>>>>>>>>>  MIDI_LOOP
// variables
DMAMEM uint32_t LOOP_time_order[TRACKS][LOOP_EVENTS]; // Initialized by LOOP_set_time_order before use.
elapsedMillis LOOP_clock = 0;                         // Physical elapsed-time counter underlying the MIDI Loop clock.
int LOOP_volume_int[TRACKS] = {0};                    // Per-track integer volume settings.
bool LOOP_run_button_state;                           // Whether the global loop run control is enabled.
bool LOOP_track_run_memo[TRACKS] = {false};           // Saved per-track run states used when toggling global loop playback.
int LOOP_stretch_int = 100;                           // Common track time-stretch setting, expressed as a percentage.
bool LOOP_original;                                   // Whether the current MIDI Loop data matches the saved version.

// Pointer
LOOP_field_description_struct LOOP_local_pointer; // Current MIDI Loop menu, track or parameter selection.

// functions
void LOOP_reset_all_data(void);                                // Clear MIDI Loop event data and reset loop state.
void LOOP_select_menu_elements(void);                          // Enable MIDI Loop menu entries according to the current loop state.
void LOOP_restart_clock(void);                                 // Reset the physical clock used for MIDI Loop scheduling.
void LOOP_stop_all_midi_tracks(void);                          // Stop every MIDI Loop track; call with audio interrupts disabled.
unsigned long LOOP_Clock(void);                                // Return the virtual loop time after applying the time-stretch factor.
unsigned long LOOP_zero_time(void);                            // Return the virtual start time of the current loop iteration.
int LOOP_normalized_time(void);                                // Return the current virtual time within one loop iteration.
unsigned long LOOP_Clock_time_from_virtual_time(int T_evento); // Convert an event's loop-relative time to its next absolute virtual time.
void LOOP_restart_procedure(int track);                        // Prepare the selected track to restart playback.
void LOOP_set_time_order(int track);                           // Build the event playback order for a track.
bool LOOP_Print_midi_loop_complete_data(int loop_id);          // Read and print all data from a stored MIDI Loop file.
bool LOOP_Compile_midi_loop_file(FsFile &file);                // Serialize the current MIDI Loop data into the supplied file.
bool LOOP_Read_midi_loop_file(FsFile &file, bool load);        // Validate a MIDI Loop file and optionally load its data into RAM.
String LOOP_Filename_midi_loop(int loop_id);                   // Build the SD filename for the requested MIDI Loop ID.
bool LOOP_Look_for_midi_loop_in_SD(int loop_id);               // Initialize SD and check for the loop file or its backup copy.
bool LOOP_Copy_midi_loop_from_RAM_to_SD(int loop_id);          // Save the current MIDI Loop to SD using the requested ID.
bool LOOP_Copy_midi_loop_from_SD_to_RAM(int loop_id);          // Load the selected MIDI Loop from SD.
bool LOOP_Delete_midi_loop_from_SD(int loop_id);               // Delete the selected MIDI Loop file from SD.
int LOOP_Get_first_loop_id_free(void);                         // Find the first unused MIDI Loop file ID on SD.
int LOOP_Get_next_loop_id_in_SD(int loop_id);                  // Find the next stored MIDI Loop ID after the supplied selection.
int LOOP_Get_previous_loop_id_in_SD(int loop_id);              // Find the previous stored MIDI Loop ID before the supplied selection.
void LOOP_stop_and_reset_runnig_loop_data(void);               // Stop running tracks and reset their playback state.

// >>>>>>>>>>>>>>>>>>>>>>>  MIXER
MX_pointer_struct MX_local_pointer;          // Current Mixer channel or parameter selection.
int volume_MONITOR = 0;                      // Input monitoring volume setting.
void Golive_MIXER(void);                     // Display the Mixer page and initialize its controls.
constexpr int LINE_IN_CHANNEL = INSTRUMENTS; // Mixer channel index reserved for line-input monitoring.

// >>>>>>>>>>>>>>>>>>>>>>>  FRAM
void Factory_setup_FRAM(void);  // Initialize persistent configuration with factory defaults.
void Require_FRAM(byte result); // Halt further operations and display an error if a FRAM access failed.

// >>>>>>>>>>>>>>>>>>>>>>> SGTL5000 Audio_shield
int headphones_volume_int = 40; // Unused audio-shield headphone volume setting, ranging from 0 to 40.

// Pre-listen Volume
int headphones_pwm_volume_int = 40;           // PWM pre-listen headphone volume setting, ranging from 0 to 40.
constexpr int headphones_pwm_volume_max = 40; // Maximum PWM pre-listen headphone volume setting.

// >>>>>>>>>>>>>>>>>>>>>>> SWITCH
void Switch_to_PERFORMANCE_patch_old(void);                // Restore the saved Performance patch and return to its page.
bool P_Jump_to_Patch(uint8_t next_patch);                  // Publish the destination patch only after its presets and tables are ready.
bool P_Save_current_patch_as_new(void);                    // Prepare the cloned patch before saving its sounds and metadata.
bool P_Rebuild_patch_old(void);                            // Restore the previous performance patch only after its tables are ready; call with audio interrupts disabled.
void Golive_with_PERFORMANCE(int patch_id);                // Enter the Performance page for the requested patch.
void Switch_from_MIDI_LOOP_to_PERFORMANCE(void);           // Stop loop tracks and return to Performance.
void Switch_from_LIVE_SAMPLING_to_PERFORMANCE(void);       // Handle recording exit and restore the previous Performance patch.
void Switch_from_DIRECT_SAMPLING_to_PERFORMANCE(void);     // Handle Direct Sampler exit and restore the previous Performance patch.
void Switch_to_MIXER(void);                                // Open the Mixer for the current patch and select the first available instrument.
void Switch_from_LIVE_SAMPLING_to_DELAY(void);             // Open Delay settings while retaining Live Sampler as the return page.
void Golive_with_LIVE_SAMPLING(void);                      // Enter and redraw the Live Sampler page with its controls and waveform.
void Switch_from_PERFORMANCE_to_LIVE_SAMPLING(void);       // Prepare the temporary Live Sampler patch and enter its page.
void Switch_from_MIDI_LOOP_to_LIVE_SAMPLING(void);         // Stop loop tracks, prepare the Live Sampler patch and enter its page.
void Switch_from_DIRECT_SAMPLING_to_LIVE_SAMPLING(void);   // Handle Direct Sampler exit before preparing Live Sampler.
void Golive_DIRECT_SAMPLING(void);                         // Enter and redraw the Direct Sampler page and its controls.
bool DS_Jump_to_DIRECT_SAMPLING_recording(int &recording); // Prepare the selected recording before updating its playback presets and display.
void Switch_to_DIRECT_SAMPLING(void);                      // Prepare the temporary recording patch and enter Direct Sampler.
void Switch_from_MIDI_LOOP_to_DIRECT_SAMPLING(void);       // Stop loop tracks and enter Direct Sampler.
void Switch_from_LIVE_SAMPLING_to_DIRECT_SAMPLING(void);   // Handle Live Sampler recording exit before entering Direct Sampler.
void Golive_MIDI_MONITOR(void);                            // Enter the MIDI Monitor page and initialize its display.
void Switch_from_MIDI_LOOP_to_MIDI_MONITOR(void);          // Stop loop tracks before entering MIDI Monitor.
void Golive_with_MIDI_LOOP(bool restart = false);          // Enter MIDI Loop; preserve running tracks unless restart is requested.
void Switch_from_PERFORMANCE_to_MIDI_LOOP(void);           // Enter MIDI Loop while retaining the current Performance patch.
void Switch_from_DIRECT_SAMPLING_to_MIDI_LOOP(void);       // Leave Direct Sampler and restore the previous patch for MIDI Loop.
void Switch_from_LIVE_SAMPLING_to_MIDI_LOOP(void);         // Handle recording exit and restore the previous patch for MIDI Loop.
void Golive_SETUP(void);                                   // Enter and initialize the Setup page.
void Switch_from_MIDI_LOOP_to_SETUP(void);                 // Keep the loop running while editing setup.
void Golive_DELAY_SETTINGS(void);                          // Enter and initialize the Delay settings page.

// >>>>>>>>>>>>>>>>>>>>>>>  PRINT
void Print_Patch(int patch_id);                                        // Print the selected patch's metadata to Serial.
void Print_Instrument(int patch_id, int instrument_id);                // Print one patch instrument's metadata to Serial.
void Print_Sound(int sound_id);                                        // Print the selected Sound's metadata to Serial.
void Print_Lilla_state(void);                                          // Print the current operating mode to Serial.
void Print_keyboard_state(int midi_channel, int from_key, int to_key); // Print key states for the specified MIDI channel and note range.
void Print_map_instrument_for_note(int midi_channel);                  // Print the instrument mapping for notes on one MIDI channel.
void DS_Print_Directory(File dir, int numSpaces);                      // Recursively print an SD directory with the requested indentation.

// >>>>>>>>>>>>>>>>>>>>>>>  TEST
bool test_devices = false;                   // Enable device tests during startup.
bool TEST_Current_Patch_SD_round_trip(void); // Check that the current patch survives configuration export and import through SD.

// >>>>>>>>>>>>>>>>>>>>>>>  PROTECTION
bool exibition = false; // Enable exhibition protection against restricted operations.

// >>>>>>>>>>>>>>>>>>>>>>>  GENERAL PURPOSE
bool changed;             // Shared flag indicating that an operation changed data or a setting.
bool confirmation;        // Shared completion flag for confirmation dialogs.
int action;               // Shared selection value for menu actions and confirmation dialogs.
int row;                  // Working display row used by UI operations.
int col;                  // Working display column used by UI operations.
int result;               // Shared integer result from UI input or an operation.
uint32_t big_result;      // Shared unsigned 32-bit result for operations requiring a wider value.
elapsedMicros microtimer; // Microsecond timer used for diagnostics and operation timing.

inline void P_UpdatePatchOriginalAndMenu(void)
{
    patch_original_0 = patch_original;
    patch_original = P_Verify_is_Patch_original(Patch_id);
    if (patch_original != patch_original_0)
    {
        P_Select_menu_elements();
        Display_Performance.P_show_Performance_menu();
    }
}

// >>>>>>>>>>>>>>>>>>>>>>>  STARTUP
bool Startup_mode(void);                 // Prepare tables for the selected startup mode before enabling MIDI callbacks.
void Startup_hardware_and_objects(void); // Initialize hardware, audio objects, buffers and UI controllers.
void Compile_tables(void);               // Build lookup tables required by playback and the interface.
void Reload_system_state(void);          // Reload persistent settings and model data into the running system.

// >>>>>>>>>>>>>>>>>>>>>>>   ENCODER - PUSHBUTTONS
bool Read_pushbutton(int element);      // Consume a pending press/change event for the specified pushbutton.
bool Read_pushbutton_fast(int element); // Read the current pushbutton state without consuming a change event.
int Read_encoder_simple(int element);   // Consume encoder rotation and return -1, 0 or 1 for its direction.
bool Read_encoder_fast(int element);    // Consume encoder rotation and report whether any movement occurred.
void Clear_UI_events(void);             // Discard all pending encoder rotation and pushbutton press events without resetting the controllers' internal states.

template <class T>
bool Read_encoder(const int encoder, T &value, const int highest, const int lowest, const int increment)
{
    auto R = Encoders_manager.Get_rotation(encoder);
    if (R == 0)
    {
        return false;
    }
    if constexpr (std::is_enum_v<T>)
    {
        auto v = static_cast<int>(value);
        if (R == -1)
        {
            if (v > lowest)
            {
                value = static_cast<T>(v - increment);
                return true;
            }
            return false;
        }
        else
        {
            if (v < highest)
            {
                value = static_cast<T>(v + increment);
                return true;
            }
            return false;
        }
    }
    else
    {
        if (R == -1)
        {
            if (value > lowest)
            {
                value = value - increment;
                return true;
            }
            return false;
        }
        else
        {
            if (value < highest)
            {
                value = value + increment;
                return true;
            }
            return false;
        }
    }
}

template <class T>
bool Read_encoder_inverse(const int encoder, T &value, const int highest, const int lowest, const int increment)
{
    auto R = Encoders_manager.Get_rotation(encoder);
    if (R == 0)
    {
        return false;
    }
    if constexpr (std::is_enum_v<T>)
    {
        auto v = static_cast<int>(value);
        if (R == 1)
        {
            if (v > lowest)
            {
                value = static_cast<T>(v - increment);
                return true;
            }
            return false;
        }
        else
        {
            if (v < highest)
            {
                value = static_cast<T>(v + increment);
                return true;
            }
            return false;
        }
    }
    else
    {
        if (R == 1)
        {
            if (value > lowest)
            {
                value = value - increment;
                return true;
            }
            return false;
        }
        else
        {
            if (value < highest)
            {
                value = value + increment;
                return true;
            }
            return false;
        }
    }
}

struct PatchEditSnapshot
{
    const int patch_id = Patch_id;
    Patch_struct patch;
    struct SavedSound
    {
        int sound_id;
        Sound_struct value;
    };

    // Teensy 4.1 allocates the heap in RAM2; only this small owner lives on the stack.
    SavedSound *sounds = nullptr;
    size_t count = 0;
    size_t capacity = 0;
    bool valid = true;
    PatchEditSnapshot(const PatchEditSnapshot &) = delete;
    PatchEditSnapshot &operator=(const PatchEditSnapshot &) = delete;
    PatchEditSnapshot(void); // Capture the editable model while preserving the caller's audio IRQ state.
    ~PatchEditSnapshot(void);
    bool Capture_sound(int sound_id);
    const Sound_struct *Find_sound(int sound_id) const;
    void Restore(void) const; // Restore the model and note maps after a failed preparation; published presets remain unchanged.
};

// *************************************************************
// *************************************************************
// ********************      SETUP     *************************
// *************************************************************
// *************************************************************

void setup()
{
    AudioNoInterrupts();
    Shifters_manager.Begin();

    /*
      AudioMemory allocates memory for all audio connections. The numberBlocks input specifies how much memory to reserve for audio data.
      Each block holds 128 audio samples, or approx 2.9 ms of sound. Usually an initial guess is made for numberBlocks and the actual
      usage is checked with AudioMemoryUsageMax().
    */
    AudioMemory(96);

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //   *************** SETUP HARDWARE E OBJECTS *****************
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    Startup_hardware_and_objects();

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // *****************    SPECIAL FUNCTIONS   *******************
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    // Very first startup
    if (Read_pushbutton(EN_PB_PreListenVol))
    {
        // Attenzione richiede 2/3 minuti per la cancellazione dei Packet!
        // Se la procedura si interrompe, ripeterla prima di usare l'archivio.

        Display_Storage.Factory_reset_wait_popup();
        Factory_setup_FRAM();
    }

    // UI devices test mode; results are showed on display and sent via Serial.print
    if (Read_pushbutton(EN_PB_TuningTone))
    {
        Display_Diagnostics.Encoder_pushbutton_test_board();
        while (true)
        {
            Shifters_manager.Update();

            for (auto i = 0; i < ENCODERS; ++i)
            {
                auto R = Encoders_manager.Get_rotation(i);
                if (R != 0)
                {
                    Display_Diagnostics.Encoder_pushbutton_test_result(1, i, R);
                    Serial.print("encoder: ");
                    Serial.print(i);
                    Serial.print(" value: ");
                    Serial.println(R);
                }
            }

            for (auto i = 0; i < PUSHBUTTONS; ++i)
            {
                auto R = Pushbuttons_manager.Get_change(i);
                if (R == true)
                {
                    Display_Diagnostics.Encoder_pushbutton_test_result(2, i, 0);
                    Serial.print("pushbutton ");
                    Serial.print(i);
                    Serial.println(" pressed");
                }
            }

            for (auto i = 0; i < SWITCHES; ++i)
            {
                auto R = Switches_manager.Get_change(i);
                if (R == true)
                {
                    auto value = Switches_manager.Get_value(i);
                    Display_Diagnostics.Encoder_pushbutton_test_result(3, i, value);
                    Serial.print("switch: ");
                    Serial.print(i);
                    Serial.print(" value: ");
                    Serial.println(value);
                }
            }
        }
    }

    // Protected mode: prevent from writing Patches and Sounds
    if (Read_pushbutton(EN_PB_Resolution)) // DOWNSAMPLING
    {
        exibition = true;
    }

    // ********************   GATE IN OUT TEST  ******************
    while (false)
    {
        Serial.println("Gate OUT high..");
        Gate_out.Write();
        delay(2000);
        Serial.println("Gate OUT low..");
        Gate_out.Reset();
        delay(2000);
    }

    while (false)
    {
        if (Gate_in.Read())
        {
            Serial.println("Gate IN received!");
            Gate_out.Write();
            delay(200);
            Gate_out.Reset();
        }
    }

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //    ********************    START   **********************
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    Reload_system_state();

    AudioInterrupts();
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//                             LOOP
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void loop()
{
    char audio_filename[NAME_FILE_SIZE];
#pragma region Area_Comune [rgba(118,110,2,0.1)]

    // Recording stops asynchronously: catch storage errors even after leaving the Sampler page.
    Require_FRAM(DirectSampler.Storage_error());

    P_Service_patch_cache();

    /*
    if (Serial.available() > 0)
    {
        const int command = Serial.read();
        // #if defined(LILLA_READ_BENCHMARK)
        if (command == 'b')
        {
            ReadBenchmark::Run(PatchCache_Manager, Audio_tables, Trigger.Is_running() && (Lilla_state != DIRECT_SAMPLING || DS_state == DS_waiting_state));
        }
        else
        // #endif
        if (command == 'd' || command == 'D')
        {
            const bool audio_enabled = NVIC_IS_ENABLED(IRQ_SOFTWARE) != 0;
            AudioNoInterrupts();
            Players_Manager.Enable_read_diagnostics(command == 'd');
            if (audio_enabled)
            {
                AudioInterrupts();
            }
            Serial.println(command == 'd' ? F("READ_DIAG: enabled/reset; play notes, then send p; D disables") : F("READ_DIAG: disabled; last/peak retained"));
        }
        else if (command == 'p')
        {
            Print_player_read_diagnostics();
        }
    }
    */

    if (audio_tables_error_pending)
    {
        audio_tables_error_pending = false;
        Serial.println(F("AudioTables preparation failed; previous playback presets retained"));
    }

    // Update del/i led presenti (varia in base a LILLA_STATE)
    Update_instruments_leds();

    // Update shift registers
    Shifters_manager.Update();

    // Panic
    if (Read_pushbutton(EN_PB_LineOutVol)) // LINE OUT VOLUME
    {
        AudioNoInterrupts();
        if (Lilla_state != DELAY_SETTINGS || Lilla_state_0 != DIRECT_SAMPLING)
        {
            Delay_data.loop_gain = 0;
            Delay_values.loop_gain = Delay_feedback(Delay_data.loop_gain);
            D_gain_L_feedback.Set_gain(Delay_values.loop_gain);
            D_gain_R_n.Set_gain(Delay_values.loop_gain);
        }
        Players_Manager.Stop_all_players();
        if (Lilla_state == MIDI_LOOP)
        {
            for (auto local_track = 0; local_track < TRACKS; ++local_track)
            {
                LOOP_track_run[local_track] = false;
            }
        }
        AudioInterrupts();

        if (Lilla_state == MIDI_LOOP)
        {
            Loop_led_set.Request_all_LED_switch_off();
        }

        if (Lilla_state == DELAY_SETTINGS && Lilla_state_0 != DIRECT_SAMPLING)
        {
            Display_Delay.D_feedback();
        }
    }

    // Pre-listen volume
    if (Read_encoder(EN_PB_PreListenVol, headphones_pwm_volume_int, headphones_pwm_volume_max, 0, 1))
    {
        float value = headphones_pwm_volume_int / (static_cast<float>(headphones_pwm_volume_max));
        AudioNoInterrupts();
        PWM_mixer_out_L.gain(value);
        PWM_mixer_out_R.gain(value);
        AudioInterrupts();
    }

    // To-do
    // Audio_shield.volume(headphones_volume_int / 40.0);

    // Resolution
    if (Read_encoder_inverse(EN_PB_Resolution, resolution, RES_MAX, 0, 1))
    {
        resolution_reset = false;

        AudioNoInterrupts();
        Players_Manager.Multicast_effects(resolution_value[resolution], downsampling);
        AudioInterrupts();

        if (Lilla_state == PERFORMANCE || Lilla_state == SOUND_EDIT || Lilla_state == INSTRUMENT_VCF || Lilla_state == DELAY_SETTINGS || Lilla_state == MIDI_LOOP)
        {
            Display_Common.Resolution();
        }
    }

    // Downsampling
    if (Read_encoder_inverse(EN_PB_Downsampling, downsampling, 60, 1, 1))
    {
        downsampling_reset = false;

        AudioNoInterrupts();
        Players_Manager.Multicast_effects(resolution_value[resolution], downsampling);
        AudioInterrupts();

        if (Lilla_state == PERFORMANCE || Lilla_state == SOUND_EDIT || Lilla_state == INSTRUMENT_VCF || Lilla_state == DELAY_SETTINGS || Lilla_state == MIDI_LOOP)
        {
            Display_Common.Downsampling();
        }
    }

    // Resolution (on/off) pushbutton
    if (Read_pushbutton(EN_PB_Resolution))
    {
        if (!resolution_reset)
        {
            resolution_cache = resolution;
            resolution = 0;
            AudioNoInterrupts();
            Players_Manager.Broadcast_reset_effect(resolution_value[resolution], downsampling, 0);
            AudioInterrupts();
        }
        else
        {
            resolution = resolution_cache;
            AudioNoInterrupts();
            Players_Manager.Multicast_effects(resolution_value[resolution], downsampling);
            AudioInterrupts();
        }

        if (Lilla_state == PERFORMANCE || Lilla_state == SOUND_EDIT || Lilla_state == INSTRUMENT_VCF || Lilla_state == DELAY_SETTINGS || Lilla_state == MIDI_LOOP)
        {
            Display_Common.Resolution();
        }
        resolution_reset = !resolution_reset;
    }

    // Downsampling (on/off) pushbutton
    if (Read_pushbutton(EN_PB_Downsampling))
    {
        if (!downsampling_reset)
        {
            downsampling_cache = downsampling;
            downsampling = 1;
            AudioNoInterrupts();
            Players_Manager.Broadcast_reset_effect(resolution_value[resolution], downsampling, 1);
            AudioInterrupts();
        }
        else
        {
            downsampling = downsampling_cache;
            AudioNoInterrupts();
            Players_Manager.Multicast_effects(resolution_value[resolution], downsampling);
            AudioInterrupts();
        }
        if (Lilla_state == PERFORMANCE || Lilla_state == SOUND_EDIT || Lilla_state == INSTRUMENT_VCF || Lilla_state == DELAY_SETTINGS || Lilla_state == MIDI_LOOP)
        {
            Display_Common.Downsampling();
        }

        downsampling_reset = !downsampling_reset;
    }

    // Pushbutton tuning tone
    if (Read_pushbutton(EN_PB_TuningTone)) // switch ON/OFF the Tuning Tone
    {

        // ****************************************************   Test FRAM  *********************************************
        // LillaFram.Destructive_Fram_Test(0x91);

        // **************************************************   Test Midi Out  *******************************************
        /*
        result = random(128);
        Midi_out.NoteOn(result, 100,  1);
        Midi_out.NoteOff(result, 100, 1);
        */

        // *********************************************     Test SD save Patch   ****************************************
        // TEST_Current_Patch_SD_round_trip();

        tuning_tone_flag = !tuning_tone_flag;
        if (Lilla_state == PERFORMANCE)
        {
            Display_Performance.P_show_TuningTone_instrument(Patch_id);
        }
        if (!tuning_tone_flag)
        {
            Tone_generator.Stop();
        }
    }

    // Tuning tone volume
    if (tuning_tone_flag && Read_encoder(EN_PB_TuningTone, tuning_tone_volume, 40, 0, 1))
    {
        if (Lilla_state == PERFORMANCE)
        {
            Display_Performance.P_show_gain_TuningTone(Patch_id);
        }
    }

    // Low-pass cutoff frequency
    if (Read_encoder(EN_PB_Cutoff, lowpass_target, LPF_MAX, 0, 1))
    {
        lowpass_flag = true;
        lowpass_direction = lowpass_target > lowpass;

        if (Lilla_state == PERFORMANCE || Lilla_state == SOUND_EDIT || Lilla_state == INSTRUMENT_VCF || Lilla_state == DELAY_SETTINGS || Lilla_state == MIDI_LOOP)
        {
            Display_Common.Lowpass_filter();
        }
    }

    // Low-pass cutoff from midi CC
    if (display_lowpass_flag)
    {
        Display_Common.Lowpass_filter();
        display_lowpass_flag = false;
    }

    // Pushbutton Low-pass flat
    if (Read_pushbutton(EN_PB_Cutoff))
    {
        if (lowpass_target < LPF_MAX)
        {
            lowpass_flag = true;
            lowpass_target = LPF_MAX;
            lowpass_direction = true;

            if (Lilla_state == PERFORMANCE || Lilla_state == SOUND_EDIT || Lilla_state == INSTRUMENT_VCF || Lilla_state == DELAY_SETTINGS || Lilla_state == MIDI_LOOP)
            {
                Display_Common.Lowpass_filter();
            }
        }
    }

    // Print AudioProcessorUsage
    if (Read_pushbutton(EN_PB_PreListenVol))
    {
        Serial.print("AudioProcessorUsage(): ");
        Serial.println(AudioProcessorUsage());
        for (auto player = 0; player < PLAYERS; ++player)
        {
            Serial.print("Player[");
            Serial.print(player);
            Serial.print("].processorUsage(): ");
            Serial.println(Player[player].processorUsage());
        }
    }

#pragma endregion // parte comune

#pragma region Performance [rgba(2, 68, 118, 0.1)]

    // *************************************************************
    // ********************    PERFORMANCE  ************************
    // *************************************************************

    if (Lilla_state == PERFORMANCE)
    {

        // Change volume_patch
        if (Read_encoder(EN_PB_LineOutVol, volume_patch, PATCH_VOLUME_MAX, 0, 1)) // LINE OUT VOLUME
        {
            AudioNoInterrupts();
            Players_Manager.Update_all_Preset_volume(Patch_id, Patch_volume_gain(volume_patch));
            Players_Manager.Broadcast_volume();
            AudioInterrupts();

            Display_Common.P_Patch_volume_value(true);
        }

        // Move pointer
        result = Read_encoder_simple(EN_PB_Select);
        if (result != 0)
        {
            Pointer_Performance.Move_pointer(result, P_menu_max);
            P_pointer = Pointer_Performance.Get_pointer();

            Clear_UI_events();
        }

        // Change values
        switch (P_pointer.field_name)
        {
        case field_P_Menu:
        {
            if (Read_pushbutton(EN_PB_Select))
            {
                switch (P_element_menu[P_pointer.element])
                {
                case value_P_Exit: // drop Sound changes
                {
                    if (Patch_id == Capture_new_patch)
                    {
                        PatchEditSnapshot previous;
                        if (!previous.valid)
                        {
                            break;
                        }
                        Patch[Patch_id] = Patch_cache_P;
                        if (!S_Pull_all_Sound_from_Sound_cache_P(&previous) || !P_Jump_to_Patch(Capture_return_patch))
                        {
                            previous.Restore();
                        }
                        break;
                    }
                    AudioNoInterrupts();
                    PatchEditSnapshot previous;
                    if (!previous.valid)
                    {
                        audio_tables_error_pending = true;
                        AudioInterrupts();
                        break;
                    }
                    Patch[Patch_id] = Patch_cache_P;
                    const bool sounds_restored = S_Pull_all_Sound_from_Sound_cache_P(&previous);
                    P_Update_all_maps_Instrument_for_notes();

                    const bool tables_rebuilt = sounds_restored && S_Fill_all_tables();
                    if (!tables_rebuilt)
                    {
                        previous.Restore();
                        AudioInterrupts();
                        break;
                    }
                    uint8_t active_bank_mask = 0;

                    // Capture the active bank mask before restoring audio interrupts.
                    if (tables_rebuilt)
                    {
                        for (uint8_t instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
                        {
                            active_bank_mask |= Audio_tables.Get_active_pointers(instrument_id).bank_mask;
                        }
                    }
                    AudioInterrupts();

                    // Report the result after restoring audio interrupts.
                    if (tables_rebuilt)
                    {
                        Serial.print(F("AudioTables restore activated, bank mask: 0x"));
                        Serial.println(active_bank_mask, HEX);
                    }
                    else
                    {
                        Serial.println(F("AudioTables restore tables not activated"));
                    }

                    LS_Capture_collect();
                    Pointer_Performance.Delete_pointer();
                    patch_original = true;
                    P_Select_menu_elements();
                    P_Update_line_of_all_instruments();

                    Display_Performance.P_show_Performance_menu(); // Draw the menu and update its navigation layout.
                    Display_Performance.P_show_all_instruments(Patch_id);
                    Performance_led_set.Restore_all_LED();
                    Update_instruments_leds();

                    Pointer_Performance.Set_pointer_to_Patch();
                    P_pointer = Pointer_Performance.Get_pointer();

                    Clear_UI_events();

                    Print_Patch(Patch_id);
                }
                break;

                case value_P_Save: // Save this Patch
                    if (!S_Save_all_Sounds_changed())
                    {
                        Golive_with_PERFORMANCE(Patch_id);
                        break;
                    }
                    Require_FRAM(Archive.Save_Delay(Patch_id, Delay_data));
                    Require_FRAM(Archive.Save_Patch(Patch_id));
                    LS_Capture_finish_save();
                    Archive.Copy_Patch_from_RAM_to_SD(Patch_id);

                    Patch_cache_P = Patch[Patch_id];
                    S_Copy_all_Sound_to_Sound_cache_P();

                    Golive_with_PERFORMANCE(Patch_id);
                    break;

                case value_P_Clone:
                case value_P_SaveAsNew:
                    if (!P_Save_current_patch_as_new())
                    {
                        Serial.println(F("AudioTables clone not activated; current patch retained"));
                    }
                    break;

                case value_P_DropPatch:
                    // Drop patch and go back to PERFORMANCE "P_Get_first_Patch_id_existing()"

                    // YES, drop
                    if (P_Ask_if_delete_this_Patch())
                    {
                        const uint8_t deleted_patch = Patch_id;
                        int next_patch = -1;
                        for (int candidate = 0; candidate < PATCHES_MAX; ++candidate)
                        {
                            if (candidate != deleted_patch && Patch[candidate].used)
                            {
                                next_patch = candidate;
                                break;
                            }
                        }
                        if (next_patch < 0 || !P_Jump_to_Patch(next_patch))
                        {
                            break;
                        }
                        Lilla_state = PERFORMANCE;

                        Patch[deleted_patch].used = false;
                        for (auto instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
                        {
                            if (Patch[deleted_patch].Instrument[instrument_id].used)
                            {
                                Sound[Get_sound_id(deleted_patch, instrument_id)].used = false;
                            }
                        }

                        if (!S_Save_all_Sounds_changed())
                        {
                            Golive_with_PERFORMANCE(Patch_id);
                            break;
                        }
                        Require_FRAM(Archive.Save_Patch(deleted_patch));
                        LS_Capture_finish_save();
                        Archive.Copy_Patch_from_RAM_to_SD(deleted_patch);

                        P_Read_all_Patches();
                        P_Update_Patches_number();

                        S_Copy_all_Sound_to_Sound_cache_P();
                        LS_Capture_collect();
                    }

                    // NO, don't drop the patch
                    else
                    {
                        P_Select_menu_elements();
                        Display_Performance.P_show_PERFORMANCE_page(false, true);
                        Performance_led_set.Restore_all_LED();
                        Update_instruments_leds();

                        Pointer_Performance.Set_pointer_to_Patch();
                        P_pointer = Pointer_Performance.Get_pointer();
                    }
                    break;

                default:
                    PRINT_ERROR(F("ERROR: switch MISSING! "));
                    break;
                }
            }
        }
        break;

        case field_P_Patch:
        {
            result = Read_encoder_simple(EN_PB_Value);
            if (result != 0)
            {
                if (result == +1)
                {
                    patch_change = P_Get_next_Patch_id_existing();
                }
                else
                {
                    patch_change = P_Get_previous_Patch_id_existing();
                }

                if (patch_change != Patch_id)
                {
                    if (!P_Verify_is_Patch_original(Patch_id)) // Patch_id NOT original
                    {
                        action = P_Ask_if_change_Patch();

                        if (action == 0) // Exit: remain in this patch_id
                        {
                            Golive_with_PERFORMANCE(Patch_id);
                        }

                        else // change patch_id
                        {
                            PatchEditSnapshot previous;
                            if (!previous.valid)
                            {
                                audio_tables_error_pending = true;
                                return;
                            }
                            if (action == 1) // No: discharge changings and switch patch_id
                            {
                                Patch[Patch_id] = Patch_cache_P;
                                if (!S_Pull_all_Sound_from_Sound_cache_P(&previous))
                                {
                                    previous.Restore();
                                    return;
                                }
                            }

                            else if (action == 2) // Yes: save changings and switch patch_id
                            {
                                if (!S_Save_all_Sounds_changed())
                                {
                                    Golive_with_PERFORMANCE(Patch_id);
                                    return;
                                }
                                Require_FRAM(Archive.Save_Delay(Patch_id, Delay_data));
                                Require_FRAM(Archive.Save_Patch(Patch_id));
                                LS_Capture_finish_save();
                                Archive.Copy_Patch_from_RAM_to_SD(Patch_id);
                            }

                            if (!P_Jump_to_Patch(patch_change))
                            {
                                previous.Restore();
                                return;
                            }

                            Patch_id_old = Patch_id;
                        }
                    }

                    else // Patch_id IS original
                    {
                        if (!P_Jump_to_Patch(patch_change))
                        {
                            return;
                        }

                        Patch_id_old = Patch_id;
                    }
                }
            }
        }
        break;

        case field_P_Instrument:
        {
            // Enter Instrument_inside area
            if (Read_pushbutton(EN_PB_Select))
            {
                Pointer_Performance.Move_pointer_from_Instrument_to_inside();
                P_pointer = Pointer_Performance.Get_pointer();

                Clear_UI_events();
            }
        }
        break;

        case field_P_Instrument_inside:
        {
            const int instrument_id = static_cast<int>(P_pointer.instrument_id); // static_cast<int>(Pointer_Performance.Get_pointer().instrument_id);
            const int element = P_pointer.element;
            const int sound_id = Get_sound_id(Patch_id, instrument_id);

            // Exit from Instrument_inside area
            if (Read_pushbutton(EN_PB_Select))
            {
                Pointer_Performance.Move_pointer_from_inside_to_Instrument();
                P_pointer = Pointer_Performance.Get_pointer();

                Clear_UI_events();
            }

            switch (Pointer_Performance.Get_pointer().element)
            {
            case value_P_Lock: // Lock
            {
                result = Read_encoder_simple(EN_PB_Value);

                if (result == 1)
                {
                    AudioNoInterrupts();
                    Patch[Patch_id].Instrument[instrument_id].lock = true;
                    Players_Manager.Update_Preset_lock(Patch_id, instrument_id);
                    Players_Manager.Multicast_reset_pitch_bend_effects(instrument_id);
                    AudioInterrupts();

                    P_Macro_Instrument_editing(Patch_id, instrument_id, element);
                    P_UpdatePatchOriginalAndMenu();
                }

                else if (result == -1)
                {
                    AudioNoInterrupts();
                    Patch[Patch_id].Instrument[instrument_id].lock = false;
                    Players_Manager.Update_Preset_lock(Patch_id, instrument_id);
                    Players_Manager.Broadcast_restore_pitch_bend_and_effects(instrument_id, pitch_bend_value[Get_midi_channel(Patch_id, instrument_id)]);
                    AudioInterrupts();

                    P_Macro_Instrument_editing(Patch_id, instrument_id, element);
                    P_UpdatePatchOriginalAndMenu();
                }
            }
            break;

            case value_P_Precedence: // Precedence
            {
                result = Read_encoder_simple(EN_PB_Value);
                if (result == 1)
                {
                    AudioNoInterrupts();
                    Patch[Patch_id].Instrument[instrument_id].precedence = true;
                    Players_Manager.Update_Preset_precedence(Patch_id, instrument_id);
                    AudioInterrupts();

                    P_Macro_Instrument_editing(Patch_id, instrument_id, element);
                    P_UpdatePatchOriginalAndMenu();
                }
                else if (result == -1)
                {
                    AudioNoInterrupts();
                    Patch[Patch_id].Instrument[instrument_id].precedence = false;
                    Players_Manager.Update_Preset_precedence(Patch_id, instrument_id);
                    AudioInterrupts();

                    P_Macro_Instrument_editing(Patch_id, instrument_id, element);
                    P_UpdatePatchOriginalAndMenu();
                }
            }
            break;

            case value_P_Midi: // Midi (channel)
            {
                result = Read_encoder_simple(EN_PB_Value);
                if (result != 0)
                {
                    midi_channel_change = S_Get_midi_channel_from_Sound(sound_id);

                    if (result == 1)
                    {
                        if (midi_channel_change < 15)
                        {
                            ++midi_channel_change;
                        }
                    }
                    else // -1
                    {
                        if (midi_channel_change > 0)
                        {
                            --midi_channel_change;
                        }
                    }
                    if (midi_channel_change != S_Get_midi_channel_from_Sound(sound_id))
                    {
                        AudioNoInterrupts();
                        Players_Manager.Multicast_release_players(sound_id);
                        P_Reset_map_Instrument_for_notes(instrument_id);
                        S_Set_midi_channel_for_Sound(sound_id, midi_channel_change);
                        Update_map_Instrument_for_notes(Patch[Patch_id].Instrument[instrument_id].from_note, Patch[Patch_id].Instrument[instrument_id].to_note, instrument_id);
                        Players_Manager.Update_Preset_midi_channel(Patch_id, instrument_id);
                        AudioInterrupts();

                        P_Macro_Instrument_editing(Patch_id, instrument_id, element);
                        P_UpdatePatchOriginalAndMenu();
                    }
                }
            }
            break;

            case value_P_RootKey: // Root key
                changed = Read_encoder(EN_PB_Value, Patch[Patch_id].Instrument[instrument_id].root_key, 127, 0, 1);
                if (Read_pushbutton(EN_PB_Value))
                {
                    changed = (changed || Patch[Patch_id].Instrument[instrument_id].root_key != 60);
                    Patch[Patch_id].Instrument[instrument_id].root_key = 60; // Restore middle C; use the normal edit path to retune active players.
                }
                if (changed)
                {
                    AudioNoInterrupts();
                    Players_Manager.Multicast_change_players_notes(Patch_id, instrument_id);
                    AudioInterrupts();

                    P_Macro_Instrument_editing(Patch_id, instrument_id, element);
                    P_UpdatePatchOriginalAndMenu();
                }
                break;

            case value_P_FromKey: // From Key
            {
                result = Read_encoder_simple(EN_PB_Value);
                const bool reset_key = Read_pushbutton(EN_PB_Value); // Consume the click even when rotation occurs in the same loop.
                if (result != 0 || reset_key)
                {
                    changed = false;
                    if (reset_key)
                    {
                        from_key_change = 0; // Extend the lower note boundary through the existing mapping update.
                        changed = Patch[Patch_id].Instrument[instrument_id].from_note != from_key_change;
                    }
                    else if (result == 1 && Patch[Patch_id].Instrument[instrument_id].from_note < Patch[Patch_id].Instrument[instrument_id].to_note)
                    {
                        from_key_change = Patch[Patch_id].Instrument[instrument_id].from_note + 1;
                        changed = true;
                    }
                    else if (result == -1 && Patch[Patch_id].Instrument[instrument_id].from_note > 0)
                    {
                        from_key_change = Patch[Patch_id].Instrument[instrument_id].from_note - 1;
                        changed = true;
                    }
                    if (changed)
                    {
                        AudioNoInterrupts();
                        Players_Manager.Change_from_key(Patch_id, instrument_id, from_key_change);
                        AudioInterrupts();

                        P_Macro_Instrument_editing(Patch_id, instrument_id, element);
                        P_UpdatePatchOriginalAndMenu();
                    }
                }
            }
            break;

            case value_P_ToKey: // To key
            {
                result = Read_encoder_simple(EN_PB_Value);
                const bool reset_key = Read_pushbutton(EN_PB_Value); // Consume the click even when rotation occurs in the same loop.
                if (result != 0 || reset_key)
                {
                    changed = false;
                    if (reset_key)
                    {
                        to_key_change = 127; // Extend the upper note boundary through the existing mapping update.
                        changed = Patch[Patch_id].Instrument[instrument_id].to_note != to_key_change;
                    }
                    else if (result == 1 && Patch[Patch_id].Instrument[instrument_id].to_note < 127)
                    {
                        to_key_change = Patch[Patch_id].Instrument[instrument_id].to_note + 1;
                        changed = true;
                    }

                    else if (result == -1 && Patch[Patch_id].Instrument[instrument_id].to_note > Patch[Patch_id].Instrument[instrument_id].from_note)
                    {
                        to_key_change = Patch[Patch_id].Instrument[instrument_id].to_note - 1;
                        changed = true;
                    }

                    if (changed)
                    {
                        AudioNoInterrupts();
                        Players_Manager.Change_to_key(Patch_id, instrument_id, to_key_change);
                        AudioInterrupts();

                        P_Macro_Instrument_editing(Patch_id, instrument_id, element);
                        P_UpdatePatchOriginalAndMenu();
                    }
                }
            }
            break;

            case value_P_Pan: // Pan
                if (Read_encoder(EN_PB_Value, Sound[sound_id].pan, 16, -16, 1))
                {
                    AudioNoInterrupts();
                    Players_Manager.Update_Preset_pan(Patch_id, instrument_id);
                    Players_Manager.Multicast_pan(instrument_id);
                    AudioInterrupts();

                    P_Macro_Instrument_editing(Patch_id, instrument_id, element);
                    P_UpdatePatchOriginalAndMenu();
                }

                // Set PAN to center
                else if (Read_pushbutton(EN_PB_Value))
                {
                    Sound[sound_id].pan = 0;

                    AudioNoInterrupts();
                    Players_Manager.Update_Preset_pan(Patch_id, instrument_id);
                    Players_Manager.Multicast_pan(instrument_id);
                    AudioInterrupts();

                    P_Macro_Instrument_editing(Patch_id, instrument_id, element);
                    P_UpdatePatchOriginalAndMenu();
                }

                break;

            case value_P_Gain: // Gain
                if (Read_encoder(EN_PB_Value, Sound[sound_id].gain, 40, 0, 1))
                {
                    AudioNoInterrupts();
                    Players_Manager.Update_Preset_volume(Patch_id, instrument_id, Patch_volume_gain(volume_patch));
                    Players_Manager.Multicast_volume_for_instrument_edit(instrument_id);
                    AudioInterrupts();

                    P_Macro_Instrument_editing(Patch_id, instrument_id, element);
                    P_UpdatePatchOriginalAndMenu();
                }
                break;
            }
        }
        break;
        }

        // Instrument volume changed from MIDI CC
        if (display_instrument_volume_flag)
        {
            Display_Performance.P_show_Instrument_description(Patch_id, instrument_volume_changed, true);

            // restore LED
            if (Performance_led_set.Read_LED_activity(instrument_volume_changed) > 0)
            {
                Display_Performance.Led_PERFORMANCE_instrument(instrument_volume_changed, true);
            }
            else
            {
                Display_Performance.Led_PERFORMANCE_instrument(instrument_volume_changed, false);
            }
            display_instrument_volume_flag = false;
        }

        // Switch to SOUND_EDIT
        for (auto Inst_id = 0; Inst_id < INSTRUMENTS; ++Inst_id)
        {
            if (Read_pushbutton(PB_Sound[Inst_id]))
            {
                if (Patch[Patch_id].Instrument[Inst_id].used)
                {
                    Instrument_id = Inst_id;

                    Lilla_state_0 = PERFORMANCE;
                    Lilla_state = SOUND_EDIT;

                    Sound_id = Get_sound_id(Patch_id, Instrument_id);

                    samples_in_file = Get_samples_in_raw_file(Sound[Sound_id].file);
                    Noclick_max = S_Calc_Noclick_max(Preset[Instrument_id].use_Wavetable);
                    S_trim_step = S_Calc_trim_step(trim_speed);

                    Display_Sound.Show_SOUND_page(Patch_id, Instrument_id);

                    S_sound_original = S_Verify_is_Sound_original(Sound_id);
                    S_Select_menu_elements();
                    Display_Sound.Show_SOUND_menu(); // displays the menu and updates "SO_menu_max" used by encoder_menu

                    // Reset pointer
                    Pointer_Sound.Set_pointer_to_first_menu_element();
                    S_pointer = Pointer_Sound.Get_pointer();
                    Pointer_Sound.Display_pointer();

                    Performance_led_set.Restore_all_LED();

                    Display_Sound.Show_wave(Instrument_id);

                    Clear_UI_events();

                    // Report
                    Serial.print("Editing Instrument: ");
                    Serial.print(Instrument_id);
                    Print_Sound(Sound_id);
                }

                else
                {
                    char message[24];
                    snprintf(message, sizeof(message), "SOUND %u IS NOT USED", static_cast<unsigned int>(Inst_id + 1));
                    Show_popup_text(message, ILI9341_WHITE, ILI9341_RED, display_coordinate_y(7));
                    delay(1000);
                    Show_popup_text(message, ILI9341_BLACK, ILI9341_BLACK, display_coordinate_y(7));
                }
            }
        }

        // Switch verso un TOOL
        if (Read_pushbutton(PB_Tools))
        {
            TOOLS_pushbutton = true;
            Shifters_manager.Switch_led(LED_Tools, true);

            switch (Switches_manager.Get_value(SwitchTools))
            {
            case SwToolsMixer:
            {
                Lilla_state_0 = PERFORMANCE;
                Patch_id_old = Patch_id;
                Switch_to_MIXER();
            }
            break;

            case SwToolsDelay:
            {
                Lilla_state_0 = PERFORMANCE;
                Patch_id_old = Patch_id;
                Golive_DELAY_SETTINGS();
            }
            break;

            case SwToolsSetup:
            {
                Lilla_state_0 = PERFORMANCE;
                Patch_id_old = Patch_id;
                Golive_SETUP();
            }
            break;

            case SwToolsTest:
            {
                Lilla_state_0 = PERFORMANCE;
                Patch_id_old = Patch_id;
                Golive_MIDI_MONITOR();
            }
            break;
            }
        }

        // Switch Mode
        if (Switches_manager.Get_change(SwitchModes))
        {
            switch (Switches_manager.Get_value(SwitchModes))
            {
            case SwModesSampler:
            {
                Patch_id_old = Patch_id;
                Switch_to_DIRECT_SAMPLING();
            }
            break;

            case SwModesLiveSampler:
            {
                Patch_id_old = Patch_id;
                Switch_from_PERFORMANCE_to_LIVE_SAMPLING();
            }
            break;

            case SwModesPerformance:
                break;

            case SwModesMidiLoop:
            {
                Switch_from_PERFORMANCE_to_MIDI_LOOP();
            }
            break;
            }
        }
    }

#pragma endregion // PERFORMANCE

#pragma region Sound Edit [rgba(2, 108, 118, 0.1)]
    // *************************************************************
    // ********************    SOUND EDIT   ************************
    // *************************************************************

    static bool sound_edit_was_active = false;
    const bool entering_sound_edit = !sound_edit_was_active && Lilla_state == SOUND_EDIT;
    sound_edit_was_active = Lilla_state == SOUND_EDIT;
    if (Lilla_state == SOUND_EDIT)
    {
        S_Refresh_source_limits(entering_sound_edit); // Check only while this page is active and redraw immediately after re-entry.
        const Sound_struct sound_before_edit = Sound[Sound_id];
        // Change volume_patch
        if (Read_encoder(EN_PB_LineOutVol, volume_patch, PATCH_VOLUME_MAX, 0, 1))
        {
            AudioNoInterrupts();
            Players_Manager.Update_all_Preset_volume(Patch_id, Patch_volume_gain(volume_patch));
            Players_Manager.Broadcast_volume();
            AudioInterrupts();
        }

        // Move pointer
        result = Read_encoder_simple(EN_PB_Select);
        if (result != 0)
        {
            Pointer_Sound.Move_pointer(result, S_menu_max);
            S_pointer = Pointer_Sound.Get_pointer();

            Clear_UI_events();
        }

        // Change values
        switch (S_pointer.field_name)
        {
        case field_S_Menu:
        {
            if (Read_pushbutton(EN_PB_Select))
            {
                switch (S_element_menu[S_pointer.menu_element])
                {
                case value_S_Return: // keep changes and exit from SOUND EDIT
                {
                    S_Set_Sound_SOLO_OFF();

                    if (Lilla_state_0 == MIDI_LOOP)
                    {
                        Golive_with_MIDI_LOOP(false);
                    }
                    else
                    {
                        Golive_with_PERFORMANCE(Patch_id);
                    }
                }
                break;

                case value_S_Clone:
                {
                    AudioNoInterrupts();
                    PatchEditSnapshot previous;
                    int new_instrument = 0;
                    bool cloned = false;
                    if (previous.valid && S_Clone_Instrument(Instrument_id, new_instrument, previous))
                    {
                        if (S_Fill_tables(new_instrument))
                        {
                            P_Update_all_maps_Instrument_for_notes();
                            cloned = true;
                        }
                        else
                        {
                            previous.Restore();
                        }
                    }
                    else
                    {
                        previous.Restore();
                    }
                    AudioInterrupts();

                    S_Set_Sound_SOLO_OFF();
                    Golive_with_PERFORMANCE(Patch_id);
                    if (cloned)
                    {
                        Pointer_Performance.Set_pointer_to_RootKey(new_instrument);
                        P_pointer = Pointer_Performance.Get_pointer();
                    }
                }
                break;

                case value_S_Drop:
                {
                    AudioNoInterrupts();
                    Players_Manager.Release_all_players_for_instrument(Instrument_id);
                    P_Delete_one_map_Instrument_for_notes(Instrument_id);
                    S_Drop_Instrument(Instrument_id); // instruments is decremented by 1
                    AudioInterrupts();

                    S_Set_Sound_SOLO_OFF();
                    Golive_with_PERFORMANCE(Patch_id);
                }
                break;

                default:
                    break;
                }
            }
        }
        break;

        case field_S_Value:
        {
            switch (S_pointer.value_element)
            {
            case value_S_File:
            {
                int result = Read_encoder_simple(EN_PB_Value);
                if (result != 0)
                {
                    int S_file_change;
                    if (result == 1)
                    {
                        S_file_change = Get_next_raw_file_in_flash(Sound[Sound_id].file);
                    }
                    else
                    {
                        S_file_change = Get_previous_raw_file_in_flash(Sound[Sound_id].file);
                    }
                    if (S_file_change != Sound[Sound_id].file)
                    {
                        Sound[Sound_id].file = S_file_change;
                        samples_in_file = Get_samples_in_raw_file(Sound[Sound_id].file);
                        Sound[Sound_id].pitch = 0;
                        Sound[Sound_id].A = 0;
                        Sound[Sound_id].B = (samples_in_file > 0 ? samples_in_file - 1 : 0);
                        if (!slicing_mode)
                        {
                            S_slicing_window = Sound[Sound_id].B - Sound[Sound_id].A + 1;
                        }
                        Noclick_max = S_Calc_Noclick_max(((Sound[Sound_id].B - Sound[Sound_id].A + 1) <= BLOCK_MIN));
                        if (Sound[Sound_id].Noclick > Noclick_max)
                        {
                            Sound[Sound_id].Noclick = Noclick_max;
                        }

                        AudioNoInterrupts();

                        if (!S_Fill_tables(Instrument_id))
                        {
                            Sound[Sound_id] = sound_before_edit;
                            Noclick_max = S_Calc_Noclick_max(Preset[Instrument_id].use_Wavetable);
                            samples_in_file = Get_samples_in_raw_file(Sound[Sound_id].file);
                            if (!slicing_mode)
                            {
                                S_slicing_window = Sound[Sound_id].B - Sound[Sound_id].A + 1;
                            }
                            Noclick_max = S_Calc_Noclick_max(((Sound[Sound_id].B - Sound[Sound_id].A + 1) <= BLOCK_MIN));
                        }
                        AudioInterrupts();

                        S_trim_step = S_Calc_trim_step(trim_speed);

                        Display_Sound.Show_File_value(Instrument_id);
                        Display_Sound.Show_wave(Instrument_id);

                        auto sound_original_0 = S_sound_original;
                        S_sound_original = S_Verify_is_Sound_original(Sound_id);
                        if (sound_original_0 != S_sound_original)
                        {
                            if (Lilla_state_0 != MIDI_LOOP)
                            {
                                S_Select_menu_elements();
                            }
                            Display_Sound.Show_SOUND_menu();
                        }
                    }
                }

                // SOLO
                if (Read_pushbutton(EN_PB_Select))
                {
                    if (!solo_flag)
                    {
                        solo_flag = true;
                        AudioNoInterrupts();
                        Players_Manager.Release_all_players_for_instrument_solo(Instrument_id);
                        S_Map_one_Instrument_for_all_notes(Instrument_id);
                        AudioInterrupts();
                    }
                    else
                    {
                        S_Set_Sound_SOLO_OFF();
                    }
                    Display_Sound.Show_wave(Instrument_id);
                }
            }
            break;

            case value_S_Midi:
            {
                int result = Read_encoder_simple(EN_PB_Value);
                if (result != 0)
                {
                    midi_channel_change = S_Get_midi_channel_from_Sound(Sound_id);
                    if (result == 1)
                    {
                        if (midi_channel_change < 15)
                        {
                            midi_channel_change++;
                        }
                    }
                    else
                    {
                        if (midi_channel_change > 0)
                        {
                            midi_channel_change--;
                        }
                    }
                    if (midi_channel_change != S_Get_midi_channel_from_Sound(Sound_id))
                    {
                        AudioNoInterrupts();
                        Players_Manager.Multicast_release_players(Sound_id);
                        P_Reset_map_Instrument_for_notes(Instrument_id);
                        S_Set_midi_channel_for_Sound(Sound_id, midi_channel_change);
                        Update_map_Instrument_for_notes(Patch[Patch_id].Instrument[Instrument_id].from_note, Patch[Patch_id].Instrument[Instrument_id].to_note, Instrument_id);
                        Players_Manager.Update_Preset_midi_channel(Patch_id, Instrument_id);
                        AudioInterrupts();

                        Display_Sound.Show_Midi_channel_value(Instrument_id);

                        auto sound_original_0 = S_sound_original;
                        S_sound_original = S_Verify_is_Sound_original(Sound_id);
                        if (sound_original_0 != S_sound_original)
                        {
                            if (Lilla_state_0 != MIDI_LOOP)
                            {
                                S_Select_menu_elements();
                            }
                            Display_Sound.Show_SOUND_menu();
                        }
                    }
                }
            }
            break;

            case value_S_Pitch:
            {
                result = Read_encoder_simple(EN_PB_Value);
                if (result != 0)
                {
                    changed = false;
                    if (result == 1)
                    {
                        if (Sound[Sound_id].pitch < 96)
                        {
                            ++Sound[Sound_id].pitch;
                            changed = true;
                        }
                    }
                    else
                    {
                        if (Sound[Sound_id].pitch > -96)
                        {
                            Sound[Sound_id].pitch--;
                            changed = true;
                        }
                    }
                    if (changed)
                    {
                        AudioNoInterrupts();
                        Players_Manager.Update_Preset_pitch(Patch_id, Instrument_id);
                        Players_Manager.Multicast_pitch_for_sound_edit(Instrument_id);
                        AudioInterrupts();

                        Display_Sound.Show_Pitch_value(Instrument_id);

                        auto sound_original_0 = S_sound_original;
                        S_sound_original = S_Verify_is_Sound_original(Sound_id);
                        if (sound_original_0 != S_sound_original)
                        {
                            if (Lilla_state_0 != MIDI_LOOP)
                            {
                                S_Select_menu_elements();
                            }
                            Display_Sound.Show_SOUND_menu();
                        }
                    }
                }

                else if (Read_pushbutton(EN_PB_Value))
                {
                    if (Sound[Sound_id].pitch == 0)
                    {
                        break;
                    }

                    Sound[Sound_id].pitch = 0;

                    AudioNoInterrupts();
                    Players_Manager.Update_Preset_pitch(Patch_id, Instrument_id);
                    Players_Manager.Multicast_pitch_for_sound_edit(Instrument_id);
                    AudioInterrupts();

                    Display_Sound.Show_Pitch_value(Instrument_id);

                    auto sound_original_0 = S_sound_original;
                    S_sound_original = S_Verify_is_Sound_original(Sound_id);
                    if (sound_original_0 != S_sound_original)
                    {
                        if (Lilla_state_0 != MIDI_LOOP)
                        {
                            S_Select_menu_elements();
                        }
                        Display_Sound.Show_SOUND_menu();
                    }
                }

                // Analyze the selected loop and tune its strongest component.
                else if (Read_pushbutton(EN_PB_Select))
                {
                    const char *auto_tune_error = S_Auto_tune_pitch(Sound_id);
                    if (auto_tune_error == nullptr)
                    {
                        AudioNoInterrupts();
                        Players_Manager.Update_Preset_pitch(Patch_id, Instrument_id);
                        Players_Manager.Multicast_pitch_for_sound_edit(Instrument_id);
                        AudioInterrupts();

                        Display_Sound.Show_Pitch_value(Instrument_id);

                        auto sound_original_0 = S_sound_original;
                        S_sound_original = S_Verify_is_Sound_original(Sound_id);
                        if (sound_original_0 != S_sound_original)
                        {
                            if (Lilla_state_0 != MIDI_LOOP)
                            {
                                S_Select_menu_elements();
                            }
                            Display_Sound.Show_SOUND_menu();
                        }
                        Show_popup_text("AUTO TUNE", ILI9341_BLACK, ILI9341_GREEN);
                        delay(800);
                        Display_Sound.Show_SOUND_page(Patch_id, Instrument_id);
                        Display_Sound.Show_SOUND_menu();
                        Display_Sound.Show_wave(Instrument_id);
                        Pointer_Sound.Display_pointer();
                        Clear_UI_events();
                    }
                    else
                    {
                        Show_popup_text(auto_tune_error, ILI9341_WHITE, ILI9341_RED, 0);
                        delay(800);
                        Display_Sound.Show_SOUND_page(Patch_id, Instrument_id);
                        Display_Sound.Show_SOUND_menu();
                        Display_Sound.Show_wave(Instrument_id);
                        Pointer_Sound.Display_pointer();
                        Clear_UI_events();
                    }
                }
            }
            break;

            case value_S_Gain:
            {
                if (Read_encoder(EN_PB_Value, Sound[Sound_id].gain, 40, 0, 1))
                {
                    AudioNoInterrupts();
                    Players_Manager.Update_Preset_volume(Patch_id, Instrument_id, Patch_volume_gain(volume_patch));
                    Players_Manager.Multicast_volume_for_instrument_edit(Instrument_id);
                    AudioInterrupts();

                    Display_Sound.Show_Gain_value(Patch_id, Instrument_id);
                    Display_Sound.Show_wave(Instrument_id);

                    auto sound_original_0 = S_sound_original;
                    S_sound_original = S_Verify_is_Sound_original(Sound_id);
                    if (sound_original_0 != S_sound_original)
                    {
                        if (Lilla_state_0 != MIDI_LOOP)
                        {
                            S_Select_menu_elements();
                        }
                        Display_Sound.Show_SOUND_menu();
                    }
                }

                // SOLO
                if (Read_pushbutton(EN_PB_Select))
                {
                    if (!solo_flag)
                    {
                        solo_flag = true;
                        AudioNoInterrupts();
                        Players_Manager.Release_all_players_for_instrument_solo(Instrument_id);
                        S_Map_one_Instrument_for_all_notes(Instrument_id);
                        AudioInterrupts();
                    }
                    else
                    {
                        S_Set_Sound_SOLO_OFF();
                    }
                    Display_Sound.Show_wave(Instrument_id);
                }
            }
            break;

            case value_S_Pan:
            {
                if (Read_encoder(EN_PB_Value, Sound[Sound_id].pan, 16, -16, 1))
                {
                    AudioNoInterrupts();
                    Players_Manager.Update_Preset_pan(Patch_id, Instrument_id);
                    Players_Manager.Multicast_pan(Instrument_id);
                    AudioInterrupts();

                    Display_Sound.Show_Pan_value(Instrument_id);

                    auto sound_original_0 = S_sound_original;
                    S_sound_original = S_Verify_is_Sound_original(Sound_id);
                    if (sound_original_0 != S_sound_original)
                    {
                        if (Lilla_state_0 != MIDI_LOOP)
                        {
                            S_Select_menu_elements();
                        }
                        Display_Sound.Show_SOUND_menu();
                    }
                }

                // Set PAN to center
                else if (Read_pushbutton(EN_PB_Value))
                {
                    Sound[Sound_id].pan = 0;

                    AudioNoInterrupts();
                    Players_Manager.Update_Preset_pan(Patch_id, Instrument_id);
                    Players_Manager.Multicast_pan(Instrument_id);
                    AudioInterrupts();

                    Display_Sound.Show_Pan_value(Instrument_id);

                    auto sound_original_0 = S_sound_original;
                    S_sound_original = S_Verify_is_Sound_original(Sound_id);
                    if (sound_original_0 != S_sound_original)
                    {
                        if (Lilla_state_0 != MIDI_LOOP)
                        {
                            S_Select_menu_elements();
                        }
                        Display_Sound.Show_SOUND_menu();
                    }
                }
            }
            break;

            case value_S_Attack:
            {
                if (Read_encoder(EN_PB_Value, Sound[Sound_id].attack, 255, 0, 1))
                {
                    AudioNoInterrupts();
                    Players_Manager.Update_Preset_attack(Patch_id, Instrument_id);
                    AudioInterrupts();

                    Display_Sound.Show_Attack_value(Instrument_id);

                    auto sound_original_0 = S_sound_original;
                    S_sound_original = S_Verify_is_Sound_original(Sound_id);
                    if (sound_original_0 != S_sound_original)
                    {
                        if (Lilla_state_0 != MIDI_LOOP)
                        {
                            S_Select_menu_elements();
                        }
                        Display_Sound.Show_SOUND_menu();
                    }
                }

                // toggle Attack curve
                else if (Read_pushbutton(EN_PB_Value))
                {
                    AudioNoInterrupts();
                    bitWrite(Sound[Sound_id].data, 0, !bitRead(Sound[Sound_id].data, 0));
                    Players_Manager.Update_Preset_attack_type(Patch_id, Instrument_id);
                    AudioInterrupts();

                    Display_Sound.Show_Attack_value(Instrument_id);

                    auto sound_original_0 = S_sound_original;
                    S_sound_original = S_Verify_is_Sound_original(Sound_id);
                    if (sound_original_0 != S_sound_original)
                    {
                        if (Lilla_state_0 != MIDI_LOOP)
                        {
                            S_Select_menu_elements();
                        }
                        Display_Sound.Show_SOUND_menu();
                    }
                }
            }
            break;

            case value_S_Decay:
            {
                if (Read_encoder(EN_PB_Value, Sound[Sound_id].decay, 255, 0, 1))
                {
                    AudioNoInterrupts();
                    Players_Manager.Update_Preset_decay(Patch_id, Instrument_id);
                    AudioInterrupts();

                    Display_Sound.Show_Decay_value(Instrument_id);

                    auto sound_original_0 = S_sound_original;
                    S_sound_original = S_Verify_is_Sound_original(Sound_id);
                    if (sound_original_0 != S_sound_original)
                    {
                        if (Lilla_state_0 != MIDI_LOOP)
                        {
                            S_Select_menu_elements();
                        }
                        Display_Sound.Show_SOUND_menu();
                    }
                }
            }
            break;

            case value_S_Sustain:
            {
                if (Read_encoder(EN_PB_Value, Sound[Sound_id].sustain, 46, 4, 5))
                {
                    AudioNoInterrupts();
                    Players_Manager.Update_Preset_sustain(Patch_id, Instrument_id);
                    AudioInterrupts();

                    Display_Sound.Show_Sustain_value(Instrument_id);

                    auto sound_original_0 = S_sound_original;
                    S_sound_original = S_Verify_is_Sound_original(Sound_id);
                    if (sound_original_0 != S_sound_original)
                    {
                        if (Lilla_state_0 != MIDI_LOOP)
                        {
                            S_Select_menu_elements();
                        }
                        Display_Sound.Show_SOUND_menu();
                    }
                }
            }
            break;

            case value_S_Release:
            {
                if (Read_encoder(EN_PB_Value, Sound[Sound_id].release, 50, 0, 1))
                {
                    AudioNoInterrupts();
                    Players_Manager.Update_Preset_release(Patch_id, Instrument_id);
                    AudioInterrupts();

                    Display_Sound.Show_Release_value(Instrument_id);

                    auto sound_original_0 = S_sound_original;
                    S_sound_original = S_Verify_is_Sound_original(Sound_id);
                    if (sound_original_0 != S_sound_original)
                    {
                        if (Lilla_state_0 != MIDI_LOOP)
                        {
                            S_Select_menu_elements();
                        }
                        Display_Sound.Show_SOUND_menu();
                    }
                }
            }
            break;

            case value_S_PlayMode:
            {
                if (Read_encoder(EN_PB_Value, Sound[Sound_id].mode, 5, 0, 1))
                {
                    AudioNoInterrupts();
                    // Capture the banks referenced before preparing the edit.
                    uint8_t player_banks_before = 0;
                    for (uint8_t player_id = 0; player_id < PLAYERS; ++player_id)
                    {
                        player_banks_before |= Player[player_id].Get_tables_reference_mask();
                    }

                    // Check the updated preset against the tables prepared before this edit.
                    const Preset_struct candidate = Players_Manager.Build_Preset(Patch_id, Instrument_id, Patch_volume_gain(volume_patch));
                    const bool tables_matched_before = Audio_tables.Get_active_pointers(Instrument_id, candidate).bank_mask != 0;

                    const bool tables_rebuilt = S_Fill_tables(Instrument_id);

                    if (!tables_rebuilt)
                    {
                        Sound[Sound_id] = sound_before_edit;
                        Noclick_max = S_Calc_Noclick_max(Preset[Instrument_id].use_Wavetable);
                    }
                    const uint8_t active_bank_mask = tables_rebuilt ? Audio_tables.Get_active_pointers(Instrument_id, Preset[Instrument_id]).bank_mask : 0;

                    if (tables_rebuilt)
                    {
                        Players_Manager.Multicast_main_settings_editing(Patch_id, Instrument_id);
                    }

                    // Capture current and pending references before audio updates resume.
                    uint8_t player_banks_queued = 0;
                    for (uint8_t player_id = 0; player_id < PLAYERS; ++player_id)
                    {
                        player_banks_queued |= Player[player_id].Get_tables_reference_mask();
                    }
                    AudioInterrupts();

                    Serial.print(F("AudioTables player refs, before: 0x"));
                    Serial.print(player_banks_before, HEX);
                    Serial.print(F(", queued: 0x"));
                    Serial.println(player_banks_queued, HEX);
                    // Report both checks after restoring audio interrupts.
                    Serial.print(F("AudioTables edit preset match, before: "));
                    Serial.print(tables_matched_before);
                    Serial.print(F(", after: "));
                    Serial.println(active_bank_mask != 0);

                    // Report the result after restoring audio interrupts.
                    if (tables_rebuilt)
                    {
                        Serial.print(F("AudioTables edit activated, bank mask: 0x"));
                        Serial.println(active_bank_mask, HEX);
                    }
                    else
                    {
                        Serial.println(F("AudioTables edit tables not activated"));
                    }

                    Display_Sound.Show_Play_mode_value(Instrument_id);

                    auto sound_original_0 = S_sound_original;
                    S_sound_original = S_Verify_is_Sound_original(Sound_id);
                    if (sound_original_0 != S_sound_original)
                    {
                        if (Lilla_state_0 != MIDI_LOOP)
                        {
                            S_Select_menu_elements();
                        }
                        Display_Sound.Show_SOUND_menu();
                    }

                    Serial.print("Change MODE: ");
                    Serial.println(Sound[Sound_id].mode);
                }
            }
            break;

            case value_S_Noclick:
            {
                result = Read_encoder_simple(EN_PB_Value);
                auto changed = false;

                if (result != 0)
                {
                    if (result == 1)
                    {
                        if (Sound[Sound_id].Noclick < Noclick_max)
                        {
                            if (Sound[Sound_id].Noclick >= 90)
                            {
                                Sound[Sound_id].Noclick += 10;
                            }
                            else if (Sound[Sound_id].Noclick >= 30)
                            {
                                Sound[Sound_id].Noclick += 4;
                            }
                            else
                            {
                                Sound[Sound_id].Noclick += 2;
                            }
                            changed = true;
                        }
                    }
                    else
                    {
                        if (Sound[Sound_id].Noclick > 0)
                        {
                            if (Sound[Sound_id].Noclick <= 30)
                            {
                                Sound[Sound_id].Noclick -= 2;
                            }
                            else if (Sound[Sound_id].Noclick <= 90)
                            {
                                Sound[Sound_id].Noclick -= 4;
                            }
                            else
                            {
                                Sound[Sound_id].Noclick -= 10;
                            }
                            changed = true;
                        }
                    }
                    if (changed)
                    {
                        AudioNoInterrupts();

                        if (!S_Fill_tables(Instrument_id))
                        {
                            Sound[Sound_id] = sound_before_edit;
                            Noclick_max = S_Calc_Noclick_max(Preset[Instrument_id].use_Wavetable);
                        }
                        else
                        {
                            Players_Manager.Multicast_main_settings_editing(Patch_id, Instrument_id);
                        }
                        AudioInterrupts();

                        S_sound_original = S_Verify_is_Sound_original(Sound_id);

                        Display_Sound.Show_Noclick_value(Instrument_id, true);
                        Display_Sound.Show_wave(Instrument_id);

                        int sound_original_0 = S_sound_original;
                        S_sound_original = S_Verify_is_Sound_original(Sound_id);
                        if (sound_original_0 != S_sound_original)
                        {
                            if (Lilla_state_0 != MIDI_LOOP)
                            {
                                S_Select_menu_elements();
                            }
                            Display_Sound.Show_SOUND_menu();
                        }
                    }
                }
            }
            break;
            }
        }
        break;
        }

        // Change trim speed
        if (Read_encoder(EN_PB_Step, trim_speed, 5, 0, 1))
        {
            S_trim_step = S_Calc_trim_step(trim_speed);
            Display_Sound.Show_Trim_step_value();
        }

        // Toggle TO/SLICE mode with the TO encoder button.
        if (Read_pushbutton(EN_PB_To))
        {
            slicing_mode = !slicing_mode;
            if (!slicing_mode)
            {
                S_slicing_window = Sound[Sound_id].B - Sound[Sound_id].A + 1;
            }
            Display_Sound.Show_wave(Instrument_id);
        }

        if (Read_pushbutton(EN_PB_Step))
        {
            trim_speed = 5;
            S_trim_step = S_Calc_trim_step(trim_speed);
            Display_Sound.Show_Trim_step_value();
        }

        // Change A
        result = Read_encoder_simple(EN_PB_From);
        if (result != 0)
        {
            uint32_t So_A_change = Sound[Sound_id].A;

            if (result == 1)
            {
                if (slicing_mode)
                {
                    if ((Sound[Sound_id].B - Sound[Sound_id].A + 1) >= (S_trim_step + MIN_SNIPPET))
                    {
                        So_A_change = Sound[Sound_id].A + S_trim_step;
                    }
                    else
                    {
                        So_A_change = Sound[Sound_id].B - MIN_SNIPPET + 1;
                    }
                }

                else
                {
                    if ((Sound[Sound_id].A + S_slicing_window + S_trim_step) <= samples_in_file)
                    {
                        So_A_change = Sound[Sound_id].A + S_trim_step;
                    }
                    else
                    {
                        So_A_change = (samples_in_file - S_slicing_window);
                    }
                }
            }

            else
            {
                if (Sound[Sound_id].A >= S_trim_step)
                {
                    So_A_change = Sound[Sound_id].A - S_trim_step;
                }
                else
                {
                    So_A_change = 0;
                }
            }

            if (So_A_change != Sound[Sound_id].A)
            {
                Sound[Sound_id].A = So_A_change;

                AudioNoInterrupts();
                if (!slicing_mode) // slicing A-Samples
                {
                    Sound[Sound_id].B = Sound[Sound_id].A + S_slicing_window - 1;
                }
                if (trim_speed == 5)
                {
                    S_trim_step = S_Calc_trim_step(5);
                }
                // Clamp the candidate crossfade before preparing the replacement bank.

                Noclick_max = S_Calc_Noclick_max(((Sound[Sound_id].B - Sound[Sound_id].A + 1) <= BLOCK_MIN));
                if (Sound[Sound_id].Noclick > Noclick_max)
                {
                    Sound[Sound_id].Noclick = Noclick_max;
                }
                // Publish the replacement tables before queuing the live edit.
                if (!S_Fill_tables(Instrument_id))
                {
                    Sound[Sound_id] = sound_before_edit;
                    Noclick_max = S_Calc_Noclick_max(Preset[Instrument_id].use_Wavetable);
                }
                else
                {
                    Players_Manager.Multicast_main_settings_editing(Patch_id, Instrument_id);
                }
                AudioInterrupts();

                Display_Sound.Show_players_Pitch_max_value(Instrument_id);
                Display_Sound.Show_wave(Instrument_id);

                int sound_original_0 = S_sound_original;
                S_sound_original = S_Verify_is_Sound_original(Sound_id);
                if (sound_original_0 != S_sound_original)
                {
                    if (Lilla_state_0 != MIDI_LOOP)
                    {
                        S_Select_menu_elements();
                    }
                    Display_Sound.Show_SOUND_menu();
                }
            }
        }

        // Set A = 0
        if (Read_pushbutton(EN_PB_From))
        {
            if (Sound[Sound_id].A != 0)
            {
                Sound[Sound_id].A = 0;

                AudioNoInterrupts();
                if (!slicing_mode) // slicing A-Samples
                {
                    Sound[Sound_id].B = Sound[Sound_id].A + S_slicing_window - 1;
                }
                if (trim_speed == 5)
                {
                    S_trim_step = S_Calc_trim_step(5);
                }
                // Clamp the candidate crossfade before preparing the replacement bank.

                Noclick_max = S_Calc_Noclick_max(((Sound[Sound_id].B - Sound[Sound_id].A + 1) <= BLOCK_MIN));
                if (Sound[Sound_id].Noclick > Noclick_max)
                {
                    Sound[Sound_id].Noclick = Noclick_max;
                }

                // Publish the replacement tables before queuing the live edit.
                if (!S_Fill_tables(Instrument_id))
                {
                    Sound[Sound_id] = sound_before_edit;
                    Noclick_max = S_Calc_Noclick_max(Preset[Instrument_id].use_Wavetable);
                }
                else
                {
                    Players_Manager.Multicast_main_settings_editing(Patch_id, Instrument_id);
                }
                AudioInterrupts();

                Display_Sound.Show_players_Pitch_max_value(Instrument_id);
                Display_Sound.Show_wave(Instrument_id);

                int sound_original_0 = S_sound_original;
                S_sound_original = S_Verify_is_Sound_original(Sound_id);
                if (sound_original_0 != S_sound_original)
                {
                    if (Lilla_state_0 != MIDI_LOOP)
                    {
                        S_Select_menu_elements();
                    }
                    Display_Sound.Show_SOUND_menu();
                }
            }
        }

        // Change B by rotating the TO encoder.
        result = Read_encoder_simple(EN_PB_To);
        if (result != 0)
        {
            uint32_t So_B_change;
            if (result == 1)
            {
                if ((Sound[Sound_id].B + 1 + S_trim_step) <= samples_in_file)
                {
                    So_B_change = Sound[Sound_id].B + S_trim_step;
                }
                else
                {
                    So_B_change = samples_in_file - 1;
                }
            }
            else
            {
                if ((Sound[Sound_id].B - Sound[Sound_id].A + 1) >= (MIN_SNIPPET + S_trim_step))
                {
                    So_B_change = Sound[Sound_id].B - S_trim_step;
                }
                else
                {
                    So_B_change = Sound[Sound_id].A + MIN_SNIPPET - 1;
                }
            }

            if (So_B_change != Sound[Sound_id].B)
            {
                Sound[Sound_id].B = So_B_change;

                AudioNoInterrupts();
                if (trim_speed == 5)
                {
                    S_trim_step = S_Calc_trim_step(5);
                }

                // Clamp the candidate crossfade before preparing the replacement bank.

                Noclick_max = S_Calc_Noclick_max(((Sound[Sound_id].B - Sound[Sound_id].A + 1) <= BLOCK_MIN));
                if (Sound[Sound_id].Noclick > Noclick_max)
                {
                    Sound[Sound_id].Noclick = Noclick_max;
                }

                // Publish the replacement tables before queuing the live edit.
                const bool tables_rebuilt = S_Fill_tables(Instrument_id);
                if (!tables_rebuilt)
                {
                    Sound[Sound_id] = sound_before_edit;
                    Noclick_max = S_Calc_Noclick_max(Preset[Instrument_id].use_Wavetable);
                }
                const uint8_t active_bank_mask = tables_rebuilt ? Audio_tables.Get_active_pointers(Instrument_id, Preset[Instrument_id]).bank_mask : 0;
                const int edited_B = Preset[Instrument_id].B;

                if (tables_rebuilt)
                {
                    Players_Manager.Multicast_main_settings_editing(Patch_id, Instrument_id);
                }

                // Capture current and pending references before audio updates resume.
                uint8_t player_banks_queued = 0;
                for (uint8_t player_id = 0; player_id < PLAYERS; ++player_id)
                {
                    player_banks_queued |= Player[player_id].Get_tables_reference_mask();
                }
                AudioInterrupts();

                // Report the trim result after restoring audio interrupts.
                Serial.print(F("AudioTables trim B: "));
                Serial.print(edited_B);
                Serial.print(F(", ready: "));
                Serial.print(tables_rebuilt && active_bank_mask != 0);
                Serial.print(F(", bank: 0x"));
                Serial.print(active_bank_mask, HEX);
                Serial.print(F(", refs: 0x"));
                Serial.println(player_banks_queued, HEX);

                if (!slicing_mode)
                {
                    S_slicing_window = Sound[Sound_id].B - Sound[Sound_id].A + 1;
                }

                Display_Sound.Show_players_Pitch_max_value(Instrument_id);
                Display_Sound.Show_wave(Instrument_id);

                int sound_original_0 = S_sound_original;
                S_sound_original = S_Verify_is_Sound_original(Sound_id);
                if (sound_original_0 != S_sound_original)
                {
                    if (Lilla_state_0 != MIDI_LOOP)
                    {
                        S_Select_menu_elements();
                    }
                    Display_Sound.Show_SOUND_menu();
                }
            }
        }

        // Switch Sound or INSTRUMENT_EDIT
        for (auto Inst_id = 0; Inst_id < INSTRUMENTS; ++Inst_id)
        {
            if (Read_pushbutton(PB_Sound[Inst_id]))
            {
                if (Inst_id == Instrument_id)
                {
                    Lilla_state = INSTRUMENT_VCF;

                    Display_VCF.VCF_show_VCF_page(Patch_id, Instrument_id);

                    // pointer
                    Pointer_VCF.Set_pointer_to_FilterType();

                    Clear_UI_events();

                    // restore all LED
                    Performance_led_set.Restore_all_LED();
                }
                else if (Patch[Patch_id].Instrument[Inst_id].used)
                {
                    AudioNoInterrupts();
                    if (solo_flag)
                    {
                        solo_flag = false;
                        P_Update_all_maps_Instrument_for_notes();
                    }
                    AudioInterrupts();

                    Instrument_id = Inst_id;

                    Sound_id = Get_sound_id(Patch_id, Instrument_id);

                    samples_in_file = Get_samples_in_raw_file(Sound[Sound_id].file);
                    Noclick_max = S_Calc_Noclick_max(((Sound[Sound_id].B - Sound[Sound_id].A + 1) <= BLOCK_MIN));
                    S_trim_step = S_Calc_trim_step(trim_speed);

                    Display_Sound.Show_SOUND_page(Patch_id, Instrument_id);

                    S_sound_original = S_Verify_is_Sound_original(Sound_id);
                    S_Select_menu_elements(); // updates "SO_menu_max" used by encoder_menu
                    Display_Sound.Show_SOUND_menu();

                    // Restore the pointer previously selected in SOUND_EDIT
                    S_pointer = Pointer_Sound.Get_pointer();
                    Pointer_Sound.Display_pointer();

                    Performance_led_set.Restore_all_LED();

                    Display_Sound.Show_wave(Instrument_id);

                    Clear_UI_events();

                    // Report
                    Serial.print("Editing Instrument: ");
                    Serial.print(Instrument_id);
                    Print_Sound(Sound_id);
                }
                else
                {
                    char message[24];
                    snprintf(message, sizeof(message), "SOUND %u IS NOT USED", static_cast<unsigned int>(Inst_id + 1));
                    Show_popup_text_tight(message, "", ILI9341_WHITE, ILI9341_RED, 114);
                    delay(1000);
                    Show_popup_text_tight(message, "", ILI9341_BLACK, ILI9341_BLACK, 114);
                }
            }
        }

        // Switch verso un TOOL
        if (Read_pushbutton(PB_Tools))
        {
            TOOLS_pushbutton = true;
            Shifters_manager.Switch_led(LED_Tools, true);

            switch (Switches_manager.Get_value(SwitchTools))
            {
            case SwToolsMixer:
            {
                S_Set_Sound_SOLO_OFF();
                Switch_to_MIXER();
            }
            break;

            case SwToolsDelay:
            {
                S_Set_Sound_SOLO_OFF();
                Golive_DELAY_SETTINGS();
            }
            break;

            case SwToolsSetup:
            {
                S_Set_Sound_SOLO_OFF();
                Golive_SETUP();
            }
            break;

            case SwToolsTest:
            {
                S_Set_Sound_SOLO_OFF();
                Golive_MIDI_MONITOR();
            }
            break;
            }
        }

        // Switch Mode
        if (Switches_manager.Get_change(SwitchModes))
        {
            switch (Switches_manager.Get_value(SwitchModes))
            {
            case SwModesSampler:
            {
                S_Set_Sound_SOLO_OFF();
                Switch_to_DIRECT_SAMPLING();
            }
            break;

            case SwModesLiveSampler:
            {
                S_Set_Sound_SOLO_OFF();
                Switch_from_PERFORMANCE_to_LIVE_SAMPLING();
            }
            break;

            case SwModesPerformance:
            {
                S_Set_Sound_SOLO_OFF();
                Golive_with_PERFORMANCE(Patch_id);
            }
            break;

            case SwModesMidiLoop:
            {
                S_Set_Sound_SOLO_OFF();

                if (Lilla_state_0 == MIDI_LOOP)
                {
                    Golive_with_MIDI_LOOP(false);
                }
                else
                {
                    Switch_from_PERFORMANCE_to_MIDI_LOOP();
                }
            }
            break;
            }
        }
    }

#pragma endregion // SOUND_EDIT

#pragma region
    // *************************************************************
    // *****************      INSTRUMENT_VCF     *******************
    // *************************************************************
    if (Lilla_state == INSTRUMENT_VCF)
    {

        // Change volume_patch
        if (Read_encoder(EN_PB_LineOutVol, volume_patch, PATCH_VOLUME_MAX, 0, 1))
        {
            AudioNoInterrupts();
            Players_Manager.Update_all_Preset_volume(Patch_id, Patch_volume_gain(volume_patch));
            Players_Manager.Broadcast_volume();
            AudioInterrupts();

            if (Lilla_state_0 == LIVE_SAMPLING)
            {
                Display_Common.P_Patch_volume_value(true); // true: YELLOW
            }
        }

        // Move pointer
        result = Read_encoder_simple(EN_PB_Select);
        if (result != 0)
        {
            Pointer_VCF.Move_pointer(result);

            Clear_UI_events();
        }

        // Change values
        switch (Pointer_VCF.Get_VCF_value_name())
        {

        case value_VCF_Menu:
        {
            if (Read_pushbutton(EN_PB_Select))
            {
                if (Lilla_state_0 == PERFORMANCE)
                {
                    S_Set_Sound_SOLO_OFF();
                    Golive_with_PERFORMANCE(Patch_id);
                }
                else if (Lilla_state_0 == MIDI_LOOP)
                {
                    S_Set_Sound_SOLO_OFF();
                    Golive_with_MIDI_LOOP(false);
                }
                else if (Lilla_state_0 == LIVE_SAMPLING)
                {
                    Golive_with_LIVE_SAMPLING();
                }
            }
        }
        break;

        case value_VCF_Gain_Volume:
        {
            // Volume
            if (Lilla_state_0 == LIVE_SAMPLING)
            {
                if (Read_encoder(EN_PB_Value, volume_patch, PATCH_VOLUME_MAX, 0, 1))
                {
                    AudioNoInterrupts();
                    Players_Manager.Update_all_Preset_volume(Patch_id, Patch_volume_gain(volume_patch));
                    Players_Manager.Broadcast_volume();
                    AudioInterrupts();

                    if (Lilla_state_0 == LIVE_SAMPLING)
                    {
                        Display_Common.P_Patch_volume_value(true); // true: YELLOW
                    }
                }
            }

            // Gain
            else
            {
                if (Read_encoder(EN_PB_Value, Sound[Sound_id].gain, 40, 0, 1))
                {
                    AudioNoInterrupts();
                    Players_Manager.Update_Preset_volume(Patch_id, Instrument_id, Patch_volume_gain(volume_patch));
                    Players_Manager.Multicast_volume_for_instrument_edit(Instrument_id);
                    AudioInterrupts();

                    Display_VCF.VCF_show_sound_gain_value(Sound_id);
                }

                // Solo
                if (Read_pushbutton(EN_PB_Value))
                {
                    AudioNoInterrupts();
                    Players_Manager.Release_all_players_for_instrument_solo(Instrument_id);
                    S_Map_one_Instrument_for_all_notes(Instrument_id);
                    AudioInterrupts();

                    Display_VCF.VCF_show_solo_value();
                }
            }
        }
        break;

        case value_VCF_FilterType:
        {
            if (Read_encoder(EN_PB_Value, Patch[Patch_id].Instrument[Instrument_id].Filter.type, 3, 0, 1))
            {
                if (Lilla_state_0 == LIVE_SAMPLING)
                {
                    Patch[Patch_id].Instrument[1].Filter.type = Patch[Patch_id].Instrument[0].Filter.type;
                    AudioNoInterrupts();
                    Players_Manager.Update_Preset_IF(Patch_id, 0);
                    Players_Manager.Update_Preset_IF(Patch_id, 1);
                    Players_Manager.Multicast_IF_update_filter_type(0);
                    Players_Manager.Multicast_IF_update_filter_type(1);
                    AudioInterrupts();
                }
                else
                {
                    AudioNoInterrupts();
                    Players_Manager.Update_Preset_IF(Patch_id, Instrument_id);
                    Players_Manager.Multicast_IF_update_filter_type(Instrument_id);
                    AudioInterrupts();
                }

                Display_VCF.VCF_show_filter_type_value(Instrument_id);
            }

            // Exclude VCF
            else if (Read_pushbutton(EN_PB_Value) || Read_pushbutton(EN_PB_Select))
            {
                Macro_VCF_filter_on_none();
                Display_VCF.VCF_show_filter_type_value(Instrument_id);
            }
        }
        break;

        case value_VCF_Cutoff:
        {
            if (Read_encoder(EN_PB_Value, Patch[Patch_id].Instrument[Instrument_id].Filter.pivot, 100, 0, 1))
            {
                AudioNoInterrupts();
                Players_Manager.Update_Preset_IF(Patch_id, Instrument_id);
                if (Preset[Instrument_id].Filter.use == 1)
                {
                    Players_Manager.Multicast_IF_pivot(Instrument_id);
                }

                if (Lilla_state_0 == LIVE_SAMPLING)
                {
                    Patch[Patch_id].Instrument[1].Filter.pivot = Patch[Patch_id].Instrument[0].Filter.pivot;
                    Instrument_id = 1;

                    Players_Manager.Update_Preset_IF(Patch_id, Instrument_id);
                    if (Preset[Instrument_id].Filter.use == 1)
                    {
                        Players_Manager.Multicast_IF_pivot(Instrument_id);
                    }

                    Instrument_id = 0;
                }
                AudioInterrupts();

                Display_VCF.VCF_show_cutoff_value(Instrument_id);
            }
        }
        break;

        case value_VCF_Resonance:
        {
            if (Read_encoder(EN_PB_Value, Patch[Patch_id].Instrument[Instrument_id].Filter.resonance, 40, 0, 1))
            {

                AudioNoInterrupts();
                Players_Manager.Update_IF_resonance(Patch_id, Instrument_id);
                if (Lilla_state_0 == LIVE_SAMPLING)
                {
                    Patch[Patch_id].Instrument[1].Filter.resonance = Patch[Patch_id].Instrument[0].Filter.resonance;
                    Instrument_id = 1;
                    Players_Manager.Update_IF_resonance(Patch_id, Instrument_id);
                    Instrument_id = 0;
                }
                AudioInterrupts();

                Display_VCF.VCF_show_resonance_value(Instrument_id);
            }
        }
        break;

        case value_VCF_LfoModulationType:
        {
            if (Read_encoder(EN_PB_Value, Patch[Patch_id].Instrument[Instrument_id].Filter.modulation, 4, 0, 1))
            {
                if (Lilla_state_0 == LIVE_SAMPLING)
                {
                    Patch[Patch_id].Instrument[1].Filter.modulation = Patch[Patch_id].Instrument[0].Filter.modulation;
                    AudioNoInterrupts();
                    Players_Manager.Update_Preset_IF_modulation(Patch_id, 0);
                    Players_Manager.Update_Preset_IF_modulation(Patch_id, 1);
                    AudioInterrupts();
                }
                else
                {
                    AudioNoInterrupts();
                    Players_Manager.Update_Preset_IF_modulation(Patch_id, Instrument_id);
                    AudioInterrupts();
                }

                Display_VCF.VCF_show_LFO_modulation_source(Instrument_id);
            }

            // Exclude VCF
            else if (Read_pushbutton(EN_PB_Value) || Read_pushbutton(EN_PB_Select))
            {
                Macro_VCF_modulation_none();
                Display_VCF.VCF_show_LFO_modulation_source(Instrument_id);
            }
        }
        break;

        case value_VCF_LfoModFreqTime:
        {
            if (Read_encoder(EN_PB_Value, Patch[Patch_id].Instrument[Instrument_id].Filter.frequency_time, 40, 0, 1))
            {
                AudioNoInterrupts();
                Players_Manager.Update_Preset_IF(Patch_id, Instrument_id);
                if ((Preset[Instrument_id].Filter.use == 1) && (Preset[Instrument_id].Filter.modulation > 0) && (Preset[Instrument_id].Filter.periodic == 1))
                {
                    Players_Manager.Multicast_IF_frequency_filter(Instrument_id);
                }

                if (Lilla_state_0 == LIVE_SAMPLING)
                {
                    Patch[Patch_id].Instrument[1].Filter.frequency_time = Patch[Patch_id].Instrument[0].Filter.frequency_time;
                    Instrument_id = 1;
                    Players_Manager.Update_Preset_IF(Patch_id, Instrument_id);
                    if ((Preset[Instrument_id].Filter.use == 1) && (Preset[Instrument_id].Filter.modulation > 0) && (Preset[Instrument_id].Filter.periodic == 1))
                    {
                        Players_Manager.Multicast_IF_frequency_filter(Instrument_id);
                    }

                    Instrument_id = 0;
                }
                AudioInterrupts();

                Display_VCF.VCF_show_LFO_freq_time(Instrument_id);
            }
        }
        break;

        case value_VCF_LfoModDepth:
        {
            if (Read_encoder(EN_PB_Value, Patch[Patch_id].Instrument[Instrument_id].Filter.index, 20, 0, 1))
            {
                AudioNoInterrupts();
                Players_Manager.Update_Preset_IF_index(Patch_id, Instrument_id);

                if (Lilla_state_0 == LIVE_SAMPLING)
                {
                    Patch[Patch_id].Instrument[1].Filter.index = Patch[Patch_id].Instrument[0].Filter.index;
                    Instrument_id = 1;
                    Players_Manager.Update_Preset_IF_index(Patch_id, Instrument_id);
                    Instrument_id = 0;
                }
                AudioInterrupts();

                Display_VCF.VCF_show_LFO_modulation_depth(Instrument_id);
            }
        }
        break;

        default:
            break;
        }

        // Switch Sound or INSTRUMENT_EDIT
        if (Lilla_state_0 == PERFORMANCE)
        {
            for (auto Inst_id = 0; Inst_id < INSTRUMENTS; ++Inst_id)
            {
                if (Read_pushbutton(PB_Sound[Inst_id]))
                {
                    if (Inst_id == Instrument_id)
                    {
                        S_Set_Sound_SOLO_OFF();
                        Golive_with_PERFORMANCE(Patch_id);
                    }

                    else if (Patch[Patch_id].Instrument[Inst_id].used)
                    {
                        AudioNoInterrupts();
                        if (solo_flag)
                        {
                            solo_flag = false;
                            P_Update_all_maps_Instrument_for_notes();
                        }
                        AudioInterrupts();

                        Instrument_id = Inst_id;

                        Lilla_state = SOUND_EDIT;

                        Sound_id = Get_sound_id(Patch_id, Instrument_id);

                        samples_in_file = Get_samples_in_raw_file(Sound[Sound_id].file);
                        Noclick_max = S_Calc_Noclick_max(Preset[Instrument_id].use_Wavetable);
                        S_trim_step = S_Calc_trim_step(trim_speed);

                        Display_Sound.Show_SOUND_page(Patch_id, Instrument_id);

                        S_sound_original = S_Verify_is_Sound_original(Sound_id);
                        S_Select_menu_elements();
                        Display_Sound.Show_SOUND_menu(); // displays the menu and updates "SO_menu_max" used by encoder_menu

                        // Select RETURN when entering SOUND_EDIT.
                        Pointer_Sound.Set_pointer_to_first_menu_element();
                        S_pointer = Pointer_Sound.Get_pointer();
                        Pointer_Sound.Display_pointer();

                        Performance_led_set.Restore_all_LED();

                        Display_Sound.Show_wave(Instrument_id);

                        Clear_UI_events();
                    }

                    else
                    {
                        char message[24];
                        snprintf(message, sizeof(message), "SOUND %u IS NOT USED", static_cast<unsigned int>(Inst_id + 1));
                        Show_popup_text_tight(message, "", ILI9341_WHITE, ILI9341_RED, 114);
                        delay(1000);
                        Show_popup_text_tight(message, "", ILI9341_BLACK, ILI9341_BLACK, 114);
                    }
                }
            }
        }

        else if (Lilla_state_0 == LIVE_SAMPLING)
        {
            if (LS_stereo)
            {
                // VCF left channel
                if (Read_pushbutton(PB_S1))
                {
                    if (Instrument_id == 0)
                    {
                        LS_instrument = 0;        // Left
                        LS_sound_id = SOUNDS_MAX; // Left
                        Golive_with_LIVE_SAMPLING();
                    }

                    else
                    {
                        Instrument_id = 0;
                        Sound_id = SOUNDS_MAX;

                        Display_VCF.VCF_show_VCF_page(Patch_id, Instrument_id);

                        // restore LED
                        Performance_led_set.Restore_all_LED();
                    }
                }

                // VCF right channel
                if (Read_pushbutton(PB_S2))
                {
                    if (Instrument_id == 1)
                    {
                        LS_instrument = 1;            // Right
                        LS_sound_id = SOUNDS_MAX + 1; // Right
                        Golive_with_LIVE_SAMPLING();
                    }

                    else
                    {
                        Instrument_id = 1;
                        Sound_id = SOUNDS_MAX + 1;
                        Display_VCF.VCF_show_VCF_page(Patch_id, Instrument_id);

                        // restore LED
                        Performance_led_set.Restore_all_LED();
                    }
                }
            }

            else
            {
                // VCF left channel
                if (Read_pushbutton(PB_S1))
                {
                    Golive_with_LIVE_SAMPLING();
                }

                // VCF right channel
                else if (Read_pushbutton(PB_S2))
                {
                    Golive_with_LIVE_SAMPLING();
                }
            }
        }

        else if (Lilla_state_0 == MIDI_LOOP)
        {
            for (auto Inst_id = 0; Inst_id < INSTRUMENTS; ++Inst_id)
            {
                if (Read_pushbutton(PB_Sound[Inst_id]))
                {
                    if (Inst_id == Instrument_id)
                    {
                        S_Set_Sound_SOLO_OFF();
                        Golive_with_MIDI_LOOP(false);
                    }

                    else if (Patch[Patch_id].Instrument[Inst_id].used)
                    {
                        AudioNoInterrupts();
                        if (solo_flag)
                        {
                            solo_flag = false;
                            P_Update_all_maps_Instrument_for_notes();
                        }
                        AudioInterrupts();

                        Instrument_id = Inst_id;

                        Lilla_state = SOUND_EDIT;

                        Sound_id = Get_sound_id(Patch_id, Instrument_id);

                        samples_in_file = Get_samples_in_raw_file(Sound[Sound_id].file);
                        Noclick_max = S_Calc_Noclick_max(Preset[Instrument_id].use_Wavetable);
                        S_trim_step = S_Calc_trim_step(trim_speed);

                        Display_Sound.Show_SOUND_page(Patch_id, Instrument_id);

                        S_sound_original = S_Verify_is_Sound_original(Sound_id);
                        S_Select_menu_elements();
                        Display_Sound.Show_SOUND_menu(); // displays the menu and updates "SO_menu_max" used by encoder_menu

                        // Select RETURN when entering SOUND_EDIT.
                        Pointer_Sound.Set_pointer_to_first_menu_element();
                        S_pointer = Pointer_Sound.Get_pointer();
                        Pointer_Sound.Display_pointer();

                        Performance_led_set.Restore_all_LED();

                        Display_Sound.Show_wave(Instrument_id);

                        Clear_UI_events();
                    }

                    else
                    {
                        char message[24];
                        snprintf(message, sizeof(message), "SOUND %u IS NOT USED", static_cast<unsigned int>(Inst_id + 1));
                        Show_popup_text_tight(message, "", ILI9341_WHITE, ILI9341_RED, 114);
                        delay(1000);
                        Show_popup_text_tight(message, "", ILI9341_BLACK, ILI9341_BLACK, 114);
                    }
                }
            }
        }

        // Switch verso un TOOL
        if (Read_pushbutton(PB_Tools))
        {
            TOOLS_pushbutton = true;
            Shifters_manager.Switch_led(LED_Tools, true);

            switch (Switches_manager.Get_value(SwitchTools))
            {
            case SwToolsMixer:
            {
                Switch_to_MIXER();
            }
            break;

            case SwToolsDelay:
            {
                if (Lilla_state_0 == PERFORMANCE)
                {
                    S_Set_Sound_SOLO_OFF();
                }
                else // Lilla_state_0 == LIVE_SAMPLING
                {
                    if (Delay_values.instrument_route[0] || Delay_values.instrument_route[1])
                    {
                        Delay_values.instrument_route[0] = true;
                        Delay_values.instrument_route[1] = true;
                    }
                }

                Golive_DELAY_SETTINGS();
            }
            break;

            case SwToolsSetup:
            {
                Golive_SETUP();
            }
            break;

            case SwToolsTest:
            {
                Golive_MIDI_MONITOR();
            }
            break;
            }
        }

        // Switch Mode
        if (Switches_manager.Get_change(SwitchModes))
        {
            switch (Switches_manager.Get_value(SwitchModes))
            {
            case SwModesSampler:
            {
                if (Lilla_state_0 == PERFORMANCE)
                {
                    S_Set_Sound_SOLO_OFF();
                    Switch_to_DIRECT_SAMPLING();
                }
                else // Lilla_state_0 == LIVE_SAMPLING
                {
                    Switch_from_LIVE_SAMPLING_to_DIRECT_SAMPLING();
                }
            }
            break;

            case SwModesLiveSampler:
            {
                if (Lilla_state_0 == PERFORMANCE)
                {
                    S_Set_Sound_SOLO_OFF();
                    Switch_from_PERFORMANCE_to_LIVE_SAMPLING();
                }
                else // Lilla_state_0 == LIVE_SAMPLING
                {
                    Golive_with_LIVE_SAMPLING();
                }
            }
            break;

            case SwModesPerformance:
            {
                if (Lilla_state_0 == PERFORMANCE)
                {
                    // S_Set_Sound_SOLO_OFF();
                    // Golive_with_PERFORMANCE(Patch_id);
                }
                else
                {
                    Switch_from_LIVE_SAMPLING_to_PERFORMANCE();
                }
            }
            break;

            case SwModesMidiLoop:
            {
                S_Set_Sound_SOLO_OFF();
                Switch_from_PERFORMANCE_to_MIDI_LOOP();
            }
            break;
            }
        }

    } // end INSTRUMENT_VCF

#pragma endregion // INSTRUMENT_VCF

#pragma region Mixer [rgba(130, 4, 40, 0.19)]
    // *************************************************************
    // ********************        MIXER      **********************
    // *************************************************************
    if (Lilla_state == MIXER)
    {
        if (Lilla_state_0 == PERFORMANCE || (Lilla_state_0 == DIRECT_SAMPLING && DS_state == DS_waiting_state) || Lilla_state_0 == LIVE_SAMPLING)
        {
            if (Read_encoder(EN_PB_LineOutVol, volume_patch, PATCH_VOLUME_MAX, 0, 1))
            {
                AudioNoInterrupts();
                Players_Manager.Update_all_Preset_volume(Patch_id, Patch_volume_gain(volume_patch));
                Players_Manager.Broadcast_volume();
                AudioInterrupts();
            }
        }

        // Move pointer
        result = Read_encoder_simple(EN_PB_Select);
        if (result != 0)
        {
            Pointer_Mixer.Move_pointer(result);
            MX_local_pointer = Pointer_Mixer.Get_pointer();

            Clear_UI_events();

            Instrument_id = (MX_local_pointer.source < LINE_IN_source ? MX_local_pointer.source : 0);
            Sound_id = Get_sound_id(Patch_id, Instrument_id);
        }

        // Change values
        if (MX_local_pointer.field_name == field_MX_Source)
        {
            // Enter inside
            if (Read_pushbutton(EN_PB_Select))
            {
                Pointer_Mixer.Move_pointer_to_field_MX_Elements();
                MX_local_pointer = Pointer_Mixer.Get_pointer();

                Clear_UI_events();
            }
        }

        else if (MX_local_pointer.field_name == field_MX_Elements)
        {
            // Exit to Source
            if (Read_pushbutton(EN_PB_Select))
            {
                Pointer_Mixer.Move_pointer_to_field_MX_Source();
                MX_local_pointer = Pointer_Mixer.Get_pointer();

                Clear_UI_events();
            }

            switch (MX_local_pointer.element)
            {
            case value_MX_Mute_Gain:
            {
                if (MX_local_pointer.source == LINE_IN_source) // MX_source == LINE_IN_CHANNEL
                {
                    if (Read_encoder(EN_PB_Value, Line_in_gain, 15, 0, 1))
                    {
                        Audio_shield.lineInLevel(Line_in_gain);

                        Display_Mixer.MX_source_values_edit(LINE_IN_source);
                    }
                }
                else
                {
                    if (Read_encoder(EN_PB_Value, Sound[Sound_id].gain, 40, 0, 1))
                    {
                        AudioNoInterrupts();
                        Players_Manager.Update_Preset_volume(Patch_id, Instrument_id, Patch_volume_gain(volume_patch));
                        Players_Manager.Multicast_volume_for_instrument_edit(Instrument_id);
                        AudioInterrupts();

                        Display_Mixer.MX_source_values_edit(Instrument_id);
                    }
                }

                if (Read_pushbutton(EN_PB_Value)) // Mute source
                {
                    MX_mute[MX_local_pointer.source] = !MX_mute[MX_local_pointer.source];

                    if (MX_local_pointer.source == LINE_IN_source)
                    {
                        if (MX_mute[MX_local_pointer.source])
                        {
                            MAIN_mixer_out_L.Mute(1);
                            MAIN_mixer_out_R.Mute(1);
                            PWM_mixer_out_L.Mute(1);
                            PWM_mixer_out_R.Mute(1);
                        }
                        else
                        {
                            MAIN_mixer_out_L.unmute(1);
                            MAIN_mixer_out_R.unmute(1);
                            PWM_mixer_out_L.unmute(1);
                            PWM_mixer_out_R.unmute(1);
                        }
                    }
                    else
                    {
                        AudioNoInterrupts();
                        Players_Manager.Update_Preset_volume(Patch_id, Instrument_id, Patch_volume_gain(volume_patch));
                        Players_Manager.Multicast_volume_for_instrument_edit(Instrument_id);
                        AudioInterrupts();
                    }

                    Display_Mixer.MX_source_values_edit(MX_local_pointer.source);
                }
            }
            break;

            case value_MX_Pan:
            {
                if (MX_local_pointer.source == LINE_IN_source)
                {
                    // not supported
                }
                else
                {
                    if (Read_encoder(EN_PB_Value, Sound[Sound_id].pan, 16, -16, 1))
                    {
                        AudioNoInterrupts();
                        Players_Manager.Update_Preset_pan(Patch_id, Instrument_id);
                        Players_Manager.Multicast_pan(Instrument_id);
                        AudioInterrupts();

                        Display_Mixer.MX_source_values_edit(Instrument_id);
                    }
                }
            }
            break;

            case value_MX_Lineout:
            {
                if (Read_pushbutton(EN_PB_Value))
                {
                    if (MX_routing_source[MX_local_pointer.source] == 0) // era tutto muto --> solo MAIN
                    {
                        MX_routing_source[MX_local_pointer.source] = 2;
                    }
                    else if (MX_routing_source[MX_local_pointer.source] == 1) // era solo MONITOR --> MONITOR e MAIN
                    {
                        MX_routing_source[MX_local_pointer.source] = 3;
                    }
                    else if (MX_routing_source[MX_local_pointer.source] == 2) // era solo MAIN --> tutto muto
                    {
                        MX_routing_source[MX_local_pointer.source] = 0;
                    }
                    else // era 3 (MONITOR e MAIN) --> solo MONITOR
                    {
                        MX_routing_source[MX_local_pointer.source] = 1;
                    }

                    if (MX_local_pointer.source == LINE_IN_source)
                    {
                        switch (MX_routing_source[MX_local_pointer.source])
                        {
                        case 0:
                            MAIN_mixer_out_L.Mute(1);
                            MAIN_mixer_out_R.Mute(1);
                            break;

                        case 1:
                            MAIN_mixer_out_L.Mute(1);
                            MAIN_mixer_out_R.Mute(1);
                            break;

                        case 2:
                            MAIN_mixer_out_L.unmute(1);
                            MAIN_mixer_out_R.unmute(1);
                            break;

                        case 3:
                            MAIN_mixer_out_L.unmute(1);
                            MAIN_mixer_out_R.unmute(1);
                            break;

                        default:
                            PRINT_ERROR(F("Switch MISSING! "));
                            break;
                        }
                    }
                    else
                    {
                        AudioNoInterrupts();
                        Players_Manager.MX_multicast_change_routing(Instrument_id);
                        AudioInterrupts();
                    }

                    Display_Mixer.MX_source_values_edit(MX_local_pointer.source);
                }
            }
            break;

            case value_MX_Monitor:
            {
                if (Read_pushbutton(EN_PB_Value))
                {
                    if (MX_routing_source[MX_local_pointer.source] == 0) // era tutto muto --> solo MONITOR
                    {
                        MX_routing_source[MX_local_pointer.source] = 1;
                    }
                    else if (MX_routing_source[MX_local_pointer.source] == 1) // era solo MONITOR --> tutto muto
                    {
                        MX_routing_source[MX_local_pointer.source] = 0;
                    }
                    else if (MX_routing_source[MX_local_pointer.source] == 2) // era solo MAIN --> MONITOR e MAIN
                    {
                        MX_routing_source[MX_local_pointer.source] = 3;
                    }
                    else // era 3 (MONITOR e MAIN) --> solo MAIN
                    {
                        MX_routing_source[MX_local_pointer.source] = 2;
                    }

                    if (MX_local_pointer.source == LINE_IN_source)
                    {
                        switch (MX_routing_source[MX_local_pointer.source])
                        {
                        case 0:
                            PWM_mixer_out_L.Mute(1);
                            PWM_mixer_out_R.Mute(1);
                            break;

                        case 1:
                            PWM_mixer_out_L.unmute(1);
                            PWM_mixer_out_R.unmute(1);
                            break;

                        case 2:
                            PWM_mixer_out_L.Mute(1);
                            PWM_mixer_out_R.Mute(1);
                            break;

                        case 3:
                            PWM_mixer_out_L.unmute(1);
                            PWM_mixer_out_R.unmute(1);
                            break;

                        default:
                            PRINT_ERROR(F("Switch MISSING! "));
                            break;
                        }
                    }

                    else
                    {
                        AudioNoInterrupts();
                        Players_Manager.MX_multicast_change_routing(Instrument_id);
                        AudioInterrupts();
                    }

                    Display_Mixer.MX_source_values_edit(MX_local_pointer.source);
                }
            }
            break;

            default:
                break;
            }
        }

        // Switch verso un TOOL
        if (Switches_manager.Get_change(SwitchTools))
        {
            switch (Switches_manager.Get_value(SwitchTools))
            {
            case SwToolsMixer:
                break;

            case SwToolsDelay:
            {
                Golive_DELAY_SETTINGS();
            }
            break;

            case SwToolsSetup:
            {
                Golive_SETUP();
            }
            break;

            case SwToolsTest:
            {
                Golive_MIDI_MONITOR();
            }
            break;
            }
        }

        // Switch Mode
        if (Read_pushbutton(PB_Tools))
        {
            TOOLS_pushbutton = false;
            Shifters_manager.Switch_led(LED_Tools, false);

            switch (Switches_manager.Get_value(SwitchModes))
            {
            case SwModesSampler:
            {
                switch (Lilla_state_0)
                {
                case PERFORMANCE:
                    Switch_to_DIRECT_SAMPLING();
                    break;

                case DIRECT_SAMPLING:
                    Lilla_state = DIRECT_SAMPLING;

                    Display_Sampler.DS_page_upper();
                    Display_Sampler.DS_page_lower(recording);

                    // Menu
                    DS_define_menu();
                    Display_Sampler.DS_menu(); // display the menu and updates DS_menu_max

                    // Pointer
                    Pointer_Sampler.Set_pointer_to_first_menu_element();
                    DS_local_pointer = Pointer_Sampler.Get_pointer();

                    // Display the VU meter
                    Display_Sampler.DS_bar(0, 0);
                    Display_Sampler.DS_bar(1, 0);

                    Clear_UI_events();
                    break;

                case LIVE_SAMPLING:
                    Switch_from_LIVE_SAMPLING_to_DIRECT_SAMPLING();
                    break;

                default:
                    PRINT_ERROR(F("Switch MISSING! "));
                    break;
                }
            }
            break;

            case SwModesLiveSampler:
            {
                switch (Lilla_state_0)
                {
                case PERFORMANCE:
                    Switch_from_PERFORMANCE_to_LIVE_SAMPLING();
                    break;

                case DIRECT_SAMPLING:
                    Switch_from_DIRECT_SAMPLING_to_LIVE_SAMPLING();
                    break;

                case LIVE_SAMPLING:
                    LS_refresh_LS_page();
                    break;

                default:
                    PRINT_ERROR(F("Switch MISSING! "));
                    break;
                }
            }
            break;

            case SwModesPerformance:
            {
                switch (Lilla_state_0)
                {
                case PERFORMANCE:
                    Golive_with_PERFORMANCE(Patch_id);
                    break;

                case DIRECT_SAMPLING:
                    Switch_from_DIRECT_SAMPLING_to_PERFORMANCE();
                    break;

                case LIVE_SAMPLING:
                    Switch_from_LIVE_SAMPLING_to_PERFORMANCE();
                    break;

                default:
                    PRINT_ERROR(F("Switch MISSING! "));
                    break;
                }
            }
            break;

            case SwModesMidiLoop:
            {
                switch (Lilla_state_0)
                {
                case PERFORMANCE:
                    Switch_from_PERFORMANCE_to_MIDI_LOOP();
                    break;

                case DIRECT_SAMPLING:
                    Switch_from_DIRECT_SAMPLING_to_MIDI_LOOP();
                    break;

                case LIVE_SAMPLING:
                    Switch_from_LIVE_SAMPLING_to_MIDI_LOOP();
                    break;

                case MIDI_LOOP:
                    Golive_with_MIDI_LOOP(false);
                    break;

                default:
                    PRINT_ERROR(F("Switch MISSING! "));
                    break;
                }
            }
            break;
            }
        }
    }

#pragma endregion // MIXER

#pragma region Delay [rgba(16, 86, 208, 0.25)]
    // *************************************************************
    // ********************   DELAY_SETTINGS  **********************
    // *************************************************************
    if (Lilla_state == DELAY_SETTINGS && Lilla_state_0 != DIRECT_SAMPLING)
    {
        // Change Patch VOLUME
        if (Read_encoder(EN_PB_LineOutVol, volume_patch, PATCH_VOLUME_MAX, 0, 1))
        {
            AudioNoInterrupts();
            Players_Manager.Update_all_Preset_volume(Patch_id, Patch_volume_gain(volume_patch));
            Players_Manager.Broadcast_volume();
            AudioInterrupts();

            Display_Common.P_Patch_volume_value(true);
        }

        // Move pointer
        result = Read_encoder_simple(EN_PB_Select);
        if (result != 0)
        {
            Pointer_Delay.Move_pointer(result);
            DELAY_local_pointer = Pointer_Delay.Get_element_name();

            Clear_UI_events();
        }

        // Change values
        switch (DELAY_local_pointer)
        {
        case value_DELAY_Feedback:
            if (D_Read_value(LOOP_GAIN))
            {
                Display_Delay.D_feedback();
            }
            break;
        case value_DELAY_Delay_time:
            if (D_Read_value(SAMPLES))
            {
                Display_Delay.D_delay_time();
                Display_Delay.D_delay_time_LR(); // The shorter base time may also reduce the stereo offset.
            }
            break;
        case value_DELAY_Delay_time_LR:
            if (D_Read_value(SAMPLES_LR))
            {
                Display_Delay.D_delay_time_LR();
            }
            break;
        case value_DELAY_Modulation_source:
            if (D_Read_value(MODULATION_SOURCE))
            {
                Display_Delay.D_modulation_source();
            }
            else if (Read_pushbutton(EN_PB_Value))
            {
                D_Set_value(MODULATION_SOURCE, 0); // Cancel any pending source selection before displaying NONE.
                Display_Delay.D_modulation_source();
            }
            break;
        case value_DELAY_Modulation_frequency:
            if (D_Read_value(MODULATION_FREQUENCY))
            {
                Display_Delay.D_modulation_frequency();
            }
            break;
        case value_DELAY_Modulation_depth:
            if (D_Read_value(MODULATION_DEPTH))
            {
                Display_Delay.D_modulation_depth();
            }
            else if (Read_pushbutton(EN_PB_Value))
            {
                D_Set_value(MODULATION_DEPTH, 0); // Fade toward zero depth through the shared transition manager.
                Display_Delay.D_modulation_depth();
            }
            break;
        case value_DELAY_Modulation_phase_LR:
            if (D_Read_value(MODULATION_PHASE_LR))
            {
                Display_Delay.D_modulation_phase_LR();
            }
            else if (Read_pushbutton(EN_PB_Value))
            {
                D_Set_value(MODULATION_PHASE_LR, 0);
                Display_Delay.D_modulation_phase_LR();
            }
            break;
        default:
            break;
        }

        // Toggle requested routing bits; the audio callback updates every affected voice.
        for (auto Inst_id = 0; Inst_id < INSTRUMENTS; ++Inst_id)
        {
            if (Read_pushbutton(PB_Sound[Inst_id]))
            {
                const int route = Delay_manager.Get_value(INSTRUMENT_ROUTE);
                const int next_route = Lilla_state_0 == LIVE_SAMPLING ? ((route & 3) != 0 ? route & ~3 : route | 3) : route ^ (1 << Inst_id);
                D_Set_value(INSTRUMENT_ROUTE, next_route); // Live sampling enables or disables both channels together, even when the stored bits differ.
                Display_Delay.D_sounds();
            }
        }
    }

    // Navigation remains available while the Direct Sampler locks the Delay controls.
    if (Lilla_state == DELAY_SETTINGS)
    {
        // Switch Mode
        if (Read_pushbutton(PB_Tools))
        {
            TOOLS_pushbutton = false;
            Shifters_manager.Switch_led(LED_Tools, false);

            switch (Switches_manager.Get_value(SwitchModes))
            {
            case SwModesSampler:
            {
                if (Patch_id < PATCHES_MAX)
                {
                    Require_FRAM(Archive.Save_Delay(Patch_id, Delay_data));
                }
                switch (Lilla_state_0)
                {
                case PERFORMANCE:
                    Switch_to_DIRECT_SAMPLING();
                    break;

                case DIRECT_SAMPLING:
                    Lilla_state = DIRECT_SAMPLING;

                    Display_Sampler.DS_page_upper();
                    Display_Sampler.DS_page_lower(recording);

                    // Menu
                    DS_define_menu();
                    Display_Sampler.DS_menu(); // display the menu and updates DS_menu_max

                    // Pointer
                    Pointer_Sampler.Set_pointer_to_first_menu_element();
                    DS_local_pointer = Pointer_Sampler.Get_pointer();

                    // Display the VU meter
                    Display_Sampler.DS_bar(0, 0);
                    Display_Sampler.DS_bar(1, 0);

                    Clear_UI_events();
                    break;

                case LIVE_SAMPLING:
                    Switch_from_LIVE_SAMPLING_to_DIRECT_SAMPLING();
                    break;

                case MIDI_LOOP:
                    // Esci da MIDI_LOOP

                    AudioNoInterrupts();
                    // Ferma i track running
                    for (auto local_track = 0; local_track < TRACKS; ++local_track) // true --> il track va suonato
                    {
                        LOOP_track_run[local_track] = false;
                    }

                    // Ferma i Player dei loop
                    Players_Manager.Release_all_players_loop();
                    AudioInterrupts();

                    Loop_led_set.Request_all_LED_switch_off();

                    Switch_to_DIRECT_SAMPLING();
                    break;

                default:
                    PRINT_ERROR(F("Switch MISSING! "));
                    break;
                }
            }
            break;

            case SwModesLiveSampler:
            {
                if (Patch_id < PATCHES_MAX)
                {
                    Require_FRAM(Archive.Save_Delay(Patch_id, Delay_data));
                }
                switch (Lilla_state_0)
                {
                case PERFORMANCE:
                    Switch_from_PERFORMANCE_to_LIVE_SAMPLING();
                    break;

                case DIRECT_SAMPLING:
                    Switch_from_DIRECT_SAMPLING_to_LIVE_SAMPLING();
                    break;

                case LIVE_SAMPLING:
                    LS_refresh_LS_page();
                    break;

                case MIDI_LOOP:
                    // Esci da MIDI_LOOP

                    AudioNoInterrupts();
                    // Ferma i track running
                    for (auto local_track = 0; local_track < TRACKS; ++local_track) // true --> il track va suonato
                    {
                        LOOP_track_run[local_track] = false;
                    }

                    // Ferma i Player dei track
                    Players_Manager.Release_all_players_loop();
                    AudioInterrupts();

                    Loop_led_set.Request_all_LED_switch_off();

                    Switch_from_PERFORMANCE_to_LIVE_SAMPLING();
                    break;

                default:
                    PRINT_ERROR(F("Switch MISSING! "));
                    break;
                }
            }
            break;

            case SwModesPerformance:
            {
                switch (Lilla_state_0)
                {
                case PERFORMANCE:

                    // Salva il Delay della Patch su FRAM.
                    if (Patch_id < PATCHES_MAX)
                    {
                        Require_FRAM(Archive.Save_Delay(Patch_id, Delay_data));
                    }

                    if (true)
                    {
                        Serial.println();
                        Serial.println(F("main() - Delay_data in RAM:"));
                        Print_Delay_data(Delay_data);
                        Serial.println(F("... has been saved in FRAM."));
                    }

                    Golive_with_PERFORMANCE(Patch_id);
                    break;

                case DIRECT_SAMPLING:
                    Switch_from_DIRECT_SAMPLING_to_PERFORMANCE();
                    break;

                case LIVE_SAMPLING:
                    Switch_from_LIVE_SAMPLING_to_PERFORMANCE();
                    break;

                case MIDI_LOOP:
                    Switch_from_MIDI_LOOP_to_PERFORMANCE();
                    break;

                default:
                    PRINT_ERROR(F("Switch MISSING! "));
                    break;
                }
            }
            break;

            case SwModesMidiLoop:
            {
                if (Patch_id < PATCHES_MAX)
                {
                    Require_FRAM(Archive.Save_Delay(Patch_id, Delay_data));
                }
                Golive_with_MIDI_LOOP(false);
            }
            break;
            }
        }

        // Switch Tool
        if (Switches_manager.Get_change(SwitchTools))
        {
            switch (Switches_manager.Get_value(SwitchTools))
            {
            case SwToolsMixer:
            {
                if (Patch_id < PATCHES_MAX)
                {
                    Require_FRAM(Archive.Save_Delay(Patch_id, Delay_data));
                }
                Switch_to_MIXER();
            }
            break;

            case SwToolsDelay:
                break;

            case SwToolsSetup:
            {
                if (Patch_id < PATCHES_MAX)
                {
                    Require_FRAM(Archive.Save_Delay(Patch_id, Delay_data));
                }
                Golive_SETUP();
            }
            break;

            case SwToolsTest:
            {
                if (Patch_id < PATCHES_MAX)
                {
                    Require_FRAM(Archive.Save_Delay(Patch_id, Delay_data));
                }
                Golive_MIDI_MONITOR();
            }
            break;
            }
        }
    }

#pragma endregion // DELAY_SETTINGS

#pragma region Live Sampler [rgba(244, 229, 26, 0.19)]
    // *************************************************************
    // ********************   LIVE SAMPLER   ***********************
    // *************************************************************
    if (Lilla_state == LIVE_SAMPLING)
    {
        // Consume audio notices and update the popup only on the Live Sampler page.

        AudioNoInterrupts();
        const bool live_unrecorded_notice = AudioPlayer::Take_live_unrecorded_notice();
        AudioInterrupts();

        Display_LiveSampler.Update_no_recorded_audio(live_unrecorded_notice);

        for (int Inst_id = 0; Inst_id < INSTRUMENTS; ++Inst_id)
        {
            if (Read_pushbutton(PB_Sound[Inst_id]))
            {
                LS_Capture_sound(Inst_id);
                return;
            }
        }

        /*
        Live Sampling (LIVE SAMPLER) consente la registrazione sia Mono che Stereo. Prevede l'uso della Patch PATCHES_MAX.

        Se la registrazione ÃƒÆ’Ã‚Â¨ mono, PATCHES_MAX comprende 1 Instrument e il Sound SOUNDS_MAX:
        - Patch[PATCHES_MAX].Instrument[0].sound_id == PATCHES_MAX

        L'Instrument ha:
        from_note = 0
        to_note = 127
        root_key = 60
        midi_ch = 0 (midi channel 1)

        Il Sound ÃƒÆ’Ã‚Â¨ associato al file Mono.liv:
        Sound[SOUNDS_MAX].file = FIRST_LIVE_SAMPLING_FILE;


        Se la registrazione ÃƒÆ’Ã‚Â¨ mono, PATCHES_MAX comprende 1 Instrument e il Sound SOUNDS_MAX:
        - Patch[PATCHES_MAX].Instrument[0].sound_id == SOUNDS_MAX --> associato a ch. Left
        - Patch[PATCHES_MAX].Instrument[1].sound_id == SOUNDS_MAX + 1 --> associato a ch. Right

        Entrambi gli Instrument hanno:
        from_note = 0
        to_note = 127
        root_key = 60
        midi_ch = 0 (midi channel 1)


        I due Sound sono associati ai file .liv:
        Sound[SOUNDS_MAX].file = FIRST_LIVE_SAMPLING_FILE + 1 (Left.liv)
        Sound[SOUNDS_MAX + 1].file = FIRST_LIVE_SAMPLING_FILE + 2 (Right.liv)

        Fisicamente, i campioni sono salvati su due buffer "virtual tape" (int16_t LS_buffer_L e int16_t LS_buffer_R) istanziati dinamicamente
        nei PSRAM chip; entrambi i buffer comprendono LS_buffer_dim campioni, con indirizzo da 0 a (LS_buffer_dim -1):
        0......................................................................................(LS_buffer_dim -1)

        L'ultimo campione scritto e' Q_sample; esempio di prima scrittura del buffer:
                                                                         Q_sample
        0>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>..................(LS_buffer_dim -1)

        La porzione di LS_buffer_L/R visualizzata (window) mostra la waveform registrata; con riferimento agli elementi di LS_buffer_L
        i samples visualizzati vanno da LS_window_A_sample a LS_window_B_sample; l'ampiezza della window e' LS_window_width.


        LS_X_sample - LS_Y_sample ÃƒÆ’Ã‚Â¨ l'intervallo di esecuzione:
        - FWD e REV : parte da LS_X_sample
        - Loop FWD e Loop FWD/REV : da LS_X_sample a LS_Y_sample.


        LS_X_sample e' sempre al centro della window; al primo accesso a LIVE SAMPLER ÃƒÆ’Ã‚Â¨ sul sample 0:
        .................................(LS_X_sample).........................................(LS_buffer_dim -1)
                  (LS_window_A_sample)+++++++++|+++++++(LS_window_B_sample)

        LS_Y_sample e' definito come LS_X_sample + LS_XY_delta. LS_Y_sample puo' essere anche esterno alla window:

        .................................(LS_X_sample)..........(LS_Y_sample)..................(LS_buffer_dim -1)
                  (LS_window_A_sample)+++++++++|+++++++(LS_window_B_sample)

        VINCOLI
        0 <= LS_window_A_sample <= (LS_buffer_dim -1)
        LS_window_A_sample < LS_window_B_sample (LS_buffer_L/R viene comunque letto come un ring tape)
        0 <= LS_X_sample <= (LS_buffer_dim -1)
        LS_Y_sample > LS_X_sample (da verificare)
        LS_XY_delta spazia da valori negativi a positivi (per consentire qualsiasi posizionamento a LS_Y_sample): -(LS_buffer_dim -1) < LS_XY_delta < (LS_buffer_dim -1)

        Live Sampler prevede che i punti di lettura LS_X_sample/LS_Y_sample possano essere:
        1) fissi su un punto del buffer (se fosse un tape sono solidali al tape, solidali ai campioni registrati): LS_XY_lock == true.
        2) spostarsi lungo il buffer (se fosse un tape sono solidali con la testa di registrazione, i campioni sottostanti cambiano con continuita'): LS_XY_lock == false

        In entrambi i casi, con il NoteOn le posizioni di partenza (modi FWD e REV) e di arrivo (modi loop FWD, loop FWD/REV) sono congelate sul buffer (non sono piÃƒÆ’Ã‚Â¹ mobili). Importante notare
        che nel modo loop il suono sambia se il segmento di buffer LS_X_sample/LS_Y_sample viene riscritto.

        Calcolo degli estremi della window
        LS_window_A_sample = LS_X_sample - (LS_window_width - 1)/2 (poi scalato ad un valore positivo: 0 <= LS_window_A_sample <= (LS_buffer_dim -1))
        LS_window_B_sample = LS_window_A_sample + LS_window_width - 1 (NON scalato se supera (LS_buffer_dim -1))

        1) Caso LS_XY_lock == true
        LS_X_sample ÃƒÆ’Ã‚Â¨ fisso su una certa posizione del buffer; la waveform cresce verso DESTRA (nuovi campioni a DESTRA)
        0 <= LS_X_sample <= (LS_buffer_dim -1)
        LS_Y_sample = LS_X_sample + LS_XY_delta

        2) LS_XY_lock == false
        LS_X_sample segue un determinato campione registrato, quindi scorre lungo il buffer; la waveform cresce verso SINISTRA (vecchi campioni a SINISTRA)
        Tutti i valori operativi sono in movimento, LS_X_sample e LS_Y_sample vanno continuamente aggiornati:
        LS_X_sample = LS_Q_sample + LS_X_delta (poi scalato per rispettare il vincolo 0 <= LS_X_sample <= (LS_buffer_dim -1)
        LS_Y_sample = LS_X_sample + LS_XY_delta

        Modifica dei parametri
        Gli step di avanzamento di:
        LS_window_width
        LS_X_sample
        LS_X_delta

        sono sempre PROPORZIONALI a LS_window_width.
        */

        // Change volume_patch
        if (Read_encoder(EN_PB_LineOutVol, volume_patch, PATCH_VOLUME_MAX, 0, 1))
        {
            AudioNoInterrupts();
            Players_Manager.Update_all_Preset_volume(Patch_id, Patch_volume_gain(volume_patch));
            Players_Manager.Broadcast_volume();
            AudioInterrupts();

            Display_LiveSampler.Volume();
        }

        // Move pointer
        result = Read_encoder_simple(EN_PB_Select);
        if (result != 0)
        {
            Pointer_LiveSampler.Move_pointer(result);
            LS_local_pointer = Pointer_LiveSampler.Get_pointer();

            Clear_UI_events();
        }

        // Change values
        if (LS_local_pointer.field_name == field_LS_Menu)
        {
            if (Read_pushbutton(EN_PB_Select))
            {
                switch (LS_local_pointer.menu_element)
                {
                case value_LS_Recording:
                {
                    LS_state = REC;

                    LS_update_menu_elements();
                    Display_LiveSampler.Menu(); // displays the menu and updates "Value_Max_encoder.LS_menu" used by encoder_menu

                    Pointer_LiveSampler.Set_pointer_to_first_menu_element();
                    LS_local_pointer = Pointer_LiveSampler.Get_pointer();

                    Clear_UI_events();

                    LiveSampler.Start(LS_stereo);
                    LS_wave_refresh_timer = 0;
                    delay(10);
                }
                break;
                case value_LS_Stop:
                {
                    LS_state = PLAYONLY;
                    LiveSampler.Stop();

                    LS_update_menu_elements();
                    Display_LiveSampler.Menu(); // displays the menu and updates "Value_Max_encoder.LS_menu" used by encoder_menu

                    Pointer_LiveSampler.Set_pointer_to_first_menu_element();
                    LS_local_pointer = Pointer_LiveSampler.Get_pointer();

                    Clear_UI_events();

                    delay(20);
                    if (!LS_XY_lock)
                    {
                        LS_update_both_X_Y_samples();
                    }
                    else // altrimenti e' gia' stato calcolato
                    {
                        LS_update_Q_sample();
                    }
                    Display_LiveSampler.Show_wave(LS_sound_id);
                }
                break;
                case value_LS_MonoStereo:
                {
                    Midi_reader.Stop(); // NON sostituire con AudioNoInterrupts!

                    AudioNoInterrupts();
                    Players_Manager.Stop_all_players();
                    AudioInterrupts();

                    LS_stereo = !LS_stereo;
                    LS_buffer_dim = (LS_stereo ? LS_CACHE_STEREO_SAMPLES : LS_CACHE_MONO_SAMPLES);
                    LS_window_width = LS_buffer_dim;
                    LS_window_step = LS_window_width / 8;
                    LS_Reset_buffer();
                    LS_state = EMPTY;
                    LS_setup_LS_Patch(LS_stereo);

                    AudioNoInterrupts();
                    Players_Manager.Update_all_Preset(Patch_id, Patch_volume_gain(volume_patch));
                    AudioInterrupts();

                    P_Update_all_maps_Instrument_for_notes();
                    Print_Patch(Patch_id);
                    LS_sound_id = SOUNDS_MAX; // mostra sempre il primo Sound
                    LS_instrument = 0;
                    LS_X_delta = 0;
                    LS_X_sample = 0;
                    LS_XY_delta = AUDIO_SAMPLE_RATE;
                    LS_Y_sample = LS_X_sample + LS_XY_delta;
                    LS_X_step = LS_window_width / LS_COMB;

                    LS_refresh_LS_page();

                    Pointer_LiveSampler.Set_pointer_to_first_menu_element();
                    LS_local_pointer = Pointer_LiveSampler.Get_pointer();

                    Clear_UI_events();

                    Midi_reader.Start();
                }
                break;
                case value_LS_Erase:
                {
                    AudioNoInterrupts();
                    Players_Manager.Stop_all_players();
                    AudioInterrupts();

                    LS_Reset_buffer();
                    LS_state = EMPTY;
                    LS_sound_id = SOUNDS_MAX; // mostra sempre il primo Sound
                    LS_instrument = 0;
                    LS_window_width = LS_buffer_dim; // LS_window_width = 441001;
                    LS_window_step = LS_window_width / 8;
                    LS_X_sample = 0;
                    LS_X_delta = 0;
                    LS_XY_delta = AUDIO_SAMPLE_RATE;
                    LS_Y_sample = LS_X_sample + LS_XY_delta;
                    LS_X_step = LS_window_width / LS_COMB;

                    Display_LiveSampler.Page();

                    // restore all LED
                    Performance_led_set.Restore_all_LED();

                    LS_update_menu_elements();
                    Display_LiveSampler.Menu();

                    Pointer_LiveSampler.Set_pointer_to_first_menu_element();
                    LS_local_pointer = Pointer_LiveSampler.Get_pointer();

                    Clear_UI_events();

                    if (!LS_XY_lock)
                    {
                        LS_update_both_X_Y_samples();
                    }
                    else // altrimenti e' gia' stato calcolato
                    {
                        LS_update_Q_sample();
                    }

                    Display_LiveSampler.Show_wave(LS_sound_id);
                }
                break;
                }
            }
        }

        else if (LS_local_pointer.field_name == field_LS_Value)
        {
            switch (LS_local_pointer.value_element)
            {
            case value_LS_Gain:
            {
                if (Read_encoder(EN_PB_Value, Line_in_gain, 15, 0, 1))
                {
                    Audio_shield.lineInLevel(Line_in_gain);
                    Display_LiveSampler.Gain();
                }
            }
            break;

            case value_LS_Play_mode:
            {
                if (Read_encoder(EN_PB_Value, LS_mode, LOOP_FWD_REV, 0, 1))
                {
                    AudioNoInterrupts();
                    Sound[SOUNDS_MAX].mode = LS_mode;
                    Players_Manager.Update_Preset_mode(Patch_id, 0);
                    Players_Manager.Multicast_main_settings_editing(Patch_id, 0);
                    if (LS_stereo)
                    {
                        Sound[SOUNDS_MAX + 1].mode = LS_mode;
                        Players_Manager.Update_Preset_mode(Patch_id, 1);
                        Players_Manager.Multicast_main_settings_editing(Patch_id, 1);
                    }
                    AudioInterrupts();

                    Pointer_LiveSampler.Show_pointer(false);
                    Display_LiveSampler.Play_mode();
                    Pointer_LiveSampler.Show_pointer(true);

                    Display_LiveSampler.Loop_time();
                    if (LS_state != REC)
                    {
                        if (!LS_XY_lock)
                        {
                            LS_update_both_X_Y_samples();
                        }
                        else // altrimenti e' gia' stato calcolato
                        {
                            LS_update_Q_sample();
                        }
                        Display_LiveSampler.Show_wave(LS_sound_id);
                    }
                }
            }
            break;
            case value_LS_Feedback:
            {
                if (Read_encoder(EN_PB_Value, LS_feedback, 8, 0, 1))
                {
                    AudioNoInterrupts();
                    LS_Compressor.Set_feedback(LS_fbk_table[LS_feedback]);
                    AudioInterrupts();

                    Pointer_LiveSampler.Show_pointer(false);
                    Display_LiveSampler.Feedback();
                    Pointer_LiveSampler.Show_pointer(true);

                    Serial.println(LS_fbk_table[LS_feedback]);
                }
            }
            break;
            case value_LS_Compressor:
            {
                if (Read_pushbutton(EN_PB_Select))
                {
                    AudioNoInterrupts();
                    const bool compressor_enabled = !LS_Compressor.Is_enabled();
                    LS_Compressor.Set_enabled(compressor_enabled);
                    AudioInterrupts();

                    Display_LiveSampler.Compressor(compressor_enabled);
                }
            }
            break;
            case value_LS_Window:
            {
                result = Read_encoder_simple(EN_PB_Value);
                if (result != 0)
                {
                    Info.LS_restart_antiflicker();
                    if (result == 1)
                    {
                        LS_window_width -= LS_window_width / 8;
                    }
                    else
                    {
                        LS_window_width += LS_window_width / 8;
                    }

                    LS_window_width = constrain(LS_window_width, 20 * AUDIO_BLOCK_SAMPLES, LS_buffer_dim); // constrain(LS_window_width, 200 * AUDIO_BLOCK_SAMPLES, LS_buffer_dim);
                    if (LS_state != REC)
                    {
                        if (!LS_XY_lock)
                        {
                            LS_update_both_X_Y_samples();
                        }
                        else // altrimenti e' gia' stato calcolato
                        {
                            LS_update_Q_sample();
                        }
                        Display_LiveSampler.Show_wave(LS_sound_id);
                    }
                    LS_X_step = LS_window_width / LS_COMB;
                    // Display_LiveSampler.Step();

                    Pointer_LiveSampler.Show_pointer(false);
                    Display_LiveSampler.Window();
                    Pointer_LiveSampler.Show_pointer(true);
                }

                // Set window_width to "ALL TAPE"
                if (Read_pushbutton(EN_PB_Value))
                {
                    Info.LS_restart_antiflicker();
                    LS_window_width = LS_buffer_dim;
                    if (LS_state != REC)
                    {
                        if (!LS_XY_lock)
                        {
                            LS_update_both_X_Y_samples();
                        }
                        else // altrimenti e' gia' stato calcolato
                        {
                            LS_update_Q_sample();
                        }
                        Display_LiveSampler.Show_wave(LS_sound_id);
                    }
                    LS_X_step = LS_window_width / LS_COMB;
                    // Display_LiveSampler.Step();

                    Pointer_LiveSampler.Show_pointer(false);
                    Display_LiveSampler.Window();
                    Pointer_LiveSampler.Show_pointer(true);
                }
            }
            }
        }

        // Change LS_X_sample o LS_X_delta
        result = Read_encoder_simple(EN_PB_From);
        if (result != 0)
        {
            // si usa LS_X_sample
            if (LS_XY_lock)
            {
                if (result == 1)
                {
                    LS_X_sample += LS_X_step;
                }
                else
                {
                    LS_X_sample -= LS_X_step;
                }

                LS_X_sample = LS_constrain_position(LS_X_sample);
                Display_LiveSampler.Start_point();

                Serial.print(F("LS_X_sample: "));
                Serial.println(LS_X_sample);

                LS_Y_sample = LS_X_sample + LS_XY_delta;
                Serial.print(F("LS_Y_sample: "));
                Serial.println(LS_Y_sample);
            }

            // si usa LS_X_delta
            else
            {
                if (result == 1)
                {
                    LS_X_delta += LS_X_step;
                }
                else
                {
                    LS_X_delta -= LS_X_step;
                }

                LS_X_delta = LS_constrain_position(LS_X_delta);
                Display_LiveSampler.Start_point();

                Serial.print(F("LS_X_delta: "));
                Serial.println(LS_X_delta);
            }

            if (LS_state > REC)
            {
                AudioNoInterrupts();
                Players_Manager.Multicast_main_settings_editing(Patch_id, 0);
                if (LS_stereo)
                {
                    Players_Manager.Multicast_main_settings_editing(Patch_id, 1);
                }
                AudioInterrupts();
            }

            if (LS_state != REC)
            {
                if (!LS_XY_lock)
                {
                    LS_update_both_X_Y_samples();
                }
                else // altrimenti e' gia' stato calcolato
                {
                    LS_update_Q_sample();
                }
                Display_LiveSampler.Show_wave(LS_sound_id);
            }
        }

        // toggle LS_XY_lock/!LS_XY_lock
        if (Read_pushbutton(EN_PB_Step))
        {
            if (LS_XY_lock)
            {
                // passando a LS_XY_lock non si deve riassegnare
                AudioNoInterrupts();
                Players_Manager.Stop_all_players();
                AudioInterrupts();

                LS_XY_lock = false;
                LS_X_delta = 0;
                Info.LS_restart_antiflicker();
            }
            else
            {
                // passando a !LS_XY_lock si deve riassegnare LS_X_sample
                LS_lock_X_sample();
                Serial.print(F("LS_X_sample "));
                Serial.println(LS_X_sample);
            }

            Display_LiveSampler.Start_point();
            Serial.print(F("LS_XY_lock: "));
            Serial.println(LS_XY_lock);

            if (LS_state != REC)
            {
                if (!LS_XY_lock)
                {
                    LS_update_both_X_Y_samples();
                }
                else // altrimenti e' gia' stato calcolato
                {
                    LS_update_Q_sample();
                }
                Display_LiveSampler.Show_wave(LS_sound_id);
            }
        }

        // Change "Loop Width" (LS_XY_delta)
        result = Read_encoder_simple(EN_PB_To);
        if (result != 0)
        {
            if (result == 1)
            {
                LS_XY_delta += LS_X_step;
            }
            else
            {
                LS_XY_delta -= LS_X_step;
            }

            LS_XY_delta = constrain(LS_XY_delta, LS_XY_DELTA_MIN, LS_buffer_dim - 1);

            AudioNoInterrupts();
            Players_Manager.Multicast_main_settings_editing(Patch_id, 0);
            if (LS_stereo)
            {
                Players_Manager.Multicast_main_settings_editing(Patch_id, 1);
            }
            AudioInterrupts();

            if (LS_XY_lock)
            {
                LS_Y_sample = LS_X_sample + LS_XY_delta;
            }
            Display_LiveSampler.Loop_time();

            Serial.print(F("LS_XY_delta: "));
            Serial.println(LS_XY_delta);

            if (LS_state != REC)
            {
                if (!LS_XY_lock)
                {
                    LS_update_both_X_Y_samples();
                }
                else // altrimenti e' gia' stato calcolato
                {
                    LS_update_Q_sample();
                }
                Display_LiveSampler.Show_wave(LS_sound_id);
            }
        }

        // Change "Step" (LS_X_step)
        result = Read_encoder_simple(EN_PB_Step);
        if (result != 0)
        {
            if (result == 1)
            {
                LS_COMB = LS_COMB / 2;
            }

            else // Aumenta LS_X_step
            {
                LS_COMB = 2 * LS_COMB;
            }

            LS_COMB = constrain(LS_COMB, 8, 1024);
            LS_X_step = LS_window_width / LS_COMB;
            Display_LiveSampler.Step();
        }

        // Update wave
        if (LS_state == REC) // Open
        {
            if (LS_wave_refresh_timer >= LS_REFRESH) // ms
            {
                LS_wave_refresh_timer = 0;

                if (!LS_XY_lock)
                {
                    LS_update_both_X_Y_samples();
                }
                else // altrimenti e' gia' stato calcolato
                {
                    LS_update_Q_sample();
                }

                Display_LiveSampler.Show_wave(LS_sound_id);
            }
        }

        // Toggle wave Left/Right and VCF
        if (LS_stereo)
        {
            // Display Left wave or VCF
            if (Read_pushbutton(EN_PB_From))
            {
                if (LS_instrument == 1) // Right
                {
                    LS_instrument = 0;        // Left
                    LS_sound_id = SOUNDS_MAX; // Left
                    if (LS_state != REC)
                    {
                        if (!LS_XY_lock)
                        {
                            LS_update_both_X_Y_samples();
                        }
                        else // altrimenti e' gia' stato calcolato
                        {
                            LS_update_Q_sample();
                        }

                        Display_LiveSampler.Show_wave(LS_sound_id);
                    }
                }
            }

            // Display Right wave or VCF
            if (Read_pushbutton(EN_PB_To))
            {
                if (LS_instrument == 0) // Left
                {
                    LS_instrument = 1;            // Right
                    LS_sound_id = SOUNDS_MAX + 1; // Right
                    if (LS_state != REC)
                    {
                        if (!LS_XY_lock)
                        {
                            LS_update_both_X_Y_samples();
                        }
                        else // altrimenti e' gia' stato calcolato
                        {
                            LS_update_Q_sample();
                        }
                        Display_LiveSampler.Show_wave(LS_sound_id);
                    }
                }
            }
        }

        // Switch Mode
        if (Switches_manager.Get_change(SwitchModes))
        {
            switch (Switches_manager.Get_value(SwitchModes))
            {
            case SwModesSampler:
            {
                Switch_from_LIVE_SAMPLING_to_DIRECT_SAMPLING();
            }
            break;

            case SwModesLiveSampler:
                break;

            case SwModesPerformance:
            {
                Switch_from_LIVE_SAMPLING_to_PERFORMANCE();
            }
            break;

            case SwModesMidiLoop:
            {
                Switch_from_LIVE_SAMPLING_to_MIDI_LOOP();
            }
            break;
            }
        }

        // Switch verso un TOOL
        if (Read_pushbutton(PB_Tools))
        {
            TOOLS_pushbutton = true;
            Shifters_manager.Switch_led(LED_Tools, true);

            switch (Switches_manager.Get_value(SwitchTools))
            {
            case SwToolsMixer:
            {
                Lilla_state_0 = LIVE_SAMPLING;
                Switch_to_MIXER();
            }
            break;

            case SwToolsDelay:
            {
                Switch_from_LIVE_SAMPLING_to_DELAY(); // setta anche: Lilla_state_0 = LIVE_SAMPLING;
            }
            break;

            case SwToolsSetup:
            {
                Lilla_state_0 = LIVE_SAMPLING;
                Golive_SETUP();
            }
            break;

            case SwToolsTest:
            {
                Lilla_state_0 = LIVE_SAMPLING;
                Golive_MIDI_MONITOR();
            }
            break;
            }
        }
    }

#pragma endregion // LIVE_SAMPLING

#pragma region Direct Sampler [rgba(179, 179, 179, 0.32)]
    // *************************************************************
    // ********************  DIRECT SAMPLING  **********************
    // *************************************************************

    if (Lilla_state == DIRECT_SAMPLING)
    {

        /*
        Direct Sampling (SAMPLER) consente la registrazione sia Mono che Stereo. Prevede l'uso della Patch PATCHES_MAX, dei Sound SOUNDS_MAX e (SOUNDS_MAX + 1) e di 2 Instrument:
        - Patch[PATCHES_MAX].Instrument[0].sound_id == SOUNDS_MAX --> associato a ch. Left oppure Mono
        - Patch[PATCHES_MAX].Instrument[1].sound_id == SOUNDS_MAX + 1 --> associato a ch. Right

        Entrambi gli instrument hanno:
        from_note = 0
        to_note = 127
        root_key = 60
        midi_ch = 0 (midi channel 1)

        Se la registrazione ÃƒÆ’Ã‚Â¨ mono, PATCHES_MAX comprende 1 Instrument e il Sound SOUNDS_MAX:

        */

        // Change volume_patch
        if (DS_state == DS_waiting_state && Read_encoder(EN_PB_LineOutVol, volume_patch, PATCH_VOLUME_MAX, 0, 1))
        {
            AudioNoInterrupts();
            Players_Manager.Update_all_Preset_volume(Patch_id, Patch_volume_gain(volume_patch));
            Players_Manager.Broadcast_volume();
            AudioInterrupts();

            Display_Sampler.DS_update_volume();
        }

        // Update VU meter
        if (DS_state == DS_pause_state || DS_state == DS_recording_state)
        {
            float val;
            if (PeakTracking_L.available())
            {
                val = 20 * log10(PeakTracking_L.read());       // 0 <= PeakTracking_L.read() <= 1.0 ; -inf < val < 0
                Display_Sampler.DS_bar(0, BAR_ELEMENTS + val); // Display_Sampler.DS_bar(0, PeakTracking_L.read() * BAR_ELEMENTS);
            }
            if (PeakTracking_R.available())
            {
                val = 20 * log10(PeakTracking_R.read());
                Display_Sampler.DS_bar(1, BAR_ELEMENTS + val); // Display_Sampler.DS_bar(1, PeakTracking_R.read() * BAR_ELEMENTS);
            }
        }

        if (DS_state == DS_recording_state)
        {
            // Update blinking REC
            if (DS_blink_timer >= 500)
            {
                DS_blink_ON = !DS_blink_ON;
                Display_Sampler.DS_sampler_txt(DS_blink_ON);
                DS_blink_timer = 0;
            }

            // Update seconds and free-memory
            if (DS_recording_time_update >= 200)
            {
                DS_recording_time_update = 0;
                Display_Sampler.DS_available_memory();
            }

            // Stop if SteroSampler has stopped
            if (!DirectSampler.Is_recording())
            {
                Require_FRAM(DirectSampler.Storage_error());
                DS_state = DS_waiting_state;

                // switch OFF Audio Input monitor
                MAIN_mixer_out_L.gain(1, 0.0);
                MAIN_mixer_out_R.gain(1, 0.0);

                if (Recording[recording].packets == 0)
                {
                    // Start from first recording existing
                    recording = DS_get_next_Recording(-1);
                }

                else
                {
                    Recording[recording].consistent = true;
                    // consistent Recording must be saved
                    Require_FRAM(Archive.Save_DS_Recording(recording));
                    DS_read_Recording(recording); // only to update .bytes and .seconds
                }

                DS_update_recordings();

                // Switch off blinking REC
                DS_blink_ON = false;

                // Menu
                DS_define_menu();
                Display_Sampler.DS_menu(); // display the menu and updates DS_menu_max

                // Pointer
                Pointer_Sampler.Set_pointer_to_first_menu_element();
                DS_local_pointer = Pointer_Sampler.Get_pointer();

                Clear_UI_events();

                Display_Sampler.DS_available_memory();

                Display_Sampler.DS_sampler_txt(false);

                VFS_Print_FAT();

                if (!DS_Jump_to_DIRECT_SAMPLING_recording(recording))
                {
                    return;
                }

                // Reporting
                P_Recording(recording);
            }
        }

        // Move pointer
        result = Read_encoder_simple(EN_PB_Select);
        if (result != 0)
        {
            Pointer_Sampler.Move_pointer(result);
            DS_local_pointer = Pointer_Sampler.Get_pointer();

            Clear_UI_events();
        }

        // Change values
        switch (DS_local_pointer.field_name)
        {
        case field_DS_Menu:
        {
            if (Read_pushbutton(EN_PB_Select) || Read_pushbutton(EN_PB_Value))
            {
                Pointer_Sampler.Print_pointer();

                int first_packet_L = 0;
                int packets_per_channel = 0;
                int last_packet_L = 0;
                int first_packet_R = 0;

                switch (DS_local_pointer.menu_element)
                {
                case 0: // Delete
                {
                    AudioNoInterrupts();
                    Players_Manager.Stop_all_players();
                    AudioInterrupts();

                    Display_Sampler.DS_hide_recording();
                    Display_Sampler.DS_advice_delete(true);

                    // Delete recording
                    P_Invalidate_recording_cache(recording);
                    Recording[recording].consistent = false;
                    Require_VFS(VFS_Clean_up_VFS());
                    Require_VFS(VFS_Defragment());
                    DS_update_recordings();
                    VFS_Print_FAT();

                    // restart from first recording (if exist)
                    recording = DS_get_next_Recording(-1);
                    if (!DS_back_to_first_DS_Recording())
                    {
                        return;
                    }
                    Display_Sampler.DS_available_memory();
                }
                break;

                case 1: // Pause+Rec (pause before recording, listening Audio Input)
                {
                    DS_state = DS_pause_state;

                    AudioNoInterrupts();
                    Midi_reader.Stop();
                    Players_Manager.Stop_all_players();
                    AudioInterrupts();

                    Display_Sampler.DS_update_volume(false); // cambia il colore del volume in bianco (fisso)

                    recording = DS_find_Recording_free();
                    Serial.println(F("*** Pause + Record: listen to Audio Input ***"));
                    Serial.print(F("**** Prossimo recording: "));
                    Serial.println(recording);

                    // Hide last recording data
                    Display_Sampler.DS_hide_recording();

                    // Switch on Line OUT monitor
                    MAIN_mixer_out_L.gain(1, 1.0);
                    MAIN_mixer_out_R.gain(1, 1.0);

                    // Menu
                    DS_define_menu();
                    Display_Sampler.DS_menu(); // display the menu and updates DS_menu_max

                    // Pointer
                    Pointer_Sampler.Set_pointer_to_first_menu_element();
                    DS_local_pointer = Pointer_Sampler.Get_pointer();

                    Clear_UI_events();
                }
                break;

                case 2: // Mono Rec
                {
                    DS_state = DS_recording_state;
                    P_Invalidate_recording_cache(recording);

                    Recording[recording].stereo = false;
                    Recording[recording].consistent = false;

                    // packet:  0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18
                    // free:    * * * * 1 2 3 4 5 6 7  8  9  10 11 12 13 14 15
                    // result:  * * * * L L L L L L L  L  L  L  L  L  L  L  L

                    packets_per_channel = VFS_Get_packets_free();             // 15
                    first_packet_L = VFS_Get_first_packet_free();             // 4
                    last_packet_L = first_packet_L + packets_per_channel - 1; // 4 + 15 - 1 = 18

                    Serial.println(F("*** Start MONO Sampling! *** "));
                    Serial.print(F("Mono recording from packet: "));
                    Serial.print(first_packet_L);
                    Serial.print(F("  up to packet: "));
                    Serial.println(last_packet_L);

                    // Menu
                    DS_define_menu();
                    Display_Sampler.DS_menu(); // display the menu and updates DS_menu_max

                    // Pointer
                    Pointer_Sampler.Set_pointer_to_first_menu_element();
                    DS_local_pointer = Pointer_Sampler.Get_pointer();

                    Clear_UI_events();

                    Display_Sampler.DS_Recording_description(recording, false);
                    Display_Sampler.DS_sampler_txt(true);

                    DS_blink_timer = 0;
                    DS_blink_ON = true;

                    DirectSampler.Start(first_packet_L, last_packet_L, recording, Recording[recording].stereo); // bool start(int from_packet, int last_packet, int recording_id_in, bool stereo_in)
                    DS_recording_time = 0;
                    DS_recording_time_update = 0;
                }
                break;

                case 3: // Stereo Rec
                {
                    DS_state = DS_recording_state;
                    P_Invalidate_recording_cache(recording);

                    Recording[recording].stereo = true;
                    Recording[recording].consistent = false;

                    // packet:  0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18
                    // free:    * * * * 1 2 3 4 5 6 7  8  9  10 11 12 13 14 15
                    // result:  * * * * L R L R L R L  R  L  R  L  R  L  R  _

                    packets_per_channel = VFS_Get_packets_free() / 2;               // (15/2) = 7
                    first_packet_L = VFS_Get_first_packet_free();                   // 4
                    last_packet_L = first_packet_L + 2 * (packets_per_channel - 1); // 16
                    first_packet_R = first_packet_L + 1;

                    Serial.println(F("*** Start STEREO Sampling! *** "));
                    Serial.print(F("Left recording from packet: "));
                    Serial.print(first_packet_L);
                    Serial.print(F("  Right recording from packet: "));
                    Serial.println(first_packet_R);

                    // Menu
                    DS_define_menu();
                    Display_Sampler.DS_menu(); // display the menu and updates DS_menu_max

                    // Pointer
                    Pointer_Sampler.Set_pointer_to_first_menu_element();
                    DS_local_pointer = Pointer_Sampler.Get_pointer();

                    Clear_UI_events();

                    Display_Sampler.DS_Recording_description(recording, false);
                    Display_Sampler.DS_sampler_txt(true);

                    DS_blink_timer = 0;
                    DS_blink_ON = true;

                    DirectSampler.Start(first_packet_L, last_packet_L, recording, Recording[recording].stereo); // bool start(int from_packet, int last_packet, int recording_id_in, bool stereo_in)
                    DS_recording_time = 0;
                    DS_recording_time_update = 0;
                }
                break;

                case 4: // Stop
                {
                    Serial.println(F("*** Pause+Recording or Recording STOPPED! *** "));
                    if (DS_state == DS_recording_state)
                    {
                        DirectSampler.Book_stop();
                    }
                    DS_state = DS_waiting_state;

                    // switch OFF Line OUT monitor
                    MAIN_mixer_out_L.gain(1, 0.0);
                    MAIN_mixer_out_R.gain(1, 0.0);

                    if (Recording[recording].packets == 0)
                    {
                        Serial.print(F("Recording: "));
                        Serial.print(recording);
                        Serial.println(F(" cancelled."));
                        recording = DS_get_next_Recording(-1);
                    }

                    else
                    {
                        Recording[recording].consistent = true;
                        // consistent Recording must be saved
                        Require_FRAM(Archive.Save_DS_Recording(recording));
                        DS_read_Recording(recording); // only to update .bytes and .seconds
                    }

                    DS_update_recordings();
                    // Switch off blinking REC
                    DS_blink_ON = false;

                    // Menu
                    DS_define_menu();
                    Display_Sampler.DS_menu(); // display the menu and updates DS_menu_max

                    // Pointer
                    Pointer_Sampler.Set_pointer_to_first_menu_element();
                    DS_local_pointer = Pointer_Sampler.Get_pointer();

                    Clear_UI_events();

                    Display_Sampler.DS_available_memory();
                    Display_Sampler.DS_sampler_txt(false);

                    // VFS_Print_FAT();
                    P_Recording(recording);

                    if (!DS_Jump_to_DIRECT_SAMPLING_recording(recording))
                    {
                        return;
                    }
                    Midi_reader.Start();
                }
                break;

                case 5: // CONVERT_REC_TO_RAW
                {
                    int raw_conversion_choice = 0;
                    bool raw_conversion_confirmed = false;
                    Display_Sampler.DS_confirm_raw_conversion_popup();
                    Clear_UI_events();
                    while (!raw_conversion_confirmed)
                    {
                        Shifters_manager.Update();
                        if (Read_encoder(EN_PB_Select, raw_conversion_choice, 1, 0, 1))
                        {
                            Display_Sampler.DS_confirm_raw_conversion_frame(raw_conversion_choice);
                        }
                        if (Read_pushbutton(EN_PB_Select) || Read_pushbutton(EN_PB_Value))
                        {
                            raw_conversion_confirmed = true;
                        }
                    }
                    Clear_UI_events();
                    Display_Sampler.DS_page_lower(recording);
                    if (raw_conversion_choice == 0)
                    {
                        break;
                    }

                    DS_state = DS_convert_state;

                    AudioNoInterrupts();
                    Players_Manager.Stop_all_players();
                    AudioInterrupts();

                    confirmation = false; // no action
                    int file_L_RAW = -1;
                    int file_R_RAW = -1;
                    int blocks_per_file = ceil(Recording[recording].bytes / 256.0f); // quanti block compongono il file

                    if (!Recording[recording].stereo)
                    {
                        if ((Get_flash_size() - Get_flash_occupation()) >= Recording[recording].bytes)
                        {
                            DS_export = -1; // no filename available;
                            for (auto i = 0; i < FIRST_RECORDING_FILE; ++i)
                            {
                                if (FileNameRegistry::Numeric_available(i) && Capture_find(i) == nullptr && !SerialFlash.exists(Get_file_name(i, audio_filename)))
                                {
                                    file_L_RAW = i;
                                    DS_export = 1;
                                    break;
                                }
                            }
                        }
                        else
                            DS_export = 0; // no space available
                    }

                    else
                    {
                        if ((Get_flash_size() - Get_flash_occupation()) >= (2 * Recording[recording].bytes))
                        {
                            DS_export = -1; // no filename available;
                            for (auto i = 0; i < FIRST_RECORDING_FILE; ++i)
                            {
                                if (FileNameRegistry::Numeric_available(i) && Capture_find(i) == nullptr && !SerialFlash.exists(Get_file_name(i, audio_filename)))
                                {
                                    file_L_RAW = i;
                                    DS_export = 1;
                                    break;
                                }
                            }
                            if (DS_export == 1)
                            {
                                for (auto i = file_L_RAW + 1; i < FIRST_RECORDING_FILE; ++i)
                                {
                                    if (FileNameRegistry::Numeric_available(i) && Capture_find(i) == nullptr && !SerialFlash.exists(Get_file_name(i, audio_filename)))
                                    {
                                        file_R_RAW = i;
                                        DS_export = 2;
                                        break;
                                    }
                                }
                            }
                        }
                        else if ((Get_flash_size() - Get_flash_occupation()) >= Recording[recording].bytes)
                        {
                            DS_export = -1; // no filename available;
                            for (auto i = 0; i < FIRST_RECORDING_FILE; ++i)
                            {
                                if (FileNameRegistry::Numeric_available(i) && Capture_find(i) == nullptr && !SerialFlash.exists(Get_file_name(i, audio_filename)))
                                {
                                    file_L_RAW = i;
                                    DS_export = 1;
                                    break;
                                }
                            }
                        }
                        else
                            DS_export = 0; // no space available
                    }

                    Serial.print("DS_export ");
                    Serial.println(DS_export);
                    Serial.print("file_L_RAW proposto ");
                    Serial.println(file_L_RAW);
                    Serial.print("file_R_RAW proposto ");
                    Serial.println(file_R_RAW);
                    Serial.println();

                    if (DS_export <= 0)
                    {
                        Display_Sampler.DS_hide_recording();
                        Display_Sampler.DS_advice_no_conversion(DS_export, true);
                        delay(7000);

                        Display_Sampler.DS_advice_no_conversion(DS_export, false);

                        // Menu
                        DS_define_menu();
                        Display_Sampler.DS_menu(); // display the menu and updates DS_menu_max

                        // Pointer
                        Pointer_Sampler.Set_pointer_to_first_menu_element();
                        DS_local_pointer = Pointer_Sampler.Get_pointer();

                        Clear_UI_events();

                        Display_Sampler.DS_Recording_description(recording, true);

                        // Restore LED
                        Performance_led_set.Restore_all_LED();

                        break;
                    }

                    // Menu
                    DS_define_menu();
                    Display_Sampler.DS_menu(); // display the menu and updates DS_menu_max

                    // Pointer
                    Pointer_Sampler.Set_pointer_to_first_menu_element();
                    DS_local_pointer = Pointer_Sampler.Get_pointer();

                    Clear_UI_events();

                    Display_Sampler.DS_conversion_options(file_L_RAW, file_R_RAW, DS_export);

                    // Choose what to do
                    while (!confirmation)
                    {
                        Shifters_manager.Update();

                        // Move pointer
                        result = Read_encoder_simple(EN_PB_Select);
                        if (result != 0)
                        {
                            Pointer_Sampler.Move_pointer_within_menu(result);
                            DS_local_pointer = Pointer_Sampler.Get_pointer();

                            Clear_UI_events();
                        }

                        // Choose element
                        if (Read_pushbutton(EN_PB_Select) || Read_pushbutton(EN_PB_Value))
                        {
                            confirmation = true;
                        }
                    }
                    Clear_UI_events();

                    switch (choice_DS_menu)
                    {
                    case 6: // Cancel (don't export)
                        Serial.println(F("Don't convert any file"));
                        break;

                    case 7:                                                   // Convert Mono (file_L)
                        DS_convert_file_L(file_L_RAW, blocks_per_file * 256); // DS_convert_file_L(int file_L_RAW, int bytes)

                        // occorre rifare lo scan di tutti i file per compilare tutti i metadati del nuovo file, dirindex compreso
                        File_scanner.Read_all_file_data();
                        break;

                    case 8:                                                   // Convert file_L
                        DS_convert_file_L(file_L_RAW, blocks_per_file * 256); // DS_convert_file_L(int file_L_RAW, int bytes)

                        // occorre rifare lo scan di tutti i file per compilare tutti i metadati del nuovo file, dirindex compreso
                        File_scanner.Read_all_file_data();
                        break;

                    case 9:                                                   // Convert file_R
                        DS_convert_file_R(file_R_RAW, blocks_per_file * 256); // DS_convert_file_R(int file_R_RAW, int bytes)

                        // occorre rifare lo scan di tutti i file per compilare tutti i metadati del nuovo file, dirindex compreso
                        File_scanner.Read_all_file_data();
                        break;

                    case 10:                                                  // Convert both file_L and file_R
                        DS_convert_file_L(file_L_RAW, blocks_per_file * 256); // DS_convert_file_L(int file_L_RAW, int bytes)
                        DS_convert_file_R(file_R_RAW, blocks_per_file * 256); // DS_convert_file_R(int file_R_RAW, int bytes)

                        // occorre rifare lo scan di tutti i file per compilare tutti i metadati del nuovo file, dirindex compreso
                        File_scanner.Read_all_file_data();
                        break;

                    default:
                        // Reporting
                        Serial.println(F("Don't convert any file"));
                        break;
                    }

                    // Delete recording
                    if (choice_DS_menu > 6)
                    {
                        P_Invalidate_recording_cache(recording);
                        Recording[recording].consistent = false;
                        Require_VFS(VFS_Clean_up_VFS());
                        Require_VFS(VFS_Defragment());
                        DS_update_recordings();
                        VFS_Print_FAT();

                        // Load first recording (if exist)
                        recording = DS_get_next_Recording(-1);
                        if (!DS_back_to_first_DS_Recording())
                        {
                            return;
                        }
                    }

                    Print_flash_file_list();

                    // Return
                    DS_state = DS_waiting_state;

                    Display_Sampler.DS_page_upper();
                    Display_Sampler.DS_page_lower(recording);

                    // Menu
                    DS_define_menu();
                    Display_Sampler.DS_menu(); // display the menu and updates DS_menu_max

                    // Pointer
                    Pointer_Sampler.Set_pointer_to_first_menu_element();
                    DS_local_pointer = Pointer_Sampler.Get_pointer();

                    Clear_UI_events();

                    // Switch bar_display ON
                    PeakTracking_L.reset();
                    PeakTracking_R.reset();
                    Display_Sampler.DS_bar(0, 0);
                    Display_Sampler.DS_bar(1, 0);
                }
                break;

                case 11: // EXPORT AS WAV TO SD
                {
                    DS_state = DS_export_SD_state;
                    const bool exported = DS_export_wav_to_SD();
                    if (!exported)
                    {
                        Show_popup_text("EXPORT FAILED", ILI9341_WHITE, ILI9341_RED, 0);
                    }
                    else if (!Recording[recording].stereo)
                    {
                        Show_popup_text("MONO WAV EXPORTED TO SD", ILI9341_BLACK, ILI9341_GREEN);
                    }
                    else
                    {
                        Show_popup_text("STEREO WAV EXPORTED TO SD", ILI9341_BLACK, ILI9341_GREEN);
                    }
                    delay(exported ? 2000 : 4000);
                    DS_state = DS_waiting_state;
                    PeakTracking_L.reset();
                    PeakTracking_R.reset();
                    Display_Sampler.DS_page_lower(recording);
                    Clear_UI_events();
                }
                break;

                default:
                    PRINT_ERROR(F("Switch MISSING! "));
                    break;
                } // END switch(choice_DS_menu)
            }
        }
        break;

        case field_DS_Value:
        {

            if (DS_local_pointer.value_element == value_DS_Gain)
            {
                // Share the LINE IN gain with Mixer; allow adjustment while monitoring or recording.
                if ((DS_state == DS_pause_state || DS_state == DS_recording_state) && Read_encoder(EN_PB_Value, Line_in_gain, 15, 0, 1))
                {
                    Audio_shield.lineInLevel(Line_in_gain);
                    Display_Sampler.DS_show_gain();
                }
            }
            else if (DS_local_pointer.value_element == value_DS_Recording && DS_state == DS_waiting_state)
            {
                result = Read_encoder_simple(EN_PB_Value);
                if (result != 0)
                {
                    DS_recording_change = recording;
                    if (result == +1)
                    {
                        DS_recording_change = DS_get_next_Recording(recording);
                    }
                    else
                    {
                        DS_recording_change = DS_get_previous_Recording(recording);
                    }

                    if (DS_recording_change != recording)
                    {
                        AudioNoInterrupts();
                        Players_Manager.Stop_all_players();
                        AudioInterrupts();

                        recording = DS_recording_change;
                        if (!DS_Jump_to_DIRECT_SAMPLING_recording(recording))
                        {
                            return;
                        }

                        // Recording
                        P_Recording(recording);
                    }
                }
            }
        }
        break;
        }

        // Switch verso un TOOL
        if (Read_pushbutton(PB_Tools))
        {
            TOOLS_pushbutton = true;
            Shifters_manager.Switch_led(LED_Tools, true);

            switch (Switches_manager.Get_value(SwitchTools))
            {
            case SwToolsMixer:
            {
                Lilla_state_0 = DIRECT_SAMPLING;
                Switch_to_MIXER();
            }
            break;

            case SwToolsDelay:
            {
                Lilla_state_0 = DIRECT_SAMPLING;
                Golive_DELAY_SETTINGS();
            }
            break;

            case SwToolsSetup:
            {
                Lilla_state_0 = DIRECT_SAMPLING;
                Golive_SETUP();
            }
            break;

            case SwToolsTest:
            {
                Lilla_state_0 = DIRECT_SAMPLING;
                Golive_MIDI_MONITOR();
            }
            break;
            }
        }

        // Switch Mode
        if (Switches_manager.Get_change(SwitchModes))
        {
            switch (Switches_manager.Get_value(SwitchModes))
            {
            case SwModesSampler:
                break;

            case SwModesLiveSampler:
            {
                Switch_from_DIRECT_SAMPLING_to_LIVE_SAMPLING();
            }
            break;

            case SwModesPerformance:
            {
                Switch_from_DIRECT_SAMPLING_to_PERFORMANCE();
            }
            break;

            case SwModesMidiLoop:
            {
                Switch_from_DIRECT_SAMPLING_to_MIDI_LOOP();
            }
            break;
            }
        }

    } // END if(Lilla_state == DIRECT_SAMPLING)

#pragma endregion // DIRECT SAMPLING

#pragma region
    // *************************************************************
    // ****************        MIDI_MONITOR      *******************
    // *************************************************************
    if (Lilla_state == MIDI_MONITOR)
    {
        // Change Patch VOLUME
        if (Read_encoder(EN_PB_LineOutVol, volume_patch, PATCH_VOLUME_MAX, 0, 1))
        {
            AudioNoInterrupts();
            Players_Manager.Update_all_Preset_volume(Patch_id, Patch_volume_gain(volume_patch));
            Players_Manager.Broadcast_volume();
            AudioInterrupts();
        }

        // Visualizza incoming MIDI
        if (display_wait)
        {
            switch (midi_message_received)
            {
            case 0: // no message received
                break;
            case 1: // note ON
                Display_Diagnostics.Midi_monitor_data(MM_midi_channel, 0, MM_note_number, MM_velocity, -1, -1);
                break;
            case 2: // note OFF
                Display_Diagnostics.Midi_monitor_data(MM_midi_channel, 1, MM_note_number, MM_velocity, -1, -1);
                break;
            case 3: // pitch bend
                Display_Diagnostics.Midi_monitor_data(MM_midi_channel, 2, -1, -1, (MM_pitch_bend_most << 7) + MM_pitch_bend_least, -1);
                break;
            case 4: // after touch poly
                Display_Diagnostics.Midi_monitor_data(MM_midi_channel, 3, MM_least_bits, -1, MM_most_bits, -1);
                break;
            case 5: // control change
                Display_Diagnostics.Midi_monitor_data(MM_midi_channel, 4, -1, -1, MM_midi_value, MM_midi_controller);
                break;
            case 6: // program change
                Display_Diagnostics.Midi_monitor_data(MM_midi_channel, 5, -1, -1, -1, MM_least_bits);
                break;
            case 7: // After Touch Channel
                Display_Diagnostics.Midi_monitor_data(MM_midi_channel, 6, -1, -1, MM_least_bits, -1);
                break;
            case 8: // System Exclusive
                Display_Diagnostics.Midi_monitor_data(MM_midi_channel, 7, -1, -1, -1, -1);
                break;
            default:
                PRINT_ERROR(F("Switch MISSING! "));
                break;
            }
            display_wait = false;
        }

        // Switch TOOL
        switch (Switches_manager.Get_value(SwitchTools))
        {
        case SwToolsMixer:
        {
            Switch_to_MIXER();
        }
        break;

        case SwToolsDelay:
        {
            Golive_DELAY_SETTINGS();
        }
        break;

        case SwToolsSetup:
        {
            Golive_SETUP();
        }
        break;

        case SwToolsTest:
            break;
        }

        // Switch Mode
        if (Read_pushbutton(PB_Tools))
        {
            TOOLS_pushbutton = false;
            Shifters_manager.Switch_led(LED_Tools, false);
            switch (Switches_manager.Get_value(SwitchModes))
            {
            case SwModesSampler:
            {
                switch (Lilla_state_0)
                {
                case PERFORMANCE:
                    Switch_to_DIRECT_SAMPLING();
                    break;

                case DIRECT_SAMPLING:
                    Lilla_state = DIRECT_SAMPLING;

                    Display_Sampler.DS_page_upper();
                    Display_Sampler.DS_page_lower(recording);

                    // Menu
                    DS_define_menu();
                    Display_Sampler.DS_menu(); // display the menu and updates DS_menu_max

                    // Pointer
                    Pointer_Sampler.Set_pointer_to_first_menu_element();
                    DS_local_pointer = Pointer_Sampler.Get_pointer();

                    // Display the VU meter
                    Display_Sampler.DS_bar(0, 0);
                    Display_Sampler.DS_bar(1, 0);

                    Clear_UI_events();
                    break;

                case LIVE_SAMPLING:
                    Switch_from_LIVE_SAMPLING_to_DIRECT_SAMPLING();
                    break;

                case MIDI_LOOP:
                    Switch_from_MIDI_LOOP_to_DIRECT_SAMPLING();
                    break;

                default:
                    PRINT_ERROR(F("Switch MISSING! "));
                    break;
                }
            }
            break;

            case SwModesLiveSampler:
            {
                switch (Lilla_state_0)
                {
                case PERFORMANCE:
                    Switch_from_PERFORMANCE_to_LIVE_SAMPLING();
                    break;

                case DIRECT_SAMPLING:
                    Switch_from_DIRECT_SAMPLING_to_LIVE_SAMPLING();
                    break;

                case LIVE_SAMPLING:
                    LS_refresh_LS_page();
                    break;

                case MIDI_LOOP:
                    Switch_from_MIDI_LOOP_to_LIVE_SAMPLING();
                    break;

                default:
                    PRINT_ERROR(F("Switch MISSING! "));
                    break;
                };
            }
            break;

            case SwModesPerformance:
            {
                switch (Lilla_state_0)
                {
                case PERFORMANCE:
                    Golive_with_PERFORMANCE(Patch_id);
                    break;

                case DIRECT_SAMPLING:
                    Switch_from_DIRECT_SAMPLING_to_PERFORMANCE();
                    break;

                case LIVE_SAMPLING:
                    Switch_from_LIVE_SAMPLING_to_PERFORMANCE();
                    break;

                case MIDI_LOOP:
                    Switch_from_MIDI_LOOP_to_PERFORMANCE();
                    break;

                default:
                    PRINT_ERROR(F("Switch MISSING! "));
                    break;
                }
            }
            break;

            case SwModesMidiLoop:
            {
                switch (Lilla_state_0)
                {
                case PERFORMANCE:
                    Switch_from_PERFORMANCE_to_MIDI_LOOP();
                    break;

                case DIRECT_SAMPLING:
                    Switch_from_DIRECT_SAMPLING_to_MIDI_LOOP();
                    break;

                case LIVE_SAMPLING:
                    Switch_from_LIVE_SAMPLING_to_MIDI_LOOP();
                    break;

                case MIDI_LOOP:
                    Golive_with_MIDI_LOOP(false);
                    break;

                default:
                    PRINT_ERROR(F("Switch MISSING! "));
                    break;
                }
            }
            break;
            }
        }
    }

#pragma endregion // MIDI_MONITOR

#pragma region[rgba(6, 209, 250, 0.13)]
    // *************************************************************
    // ******************       MIDI_LOOP      *********************
    // *************************************************************

    if (Lilla_state == MIDI_LOOP)
    {
        if (Display_MidiLoop.Update_save_failed())
        {
            Pointer_MidiLoop.Show_pointer(true);
        }

        // Change volume_patch
        if (Read_encoder(EN_PB_LineOutVol, volume_patch, PATCH_VOLUME_MAX, 0, 1))
        {
            AudioNoInterrupts();
            Players_Manager.Update_all_Preset_volume(Patch_id, Patch_volume_gain(volume_patch));
            Players_Manager.Broadcast_volume();
            AudioInterrupts();

            Display_Common.P_Patch_volume_value(true);
        }

        // Move pointerMenu
        if (LOOP_events[MASTER_TRACK] > 0)
        {
            result = Read_encoder_simple(EN_PB_Select);
            if (result != 0)
            {
                Pointer_MidiLoop.Move_pointer(result);
                LOOP_local_pointer = Pointer_MidiLoop.Get_pointer();

                Clear_UI_events();
            }
        }

        // Change LOOP_id
        result = Read_encoder_simple(EN_PB_Loop);
        if (result != 0)
        {
            int new_loop_id;
            if (result == +1)
            {
                new_loop_id = LOOP_Get_next_loop_id_in_SD(LOOP_id);
            }
            else if (result == -1)
            {
                new_loop_id = LOOP_Get_previous_loop_id_in_SD(LOOP_id);
            }

            PRINT_CONTROL_POINT(new_loop_id);

            if (new_loop_id == -1)
            {
                LOOP_run_button_state = true;
            }

            else if (new_loop_id != LOOP_id)
            {
                // Validation leaves the current loop untouched; a failed second pass clears it safely.
                const bool loaded = LOOP_Copy_midi_loop_from_SD_to_RAM(new_loop_id);
                if (loaded || LOOP_events[MASTER_TRACK] == 0)
                {
                    if (loaded)
                    {
                        LOOP_id = new_loop_id;
                    }

                    // Show LOOP_id on display
                    Display_MidiLoop.Show_loop_id();

                    // Show LOOP_time on display
                    Display_MidiLoop.Loop_total_time();

                    // Show track infos on display
                    for (auto track = 0; track < TRACKS; ++track)
                    {
                        Display_MidiLoop.Show_track_all_data(track);
                    }

                    // Update menu and pointer
                    Pointer_MidiLoop.Show_pointer(false);
                    LOOP_select_menu_elements();
                    Display_MidiLoop.Show_menu();
                    Pointer_MidiLoop.Set_pointer_to_first_menu_element();
                    LOOP_local_pointer = Pointer_MidiLoop.Get_pointer();

                    Clear_UI_events();

                    // Switch off all tracks LEDs on display
                    Loop_led_set.Request_all_LED_switch_off();

                    if (LOOP_time > 0)
                    {
                        LOOP_metronomo.Led_ON(0);
                        LOOP_metronomo.Setup(LOOP_time);
                    }

                    // restart clock
                    LOOP_restart_clock();

                    // Set first event for each track
                    for (auto track = 0; track < TRACKS; ++track)
                    {
                        LOOP_play_event[track] = 0;
                    }

                    // Sort events by timestamp
                    for (auto track = 0; track < TRACKS; ++track)
                    {
                        LOOP_set_time_order(track);
                    }

                    // Simulate all tracks Start/Stop, with all tracks active
                    LOOP_run_button_state = LOOP_events[MASTER_TRACK] == 0;

                    // Save track states before stopping
                    for (auto track = 0; track < TRACKS; ++track)
                    {
                        LOOP_track_run_memo[track] = LOOP_events[track] > 0;
                        LOOP_track_run[track] = false;
                    }
                }

                // Report
                if (loaded)
                {
                    Serial.println("Loop uploaded; data in RAM:");
                    LOOP_Print_midi_loop_complete_data(LOOP_id);
                }
                else
                {
                    Serial.println(F("Loop import failed."));
                }
            }
        }

        // Use the first track encoder rotation on a menu item to select LEVEL.
        if (LOOP_events[MASTER_TRACK] > 0 && LOOP_local_pointer.field_name == field_LOOP_Menu)
        {
            bool select_level = false;
            for (auto track = 0; track < TRACKS; ++track)
            {
                if (Read_encoder_simple(EN_PB_Track[track]) != 0)
                {
                    select_level = true;
                }
            }
            if (select_level)
            {
                Pointer_MidiLoop.Set_pointer_to_level();
                LOOP_local_pointer = Pointer_MidiLoop.Get_pointer();
            }
        }

        // Recording
        for (auto track = 0; track < TRACKS; ++track)
        {
            if (Read_pushbutton(PB_Rec[track]) && LOOP_run_button_state && (track == MASTER_TRACK || LOOP_events[MASTER_TRACK] != 0))
            {
                LOOP_learning_track = track; // LOOP_learning_track e' il nuovo loop

                // High priority
                AudioNoInterrupts();
                if (LOOP_events[LOOP_learning_track] != 0)
                {
                    // se si tratta di MASTER_TRACK si fermano e cancellano tutti i track
                    if (LOOP_learning_track == MASTER_TRACK)
                    {
                        // Interrompi i Player che eseguono note di qualsiasi track
                        Players_Manager.Release_all_players_loop();

                        for (auto local_track = 0; local_track < TRACKS; ++local_track)
                        {
                            // Interrompe la lettura
                            LOOP_track_run[local_track] = false;

                            // Cancella il loop
                            LOOP_events[local_track] = 0;

                            // Resetta slide
                            LOOP_slide[local_track] = 0;

                            // Resetta pitch
                            LOOP_pitch_int[local_track] = 0;
                        }

                        // ferma il metronomo
                        LOOP_metronomo_run = false;
                        // LOOP_metronomo_flag_OUT = false; // da eliminare

                        // Annulla (se ci fosse) l'ultima richiesta di aggiornamento del metronomo proveniente da MidiReader
                        LOOP_metronomo_flag_IN[1] = false;

                        // Nuovo loop
                        LOOP_original = true;
                    }
                    else
                    {
                        if (LOOP_track_run[LOOP_learning_track])
                        {
                            // Interrompi i Player di LOOP_learning_track
                            Players_Manager.Release_all_players_loop(LOOP_learning_track);
                        }
                        // Interrompe la lettura
                        LOOP_track_run[LOOP_learning_track] = false;

                        // Cancella il loop
                        LOOP_events[LOOP_learning_track] = 0; // Loop cancellato

                        // Resetta slide
                        LOOP_slide[LOOP_learning_track] = 0;

                        // Resetta pitch
                        LOOP_pitch_int[LOOP_learning_track] = 0;

                        // loop esistente
                        LOOP_original = false;
                    }
                }
                AudioInterrupts();

                // LED_Rec ON
                Shifters_manager.Switch_led(LED_Rec[LOOP_learning_track], true);

                // Display update, and other low priority procedures
                if (LOOP_learning_track == MASTER_TRACK)
                {
                    // Spegni i led del metronomo
                    LOOP_metronomo.Leds_off(); // va eseguito fuori da AudioNoInterrupt()

                    // nomina loop_id
                    LOOP_id = NEW_LOOP;
                    Display_MidiLoop.Show_loop_id();

                    LOOP_time = 0;
                    Display_MidiLoop.Loop_total_time(); // accanto ai led del metronomo appare il tempo totale 0.0s
                }

                // Se si tratta del track master (0) non ancora esistente, oppure si tratta di un altro track ma con track master esistente
                if (LOOP_learning_track == MASTER_TRACK || LOOP_events[MASTER_TRACK] != 0)
                {
                    // Show (or delete) all tracks infos
                    for (auto track = 0; track < TRACKS; ++track)
                    {
                        Display_MidiLoop.Show_track_all_data(track);

                        if (LOOP_events[track] == 0)
                        {
                            Pointer_MidiLoop.Show_pointerTrack(track, false);
                        }
                    }

                    // Display "n-REC"
                    Display_MidiLoop.Loop_REC_advice(LOOP_learning_track, true);

                    // Switch off all tracks LEDs
                    Loop_led_set.Request_all_LED_switch_off();

                    // Update menu and pointer
                    Pointer_MidiLoop.Show_pointer(false);
                    LOOP_select_menu_elements();
                    Display_MidiLoop.Show_menu();
                    Pointer_MidiLoop.Set_pointer_to_first_menu_element();
                    LOOP_local_pointer = Pointer_MidiLoop.Get_pointer();

                    Clear_UI_events();

                    // Prepare learning
                    LOOP_learn_clock = 0;
                    LOOP_elements = 0;      // ancora nessun evento
                    LOOP_learn_flag = true; // avvia il learning

                    // Feedback
                    Serial.println(F("Learning inizializzato!"));

                    // Learning
                    while (LOOP_learn_flag)
                    {
                        if (Display_MidiLoop.Update_save_failed())
                        {
                            Pointer_MidiLoop.Show_pointer(true);
                        }
                        Shifters_manager.Update();

                        // Stop learning
                        if (Read_pushbutton(PB_Rec[LOOP_learning_track]))
                        {
                            // Chiude il learning
                            LOOP_learn_flag = false;

                            // Feedback
                            Serial.println("Loop correttamente chiuso manualmente!");
                            break;
                        }

                        // After 20 secons without events --> cancel track
                        else if (LOOP_elements == 0 && LOOP_learn_clock > 20000)
                        {
                            // Close learning
                            LOOP_learn_flag = false;

                            // Feedback
                            Serial.println(F("Loop chiuso e cancellato perche' dimenticato aperto!"));
                            break;
                        }

                        // se si tratta del track master (0), MidiReader chiede l'accensione del primo led del metronomo quando riceve il primo NoteOn
                        else if (LOOP_learning_track == MASTER_TRACK && LOOP_metronomo_flag_IN[0])
                        {
                            LOOP_metronomo_flag_IN[0] = false;

                            // Ask metronomo to switch on first led (metronomo is not runnig)
                            LOOP_metronomo.Led_ON(0);
                        }

                        // Continously update LEDs
                        Update_instruments_leds();

                        // se NON si tratta del track master (0) c'e' l'aggiornamento continuo del metronomo
                        if (LOOP_metronomo_flag_IN[1])
                        {
                            LOOP_metronomo_flag_IN[1] = false;
                            LOOP_metronomo.Update();
                            LOOP_metronomo.metro_time += LOOP_metronomo.Read_metro_delta_ms();
                        }
                    }

                    // Learnig closed. From here: LOOP_learn_flag == false
                    LOOP_events[LOOP_learning_track] = LOOP_elements; // se LOOP_events[LOOP_learning_track] == 0 significa che il LOOP_learning_track ÃƒÆ’Ã‚Â¨ vuoto e non viene eseguito

                    Clear_UI_events();

                    // LED_Rec OFF
                    Shifters_manager.Switch_led(LED_Rec[LOOP_learning_track], false);

                    // Feedback
                    Serial.println("Learning closed!");

                    // The new track is valid (contains events)
                    if (LOOP_events[LOOP_learning_track] > 0)
                    {
                        // If MASTER_TRACK
                        if (LOOP_learning_track == MASTER_TRACK)
                        {
                            // Setup di LOOP_time (durata di tutti i loop)
                            LOOP_time = LOOP_learn_clock;

                            // Restart clock
                            LOOP_restart_clock();

                            // Reset stretch
                            LOOP_stretch_int = 100;
                            LOOP_stretch = 1.0;

                            // Report
                            Serial.print("LOOP_time:");
                            Serial.println(LOOP_time);
                        }

                        // aggiungi info di slide
                        LOOP_slide[LOOP_learning_track] = 0; // ms

                        // aggiungi info di pitch
                        LOOP_pitch_int[LOOP_learning_track] = 0; // 0 --> pitch = 1.0

                        // evento di avvio
                        LOOP_play_event[LOOP_learning_track] = 0;

                        // calcolo istante esecuzione evento di avvio (LOOP_play_time) e prossimo switch del metronomo rispetto a LOOP_Clock
                        if (LOOP_learning_track == MASTER_TRACK)
                        {
                            LOOP_play_time[LOOP_learning_track] = 0;

                            // setup metronomo
                            LOOP_metronomo.Setup(LOOP_time);

                            // set next metronomo step
                            LOOP_metronomo.metro_time = 0 + LOOP_metronomo.Read_metro_delta_ms();

                            // metronomo switch-on
                            LOOP_metronomo_run = true;
                        }
                        else
                        {
                            LOOP_play_time[LOOP_learning_track] = LOOP_Clock_time_from_virtual_time(LOOP_element[LOOP_learning_track][0].time);
                        }

                        // effettua l'ordinamento temporale degli eventi
                        LOOP_set_time_order(LOOP_learning_track);

                        // avvio
                        LOOP_track_run[LOOP_learning_track] = true;

                        // update menu
                        if (LOOP_id == NEW_LOOP)
                        {
                            LOOP_original = true;
                        }

                        else if (LOOP_learning_track == MASTER_TRACK && LOOP_original)
                        {
                            LOOP_original = false;
                        }

                        else if (LOOP_learning_track > MASTER_TRACK)
                        {
                            LOOP_original = false;
                        }

                        // Update menu and pointer on display
                        Pointer_MidiLoop.Show_pointer(false);
                        LOOP_select_menu_elements();
                        Display_MidiLoop.Show_menu();

                        // Report
                        Serial.println(" **************** ");
                        Serial.print("eventi:");
                        Serial.println(LOOP_events[LOOP_learning_track]);
                        for (uint32_t event = 0; event < LOOP_events[LOOP_learning_track]; ++event)
                        {
                            Serial.print(event);
                            Serial.print(" time:");
                            Serial.print(LOOP_element[LOOP_learning_track][event].time);
                            Serial.print(" midi_channel:");
                            Serial.print(LOOP_element[LOOP_learning_track][event].midi_channel);
                            Serial.print(" note_number:");
                            Serial.print(LOOP_element[LOOP_learning_track][event].note_number);
                            Serial.print(" velocity:");
                            Serial.print(LOOP_element[LOOP_learning_track][event].velocity);
                            Serial.print(" note_on:");
                            Serial.println(LOOP_element[LOOP_learning_track][event].note_on ? "NoteOn" : "NoteOff");
                        }

                        Serial.print("Ordine temporale degli eventi: ");
                        for (uint32_t event = 0; event < LOOP_events[LOOP_learning_track]; ++event)
                        {
                            Serial.print(LOOP_time_order[LOOP_learning_track][event]);
                            Serial.print(" - ");
                        }
                        Serial.println();
                        Serial.print("Si inizia con l'evento:");
                        Serial.println(LOOP_play_event[LOOP_learning_track]);
                        Serial.print("Tra ms:");
                        Serial.println(LOOP_play_time[LOOP_learning_track] - LOOP_Clock());
                        Serial.println("*************");
                    }

                    else if (LOOP_events[MASTER_TRACK] == 0)
                    {
                        // spegni il primo led se acceso
                        LOOP_metronomo.Leds_off();

                        // cancella la richiesta per il primo led se arrivata (non dovrebbe essere possibile)
                        LOOP_metronomo_flag_IN[0] = false;
                    }

                    Display_MidiLoop.Show_track_all_data(track);

                    // pointer
                    Pointer_MidiLoop.Set_pointer_to_first_menu_element();
                    LOOP_local_pointer = Pointer_MidiLoop.Get_pointer();

                    // Display loop time
                    if (LOOP_learning_track == MASTER_TRACK)
                    {
                        Display_MidiLoop.Loop_total_time();
                    }

                    Clear_UI_events();
                }
            }

            // Change values
            if (LOOP_events[track] > 0)
            {

                // Track start-stop
                if (Read_pushbutton(EN_PB_Track[track]))
                {
                    // Stop track
                    if (LOOP_track_run[track])
                    {
                        LOOP_track_run[track] = false;

                        // interrompi i Player di track
                        AudioNoInterrupts();
                        Players_Manager.Release_all_players_loop(track);
                        AudioInterrupts();

                        // spegni i led del loop
                        Loop_led_set.Request_track_LED_switch_off(track);
                    }

                    // Play track
                    else
                    {
                        // accendi il primo led del metronomo
                        // LOOP_metronomo.Led_ON(0);
                        AudioNoInterrupts();
                        // dopo uno stop a tutti i loop, alla prima ripartenza va azzerato LOOP_clock e va fatto ripartire il metronomo
                        if (!LOOP_metronomo_run)
                        {
                            // se LOOP_run_button_state == false va ripristinato
                            LOOP_run_button_state = true;

                            LOOP_restart_clock();

                            // calcolo prossimo evento metronomo
                            LOOP_metronomo.metro_time = 0 + LOOP_metronomo.Read_metro_delta_ms();

                            // avvia il metronomo
                            LOOP_metronomo_run = true;
                        }
                        LOOP_restart_procedure(track); // Procedura di ripartenza
                        AudioInterrupts();
                    }
                }

                if (LOOP_local_pointer.field_name == field_LOOP_TrackValues)
                {
                    // Change track parameters
                    switch (LOOP_local_pointer.track_value_element)
                    {
                    // Slide temporale
                    case value_LOOP_shift:
                    {
                        result = Read_encoder_simple(EN_PB_Track[track]);
                        if (result != 0)
                        {
                            LOOP_original = false;

                            int jump;
                            if (result == 1)
                            {
                                jump = 100;
                            }
                            else
                            {
                                if (LOOP_time >= 100)
                                {
                                    jump = LOOP_time - 100;
                                }
                                else
                                {
                                    jump = 0;
                                }
                            }

                            // Report
                            Serial.print("Shift ms:");
                            Serial.println(jump);

                            AudioNoInterrupts();
                            for (uint32_t event = 0; event < LOOP_events[track]; ++event)
                            {
                                LOOP_element[track][event].time = (LOOP_element[track][event].time + jump) % LOOP_time;
                            }

                            // Stop Players for this track
                            Players_Manager.Release_all_players_loop(track);

                            // Sort events by timestamp
                            LOOP_set_time_order(LOOP_learning_track);

                            // Restart procedure
                            LOOP_restart_procedure(track);
                            AudioInterrupts();

                            // Switch off track LEDs
                            Loop_led_set.Request_track_LED_switch_off(track);

                            LOOP_slide[track] = (LOOP_slide[track] + jump) % LOOP_time;

                            Display_MidiLoop.Show_track_all_data(track);
                        }

                        // REMOVED - Cancel time slide
                        /*
                            int jump = LOOP_time - LOOP_slide[track];

                            AudioNoInterrupts();
                            for (uint32_t event = 0; event < LOOP_events[track]; ++event)
                            {
                                LOOP_element[track][event].time = (LOOP_element[track][event].time + jump) % LOOP_time;
                            }

                            // Stop all track Players
                            Players_Manager.Release_all_players_loop(track);

                            // Effettua l'ordinamento temporale degli eventi
                            LOOP_set_time_order(LOOP_learning_track);

                            // Restart procedure
                            LOOP_restart_procedure(track);
                            AudioInterrupts();

                            // Switch off track LEDs
                            Loop_led_set.Request_track_LED_switch_off(track);

                            LOOP_slide[track] = (LOOP_slide[track] + jump) % LOOP_time;
                            Display_MidiLoop.Show_track_all_data(track);
                        */
                    }
                    break;

                    case value_LOOP_pitch:
                    {
                        if (Read_encoder(EN_PB_Track[track], LOOP_pitch_int[track], 24, -24, 1))
                        {
                            LOOP_original = false;

                            Display_MidiLoop.Show_track_all_data(track);

                            // Report
                            Serial.print("LOOP_pitch_int: ");
                            Serial.println(LOOP_pitch_int[track]);
                        }
                    }
                    break;

                    case value_LOOP_level:
                    {
                        if (Read_encoder(EN_PB_Track[track], LOOP_volume_int[track], 40, 0, 1))
                        {
                            LOOP_original = false;

                            AudioNoInterrupts();
                            LOOP_volume[track] = LOOP_volume_int[track] / 20.0f;
                            Players_Manager.Multicast_volume_for_MIDI_LOOP_running(track, LOOP_volume[track]);
                            AudioInterrupts();

                            Display_MidiLoop.Show_track_all_data(track);

                            // Report
                            Serial.print("LOOP_volume: ");
                            Serial.println(LOOP_volume[track]);
                        }
                    }
                    break;
                    }
                }
            }
        }

        // All track active commands (if MASTER_TRACK exists)
        if (LOOP_events[MASTER_TRACK] > 0)
        {
            // Update metronomo
            if (LOOP_metronomo_flag_IN[1])
            {
                LOOP_metronomo_flag_IN[1] = false;
                LOOP_metronomo.Update();
                LOOP_metronomo.metro_time += LOOP_metronomo.Read_metro_delta_ms();
            }

            // Choose menu item
            if (LOOP_local_pointer.field_name == field_LOOP_Menu)
            {
                if (Read_pushbutton(EN_PB_Select))
                {
                    switch (LOOP_local_pointer.menu_element)
                    {
                    case value_LOOP_New:
                    {
                        LOOP_stop_and_reset_runnig_loop_data(); // LOOP_track_run[track] = false; LOOP_metronomo_run == false; LOOP_metronomo_flag_IN[1] = false;
                        LOOP_id = NEW_LOOP;
                        LOOP_original = true;
                        LOOP_run_button_state = true;

                        Golive_with_MIDI_LOOP(true);
                    }
                    break;

                    case value_LOOP_Save:
                    {
                        const int saved_loop_id = LOOP_id == NEW_LOOP ? LOOP_Get_first_loop_id_free() : LOOP_id;
                        if (saved_loop_id < 0 || !LOOP_Copy_midi_loop_from_RAM_to_SD(saved_loop_id))
                        {
                            Serial.println(F("Loop save failed; RAM loop remains unsaved."));
                            Display_MidiLoop.Show_save_failed();
                            Clear_UI_events();
                            break;
                        }
                        LOOP_id = saved_loop_id;
                        LOOP_original = true;
                        Display_MidiLoop.Show_loop_id();

                        // Update menu and pointerMenu
                        Pointer_MidiLoop.Show_pointer(false);
                        LOOP_select_menu_elements();
                        Display_MidiLoop.Show_menu();
                        Pointer_MidiLoop.Set_pointer_to_first_menu_element();
                        LOOP_local_pointer = Pointer_MidiLoop.Get_pointer();

                        Clear_UI_events();
                    }
                    break;

                    case value_LOOP_SaveAsNew:
                    {
                        result = LOOP_Get_first_loop_id_free();
                        if (result >= 0)
                        {
                            if (!LOOP_Copy_midi_loop_from_RAM_to_SD(result))
                            {
                                Serial.println(F("Loop Save As New failed; loop ID unchanged."));
                                Display_MidiLoop.Show_save_failed();
                                Clear_UI_events();
                                break;
                            }
                            LOOP_id = result;

                            // Update menu
                            LOOP_original = true;

                            // Update menu and pointerMenu
                            Pointer_MidiLoop.Show_pointer(false);
                            LOOP_select_menu_elements();
                            Display_MidiLoop.Show_menu();
                            Pointer_MidiLoop.Set_pointer_to_first_menu_element();
                            LOOP_local_pointer = Pointer_MidiLoop.Get_pointer();

                            // Update loop_id
                            Display_MidiLoop.Show_loop_id();

                            Clear_UI_events();
                        }
                        else
                        {
                            Display_MidiLoop.Show_save_failed();
                            Clear_UI_events();
                        }
                    }
                    break;

                    case value_LOOP_Delete:
                    {
                        if (LOOP_id != NEW_LOOP && !LOOP_Delete_midi_loop_from_SD(LOOP_id))
                        {
                            Serial.println(F("Loop delete failed."));
                            Clear_UI_events();
                            break;
                        }

                        // new
                        LOOP_stop_and_reset_runnig_loop_data(); // LOOP_track_run[track] = false; LOOP_metronomo_run == false; LOOP_metronomo_flag_IN[1] = false;
                        LOOP_id = NEW_LOOP;
                        LOOP_original = true;
                        LOOP_run_button_state = true;

                        Golive_with_MIDI_LOOP(true);
                    }
                    break;
                    }
                }
            }

            // Change tempo
            if (Read_encoder_inverse(EN_PB_Tempo, LOOP_stretch_int, 198, 1, 1))
            {
                AudioNoInterrupts();
                // Memorizza il tempo virtuale attuale
                LOOP_clock_memo = LOOP_Clock();

                // Update LOOP_stretch
                if (LOOP_stretch_int <= 100)
                {
                    LOOP_stretch = LOOP_stretch_int / 100.0;
                }
                else
                {
                    LOOP_stretch = 1.0 / (2.0f - LOOP_stretch_int / 100.0f);
                }

                // Update LOOP_clock
                LOOP_clock = LOOP_clock_memo * LOOP_stretch;
                AudioInterrupts();

                Display_MidiLoop.Loop_total_time();

                // Report
                Serial.print("LOOP_stretch: ");
                Serial.println(LOOP_stretch);
            }

            // Back to original tempo
            if (Read_pushbutton(EN_PB_Tempo))
            {
                AudioNoInterrupts();
                // Memorizza il tempo virtuale attuale
                LOOP_clock_memo = LOOP_Clock();

                // Aggiorna LOOP_stretch
                LOOP_stretch_int = 100;
                LOOP_stretch = 1.0;

                // Ricalcolo LOOP_clock
                LOOP_clock = LOOP_clock_memo;
                AudioInterrupts();

                Display_MidiLoop.Loop_total_time();

                Serial.print("LOOP_stretch: ");
                Serial.println(LOOP_stretch);
            }

            // Start/Stop all tracks
            if (Read_pushbutton(EN_PB_Loop))
            {
                // stop all tracks
                if (LOOP_run_button_state)
                {
                    LOOP_run_button_state = false;

                    AudioNoInterrupts();

                    // memorizza lo stato dei track prima di fermarli
                    for (auto local_track = 0; local_track < TRACKS; ++local_track)
                    {
                        LOOP_track_run_memo[local_track] = LOOP_track_run[local_track];
                        LOOP_track_run[local_track] = false;
                    }

                    // Interrompi i Player di loop
                    Players_Manager.Release_all_players_loop();

                    // Ferma il metronomo
                    LOOP_metronomo_run = false;

                    // Annulla l'ultima richiesta di aggiornamento proveniente da MidiReader
                    LOOP_metronomo_flag_IN[1] = false;

                    AudioInterrupts();

                    // Aggiorna (spegni) tutti i led
                    Loop_led_set.Request_all_LED_switch_off();

                    // Spegni i led del metronomo
                    LOOP_metronomo.Leds_off();

                    // Accendi led_0
                    LOOP_metronomo.Led_ON(0);
                }

                // enable all tracks
                else
                {
                    LOOP_run_button_state = true;

                    AudioNoInterrupts();
                    LOOP_restart_clock();

                    // il led 0 e' gia' acceso, riavvia il metronomo
                    LOOP_metronomo.metro_time = 0 + LOOP_metronomo.Read_metro_delta_ms();
                    LOOP_metronomo_run = true;
                    for (auto local_track = 0; local_track < TRACKS; ++local_track)
                    {
                        if (LOOP_track_run_memo[local_track])
                        {
                            LOOP_restart_procedure(local_track);
                        }
                    }
                    AudioInterrupts();
                }
            }
        }

        // Switch verso un TOOL
        if (Read_pushbutton(PB_Tools))
        {
            TOOLS_pushbutton = true;
            Shifters_manager.Switch_led(LED_Tools, true);

            switch (Switches_manager.Get_value(SwitchTools))
            {
            case SwToolsMixer:
            {
                Lilla_state_0 = MIDI_LOOP;
                Switch_to_MIXER();
            }
            break;

            case SwToolsDelay:
            {
                Lilla_state_0 = MIDI_LOOP;
                Golive_DELAY_SETTINGS();
            }
            break;

            case SwToolsSetup:
            {
                Switch_from_MIDI_LOOP_to_SETUP();
            }
            break;

            case SwToolsTest:
            {
                Switch_from_MIDI_LOOP_to_MIDI_MONITOR();
            }
            break;
            }
        }

        // Switch Mode
        if (Switches_manager.Get_change(SwitchModes))
        {
            switch (Switches_manager.Get_value(SwitchModes))
            {
            case SwModesSampler:
            {
                Switch_from_MIDI_LOOP_to_DIRECT_SAMPLING();
            }
            break;

            case SwModesLiveSampler:
            {
                Switch_from_MIDI_LOOP_to_LIVE_SAMPLING();
            }
            break;

            case SwModesPerformance:
            {
                Switch_from_MIDI_LOOP_to_PERFORMANCE();
            }
            break;

            case SwModesMidiLoop:
                break;
            }
        }

        // Edit Sounds
        for (auto Inst_id = 0; Inst_id < INSTRUMENTS; ++Inst_id)
        {
            if (Read_pushbutton(PB_Sound[Inst_id]))
            {
                if (Patch[Patch_id].Instrument[Inst_id].used)
                {
                    Lilla_state_0 = MIDI_LOOP;
                    Lilla_state = SOUND_EDIT;

                    Instrument_id = Inst_id;
                    Sound_id = Get_sound_id(Patch_id, Instrument_id);

                    samples_in_file = Get_samples_in_raw_file(Sound[Sound_id].file);
                    Noclick_max = S_Calc_Noclick_max(Preset[Instrument_id].use_Wavetable);
                    S_trim_step = S_Calc_trim_step(trim_speed);

                    Display_Sound.Show_SOUND_page(Patch_id, Instrument_id);

                    // Menu
                    S_sound_original = S_Verify_is_Sound_original(Sound_id);
                    S_Select_menu_elements(); // updates "SO_menu_max" used by encoder_menu
                    Display_Sound.Show_SOUND_menu();

                    // Pointer
                    Pointer_Sound.Set_pointer_to_first_menu_element();
                    S_pointer = Pointer_Sound.Get_pointer();
                    Pointer_Sound.Display_pointer();

                    Performance_led_set.Restore_all_LED();

                    Display_Sound.Show_wave(Instrument_id);

                    Clear_UI_events();

                    // Report
                    Serial.print("Editing Instrument: ");
                    Serial.print(Instrument_id);
                    Print_Sound(Sound_id);
                }
            }
        }
    }
#pragma endregion // MIDI_LOOP

#pragma region[rgba(54, 135, 210, 0.2)]
    // *************************************************************
    // ********************      SETUP      ************************
    // *************************************************************
    if (Lilla_state == SETUP)
    {
        // Change Patch VOLUME
        if (Read_encoder(EN_PB_LineOutVol, volume_patch, PATCH_VOLUME_MAX, 0, 1))
        {
            AudioNoInterrupts();
            Players_Manager.Update_all_Preset_volume(Patch_id, Patch_volume_gain(volume_patch));
            Players_Manager.Broadcast_volume();
            AudioInterrupts();
        }

        // Set Key Step
        if (SET_menu == 0 && Read_encoder_inverse(EN_PB_Value, key_step, 3, 0, 1))
        {
            Display_Setup.SETUP_show_Key_step_value();
            Calc_pitch_from_note(key_step);
            Require_FRAM(Archive.Save_key_step(static_cast<uint8_t>(key_step)));
        }

        // Set Prima ottava
        if (SET_menu == 1 && Read_encoder(EN_PB_Value, first_octave, 0, -2, 1))
        {
            Display_Setup.SETUP_show_First_octave_value();
        }

        // Change menu item  -  uint8_t SET_menu;
        result = Read_encoder_simple(EN_PB_Select);
        if (result != 0)
        {
            SET_menu = (SET_menu + result + 7) % 7;
            Display_Setup.SETUP_show_frame(SET_menu);

            Clear_UI_events();
        }

        // Choose menu item
        if (Read_pushbutton(EN_PB_Select))
        {
            switch (SET_menu)
            {
            case 2: // switch to CC Settings
                Lilla_state = CC_SETTINGS;

                display_wait = false;

                for (auto local_instrument_id = 0; local_instrument_id < INSTRUMENTS; ++local_instrument_id)
                {
                    CC_Sound_gain_cache[local_instrument_id] = CC_Sound_gain[local_instrument_id];
                }

                CC_lowpass_filter_cache = CC_lowpass_filter_value;
                Display_Setup.CC_show_ControlChange_page();

                Display_Setup.CC_show_all_sound_gains();
                Display_Setup.CC_show_lowpass_filter_value();

                CC_menu = 0;
                Display_Setup.CC_show_frame_menu(CC_menu);

                Clear_UI_events();
                break;

                // case 2: // USB access to SD card - funzionalita' MTP
                // break;

            case 3: // import RAW files from SD
            {
                const bool resume_controls = Trigger.Is_running();
                if (!P_Quiesce_audio_players())
                {
                    break;
                }

                bool flash_changed = false;
                if (SET_Copy_audio_files_from_SD_to_Flash(flash_changed))
                {
                    VFS_Make_VFS();
                    DS_seed_all_Recordings();
                    File_scanner.Read_all_file_data();

                    // switch off Tools LED
                    TOOLS_pushbutton = false;
                    Shifters_manager.Switch_led(LED_Tools, false);

                    Reload_system_state();
                }
                else
                {
                    // Cancellation can resume the old inventory; a failed destructive import cannot.
                    if (!flash_changed && resume_controls)
                    {
                        AudioNoInterrupts();
                        const bool ready = S_Fill_all_tables();
                        if (ready)
                        {
                            Midi_reader.Start();
                            Trigger.Start();
                        }
                        AudioInterrupts();
                    }
                    if (flash_changed)
                    {
                        Serial.println(F("RAW import failed; audio remains stopped. Retry the import."));
                    }
                    Display_Setup.SETUP_show_SETUP_page();
                    Display_Setup.SETUP_show_frame(SET_menu);
                }
                break;
            }

            case 4: // Restore configuration and Recording audio from the backup root.
                Display_Storage.Confirm_config_import_popup();
                Display_Storage.Confirm_config_import_frame(0);
                SET_Ask_if_IMPORT_EXPORT_setup();
                if (result == 0)
                {
                    Display_Setup.SETUP_show_SETUP_page();
                    Display_Setup.SETUP_show_frame(SET_menu);
                    break;
                }

                // check SD presence
                if (!SD.begin(BUILTIN_SDCARD))
                {
                    Display_Setup.SETUP_show_SETUP_page();
                    Display_Setup.SETUP_show_frame(SET_menu);
                    break;
                }
                if (!SD.exists("/LILLABACKUP/LILLA_CONFIG.fram"))
                {
                    Display_Setup.SETUP_show_SETUP_page();
                    Display_Setup.SETUP_show_frame(SET_menu);
                    break;
                }
                else
                {
                    if (!P_Quiesce_audio_players())
                    {
                        break;
                    }
                    bool config_error = false;
                    if (!BACKUP_Restore(&config_error))
                    {
                        if (config_error)
                        {
                            Display_Setup.SETUP_show_SETUP_page();
                            Display_Setup.SETUP_show_frame(SET_menu);
                            break;
                        }
                        Serial.println(F("Full restore failed: check configuration, audio CRCs and packet capacity. Retry from /LILLABACKUP."));
                        Display_Storage.Config_import_FILE_error_popup();
                        delay(5000);
                        Reload_system_state();
                        break;
                    }
                    Serial.println(F("Configuration and Recording audio restored and verified."));

                    // switch off Tools LED
                    TOOLS_pushbutton = false;
                    Shifters_manager.Switch_led(LED_Tools, false);

                    Reload_system_state();
                }
                break;

            case 5: // Create a new numbered backup with Recording audio.
                Display_Storage.Confirm_config_export_popup();
                Display_Storage.Confirm_config_import_frame(0);
                SET_Ask_if_IMPORT_EXPORT_setup();
                if (result == 0)
                {
                    Display_Setup.SETUP_show_SETUP_page();
                    Display_Setup.SETUP_show_frame(SET_menu);
                    break;
                }

                // check SD presence
                if (SD.begin(BUILTIN_SDCARD))
                {
                    if (BACKUP_Export())
                    {
                        Display_Storage.Config_export_save_popup();
                    }
                    else
                    {
                        Display_Storage.Config_export_SD_error_popup();
                    }
                    delay(5000);
                    Display_Setup.SETUP_show_SETUP_page();
                    Display_Setup.SETUP_show_frame(SET_menu);
                }

                else
                {
                    Display_Storage.SD_missing(ILI9341_BLACK);
                    delay(5000);
                    Display_Setup.SETUP_show_SETUP_page();
                    Display_Setup.SETUP_show_frame(SET_menu);
                    break;
                }

                break;

            case 6: // Factory reset
                Display_Storage.Confirm_factory_reset_popup();
                Display_Storage.Confirm_config_import_frame(0);

                SET_Ask_if_FACTORY_RESET();
                if (result == 0)
                {
                    Display_Setup.SETUP_show_SETUP_page();
                    Display_Setup.SETUP_show_frame(SET_menu);
                    break;
                }
                Display_Storage.Factory_reset_wait_popup();

                delay(3000); // per ripensamenti last minute!

                // switch off Tools LED
                TOOLS_pushbutton = false;
                Shifters_manager.Switch_led(LED_Tools, false);

                if (!P_Quiesce_audio_players())
                {
                    break;
                }
                Factory_setup_FRAM();
                Reload_system_state();
                break;

            default:
                PRINT_ERROR(F("Switch MISSING! "));
                break;
            }
        }

        // Switch verso un TOOL
        if (Switches_manager.Get_change(SwitchTools))
        {
            switch (Switches_manager.Get_value(SwitchTools))
            {
            case SwToolsMixer:
            {
                if (first_octave != first_octave_cache)
                {
                    Require_FRAM(Archive.Save_first_octave(first_octave));
                }
                Switch_to_MIXER();
                break;
            }
            break;

            case SwToolsDelay:
            {
                if (first_octave != first_octave_cache)
                {
                    Require_FRAM(Archive.Save_first_octave(first_octave));
                }

                Golive_DELAY_SETTINGS();
                break;
            }
            break;

            case SwToolsSetup:
                break;

            case SwToolsTest:
            {
                if (first_octave != first_octave_cache)
                {
                    Require_FRAM(Archive.Save_first_octave(first_octave));
                }

                Golive_MIDI_MONITOR();
                break;
            }
            break;
            }
        }

        // Switch Mode
        if (Read_pushbutton(PB_Tools))
        {
            TOOLS_pushbutton = false;
            Shifters_manager.Switch_led(LED_Tools, false);

            switch (Switches_manager.Get_value(SwitchModes))
            {
            case SwModesSampler:
            {
                if (first_octave != first_octave_cache)
                {
                    Require_FRAM(Archive.Save_first_octave(first_octave));
                }

                switch (Lilla_state_0)
                {
                case PERFORMANCE:
                    Switch_to_DIRECT_SAMPLING();
                    break;

                case DIRECT_SAMPLING:
                    Lilla_state = DIRECT_SAMPLING;

                    Display_Sampler.DS_page_upper();
                    Display_Sampler.DS_page_lower(recording);

                    // Menu
                    DS_define_menu();
                    Display_Sampler.DS_menu(); // display the menu and updates DS_menu_max

                    // Pointer
                    Pointer_Sampler.Set_pointer_to_first_menu_element();
                    DS_local_pointer = Pointer_Sampler.Get_pointer();

                    // Display the VU meter
                    Display_Sampler.DS_bar(0, 0);
                    Display_Sampler.DS_bar(1, 0);

                    Clear_UI_events();
                    break;

                case LIVE_SAMPLING:
                    Switch_from_LIVE_SAMPLING_to_DIRECT_SAMPLING();
                    break;

                case MIDI_LOOP:
                    Switch_from_MIDI_LOOP_to_DIRECT_SAMPLING();
                    break;

                default:
                    PRINT_ERROR(F("Switch MISSING! "));
                    break;
                }
                break;
            }
            break;

            case SwModesLiveSampler:
            {
                if (first_octave != first_octave_cache)
                {
                    Require_FRAM(Archive.Save_first_octave(first_octave));
                }

                switch (Lilla_state_0)
                {
                case PERFORMANCE:
                    Switch_from_PERFORMANCE_to_LIVE_SAMPLING();
                    break;

                case DIRECT_SAMPLING:
                    Switch_from_DIRECT_SAMPLING_to_LIVE_SAMPLING();
                    break;

                case LIVE_SAMPLING:

                    LS_refresh_LS_page();
                    break;

                case MIDI_LOOP:
                    Switch_from_MIDI_LOOP_to_LIVE_SAMPLING();
                    break;

                default:
                    PRINT_ERROR(F("Switch MISSING! "));
                    break;
                }
                break;
            }
            break;

            case SwModesPerformance:
            {
                if (first_octave != first_octave_cache)
                {
                    Require_FRAM(Archive.Save_first_octave(first_octave));
                }

                switch (Lilla_state_0)
                {
                case PERFORMANCE:
                    Golive_with_PERFORMANCE(Patch_id);
                    break;

                case DIRECT_SAMPLING:
                    Switch_from_DIRECT_SAMPLING_to_PERFORMANCE();
                    break;

                case LIVE_SAMPLING:
                    Switch_from_LIVE_SAMPLING_to_PERFORMANCE();
                    break;

                case MIDI_LOOP:
                    Switch_from_MIDI_LOOP_to_PERFORMANCE();
                    break;

                default:
                    PRINT_ERROR(F("Switch MISSING! "));
                    break;
                }
                break;
            }
            break;

            case SwModesMidiLoop:
            {
                if (first_octave != first_octave_cache)
                {
                    Require_FRAM(Archive.Save_first_octave(first_octave));
                }
                switch (Lilla_state_0)
                {
                case PERFORMANCE:
                    Switch_from_PERFORMANCE_to_MIDI_LOOP();
                    break;

                case DIRECT_SAMPLING:
                    Switch_from_DIRECT_SAMPLING_to_MIDI_LOOP();
                    break;

                case LIVE_SAMPLING:
                    Switch_from_LIVE_SAMPLING_to_MIDI_LOOP();
                    break;

                case MIDI_LOOP:
                    Golive_with_MIDI_LOOP(false);
                    break;

                default:
                    PRINT_ERROR(F("Switch MISSING! "));
                    break;
                }
                break;
            }
            break;
            }
        }
    }

#pragma endregion // SETUP

#pragma region CC Settings [rgba(197, 197, 192, 0.37)]
    // *************************************************************
    // ********************   CC_SETTINGS   ************************
    // *************************************************************
    if (Lilla_state == CC_SETTINGS)
    {

        if (Read_encoder(EN_PB_Select, CC_menu, 9, 0, 1))
        {
            Display_Setup.CC_show_frame_menu(CC_menu);

            Clear_UI_events();

            if (CC_menu > 0 && CC_menu < 9)
            {
                CC_number = CC_Sound_gain[CC_menu - 1];
            }
            else if (CC_menu == 9)
            {
                CC_number = CC_lowpass_filter_value;
            }
        }

        if (Read_encoder(EN_PB_Value, CC_number, 127, 0, 1))
        {
            if (CC_menu > 0 && CC_menu < 9)
            {
                CC_Sound_gain[CC_menu - 1] = CC_number;
                Display_Setup.CC_show_sound_gain(CC_menu - 1);
            }
            else if (CC_menu == 9)
            {
                CC_lowpass_filter_value = CC_number;
                Display_Setup.CC_show_lowpass_filter_value();
            }
        }

        // scegli l'item
        if (Read_pushbutton(EN_PB_Value))
        {
            if (CC_menu > 0 && CC_menu < 9)
            {
                CC_Sound_gain[CC_menu - 1] = 0;
                Display_Setup.CC_show_sound_gain(CC_menu - 1);
            }
            else if (CC_menu == 9)
            {
                CC_lowpass_filter_value = 0;
                Display_Setup.CC_show_lowpass_filter_value();
            }
        }

        // Autolearning
        if (display_wait)
        {
            if (CC_menu > 0 && CC_menu < 9)
            {
                CC_Sound_gain[CC_menu - 1] = CC_midi_controller;
                Display_Setup.CC_show_sound_gain(CC_menu - 1);
                CC_number = CC_Sound_gain[CC_menu - 1];
            }
            else if (CC_menu == 9)
            {
                CC_lowpass_filter_value = CC_midi_controller;
                Display_Setup.CC_show_lowpass_filter_value();
                CC_number = CC_lowpass_filter_value;
            }
            display_wait = false;
        }

        // Return to SETUP
        if (Read_pushbutton(EN_PB_Select) && CC_menu == 0)
        {
            CC_Save_settings();
            Golive_SETUP();
        }
    }
}

#pragma endregion // CC_SETTINGS

// **************************************************************************************************************************
// **************************************************************************************************************************
// *************************************************   FUNCTIONS   **********************************************************
// **************************************************************************************************************************
// **************************************************************************************************************************

// Patch edit snapshots: capture and restore the editable model.
PatchEditSnapshot::PatchEditSnapshot(void) // Capture the editable model while preserving the caller's audio IRQ state.
{
    const bool enabled = NVIC_IS_ENABLED(IRQ_SOFTWARE) != 0;
    AudioNoInterrupts();
    patch = Patch[patch_id];
    for (int instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
    {
        if (patch.Instrument[instrument_id].used && !Capture_sound(patch.Instrument[instrument_id].sound_id))
        {
            break;
        }
    }
    if (enabled)
    {
        AudioInterrupts();
    }
}

PatchEditSnapshot::~PatchEditSnapshot(void)
{
    delete[] sounds;
}

bool PatchEditSnapshot::Capture_sound(int sound_id)
{
    if (!valid || sound_id < 0 || sound_id >= SOUNDS_MAX + 2)
    {
        valid = false;
        audio_tables_error_pending = true;
        return false;
    }
    for (size_t i = 0; i < count; ++i)
    {
        if (sounds[i].sound_id == sound_id)
        {
            return true;
        }
    }
    if (count == capacity)
    {
        const size_t next_capacity = capacity == 0 ? INSTRUMENTS : (capacity * 2 > SOUNDS_MAX + 2 ? SOUNDS_MAX + 2 : capacity * 2);
        SavedSound *expanded = new (std::nothrow) SavedSound[next_capacity];
        if (expanded == nullptr)
        {
            valid = false;
            audio_tables_error_pending = true;
            return false;
        }
        if (count != 0)
        {
            memcpy(expanded, sounds, count * sizeof(SavedSound));
        }
        delete[] sounds;
        sounds = expanded;
        capacity = next_capacity;
    }
    const bool enabled = NVIC_IS_ENABLED(IRQ_SOFTWARE) != 0;
    AudioNoInterrupts();
    sounds[count].sound_id = sound_id;
    sounds[count].value = Sound[sound_id];
    ++count;
    if (enabled)
    {
        AudioInterrupts();
    }
    return true;
}

const Sound_struct *PatchEditSnapshot::Find_sound(int sound_id) const
{
    for (size_t i = 0; i < count; ++i)
    {
        if (sounds[i].sound_id == sound_id)
        {
            return &sounds[i].value;
        }
    }
    return nullptr;
}

void PatchEditSnapshot::Restore(void) const // Restore the model and note maps after a failed preparation; published presets remain unchanged.
{
    const bool enabled = NVIC_IS_ENABLED(IRQ_SOFTWARE) != 0;
    AudioNoInterrupts();
    Patch_id = patch_id;
    Patch[patch_id] = patch;
    for (size_t i = 0; i < count; ++i)
    {
        Sound[sounds[i].sound_id] = sounds[i].value;
    }
    P_Update_all_maps_Instrument_for_notes();
    if (enabled)
    {
        AudioInterrupts();
    }
}

// ***************************************************************************************************************
// **********************************                   TABLES                  **********************************
// ***************************************************************************************************************
FLASHMEM
void Compile_tables(void)
{
    const float value_float = 16.0;
    Serial.println("void Compile_tables(void)");

    for (auto i = 0; i < 10; ++i)
    {
        m_exp_table[i] = exp_table[i + 1] - exp_table[i];
        Serial.println(m_exp_table[i], 20);

        m_sin_table[i] = sin_table[i + 1] - sin_table[i];
        m_decay_table[i] = decay_table[i + 1] - decay_table[i];
        m_release_table[i] = release_table[i + 1] - release_table[i];
    }
    for (auto i = 0; i <= 32; ++i)
    {
        pan_gain_L_table[i] = sin((value_float - (i - 16)) * 0.049087f); // Left channel , 0.049087 = M_PI/64.0
        pan_gain_R_table[i] = sin((value_float + (i - 16)) * 0.049087f); // Right channel , 0.049087 = M_PI/64.0
    }
}

// ***************************************************************************************************************
// **********************************                 UTILITIES                 **********************************
// ***************************************************************************************************************

void Calc_pitch_from_note(const int &key_step)
{
    const float keys = static_cast<float>(12 << key_step); // 12, 24, 48 or 96 MIDI keys per octave.
    for (auto note = 0; note < NOTE_NUMBERS; ++note)
    {
        pitch_from_note[note] = pow(2.0f, (note - 60.0f) / keys); // array used to translate note number to pitch value
    }

    return;
}

// ***************************************************************************************************************
// **********************************          INSTRUMENT MAPPING               **********************************
// ***************************************************************************************************************

void P_Reset_map_Instrument_for_notes(const int instrument_id)
{
    for (auto note = 0; note < NOTE_NUMBERS; ++note)
    {
        bitWrite(map_instrument_for_note[Get_midi_channel(Patch_id, instrument_id)][note], instrument_id, 0);
    }
}

void P_Delete_one_map_Instrument_for_notes(const int instrument_id)
{
    for (auto note = 0; note < NOTE_NUMBERS; ++note)
    {
        bitWrite(map_instrument_for_note[Get_midi_channel(Patch_id, instrument_id)][note], instrument_id, 0);
    }
}

void P_Update_all_maps_Instrument_for_notes()
{
    P_Reset_all_maps_Instrument_for_notes();
    for (auto instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
    {
        if (Patch[Patch_id].Instrument[instrument_id].used)
        {
            Update_map_Instrument_for_notes(Patch[Patch_id].Instrument[instrument_id].from_note, Patch[Patch_id].Instrument[instrument_id].to_note, instrument_id);
        }
    }
}

void S_Map_one_Instrument_for_all_notes(const int instrument_id)
{
    P_Reset_all_maps_Instrument_for_notes();
    for (auto note = 0; note < NOTE_NUMBERS; ++note)
    {
        bitWrite(map_instrument_for_note[Get_midi_channel(Patch_id, instrument_id)][note], instrument_id, 1);
    }
}

void P_Reset_all_maps_Instrument_for_notes()
{
    for (auto midi_ch = 0; midi_ch < 16; ++midi_ch)
    {
        for (auto note = 0; note < NOTE_NUMBERS; ++note)
        {
            map_instrument_for_note[midi_ch][note] = 0;
        }
    }
}

// ***************************************************************************************************************
// **********************************                 PERFORMANCE               **********************************
// ***************************************************************************************************************

FLASHMEM
void P_Delete_all_Patches_and_Sounds(void)
{
    const Patch_struct empty_patch{};

    for (auto patch_id = 0; patch_id < PATCHES_MAX; ++patch_id)
    {
        Patch[patch_id] = empty_patch;
    }

    Sound_struct empty_sound{};
    empty_sound.data = 2;

    for (auto sound_id = 0; sound_id < SOUNDS_MAX; ++sound_id)
    {
        Sound[sound_id] = empty_sound;
    }
}

void P_Read_all_Patches(void)
{
    for (auto patch_id = 0; patch_id < PATCHES_MAX; ++patch_id)
    {
        Require_FRAM(Archive.Read_Patch(patch_id));
    }
}

void P_Update_Patches_number(void)
{
    patches_number = 0;
    for (auto patch_id = 0; patch_id < PATCHES_MAX; ++patch_id)
    {
        if (Patch[patch_id].used)
        {
            ++patches_number;
        }
    }
}

uint8_t P_Get_first_Patch_id_existing(void)
{
    for (auto patch_id = 0; patch_id < PATCHES_MAX; ++patch_id)
    {
        if (Patch[patch_id].used)
        {
            return patch_id;
        }
    }
    return 0;
}

uint8_t P_Get_next_Patch_id_existing(void)
{
    uint8_t patch_id = Patch_id;
    do
    {
        ++patch_id;
        if (patch_id == PATCHES_MAX)
        {
            return Patch_id;
        }
        else if (Patch[patch_id].used)
        {
            return patch_id;
        }
    } while (1);
}

uint8_t P_Get_previous_Patch_id_existing(void)
{
    int patch_id = Patch_id;
    do
    {
        --patch_id;
        if (patch_id == -1)
        {
            return Patch_id;
        }
        if (Patch[patch_id].used)
        {
            return patch_id;
        }
    } while (1);
}

bool P_Verify_if_Instrument_original(const int patch_id, const int instrument_id)
{
    if (!Patch[patch_id].Instrument[instrument_id].used && !Patch_cache_P.Instrument[instrument_id].used)
    {
        return true;
    }

    return (Patch[patch_id].Instrument[instrument_id] == Patch_cache_P.Instrument[instrument_id]) && S_Verify_is_Sound_original(Get_sound_id(patch_id, instrument_id));
}

bool P_Rebuild_patch_old(void)
{
    Preset_struct next_presets[INSTRUMENTS] = {};
    uint16_t tables_mask = 0;
    if (!P_Prepare_audio_tables(Patch_id_old, Patch_volume_gain(volume_patch), next_presets, tables_mask, true))
    {
        audio_tables_error_pending = true;
        return false;
    }
    if (!Players_Manager.Activate_prepared_presets(next_presets))
    {
        Audio_tables.Cancel_prepare();
        audio_tables_error_pending = true;
        return false;
    }
    Players_Manager.Release_softly_all_players(Patch_id);
    Players_statistics.Reset_total_Players_per_instrument();
    Patch_id = Patch_id_old;
    if (Patch_id == Capture_new_patch)
    {
        Delay_manager.New_values(&Capture_patch_delay);
    }
    P_Update_line_of_all_instruments();
    P_Update_all_maps_Instrument_for_notes();
    return true;
}

FLASHMEM
int P_Ask_if_change_Patch(void)
{
    bool confirmation = false;
    int action = 0;

    Display_Performance.P_Confirm_patch_change_popup();
    Display_Performance.P_Confirm_patch_change_popup_frame(0);
    delay(200);

    Serial.println("OK P_Ask_if_change_Patch(void)");

    Clear_UI_events();
    while (!confirmation)
    {
        Shifters_manager.Update();

        if (Read_encoder(EN_PB_Select, action, 2, 0, 1))
        {
            Display_Performance.P_Confirm_patch_change_popup_frame(action);
        }

        if (Read_pushbutton(EN_PB_Select))
        {
            confirmation = true;
        }
    }
    Clear_UI_events();

    return action;
}

FLASHMEM
bool P_Ask_if_delete_this_Patch(void)
{
    bool confirmation = false;
    int action = 0; // NO

    Display_Performance.P_Confirm_patch_delete_popup();
    Display_Performance.P_Confirm_patch_delete_popup_frame(0);
    delay(200);

    Clear_UI_events();
    while (!confirmation)
    {
        Shifters_manager.Update();

        if (Read_encoder(EN_PB_Select, action, 1, 0, 1))
        {
            Display_Performance.P_Confirm_patch_delete_popup_frame(action);
        }
        if (Read_pushbutton(EN_PB_Select))
        {
            confirmation = true;
        }
    }
    Clear_UI_events();

    return (action == 1 ? true : false);
}

bool P_Jump_to_Patch(uint8_t next_patch)
{
    Delay_data_struct next_delay{};
    Require_FRAM(Archive.Read_Delay(next_patch, next_delay));
    Preset_struct next_presets[INSTRUMENTS] = {};
    uint16_t next_tables_mask = 0;
    if (!P_Prepare_audio_tables(next_patch, Patch_volume_gain(volume_patch), next_presets, next_tables_mask, true))
    {
        audio_tables_error_pending = true;
        return false;
    }
    const bool enabled = NVIC_IS_ENABLED(IRQ_SOFTWARE) != 0;
    AudioNoInterrupts();
    if (!Players_Manager.Activate_prepared_presets(next_presets))
    {
        Audio_tables.Cancel_prepare();
        audio_tables_error_pending = true;
        if (enabled)
        {
            AudioInterrupts();
        }
        return false;
    }
    Players_Manager.Release_softly_all_players(Patch_id);
    Players_statistics.Reset_total_Players_per_instrument();
    Patch_id = next_patch;
    Delay_manager.New_values(&next_delay);
    P_Update_all_maps_Instrument_for_notes();
    Players_Manager.Update_all_Preset_volume(Patch_id, Patch_volume_gain(volume_patch));
    uint8_t active_bank_mask = 0;
    for (uint8_t instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
    {
        active_bank_mask |= Audio_tables.Get_active_pointers(instrument_id).bank_mask;
    }
    if (enabled)
    {
        AudioInterrupts();
    }
    Serial.print(F("AudioTables patch activated, bank mask: 0x"));
    Serial.print(active_bank_mask, HEX);
    Serial.print(F(", instruments: 0x"));
    Serial.println(next_tables_mask, HEX);
    Patch_id_old = Patch_id;
    Patch_cache_P = Patch[Patch_id];
    S_Copy_all_Sound_to_Sound_cache_P();
    if (Capture_new_patch >= 0 && !Patch[Capture_new_patch].used)
    {
        Capture_new_patch = -1;
        Capture_target = -1;
    }
    LS_Capture_collect();
    P_Update_Patches_number();
    Golive_with_PERFORMANCE(Patch_id);
    return true;
}

FLASHMEM
bool P_Save_current_patch_as_new(void)
{
    if (!LS_Capture_materialize())
    {
        Golive_with_PERFORMANCE(Patch_id);
        return false;
    }
    const Delay_data_struct cloned_delay = Delay_data;
    const bool enabled = NVIC_IS_ENABLED(IRQ_SOFTWARE) != 0;
    AudioNoInterrupts();
    PatchEditSnapshot previous;
    const int new_patch = S_Get_Patch_id_free();
    if (!previous.valid || new_patch < 0)
    {
        if (enabled)
        {
            AudioInterrupts();
        }
        return false;
    }
    const Patch_struct unused_patch = Patch[new_patch];
    if (!S_Pull_all_Sound_from_Sound_cache_P(&previous))
    {
        previous.Restore();
        if (enabled)
        {
            AudioInterrupts();
        }
        return false;
    }
    Patch[previous.patch_id] = Patch_cache_P;
    Patch[new_patch] = previous.patch;
    bool complete = true;
    for (uint8_t instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
    {
        if (!Patch[new_patch].Instrument[instrument_id].used)
        {
            continue;
        }
        const int sound_id = S_Get_sound_free();
        if (sound_id < 0)
        {
            complete = false;
            break;
        }
        const Sound_struct *source = previous.Find_sound(previous.patch.Instrument[instrument_id].sound_id);
        if (source == nullptr)
        {
            complete = false;
            break;
        }
        const Sound_struct cloned_sound = *source;
        if (!previous.Capture_sound(sound_id))
        {
            complete = false;
            break;
        }
        Sound[sound_id] = cloned_sound;
        Patch[new_patch].Instrument[instrument_id].sound_id = sound_id;
    }
    Patch_id = new_patch;
    if (!complete || !S_Fill_all_tables())
    {
        Patch[new_patch] = unused_patch;
        previous.Restore();
        if (enabled)
        {
            AudioInterrupts();
        }
        return false;
    }
    Players_Manager.Release_softly_all_players(previous.patch_id);
    Players_statistics.Reset_total_Players_per_instrument();
    P_Update_all_maps_Instrument_for_notes();
    if (enabled)
    {
        AudioInterrupts();
    }
    // Persistent writes follow successful publication, so a table error cannot save a half-applied clone.
    if (!S_Save_all_Sounds_changed())
    {
        return false;
    }
    Require_FRAM(Archive.Save_Patch(Patch_id));
    Require_FRAM(Archive.Save_Delay(Patch_id, cloned_delay));
    if (Capture_new_patch == previous.patch_id)
    {
        Capture_new_patch = -1;
    }
    if (Capture_target == previous.patch_id)
    {
        Capture_target = Patch_id;
    }
    LS_Capture_finish_save();
    Archive.Copy_Patch_from_RAM_to_SD(Patch_id);
    P_Update_Patches_number();
    Patch_cache_P = Patch[Patch_id];
    S_Copy_all_Sound_to_Sound_cache_P();
    P_Update_line_of_all_instruments();
    Golive_with_PERFORMANCE(Patch_id);
    Print_Patch(Patch_id);
    return true;
}

void P_Macro_Instrument_editing(const int patch_id, const int instrument_id, const int element)
{
    switch (element)
    {
    case value_P_Lock: // Lock
        Display_Performance.P_show_Lock_value(patch_id, instrument_id, true);
        break;

    case value_P_Precedence: // Precedence
        Display_Performance.P_show_Precedence_value(patch_id, instrument_id, true);
        break;

    case value_P_Midi: // Midi (channel)
        Display_Performance.P_show_Midi_value(patch_id, instrument_id, true);
        break;

    case value_P_RootKey: // Root key
        Display_Performance.P_show_RootKey_value(patch_id, instrument_id, true);
        break;

    case value_P_FromKey: // From Key
        Display_Performance.P_show_FromKey_value(patch_id, instrument_id, true);
        break;

    case value_P_ToKey: // To key
        Display_Performance.P_show_ToKey_value(patch_id, instrument_id, true);
        break;

    case value_P_Pan: // Pan
        Display_Performance.P_show_Pan_value(patch_id, instrument_id, true);
        break;

    case value_P_Gain: // Gain
        Display_Performance.P_show_Gain_value(patch_id, instrument_id, true);
        break;
    }
}

bool P_Verify_is_Patch_original(const int patch_id)
{
    if (patch_id == Capture_new_patch)
    {
        return false;
    }
    bool result = true;

    if (Patch[patch_id].instruments == Patch_cache_P.instruments)
    {
        for (auto instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
        {
            result = result && P_Verify_if_Instrument_original(patch_id, instrument_id);
        }

        Serial.print("VERIFY_is_Patch_original: ");
        Serial.println(result);

        return result;
    }
    else
    {
        return false;
    }
}

void P_Select_menu_elements(void)
{
    // voices that can be displayed
    Menu_P[value_P_Exit] = true;
    Menu_P[value_P_Save] = true;
    Menu_P[value_P_Clone] = true;
    Menu_P[value_P_SaveAsNew] = true;
    Menu_P[value_P_DropPatch] = true;

    if (exibition)
    {
        Menu_P[value_P_Save] = false;      // Save
        Menu_P[value_P_Clone] = false;     // Clone
        Menu_P[value_P_SaveAsNew] = false; // SaveAsNew
        Menu_P[value_P_DropPatch] = false; // DropPatch
    }

    if (patch_original)
    {
        Menu_P[value_P_Exit] = false;      // Exit
        Menu_P[value_P_Save] = false;      // Save
        Menu_P[value_P_SaveAsNew] = false; // SaveAsNew
    }

    if (!patch_original)
    {
        Menu_P[value_P_Clone] = false; // Clone
    }

    if (patches_number == PATCHES_MAX)
    {
        Menu_P[value_P_Clone] = false;     // Clone
        Menu_P[value_P_SaveAsNew] = false; // SaveAsNew
    }

    if (patches_number == 1)
    {
        Menu_P[value_P_DropPatch] = false;
    }

    if (S_Get_sounds_free() < Patch[Patch_id].instruments)
    {
        Menu_P[value_P_Clone] = false;     // Clone
        Menu_P[value_P_SaveAsNew] = false; // SaveAsNew
    }

    P_menu_max = Menu_P[value_P_Exit] + Menu_P[value_P_Save] + Menu_P[value_P_Clone] + Menu_P[value_P_SaveAsNew] + Menu_P[value_P_DropPatch] - 1;
}

// ***************************************************************************************************************
// **********************************           SOUND, INSTRUMENT              ***********************************
// ***************************************************************************************************************

FLASHMEM const char *S_Auto_tune_pitch(int sound_id)
{
    const auto &sound = Sound[sound_id];
    if (sound.B <= sound.A)
    {
        return "AUTO-TUNE: INVALID RANGE";
    }

    // A timed-out request keeps draining its complete window. Never overwrite its samples.
    const uint32_t pending_started = millis();
    while (!AutoTune_input.Idle())
    {
        if (static_cast<uint32_t>(millis() - pending_started) >= 100u)
        {
            return "AUTO-TUNE: AUDIO BUSY";
        }
        yield();
    }

    const AudioFileSource source = PatchCache_Manager.Get_source(sound.file);
    if (source.samples <= sound.B)
    {
        return "AUTO-TUNE: SOURCE UNAVAILABLE";
    }

    static DMAMEM int16_t window[1024];
    static DMAMEM int16_t short_loop[1024];
    const uint32_t length = sound.B - sound.A + 1u;
    const bool short_grain = length < 1024u;
    const bool looping = sound.mode >= LOOP_FWD;
    const bool pingpong = sound.mode == LOOP_FWD_REV || sound.mode == LOOP_REV_FWD;
    const auto &preset = Preset[Instrument_id];
    const uint32_t crossfade = looping && !pingpong ? static_cast<uint32_t>(preset.Noclick) : 0u;
    const uint32_t period = pingpong ? 2u * (length - 1u) : length - crossfade;

    AudioTables::Pointers tables;
    if (short_grain)
    {
        AudioNoInterrupts();
        tables = Audio_tables.Get_active_pointers(Instrument_id, preset);
        AudioInterrupts();

        if ((preset.use_Wavetable && tables.wavetable == nullptr) || (looping && crossfade > 0 && tables.noclick == nullptr))
        {
            return "AUTO-TUNE: TABLES UNAVAILABLE";
        }

        if (!preset.use_Wavetable)
        {
            if (source.storage == Psram && source.psram_ptr != nullptr)
            {
                memcpy(short_loop, source.psram_ptr + sound.A, length * sizeof(int16_t));
            }
            else if (!LillaSerialFlashFile::Read_audio_samples(sound.file, short_loop, sound.A, length))
            {
                return "AUTO-TUNE: READ FAILED";
            }
        }
    }

    float peak = 0.0f;
    unsigned int peak_bin = 0;
    float left = 0.0f;
    float right = 0.0f;
    const uint32_t positions = length > 1024u ? length - 1024u : 0u;
    const unsigned int windows = positions == 0 ? 1 : 4;

    for (unsigned int pass = 0; pass < windows; ++pass)
    {
        const uint32_t first = sound.A + (windows == 1 ? 0u : positions * pass / (windows - 1u));

        if (short_grain)
        {
            // Repeat loop playback periods; preserve one-shot direction and pad the remaining FFT window with silence.
            for (uint32_t i = 0; i < 1024u; ++i)
            {
                if (!looping)
                {
                    if (i >= length)
                    {
                        window[i] = 0;
                    }
                    else if (preset.use_Wavetable)
                    {
                        window[i] = tables.wavetable[sound.mode == ONCE_REV ? length - 1u - i : i];
                    }
                    else
                    {
                        window[i] = short_loop[sound.mode == ONCE_REV ? length - 1u - i : i];
                    }
                }
                else
                {
                    const uint32_t phase = i % period;
                    if (preset.use_Wavetable)
                    {
                        window[i] = tables.wavetable[phase];
                    }
                    else if (pingpong)
                    {
                        window[i] = short_loop[phase < length ? phase : period - phase];
                    }
                    else if (sound.mode == LOOP_REV)
                    {
                        const uint32_t index = period - 1u - phase;
                        window[i] = index < crossfade ? tables.noclick[index] : short_loop[index];
                    }
                    else
                    {
                        const uint32_t plain = length - 2u * crossfade;
                        window[i] = phase < plain ? short_loop[crossfade + phase] : tables.noclick[phase - plain];
                    }
                }
            }
        }
        else if (source.storage == Psram && source.psram_ptr != nullptr)
        {
            memcpy(window, source.psram_ptr + first, sizeof(window));
        }
        else if (!LillaSerialFlashFile::Read_audio_samples(sound.file, window, first, 1024))
        {
            return "AUTO-TUNE: READ FAILED";
        }

        int32_t sum = 0;
        for (int16_t sample : window)
        {
            sum += sample;
        }

        const int32_t mean = sum / 1024;
        int32_t amplitude = 0;
        for (int16_t sample : window)
        {
            amplitude = max(amplitude, abs(static_cast<int32_t>(sample) - mean));
        }

        if (amplitude <= 2)
        {
            continue;
        }

        // Normalize only the analysis copy so quiet recordings retain FFT precision.
        for (int16_t &sample : window)
        {
            sample = (static_cast<int32_t>(sample) - mean) * 30000 / amplitude;
        }

        AutoTune_fft.available();

        AudioNoInterrupts();
        AutoTune_input.Begin(window);
        AudioInterrupts();

        const uint32_t started = millis();
        while (!AutoTune_input.Idle())
        {
            if (static_cast<uint32_t>(millis() - started) >= 100u)
            {
                return "AUTO-TUNE: FFT TIMEOUT";
            }
            yield();
        }

        // The feeder precedes the FFT in the audio IRQ. After all eight blocks, the last output contains only this window, even when the FFT retained 512 old samples.
        if (!AutoTune_fft.available())
        {
            return "AUTO-TUNE: FFT NO RESULT";
        }

        const float original_scale = amplitude / 30000.0f;
        for (unsigned int bin = 1; bin < 511; ++bin)
        {
            const float magnitude = AutoTune_fft.read(bin) * original_scale;
            if (magnitude > peak)
            {
                peak = magnitude;
                peak_bin = bin;
                left = AutoTune_fft.read(bin - 1) * original_scale;
                right = AutoTune_fft.read(bin + 1) * original_scale;
            }
        }
    }
    if (peak <= 0.0f)
    {
        return "AUTO-TUNE: NO SIGNAL";
    }

    const float curvature = left - 2.0f * peak + right;
    const float offset = curvature < 0.0f ? constrain(0.5f * (left - right) / curvature, -0.5f, 0.5f) : 0.0f;
    float source_frequency = (peak_bin + offset) * AUDIO_SAMPLE_RATE / 1024.0f;

    if (short_grain && looping && period <= 1024u)
    {
        // A repeated short loop has spectral lines at integer multiples of its exact period.
        const float harmonic = max(1.0f, roundf(source_frequency * period / AUDIO_SAMPLE_RATE));
        source_frequency = harmonic * AUDIO_SAMPLE_RATE / period;
    }

    const float frequency = source_frequency * powf(2.0f, sound.pitch / 192.0f);
    const float nearest_note = roundf(69.0f + 12.0f * log2f(frequency / 440.0f));
    const float target_frequency = 440.0f * powf(2.0f, (nearest_note - 69.0f) / 12.0f);
    const int corrected_pitch = static_cast<int>(sound.pitch + roundf(192.0f * log2f(target_frequency / frequency)));

    if (corrected_pitch < -96 || corrected_pitch > 96)
    {
        return "AUTO-TUNE: PITCH LIMIT";
    }

    Sound[sound_id].pitch = corrected_pitch;

    return nullptr;
}

void S_Refresh_source_limits(bool force) // Keep the Sound display aligned with the preset source without scanning players or blocking audio.
{
    static uint32_t last_ms = 0;
    static int displayed_instrument = -1;
    static int displayed_file = -1;
    static float displayed_pitch_limit = -1.0f;
    static int displayed_voices = -1;
    const uint32_t now_ms = millis();
    if (!force && static_cast<uint32_t>(now_ms - last_ms) < 20u)
    {
        return;
    }
    last_ms = now_ms;
    const auto &preset = Preset[Instrument_id];
    const bool live = preset.file >= FIRST_LIVE_SAMPLING_FILE;
    const float limit = Playback_pitch_limit(preset.use_Wavetable, preset.source.storage == Psram, live);
    const int voices = PLAYERS;
    if (force || displayed_instrument != Instrument_id || displayed_file != preset.file || displayed_pitch_limit != limit || displayed_voices != voices)
    {
        Display_Sound.Show_players_Pitch_max_value(Instrument_id); // Redraw only changed limits, except when a new page entry requires a fresh display.
        displayed_instrument = Instrument_id;
        displayed_file = preset.file;
        displayed_pitch_limit = limit;
        displayed_voices = voices;
    }
}

bool S_Verify_is_Sound_original(const int sound_id)
{
    return Sound[sound_id] == S_Sound_cache_P[sound_id];
}

void S_Copy_all_Sound_to_Sound_cache_P(void)
{
    for (auto sound_id = 0; sound_id < SOUNDS_MAX; ++sound_id)
    {
        S_Sound_cache_P[sound_id] = Sound[sound_id];
    }
}

FLASHMEM
bool S_Save_all_Sounds_changed(void)
{
    if (!LS_Capture_materialize())
    {
        return false;
    }
    for (auto sound_id = 0; sound_id < SOUNDS_MAX; ++sound_id)
    {
        // Sound which have been changed only for .used
        if (Sound[sound_id].used != S_Sound_cache_P[sound_id].used)
        {
            Require_FRAM(Archive.Save_Sound(sound_id));
            Serial.println("S_Save_all_Sounds_changed: attenzione! Sound[sound_id].used e' variato per sound_id: ");
            Serial.println(sound_id);
        }

        // Sound used which have been changed
        else if ((Sound[sound_id].used == 1) && !S_Verify_is_Sound_original(sound_id)) // save Sound used and changed in phisical properties
        {
            Require_FRAM(Archive.Save_Sound(sound_id));
            Serial.println("S_Save_all_Sounds_changed: attenzione! S_Verify_is_Sound_original ha dato esito NEGATIVO che ha richiesto salvataggio su FRAM per sound_id: ");
            Serial.println(sound_id);
        }
    }
    return true;
}

bool S_Pull_all_Sound_from_Sound_cache_P(PatchEditSnapshot *snapshot)
{
    for (auto sound_id = 0; sound_id < SOUNDS_MAX; ++sound_id)
    {
        if (snapshot != nullptr && memcmp(&Sound[sound_id], &S_Sound_cache_P[sound_id], sizeof(Sound_struct)) != 0 && !snapshot->Capture_sound(sound_id))
        {
            audio_tables_error_pending = true;
            return false;
        }
        Sound[sound_id] = S_Sound_cache_P[sound_id];
    }
    return true;
}

uint16_t S_Get_sounds_free(void)
{
    auto result = 0;

    for (auto sound_id = 0; sound_id < SOUNDS_MAX; ++sound_id)
    {
        if (!Sound[sound_id].used)
        {
            result++;
        }
    }
    return result;
}

bool S_Read_all_Sounds(PatchEditSnapshot *snapshot)
{
    for (auto sound_id = 0; sound_id < SOUNDS_MAX; ++sound_id)
    {
        if (snapshot != nullptr && !snapshot->Capture_sound(sound_id))
        {
            audio_tables_error_pending = true;
            return false;
        }
        Require_FRAM(Archive.Read_Sound(sound_id));
    }
    return true;
}

void S_Set_midi_channel_for_Sound(int sound_id, int midi_channel)
{
    // .data contains midi channel in its bits: 7 6 5 M I D I 0
    Sound[sound_id].data = (midi_channel << 1) + (Sound[sound_id].data & 0b11100000);
}

uint8_t S_Get_midi_channel_from_Sound(int sound_id)
{
    // .data contains midi channel in its bits: 7 6 5 M I D I 0
    return ((Sound[sound_id].data & 30) >> 1);
}

int S_Get_Patch_id_free(void)
{
    for (auto local_patch = 0; local_patch < PATCHES_MAX; ++local_patch)
    {
        if (!Patch[local_patch].used)
        {
            return local_patch;
        }
    }
    return -1;
}

int S_Get_sound_free(void)
{
    for (auto sound_id = 0; sound_id < SOUNDS_MAX; ++sound_id)
    {
        if (!Sound[sound_id].used)
        {
            return sound_id;
            Sound[sound_id].used = true;
        }
    }
    return -1;
}

void S_Set_Sound_SOLO_OFF(void)
{
    AudioNoInterrupts();
    if (solo_flag)
    {
        solo_flag = false;
        P_Update_all_maps_Instrument_for_notes();
    }
    AudioInterrupts();
}

uint32_t S_Calc_trim_step(int value)
{
    value = value % 6;
    switch (value)
    {
    case 0:
        return 1;
        break;
    case 1:
        return 10;
        break;
    case 2:
        return 100;
        break;
    case 3:
        return 1000;
        break;
    case 4:
        return 10000;
        break;
    case 5:
        return (Sound[Sound_id].B - Sound[Sound_id].A + 1) / 16;
        break;

    default:
        return 1;
        break;
    }
}

void S_Drop_Instrument(const int instrument_id)
{
    Sound[Get_sound_id(Patch_id, instrument_id)].used = false;
    Patch[Patch_id].Instrument[instrument_id].used = false;
    Patch[Patch_id].instruments--;
    Preset[instrument_id] = {};
    PatchCache_Manager.Set_required_files(Preset);
    Players_Manager.Refresh_cache_sources();
}

bool S_Clone_Instrument(const int instrument_id, int &new_instrument, PatchEditSnapshot &snapshot)
{
    for (new_instrument = 0; new_instrument < INSTRUMENTS; ++new_instrument)
    {
        if (!Patch[Patch_id].Instrument[new_instrument].used)
        {
            Patch[Patch_id].Instrument[new_instrument] = Patch[Patch_id].Instrument[instrument_id];
            int sound_id_new = S_Get_sound_free();
            if (sound_id_new >= 0)
            {
                if (!snapshot.Capture_sound(sound_id_new))
                {
                    audio_tables_error_pending = true;
                    return false;
                }
                Sound[sound_id_new] = Sound[Get_sound_id(Patch_id, instrument_id)];
                Sound[sound_id_new].gain = 0;
                Patch[Patch_id].Instrument[new_instrument].sound_id = sound_id_new;
                Patch[Patch_id].instruments++;
                return true;
            }
            else
            {
                return false;
            }
        }
    }
    return false;
}

// ***************************************************************************************************************
// **********************************                 LEDS                     ***********************************
// ***************************************************************************************************************

void Update_instruments_leds()
{
    if (Lilla_state == MIDI_LOOP)
    {
        for (auto track = 0; track < TRACKS; ++track)
        {
            for (auto instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
            {
                if (Patch[Patch_id].Instrument[instrument_id].used) // check if i is used
                {
                    // check led activity
                    const int activity = Loop_led_set.Consume_LED_activity(track, instrument_id);
                    if (activity == 2)
                    {
                        Display_MidiLoop.Loop_led(track, instrument_id, true);
                    }
                    if (activity == -2)
                    {
                        Display_MidiLoop.Loop_led(track, instrument_id, false);
                    }
                }
            }
        }
    }

    else if (Lilla_state == PERFORMANCE)
    {
        for (auto instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
        {
            if (Patch[Patch_id].Instrument[instrument_id].used) // check if i is used
            {
                // // check led activity
                const int activity = Performance_led_set.Consume_LED_activity(instrument_id);
                if (activity == 2)
                {
                    Display_Performance.Led_PERFORMANCE_instrument(instrument_id, true);
                }
                if (activity == -2)
                {
                    Display_Performance.Led_PERFORMANCE_instrument(instrument_id, false);
                }
            }
        }

        if (TT_led_flag) // TUNING_TONE
        {
            TT_led_flag = false;
            Display_Performance.Led_tuning_tone(Patch_id);
        }
    }

    else if (Lilla_state == SOUND_EDIT)
    {
        // check led activity
        const int activity = Performance_led_set.Consume_LED_activity(Instrument_id);
        if (activity == 2)
        {
            Display_Sound.Led_SOUND_EDIT_instrument(Instrument_id, true);
        }
        if (activity == -2)
        {
            Display_Sound.Led_SOUND_EDIT_instrument(Instrument_id, false);
        }

        if (TT_led_flag) // TUNING_TONE
        {
            TT_led_flag = false;
        }
    }

    else if (Lilla_state == INSTRUMENT_VCF)
    {
        // check led activity
        const int activity = Performance_led_set.Consume_LED_activity(Instrument_id);
        if (activity == 2)
        {
            Display_VCF.Led_INSTRUMENT_VCF_instrument(Instrument_id, true);
        }
        if (activity == -2)
        {
            Display_VCF.Led_INSTRUMENT_VCF_instrument(Instrument_id, false);
        }

        if (TT_led_flag) // TUNING_TONE
        {
            TT_led_flag = false;
        }
    }

    else if (Lilla_state == LIVE_SAMPLING)
    {
        // check led activity

        const int activity = Performance_led_set.Consume_LED_activity(0);
        if (activity == 2)
        {
            Display_LiveSampler.Led_LIVE_SAMPLING(true);
        }

        else if (activity == -2)
        {
            Display_LiveSampler.Led_LIVE_SAMPLING(false);
        }

        if (TT_led_flag) // TUNING_TONE
        {
            TT_led_flag = false;
        }
    }
    else if (Lilla_state == DIRECT_SAMPLING)
    {

        // Snapshot and acknowledge both instruments without losing requests from the audio IRQ.
        const bool audio_enabled = NVIC_IS_ENABLED(IRQ_SOFTWARE) != 0;
        AudioNoInterrupts();
        const int left_activity = Performance_led_set.Consume_LED_activity(0);
        const int right_activity = Performance_led_set.Consume_LED_activity(1);
        const bool activity_changed = abs(left_activity) == 2 || abs(right_activity) == 2;
        if (audio_enabled)
        {
            AudioInterrupts();
        }

        // Draw outside the IRQ lock; either active instrument keeps the shared LED on.
        if (DS_recording_led_visible && (activity_changed || DS_recording_led_redraw))
        {
            Display_Sampler.Led_DIRECT_SAMPLING(left_activity > 0 || right_activity > 0);
        }
        DS_recording_led_redraw = false;

        if (TT_led_flag) // TUNING_TONE
        {
            TT_led_flag = false;
        }
    }
    else
    {
        return;
    }
}

// ***************************************************************************************************************
// ******************************************       DIRECT_SAMPLING       ****************************************
// ***************************************************************************************************************

bool DS_setup_DIRECT_SAMPLING_Patch_and_Preset(void)
{
    PatchEditSnapshot previous;
    if (!previous.Capture_sound(SOUNDS_MAX) || !previous.Capture_sound(SOUNDS_MAX + 1))
    {
        audio_tables_error_pending = true;
        return false;
    }
    const Patch_struct previous_sampler_patch = Patch[PATCHES_MAX];
    const int previous_recording = recording;
    DS_set_DS_Sampling_Patch();
    Patch_id = PATCHES_MAX;
    recording = DS_get_last_Recording();

    PRINT_CONTROL_POINT(recording);

    // Set up Sound parameters
    if (recording >= 0)
    {
        Sound[SOUNDS_MAX].file = 2 * recording + FIRST_RECORDING_FILE;
        Sound[SOUNDS_MAX].B = DS_get_samples_in_Recording(recording) - 1;

        Sound[SOUNDS_MAX + 1].file = Sound[SOUNDS_MAX].file + (Recording[recording].stereo ? 1 : 0);
        Sound[SOUNDS_MAX + 1].B = Sound[SOUNDS_MAX].B;
        P_Recording(recording);
    }
    else
    {
        Sound[SOUNDS_MAX].file = 0;
        Sound[SOUNDS_MAX].B = 100000;

        Sound[SOUNDS_MAX + 1].file = 0;
        Sound[SOUNDS_MAX + 1].B = 100000;
    }
    if (!S_Fill_all_tables())
    {
        Patch[PATCHES_MAX] = previous_sampler_patch;
        previous.Restore();
        recording = previous_recording;
        return false;
    }
    P_Update_all_maps_Instrument_for_notes();
    Turn_ON_Delay(false);
    return true;
}

bool DS_export_wav_to_SD(void)
{
    char packet_filename[NAME_PACKET_SIZE];
    const auto &entry = Recording[recording];
    const int channels = entry.stereo ? 2 : 1;
    if (entry.first_packet < 0 || entry.packets <= 0 || entry.packets > VFS_PACKETS_MAX / channels || entry.first_packet > VFS_PACKETS_MAX - entry.packets * channels || entry.bytes <= 0 || entry.bytes > entry.packets * PACKET_DIM || (entry.bytes & 1) != 0)
    {
        return false;
    }
    if (!SD.begin(BUILTIN_SDCARD) || (!SD.exists("/LILLAWAV_EXPORT") && !SD.mkdir("/LILLAWAV_EXPORT")))
    {
        return false;
    }
    char paths[2][48];
    bool available = false;
    for (int id = 0; id < 100000; ++id)
    {
        snprintf(paths[0], sizeof(paths[0]), "/LILLAWAV_EXPORT/%dM.wav", id);
        snprintf(paths[1], sizeof(paths[1]), "/LILLAWAV_EXPORT/%dS.wav", id);
        if (!SD.exists(paths[0]) && !SD.exists(paths[1]))
        {
            available = true;
            break;
        }
    }
    if (!available)
    {
        return false;
    }
    const char *path = paths[entry.stereo ? 1 : 0];
    FsFile destination = SD.sdfs.open(path, O_WRONLY | O_CREAT | O_EXCL);
    if (!destination)
    {
        return false;
    }

    // Serialize the canonical 44-byte PCM header explicitly in little-endian order.
    byte header[44] = {};
    const auto put16 = [&header](int offset, uint16_t value)
    {
        header[offset] = static_cast<byte>(value);
        header[offset + 1] = static_cast<byte>(value >> 8);
    };
    const auto put32 = [&header](int offset, uint32_t value)
    {
        header[offset] = static_cast<byte>(value);
        header[offset + 1] = static_cast<byte>(value >> 8);
        header[offset + 2] = static_cast<byte>(value >> 16);
        header[offset + 3] = static_cast<byte>(value >> 24);
    };
    const uint32_t data_bytes = static_cast<uint32_t>(entry.bytes) * channels;
    memcpy(header, "RIFF", 4);
    put32(4, data_bytes + 36);
    memcpy(header + 8, "WAVEfmt ", 8);
    put32(16, 16);
    put16(20, 1);
    put16(22, channels);
    put32(24, 44100);
    put32(28, 44100 * channels * 2);
    put16(32, channels * 2);
    put16(34, 16);
    memcpy(header + 36, "data", 4);
    put32(40, data_bytes);
    bool exported = destination.write(header, sizeof(header)) == sizeof(header);
    byte channel_buffer[2][256];
    byte interleaved[512];
    uint32_t remaining = static_cast<uint32_t>(entry.bytes);
    for (int index = 0; remaining > 0 && exported; ++index)
    {
        SerialFlashFile source[2];
        for (int channel = 0; channel < channels; ++channel)
        {
            source[channel] = SerialFlash.open(Get_packet_name(entry.first_packet + index * channels + channel, packet_filename));
            if (!source[channel])
            {
                exported = false;
                break;
            }
        }
        uint32_t packet_remaining = remaining < PACKET_DIM ? remaining : PACKET_DIM;
        while (packet_remaining > 0 && exported)
        {
            const uint32_t count = packet_remaining < sizeof(channel_buffer[0]) ? packet_remaining : sizeof(channel_buffer[0]);
            for (int channel = 0; channel < channels; ++channel)
            {
                if (source[channel].read(channel_buffer[channel], count) != count)
                {
                    exported = false;
                    break;
                }
            }
            if (!exported)
            {
                break;
            }
            const byte *output = channel_buffer[0];
            if (entry.stereo)
            {
                for (uint32_t offset = 0; offset < count; offset += 2)
                {
                    interleaved[offset * 2] = channel_buffer[0][offset];
                    interleaved[offset * 2 + 1] = channel_buffer[0][offset + 1];
                    interleaved[offset * 2 + 2] = channel_buffer[1][offset];
                    interleaved[offset * 2 + 3] = channel_buffer[1][offset + 1];
                }
                output = interleaved;
            }
            exported = destination.write(output, count * channels) == count * channels;
            packet_remaining -= count;
            remaining -= count;
        }
        for (int channel = 0; channel < channels; ++channel)
        {
            source[channel].close();
        }
    }
    const bool synced = destination.sync();
    const bool sized = destination.fileSize() == data_bytes + sizeof(header);
    const bool closed = destination.close();
    exported = exported && synced && sized && closed;
    if (!exported && !SD.remove(path))
    {
        Serial.print(F("Unable to remove incomplete export: "));
        Serial.println(path);
    }
    return exported;
}

void DS_ask_if_EXIT_from_DS(void)
{
    confirmation = false;
    action = 0; // NO
    Display_Sampler.DS_confirm_EXIT_from_DS();
    Display_Common.Confirm_no_yes_popup_frame(0);
    delay(200);

    Clear_UI_events();
    while (!confirmation)
    {
        Shifters_manager.Update();

        if (Read_encoder(EN_PB_Select, action, 1, 0, 1))
        {
            Display_Common.Confirm_no_yes_popup_frame(action);
        }
        if (Read_pushbutton(EN_PB_Select))
        {
            confirmation = true;
        }
    }
    Clear_UI_events();
}

bool DS_Jump_to_DIRECT_SAMPLING_recording(int &recording)
{
    const Sound_struct previous_left = Sound[SOUNDS_MAX];
    const Sound_struct previous_right = Sound[SOUNDS_MAX + 1];
    const int previous_recording = Preset[0].file >= FIRST_RECORDING_FILE && Preset[0].file < FIRST_LIVE_SAMPLING_FILE ? (Preset[0].file - FIRST_RECORDING_FILE) / 2 : -1;
    if (recording >= 0)
    {
        Sound[SOUNDS_MAX].file = 2 * recording + FIRST_RECORDING_FILE;
        Sound[SOUNDS_MAX].B = DS_get_samples_in_Recording(recording) - 1;

        Sound[SOUNDS_MAX + 1].file = Sound[SOUNDS_MAX].file + (Recording[recording].stereo ? 1 : 0);
        Sound[SOUNDS_MAX + 1].B = Sound[SOUNDS_MAX].B;
    }
    else
    {
        Sound[SOUNDS_MAX].file = 0;
        Sound[SOUNDS_MAX].B = 100000;
        Sound[SOUNDS_MAX + 1].file = 0;
        Sound[SOUNDS_MAX + 1].B = 100000;
    }

    AudioNoInterrupts();
    if (!S_Fill_all_tables())
    {
        Sound[SOUNDS_MAX] = previous_left;
        Sound[SOUNDS_MAX + 1] = previous_right;
        recording = previous_recording;
        AudioInterrupts();
        return false;
    }
    AudioInterrupts();

    Display_Sampler.DS_hide_recording();
    Display_Sampler.DS_Recording_description(recording, true);

    // Restore LEDs
    Performance_led_set.Restore_all_LED();

    // Report
    Print_Sound(SOUNDS_MAX);
    Print_Sound(SOUNDS_MAX + 1);
    return true;
}

bool DS_back_to_first_DS_Recording(void)
{
    const Sound_struct previous_left = Sound[SOUNDS_MAX];
    const Sound_struct previous_right = Sound[SOUNDS_MAX + 1];
    const int previous_recording = Preset[0].file >= FIRST_RECORDING_FILE && Preset[0].file < FIRST_LIVE_SAMPLING_FILE ? (Preset[0].file - FIRST_RECORDING_FILE) / 2 : -1;
    if (recording >= 0)
    {
        Sound[SOUNDS_MAX].file = 2 * recording + FIRST_RECORDING_FILE;
        Sound[SOUNDS_MAX].B = DS_get_samples_in_Recording(recording) - 1;

        Sound[SOUNDS_MAX + 1].file = Sound[SOUNDS_MAX].file + (Recording[recording].stereo ? 1 : 0);
        Sound[SOUNDS_MAX + 1].B = Sound[SOUNDS_MAX].B;
    }
    else
    {
        Sound[SOUNDS_MAX].file = 0;
        Sound[SOUNDS_MAX].B = 100000;
        Sound[SOUNDS_MAX + 1].file = 0;
        Sound[SOUNDS_MAX + 1].B = 100000;
    }

    AudioNoInterrupts();
    if (!S_Fill_all_tables())
    {
        Sound[SOUNDS_MAX] = previous_left;
        Sound[SOUNDS_MAX + 1] = previous_right;
        recording = previous_recording;
        AudioInterrupts();
        return false;
    }
    AudioInterrupts();

    Print_Sound(SOUNDS_MAX);
    Print_Sound(SOUNDS_MAX + 1);

    // Menu
    DS_define_menu();
    Display_Sampler.DS_menu(); // display the menu and updates DS_menu_max

    // Pointer
    Pointer_Sampler.Set_pointer_to_first_menu_element();
    DS_local_pointer = Pointer_Sampler.Get_pointer();

    Clear_UI_events();

    Display_Sampler.DS_hide_recording();
    Display_Sampler.DS_Recording_description(recording, true);

    // restore LEDs
    Performance_led_set.Restore_all_LED();
    return true;
}

FLASHMEM
void DS_convert_file_L(int file_L_RAW, int bytes) // Convert the left recording channel into a RAW file.
{
    if (!FileNameRegistry::Bind_numeric(file_L_RAW) || !FileNameRegistry::Save())
    {
        Show_popup_text("CANNOT SAVE FILE NAME", ILI9341_WHITE, ILI9341_RED, 75);
        return;
    }
    char audio_filename[NAME_FILE_SIZE];
    char packet_filename[NAME_PACKET_SIZE];
    P_Invalidate_file_cache(file_L_RAW);
    Serial.println("*** Convert file_L ***");

    // create the file on the Flash chip and copy data
    Serial.print(F("Create file: "));
    Serial.print(Get_file_name(file_L_RAW, audio_filename));
    Serial.print(F(" dimension (bytes): "));
    Serial.println(bytes);

    char buffer[256];
    int packet = 0;
    int last_blocks = -1;

    // creazione file vuoto
    SerialFlash.create(Get_file_name(file_L_RAW, audio_filename), bytes);

    // apertura file
    SerialFlashFile destination_file = SerialFlash.open(Get_file_name(file_L_RAW, audio_filename));

    // copia file, caso MONO
    if (!Recording[recording].stereo)
    {
        // copia dal primo al penultimo packet
        if (Recording[recording].packets > 1)
        {
            for (packet = Recording[recording].first_packet; packet < (Recording[recording].first_packet + Recording[recording].packets - 1); ++packet)
            {
                SerialFlashFile source_file = SerialFlash.open(Get_packet_name(packet, packet_filename));
                for (auto i = 0; i < 256; ++i)
                {
                    source_file.read(buffer, 256);
                    destination_file.write(buffer, 256);
                }
            }
        }
        // copia l'ultimo packet
        packet = Recording[recording].first_packet + Recording[recording].packets - 1;
        SerialFlashFile source_file = SerialFlash.open(Get_packet_name(packet, packet_filename));
        last_blocks = (Recording[recording].bytes % PACKET_DIM) % 256;
        for (auto i = 0; i < last_blocks; ++i)
        {
            source_file.read(buffer, 256);
            destination_file.write(buffer, 256);
        }
    }

    // copia file, caso MONO
    else
    {
        // file_L
        // copia dal primo al penultimo packet
        if (Recording[recording].packets > 1)
        {
            for (packet = Recording[recording].first_packet; packet < (Recording[recording].first_packet + 2 * (Recording[recording].packets - 1)); packet += 2)
            {
                SerialFlashFile source_file = SerialFlash.open(Get_packet_name(packet, packet_filename));
                for (auto i = 0; i < 256; ++i)
                {
                    source_file.read(buffer, 256);
                    destination_file.write(buffer, 256);
                }
            }
        }
        // copia l'ultimo packet
        packet = Recording[recording].first_packet + 2 * (Recording[recording].packets - 1);
        SerialFlashFile source_file = SerialFlash.open(Get_packet_name(packet, packet_filename));
        last_blocks = (Recording[recording].bytes % PACKET_DIM) % 256;
        for (auto i = 0; i < last_blocks; ++i)
        {
            source_file.read(buffer, 256);
            destination_file.write(buffer, 256);
        }
    }
}

FLASHMEM
void DS_convert_file_R(int file_R_RAW, int bytes) // Convert the right recording channel into a RAW file.
{
    if (!FileNameRegistry::Bind_numeric(file_R_RAW) || !FileNameRegistry::Save())
    {
        Show_popup_text("CANNOT SAVE FILE NAME", ILI9341_WHITE, ILI9341_RED, 75);
        return;
    }
    char audio_filename[NAME_FILE_SIZE];
    char packet_filename[NAME_PACKET_SIZE];
    P_Invalidate_file_cache(file_R_RAW);
    Serial.println("*** Convert file_R ***");

    // create the file on the Flash chip and copy data
    Serial.print(F("Create file: "));
    Serial.print(Get_file_name(file_R_RAW, audio_filename));
    Serial.print(F(" dimension (bytes): "));
    Serial.println(bytes);

    char buffer[256];
    int packet = 0;
    int last_blocks = -1;

    SerialFlash.create(Get_file_name(file_R_RAW, audio_filename), bytes);
    SerialFlashFile destination_file = SerialFlash.open(Get_file_name(file_R_RAW, audio_filename));

    // copia dal primo al penultimo packet
    if (Recording[recording].packets > 1)
    {
        for (packet = Recording[recording].first_packet + 1; packet < (Recording[recording].first_packet + 1 + 2 * (Recording[recording].packets - 1)); packet += 2)
        {
            SerialFlashFile source_file = SerialFlash.open(Get_packet_name(packet, packet_filename));
            for (auto i = 0; i < 256; ++i)
            {
                source_file.read(buffer, 256);
                destination_file.write(buffer, 256);
            }
        }
    }

    // copia l'ultimo packet
    packet = Recording[recording].first_packet + 1 + 2 * (Recording[recording].packets - 1);
    SerialFlashFile source_file = SerialFlash.open(Get_packet_name(packet, packet_filename));
    last_blocks = (Recording[recording].bytes % PACKET_DIM) % 256;
    for (auto i = 0; i < last_blocks; ++i)
    {
        source_file.read(buffer, 256);
        destination_file.write(buffer, 256);
    }
}

void DS_Print_Directory(File dir, int numSpaces)
{
    while (true)
    {
        File entry = dir.openNextFile();
        if (!entry)
        {
            // Serial.println("** no more files **");
            break;
        }
        Serial.print("   ");
        Serial.print(entry.name());
        if (entry.isDirectory())
        {
            Serial.println("/");
            DS_Print_Directory(entry, numSpaces + 2);
        }
        else
        {
            // files have sizes, directories do not
            Serial.print("   ");
            Serial.println(entry.size(), DEC);
            big_result += entry.size();
        }
        entry.close();
    }
}

void DS_seed_all_Recordings(void)
{
    Serial.println(F("*** DS_seed_all_Recordings() ***"));
    for (auto i = 0; i < RECORDINGS; ++i)
    {
        Recording[i].first_packet = 0;
        Recording[i].packets = 0;
        Recording[i].bytes = 0;
        Recording[i].seconds = 0.0; // float
        Recording[i].stereo = 0;
        Recording[i].consistent = true;
        Require_FRAM(Archive.Save_DS_Recording(i));
        Serial.print(F("Seeded Recording: "));
        Serial.println(i);
    }
    Serial.println(F("*** Finished ***"));
    Serial.println();
}

void DS_update_recordings(void)
{
    recordings = 0;

    for (auto i = 0; i < RECORDINGS; ++i)
    {
        if (Recording[i].packets > 0 && Recording[i].consistent == true)
        {
            ++recordings;
        }
    }

    Serial.print(F("recordings are: "));
    Serial.println(recordings);
    Serial.println();
}

byte DS_read_all_Recordings(void)
{
    for (auto i = 0; i < RECORDINGS; ++i)
    {
        const byte result = DS_read_Recording(i);
        if (result != LillaFRAM_2x512::ERROR_0)
        {
            return result;
        }
    }
    return LillaFRAM_2x512::ERROR_0;
}

byte DS_read_Recording(int recording)
{
    if (recording >= 0 && recording < RECORDINGS)
    {
        const byte result = Archive.Read_DS_Recording(recording);
        if (result != LillaFRAM_2x512::ERROR_0)
        {
            return result;
        }
        Recording[recording].bytes = 2 * DS_get_samples_in_Recording(recording);
        Recording[recording].seconds = DS_get_Recording_seconds(recording);

        Serial.print(F("Read from FRAM Recording: "));
        Serial.println(recording);
        P_Recording(recording);
        return LillaFRAM_2x512::ERROR_0;
    }

    Serial.println(F("***** WARNING! --> DS_read_Recording: 'recording' out of range"));
    return LillaFRAM_2x512::ERROR_11;
}

float DS_get_Recording_seconds(int value)
{
    return (DS_get_samples_in_Recording(value) / AUDIO_BLOCK_SAMPLES * 0.0029f); // float
}

int DS_find_Recording_free(void)
{
    for (auto i = 0; i < RECORDINGS; ++i)
        if (Recording[i].packets == 0)
        {
            return i;
        }
    return -1;
}

int DS_get_next_Recording(int value)
{
    if (value == RECORDINGS - 1)
    {
        return value;
    }
    for (auto i = (value + 1); i < RECORDINGS; ++i)
    {
        if (Recording[i].consistent && Recording[i].packets > 0)
        {
            return i;
        }
    }
    return value;
}

int DS_get_last_Recording(void)
{
    for (auto i = RECORDINGS - 1; i >= 0; --i)
    {
        if (Recording[i].consistent && Recording[i].packets > 0)
        {
            return i;
        }
    }
    return -1;
}

int DS_get_previous_Recording(int value)
{
    if (value < 0)
        return -1;

    for (auto i = (value - 1); i >= 0; --i)
    {
        if (Recording[i].consistent && Recording[i].packets > 0)
        {
            return i;
        }
    }
    return value;
}

bool DS_check_conversion(void)
{
    char audio_filename[NAME_FILE_SIZE];
    if ((Get_flash_size() - Get_flash_occupation()) >= Recording[recording].bytes)
    {
        for (auto i = 0; i < FIRST_RECORDING_FILE; ++i)
        {
            if (FileNameRegistry::Numeric_available(i) && Capture_find(i) == nullptr && !SerialFlash.exists(Get_file_name(i, audio_filename)))
            {
                return true;
            }
        }
    }
    return false;
}

void DS_define_menu(void) // {"Exit"}, {"Delete"}, {"Pause+Rec"}, {"Mono Rec"}, {"Stereo Rec"}, {"Stop"}
{
    // voices that can be displayed
    Menu_DS[0] = true; // CANCEL_RECORDING
    Menu_DS[1] = true; // PAUSE+REC
    Menu_DS[2] = true; // MONO-REC
    Menu_DS[3] = true; // STEREO-REC
    Menu_DS[4] = true; // STOP
    Menu_DS[5] = true; // CONVERT REC-TO-RAW

    Menu_DS[6] = true;  // CANCEL
    Menu_DS[7] = true;  // CONVERT MONO
    Menu_DS[8] = true;  // CONVERT LEFT
    Menu_DS[9] = true;  // CONVERT RIGHT
    Menu_DS[10] = true; // CONVERT BOTH
    Menu_DS[11] = true; // EXPORT_WAV_TO_SD

    if (DS_state == DS_pause_state || DS_state == DS_recording_state || DS_state == DS_convert_state || recordings == 0)
    {
        Menu_DS[11] = false; // EXPORT TO SD
    }

    if (DS_state != DS_convert_state)
    {
        Menu_DS[6] = false;  // CANCEL
        Menu_DS[7] = false;  // CONVERT MONO
        Menu_DS[8] = false;  // CONVERT LEFT
        Menu_DS[9] = false;  // CONVERT RIGHT
        Menu_DS[10] = false; // CONVERT BOTH
    }

    if (DS_state == DS_convert_state && DS_export > 0)
    {
        Menu_DS[0] = false; // DELETE
        Menu_DS[1] = false; // PAUSE+REC
        Menu_DS[2] = false; // MONO-REC
        Menu_DS[3] = false; // STEREO-REC
        Menu_DS[4] = false; // STOP
        Menu_DS[5] = false; // CONVERT REC-TO-RAW
    }

    if (DS_state == DS_convert_state && DS_export == 0)
    {
        Menu_DS[7] = false;  // CONVERT_MONO
        Menu_DS[8] = false;  // CONVERT_LEFT
        Menu_DS[9] = false;  // CONVERT_RIGHT
        Menu_DS[10] = false; // CONVERT_BOTH
    }

    if (DS_state == DS_convert_state && DS_export == 1)
    {
        Menu_DS[8] = false;  // CONVERT LEFT
        Menu_DS[9] = false;  // CONVERT RIGHT
        Menu_DS[10] = false; // CONVERT BOTH
    }

    if (DS_state == DS_convert_state && DS_export == 2)
    {
        Menu_DS[7] = false; // CONVERT MONO
    }

    if (recordings == 0)
    {
        Menu_DS[0] = false; // CANCEL
    }

    if (VFS_Get_packets_free() < 4)
    {
        Serial.print("VFS_Get_packets_free(): ");
        Serial.println(VFS_Get_packets_free());

        Menu_DS[1] = false; // PAUSE+REC
    }

    if (DS_state == DS_waiting_state)
    {
        Menu_DS[2] = false; // MONO-REC
        Menu_DS[3] = false; // STEREO-REC
        Menu_DS[4] = false; // STOP
    }

    if (DS_state == DS_pause_state) // Pause+Rec
    {
        Menu_DS[0] = false; // CANCEL
        Menu_DS[1] = false; // PAUSE+REC
        Menu_DS[5] = false; // CONVERT REC-TO-RAW
    }

    if (DS_state == DS_recording_state || DS_state == DS_convert_state) // Recording
    {
        Menu_DS[0] = false; // DELETE
        Menu_DS[1] = false; // PAUSE+REC
        Menu_DS[2] = false; // MONO-REC
        Menu_DS[3] = false; // STEREO-REC
        Menu_DS[5] = false; // CONVERT REC-TO-RAW
    }

    if (recording == -1)
    {
        Menu_DS[5] = false; // CONVERT REC-TO-RAW
    }
    else
    {
        if ((Get_flash_size() - Get_flash_occupation()) < Recording[recording].bytes) // if recording is stereo, at least one file can be saved
        {
            Menu_DS[5] = false; // CONVERT REC-TO-RAW
        }
    }

    Display_Sampler.DS_set_recording_controls(DS_state == DS_pause_state || DS_state == DS_recording_state);

    DS_menu_max = -1;
    for (auto i = 0; i < DS_menu_elements; ++i)
    {
        DS_menu_max += Menu_DS[i];
    }

    // Reporting
    if (true)
    {
        Serial.println("DS_define_menu(void) - Result: ");
        for (auto i = 0; i < DS_menu_elements; ++i)
        {
            Serial.println(Menu_DS[i]);
        }
        Serial.println("*****************************");
    }
}

FLASHMEM
void DS_set_DS_Sampling_Patch(void)
{
    // set DS Patch and Sounds
    Patch[PATCHES_MAX].used = true;
    Patch[PATCHES_MAX].instruments = 2;

    // Left channel recording/instrument
    Patch[PATCHES_MAX].Instrument[0].used = true;
    Patch[PATCHES_MAX].Instrument[0].sound_id = SOUNDS_MAX;
    Patch[PATCHES_MAX].Instrument[0].root_key = 60;
    Patch[PATCHES_MAX].Instrument[0].from_note = 0;
    Patch[PATCHES_MAX].Instrument[0].to_note = 127;
    Patch[PATCHES_MAX].Instrument[0].precedence = false;
    Patch[PATCHES_MAX].Instrument[0].lock = false;
    Patch[PATCHES_MAX].Instrument[0].Filter.use = false;

    // Right channel recording/instrument
    Patch[PATCHES_MAX].Instrument[1].used = true;
    Patch[PATCHES_MAX].Instrument[1].sound_id = SOUNDS_MAX + 1;
    Patch[PATCHES_MAX].Instrument[1].root_key = 60;
    Patch[PATCHES_MAX].Instrument[1].from_note = 0;
    Patch[PATCHES_MAX].Instrument[1].to_note = 127;
    Patch[PATCHES_MAX].Instrument[1].precedence = false;
    Patch[PATCHES_MAX].Instrument[1].lock = false;
    Patch[PATCHES_MAX].Instrument[1].Filter.use = false;

    Patch[PATCHES_MAX].Instrument[2].used = false;
    Patch[PATCHES_MAX].Instrument[3].used = false;
    Patch[PATCHES_MAX].Instrument[4].used = false;
    Patch[PATCHES_MAX].Instrument[5].used = false;
    Patch[PATCHES_MAX].Instrument[6].used = false;
    Patch[PATCHES_MAX].Instrument[7].used = false;

    // LEFT Sound
    Sound[SOUNDS_MAX].used = true;
    Sound[SOUNDS_MAX].mode = 0;
    // Sound[SOUNDS_MAX].file = must be defined;
    Sound[SOUNDS_MAX].pitch = 0; // -128 + 127
    Sound[SOUNDS_MAX].A = 0;
    // Sound[SOUNDS_MAX].B = must be defined;
    Sound[SOUNDS_MAX].Noclick = 0;
    Sound[SOUNDS_MAX].pan = -16; // full Left
    Sound[SOUNDS_MAX].data = 0;  // bit4-3-2-1: midi_channel bit0: Attack ramp ("0" Slow, "1" Fast)
    Sound[SOUNDS_MAX].attack = 0;
    Sound[SOUNDS_MAX].decay = 50;
    Sound[SOUNDS_MAX].sustain = 50;
    Sound[SOUNDS_MAX].release = 10;
    Sound[SOUNDS_MAX].gain = 28;

    // RIGHT Sound
    Sound[SOUNDS_MAX + 1].used = true;
    Sound[SOUNDS_MAX + 1].mode = 0;
    // Sound[SOUNDS_MAX + 1].file = must be defined;
    Sound[SOUNDS_MAX + 1].pitch = 0; // -128 + 127
    Sound[SOUNDS_MAX + 1].A = 0;
    // Sound[SOUNDS_MAX + 1].B = must be defined;
    Sound[SOUNDS_MAX + 1].Noclick = 0;
    Sound[SOUNDS_MAX + 1].pan = 16; // full Right
    Sound[SOUNDS_MAX + 1].data = 0; // bit4-3-2-1: midi_channel bit0: Attack ramp ("0" Slow, "1" Fast)
    Sound[SOUNDS_MAX + 1].attack = 0;
    Sound[SOUNDS_MAX + 1].decay = 50;
    Sound[SOUNDS_MAX + 1].sustain = 50;
    Sound[SOUNDS_MAX + 1].release = 10;
    Sound[SOUNDS_MAX + 1].gain = 28;
}

int DS_get_samples_in_Recording(int recording)
{
    return Info.DS_recording_samples(recording);
}

FLASHMEM
void P_Recording(int value)
{
    Serial.print("Recording[");
    Serial.print(value);
    Serial.println("]");
    Serial.print(".first_packet = ");
    Serial.println(Recording[value].first_packet);
    Serial.print(".packets = ");
    Serial.print(Recording[value].packets);
    Serial.println(F(" (totale packets if mono, only file_L packets if stereo)"));
    Serial.print(".bytes = ");
    Serial.print(Recording[value].bytes);
    Serial.println(F(" (totale bytes if mono, only file_L bytes if stereo) "));
    Serial.print(".seconds = ");
    Serial.println(Recording[value].seconds, 1); // float
    Serial.print(".stereo = ");
    Serial.println((Recording[value].stereo ? "stereo" : "mono"));
    Serial.print(".consistent = ");
    Serial.println((Recording[value].consistent ? "yes" : "no"));
    Serial.println();
}

// ***************************************************************************************************************
// ******************************************            SWITCH           ****************************************
// ***************************************************************************************************************

void Golive_with_LIVE_SAMPLING(void)
{
    // Discard notices from the previous page before entering Live Sampler.

    AudioNoInterrupts();
    AudioPlayer::Take_live_unrecorded_notice();
    AudioInterrupts();

    Lilla_state = LIVE_SAMPLING;

    Display_LiveSampler.Page();

    Clear_UI_events();

    // restore LEDs
    Performance_led_set.Restore_all_LED();

    LS_update_menu_elements();
    Display_LiveSampler.Menu();
    Pointer_LiveSampler.Set_pointer_to_first_menu_element();
    LS_local_pointer = Pointer_LiveSampler.Get_pointer();

    if (!LS_XY_lock)
    {
        LS_update_both_X_Y_samples();
    }
    else // altrimenti e' gia' stato calcolato
    {
        LS_update_Q_sample();
    }

    Display_LiveSampler.Show_wave(LS_sound_id);

    Print_Patch(Patch_id);
}

void Golive_DIRECT_SAMPLING(void)
{
    Lilla_state = DIRECT_SAMPLING;

    Clear_UI_events();

    DS_state = DS_waiting_state;

    // Switch ON the VU meter
    PeakTracking_L.reset();
    PeakTracking_R.reset();

    Display_Sampler.DS_page_upper();
    Display_Sampler.DS_page_lower(recording);

    // Menu
    DS_define_menu();
    Display_Sampler.DS_menu(); // display the menu and updates DS_menu_max

    // Pointer
    Pointer_Sampler.Set_pointer_to_first_menu_element();
    DS_local_pointer = Pointer_Sampler.Get_pointer();

    // Display the VU meter
    Display_Sampler.DS_bar(0, 0);
    Display_Sampler.DS_bar(1, 0);

    // Reporting
    Print_Patch(Patch_id);
    Serial.println(F("*** DIRECT_SAMPLING ***  Sounds are:"));
    Print_Sound(SOUNDS_MAX);
    Print_Sound(SOUNDS_MAX + 1);
}

void Golive_with_PERFORMANCE(int patch_id)
{
    Lilla_state = PERFORMANCE;

    patch_original = P_Verify_is_Patch_original(patch_id);
    P_Select_menu_elements();
    P_Update_line_of_all_instruments();

    Display_Performance.P_show_PERFORMANCE_page(true, true);
    Performance_led_set.Restore_all_LED();
    Update_instruments_leds();

    Clear_UI_events();

    // pointer
    Pointer_Performance.Set_pointer_to_Patch();
    P_pointer = Pointer_Performance.Get_pointer();

    Print_Lilla_state();
    Print_Patch(patch_id);
}

void Golive_DELAY_SETTINGS(void)
{
    Lilla_state = DELAY_SETTINGS;

    Display_Delay.D_show_page();
    if (Lilla_state_0 != DIRECT_SAMPLING)
    {
        Pointer_Delay.Set_pointer_to_Feedback();
        DELAY_local_pointer = Pointer_Delay.Get_element_name();
    }

    Clear_UI_events();
}

void Golive_with_MIDI_LOOP(bool restart)
{
    Lilla_state = MIDI_LOOP;

    Clear_UI_events();

    LOOP_select_menu_elements();
    Display_MidiLoop.Show_Loop_page();

    // Pointer
    Pointer_MidiLoop.Set_pointer_to_first_menu_element();
    LOOP_local_pointer = Pointer_MidiLoop.Get_pointer();

    // LEDs setup
    if (restart)
    {
        Players_statistics.Reset_total_Players_per_track_instrument();
        Loop_led_set.Request_all_LED_switch_off();
    }

    // Update_instruments_leds();
    LOOP_metronomo.Leds_off(); // spegni i LED del metronomo

    // Se esiste loop_0, accendi il metronomo
    if (LOOP_events[0] != 0)
    {
        if (restart)
        {
            LOOP_restart_clock();

            // Accendi primo led metronomo
            LOOP_metronomo.Led_ON(0);

            /*
            // calcolo prossimo evento metronomo
            LOOP_metronomo.metro_time = 0 + LOOP_metronomo.Read_metro_delta_ms();

            // avvia il metronomo
            LOOP_metronomo_run = true
            */
        }
    }
    else
    {
        LOOP_run_button_state = true; // stato pulsante EN_PB_Loop (arresta/riavvia tutti i loop)
    }
}

void Switch_to_DIRECT_SAMPLING(void)
{
    AudioNoInterrupts();
    Players_Manager.Stop_all_players();
    if (!DS_setup_DIRECT_SAMPLING_Patch_and_Preset())
    {
        AudioInterrupts();
        return;
    }
    AudioInterrupts();
    Golive_DIRECT_SAMPLING();
}

void Switch_from_MIDI_LOOP_to_DIRECT_SAMPLING(void)
{
    AudioNoInterrupts();
    LOOP_stop_all_midi_tracks();
    if (!DS_setup_DIRECT_SAMPLING_Patch_and_Preset())
    {
        AudioInterrupts();
        return;
    }
    AudioInterrupts();
    Golive_DIRECT_SAMPLING();
}

void Switch_from_LIVE_SAMPLING_to_DIRECT_SAMPLING(void)
{
    if (LS_state == REC)
    {
        if (!LS_ask_if_exit_from_LS()) // false: remain
        {
            if (Lilla_state == MIXER)
            {
                Golive_MIXER();
            }

            else if (Lilla_state == LIVE_SAMPLING)
            {
                LS_refresh_LS_page();
            }
            else if (Lilla_state == INSTRUMENT_VCF)
            {
                Display_VCF.VCF_show_VCF_page(Patch_id, Instrument_id);

                // restore LEDs
                Performance_led_set.Restore_all_LED();
            }
            else if (Lilla_state == SETUP)
            {
                Golive_SETUP();
            }
        }
        else // true: stop and exit
        {
            LS_state = PLAYONLY;
            LiveSampler.Stop();
            Switch_to_DIRECT_SAMPLING();
        }
    }
    else // true: stop and exit
    {
        Switch_to_DIRECT_SAMPLING();
    }
}

void Switch_from_PERFORMANCE_to_MIDI_LOOP(void)
{
    AudioNoInterrupts();
    Players_Manager.Stop_all_players();
    AudioInterrupts();
    Golive_with_MIDI_LOOP(true);
}

void Switch_from_DIRECT_SAMPLING_to_MIDI_LOOP(void)
{
    switch (DS_state)
    {
    case DS_waiting_state: // no activity
        AudioNoInterrupts();
        if (!P_Rebuild_patch_old())
        {
            AudioInterrupts();
            return;
        }
        Turn_ON_Delay(true);
        AudioInterrupts();
        Switch_from_PERFORMANCE_to_MIDI_LOOP();
        break;
    case DS_pause_state: // pause + rec
        // switch OFF Line OUT monitor
        MAIN_mixer_out_L.gain(1, 0.0);
        MAIN_mixer_out_R.gain(1, 0.0);
        // Switch off blinking REC
        DS_blink_ON = false;

        AudioNoInterrupts();
        if (!P_Rebuild_patch_old())
        {
            AudioInterrupts();
            return;
        }
        Turn_ON_Delay(true);
        AudioInterrupts();
        Switch_from_PERFORMANCE_to_MIDI_LOOP();
        break;
    case DS_recording_state: // recording
        DS_ask_if_EXIT_from_DS();
        if (action == 0) // remain
        {
            Lilla_state = DIRECT_SAMPLING;

            Display_Sampler.DS_page_upper();
            Display_Sampler.DS_page_lower(recording);

            // Menu
            DS_define_menu();
            Display_Sampler.DS_menu(); // display the menu and updates DS_menu_max

            // Pointer
            Pointer_Sampler.Set_pointer_to_first_menu_element();
            DS_local_pointer = Pointer_Sampler.Get_pointer();

            // Display the VU meter
            Display_Sampler.DS_bar(0, 0);
            Display_Sampler.DS_bar(1, 0);

            Clear_UI_events();
        }
        else // stop and exit
        {
            DirectSampler.Book_stop();
            // switch OFF Line OUT monitor
            MAIN_mixer_out_L.gain(1, 0.0);
            MAIN_mixer_out_R.gain(1, 0.0);
            if (Recording[recording].packets == 0)
            {
                // Recording cancelled
                recording = DS_get_next_Recording(-1);
            }
            else
            {
                Recording[recording].consistent = true;
                // consistent Recording must be saved
                Require_FRAM(Archive.Save_DS_Recording(recording));
                DS_read_Recording(recording); // only to update .bytes and .seconds
            }

            DS_update_recordings();
            // Switch off blinking REC
            DS_blink_ON = false;

            AudioNoInterrupts();
            if (!P_Rebuild_patch_old())
            {
                AudioInterrupts();
                return;
            }
            Turn_ON_Delay(true);
            AudioInterrupts();
            Switch_from_PERFORMANCE_to_MIDI_LOOP();
        }
        break;
    case DS_convert_state: // convert REC --> RAW
        AudioNoInterrupts();
        if (!P_Rebuild_patch_old())
        {
            AudioInterrupts();
            return;
        }
        Turn_ON_Delay(true);
        AudioInterrupts();
        Switch_from_PERFORMANCE_to_MIDI_LOOP();
        break;

    default:
        PRINT_ERROR(F("Switch MISSING! "));
        break;
    }
}

void Switch_from_LIVE_SAMPLING_to_MIDI_LOOP(void)
{
    if (LS_state == REC)
    {
        if (!LS_ask_if_exit_from_LS()) // false: remain
        {
            if (Lilla_state == LIVE_SAMPLING)
            {
                LS_refresh_LS_page();
            }
            else if (Lilla_state == INSTRUMENT_VCF)
            {
                Display_VCF.VCF_show_VCF_page(Patch_id, Instrument_id);

                // restore LEDs
                Performance_led_set.Restore_all_LED();
            }
        }
        else // true: stop and exit
        {
            LS_state = PLAYONLY;
            LiveSampler.Stop();

            AudioNoInterrupts();
            if (!P_Rebuild_patch_old())
            {
                AudioInterrupts();
                return;
            }
            AudioInterrupts();
            Switch_from_PERFORMANCE_to_MIDI_LOOP();
        }
    }
    else // true: stop and exit
    {
        AudioNoInterrupts();
        if (!P_Rebuild_patch_old())
        {
            AudioInterrupts();
            return;
        }
        AudioInterrupts();
        Switch_from_PERFORMANCE_to_MIDI_LOOP();
    }
}

void Switch_from_PERFORMANCE_to_LIVE_SAMPLING(void)
{
    AudioNoInterrupts();
    Players_Manager.Stop_all_players();

    // Setup LIVE_SAMPLING

    Patch_id = PATCHES_MAX; // Live Sampler uses PATCHES_MAX
    LS_setup_LS_Patch(LS_stereo);

    P_Update_all_maps_Instrument_for_notes();
    Players_Manager.Update_all_Preset(Patch_id, Patch_volume_gain(volume_patch));
    AudioInterrupts();

    Golive_with_LIVE_SAMPLING();
}

void Switch_from_MIDI_LOOP_to_LIVE_SAMPLING(void)
{
    AudioNoInterrupts();
    LOOP_stop_all_midi_tracks();

    // imposta LIVE_SAMPLING

    Patch_id = PATCHES_MAX; // Live Sampler uses PATCHES_MAX
    LS_setup_LS_Patch(LS_stereo);

    P_Update_all_maps_Instrument_for_notes();
    Players_Manager.Update_all_Preset(Patch_id, Patch_volume_gain(volume_patch));
    AudioInterrupts();

    Golive_with_LIVE_SAMPLING();
}

void Switch_from_DIRECT_SAMPLING_to_LIVE_SAMPLING(void)
{
    switch (DS_state)
    {
    case DS_waiting_state: // no activity
        Switch_from_PERFORMANCE_to_LIVE_SAMPLING();
        AudioNoInterrupts();
        Turn_ON_Delay(true);
        AudioInterrupts();
        break;
    case DS_pause_state: // pause + rec
        // switch OFF Line OUT monitor
        MAIN_mixer_out_L.gain(1, 0.0);
        MAIN_mixer_out_R.gain(1, 0.0);
        // Switch off blinking REC
        DS_blink_ON = false;

        Switch_from_PERFORMANCE_to_LIVE_SAMPLING();
        AudioNoInterrupts();
        Turn_ON_Delay(true);
        AudioInterrupts();
        break;
    case DS_recording_state: // recording
        DS_ask_if_EXIT_from_DS();
        if (action == 0) // remain
        {
            Lilla_state = DIRECT_SAMPLING;

            Display_Sampler.DS_page_upper();
            Display_Sampler.DS_page_lower(recording);

            // Menu
            DS_define_menu();
            Display_Sampler.DS_menu(); // display the menu and updates DS_menu_max

            // Pointer
            Pointer_Sampler.Set_pointer_to_first_menu_element();
            DS_local_pointer = Pointer_Sampler.Get_pointer();

            // Display the VU meter
            Display_Sampler.DS_bar(0, 0);
            Display_Sampler.DS_bar(1, 0);

            Clear_UI_events();
        }
        else // stop and exit
        {
            DirectSampler.Book_stop();
            // switch OFF Line OUT monitor
            MAIN_mixer_out_L.gain(1, 0.0);
            MAIN_mixer_out_R.gain(1, 0.0);
            if (Recording[recording].packets == 0)
            {
                // Recording cancelled
                recording = DS_get_next_Recording(-1);
            }
            else
            {
                Recording[recording].consistent = true;
                // consistent Recording must be saved
                Require_FRAM(Archive.Save_DS_Recording(recording));
                DS_read_Recording(recording); // only to update .bytes and .seconds
            }

            DS_update_recordings();
            // Switch off blinking REC
            DS_blink_ON = false;

            Switch_from_PERFORMANCE_to_LIVE_SAMPLING();
            AudioNoInterrupts();
            Turn_ON_Delay(true);
            AudioInterrupts();
        }
        break;
    case DS_convert_state: // convert REC --> RAW
        Switch_from_PERFORMANCE_to_LIVE_SAMPLING();
        AudioNoInterrupts();
        Turn_ON_Delay(true);
        AudioInterrupts();
        break;

    default:
        PRINT_ERROR(F("Switch MISSING! "));
        break;
    }
}

void Switch_to_PERFORMANCE_patch_old(void)
{
    Delay_data_struct next_delay{};
    Require_FRAM(Archive.Read_Delay(Patch_id_old, next_delay));
    AudioNoInterrupts();
    if (!P_Rebuild_patch_old())
    {
        AudioInterrupts();
        return;
    }
    Delay_manager.New_values(&next_delay);
    AudioInterrupts();
    Golive_with_PERFORMANCE(Patch_id);
}

void Switch_from_MIDI_LOOP_to_PERFORMANCE(void)
{
    if (Patch_id < PATCHES_MAX)
    {
        Require_FRAM(Archive.Save_Delay(Patch_id, Delay_data));
    }
    AudioNoInterrupts();
    LOOP_stop_all_midi_tracks();
    AudioInterrupts();

    // fissa lo stato run delle track (se registrate) e fermale; in questo modo al riento in MIDI_LOOP ripartiranno con ALL_START/STOP
    for (auto track = 0; track < TRACKS; ++track)
    {
        LOOP_track_run_memo[track] = LOOP_events[track] > 0;
        LOOP_track_run[track] = false;
    }
    Golive_with_PERFORMANCE(Patch_id);
}

void Switch_from_LIVE_SAMPLING_to_PERFORMANCE(void)
{
    if (LS_state == REC) // Recording
    {
        if (!LS_ask_if_exit_from_LS()) // false: remain
        {
            if (Lilla_state == LIVE_SAMPLING)
            {
                LS_refresh_LS_page();
            }
            else if (Lilla_state == INSTRUMENT_VCF)
            {
                // restore LEDs
                Performance_led_set.Restore_all_LED();
                Display_VCF.VCF_show_VCF_page(Patch_id, Instrument_id);
            }
            else if (Lilla_state == MIXER)
            {
                Switch_to_MIXER();
            }
            else if (Lilla_state == DELAY_SETTINGS)
            {
                Display_Delay.D_show_page();
                Pointer_Delay.Set_pointer_to_Feedback();
                DELAY_local_pointer = Pointer_Delay.Get_element_name();
            }
        }
        else // true: stop and exit
        {
            LS_state = PLAYONLY;
            LiveSampler.Stop();
            Switch_to_PERFORMANCE_patch_old();
        }
    }
    else
    {
        Switch_to_PERFORMANCE_patch_old();
    }
}

void Switch_from_DIRECT_SAMPLING_to_PERFORMANCE(void)
{
    switch (DS_state)
    {
    case DS_waiting_state:   // no activity
        Turn_ON_Delay(true); // switch on/off Delay (using Instrument routing)
        Switch_to_PERFORMANCE_patch_old();
        break;
    case DS_pause_state: // pause + rec
        // switch OFF Line OUT monitor
        MAIN_mixer_out_L.gain(1, 0.0);
        MAIN_mixer_out_R.gain(1, 0.0);

        // Switch off blinking REC
        DS_blink_ON = false;

        Turn_ON_Delay(true); // switch on/off Delay (using Instrument routing)
        Switch_to_PERFORMANCE_patch_old();
        break;
    case DS_recording_state: // recording
        DS_ask_if_EXIT_from_DS();
        if (action == 0) // remain
        {
            Lilla_state = DIRECT_SAMPLING;

            Display_Sampler.DS_page_upper();
            Display_Sampler.DS_page_lower(recording);

            // Menu
            DS_define_menu();
            Display_Sampler.DS_menu(); // display the menu and updates DS_menu_max

            // Pointer
            Pointer_Sampler.Set_pointer_to_first_menu_element();
            DS_local_pointer = Pointer_Sampler.Get_pointer();

            // Display the VU meter
            Display_Sampler.DS_bar(0, 0);
            Display_Sampler.DS_bar(1, 0);

            Clear_UI_events();
        }
        else // stop and exit
        {
            DirectSampler.Book_stop();

            // switch OFF Line OUT monitor
            MAIN_mixer_out_L.gain(1, 0.0);
            MAIN_mixer_out_R.gain(1, 0.0);
            if (Recording[recording].packets == 0)
            {
                Serial.print(F("Recording: "));
                Serial.print(recording);
                Serial.println(F(" cancelled."));
                recording = DS_get_next_Recording(-1);
            }
            else
            {
                Recording[recording].consistent = true;

                // consistent Recording must be saved
                Require_FRAM(Archive.Save_DS_Recording(recording));
                DS_read_Recording(recording); // call for updating .bytes and .seconds
            }

            DS_update_recordings();

            // Switch off blinking REC
            DS_blink_ON = false;

            Turn_ON_Delay(true); // switch on/off Delay (using Instrument routing)
            Switch_to_PERFORMANCE_patch_old();
        }
        break;
    case DS_convert_state:
        Turn_ON_Delay(true);
        Switch_to_PERFORMANCE_patch_old();
        break;

    default:
        PRINT_ERROR(F("Switch MISSING! "));
        break;
    }
}

void Switch_from_MIDI_LOOP_to_MIDI_MONITOR(void)
{
    AudioNoInterrupts();
    LOOP_stop_all_midi_tracks();
    AudioInterrupts();

    Golive_MIDI_MONITOR();
}

void Switch_from_MIDI_LOOP_to_SETUP(void)
{
    Lilla_state_0 = MIDI_LOOP;
    Golive_SETUP();
}

void Switch_from_LIVE_SAMPLING_to_DELAY(void)
{
    Lilla_state_0 = LIVE_SAMPLING;

    if (Delay_values.instrument_route[0] || Delay_values.instrument_route[1])
    {
        Delay_values.instrument_route[0] = true;
        Delay_values.instrument_route[1] = true;
    }

    Golive_DELAY_SETTINGS();
}

void Golive_MIDI_MONITOR(void)
{

    AudioNoInterrupts();
    Players_Manager.Stop_all_players();
    AudioInterrupts();

    Lilla_state = MIDI_MONITOR;

    display_wait = false;
    Display_Diagnostics.Midi_monitor_page();

    Clear_UI_events();
}

void Golive_SETUP(void)
{
    Lilla_state = SETUP;

    SET_menu = 0;
    Display_Setup.SETUP_show_SETUP_page();

    Clear_UI_events();

    Display_Setup.SETUP_show_frame(SET_menu);
}

// ***************************************************************************************************************
// **********************************                  DELAY                    **********************************
// ***************************************************************************************************************

void D_Set_value(int item, int value) // Publish one UI request through the same parameter owner used by patch changes.
{
    const bool enabled = NVIC_IS_ENABLED(IRQ_SOFTWARE) != 0;
    AudioNoInterrupts();
    Delay_manager.Set_value(item, value); // Retarget only this parameter; the audio callback applies the change.
    if (enabled)
    {
        AudioInterrupts();
    }
}

bool D_Read_value(int item) // Edit a requested value locally so the encoder never writes intermediate DSP state.
{
    int value = Delay_manager.Get_value(item);
    const int offset_limit = item == SAMPLES_LR ? Calc_delay_samples_LR_limit(Delay_manager.Get_value(SAMPLES)) : 0;
    const int lowest = item == SAMPLES_LR ? -offset_limit : (item < DELAY_LPF_ITEMS ? Delay_data_limits[item][0] : 0);
    const int highest = item == SAMPLES_LR ? offset_limit : (item < DELAY_LPF_ITEMS ? Delay_data_limits[item][1] : 2);
    if (!Read_encoder(EN_PB_Value, value, highest, lowest, 1))
    {
        return false;
    }
    D_Set_value(item, value); // Submit the encoder result with audio interrupts disabled.
    return true;
}

// ***************************************************************************************************************
// **********************************                MIDI_LOOP                  **********************************
// ***************************************************************************************************************
// Setup MIDI_LOOP

void LOOP_reset_all_data(void)
{
    LOOP_time = 0;
    LOOP_stretch_int = 100;
    LOOP_stretch = 1.0f;
    for (auto track = 0; track < TRACKS; ++track)
    {
        LOOP_events[track] = 0;        // numero di eventi nel track
        LOOP_track_run[track] = false; // true --> il track va suonato
        LOOP_volume[track] = 1.0;
        LOOP_volume_int[track] = 20;
        LOOP_slide[track] = 0;     // slittamento temporale
        LOOP_pitch_int[track] = 0; // slittamento pitch
    }
}

void LOOP_stop_all_midi_tracks(void) // to be called inside AudioNoInterrupt()
{
    // stops running tracks
    for (auto track = 0; track < TRACKS; ++track)
    {
        LOOP_track_run[track] = false; // true --> il track va suonato
    }

    Players_Manager.Stop_all_players();

    // stops the metronome
    LOOP_metronomo_run = false;

    // cancel (if exists) last metronomo update request from MidiReader
    LOOP_metronomo_flag_IN[1] = false;

    // simulates all tracks Start/Stop when all tracks are active
    LOOP_run_button_state = false;
}

void LOOP_stop_and_reset_runnig_loop_data(void)
{
    AudioNoInterrupts();

    // stop all Player that execute notes of any track
    Players_Manager.Release_all_players_loop();

    LOOP_reset_all_data();

    // stops the metronome
    LOOP_metronomo_run = false;

    // cancels (if exists) last metronomo update request from MidiReader
    LOOP_metronomo_flag_IN[1] = false;

    AudioInterrupts();

    // switchs off metronomo leds
    LOOP_metronomo.Leds_off(); // va eseguito fuori da AudioNoInterrupt()
}

void LOOP_select_menu_elements(void)
{
    // voices that can be displayed
    Menu_LOOP[0] = true; // New
    Menu_LOOP[1] = true; // Save
    Menu_LOOP[2] = true; // Save as New
    Menu_LOOP[3] = true; // Delete

    if (LOOP_id >= 0 && LOOP_events[0] == 0) // loop vuoto
    {
        Menu_LOOP[0] = false; // New
        Menu_LOOP[1] = false; // Save
        Menu_LOOP[2] = false; // Save as New
    }

    if (LOOP_id >= 0 && LOOP_original) // loop su SD e inalterato
    {
        Menu_LOOP[1] = false; // Save
        Menu_LOOP[2] = false; // Save as New
    }

    if (LOOP_id == NEW_LOOP) // nuovo loop
    {
        Menu_LOOP[0] = false;    // New
        Menu_LOOP[2] = false;    // Save as New
        if (LOOP_events[0] == 0) // nuovo loop vuoto
        {
            Menu_LOOP[1] = false; // Save
            Menu_LOOP[3] = false; // Delete
        }
    }

    LOOP_menu_max = Menu_LOOP[0] + Menu_LOOP[1] + Menu_LOOP[2] + Menu_LOOP[3] - 1;
}

void LOOP_restart_clock(void)
{
    LOOP_clock = 0;
    Serial.println("restart LOOP_clock");
}

unsigned long LOOP_Clock(void)
{
    return LOOP_clock / LOOP_stretch;
}

unsigned long LOOP_zero_time(void)
{
    // Ultimo passaggio per lo 0 espresso in tempo virtuale LOOP_Clock
    //                               LOOP_zero_time()    LOOP_Clock()
    // 0-------------------------------------*---------------X----------------------------> LOOP_Clock()
    // 0------------------0------------------0---------------X-0-----------------0--------> LOOP_normalized_time()
    //
    return LOOP_Clock() - LOOP_normalized_time();
}

int LOOP_normalized_time(void)
{
    // Calcola tempo attuale normalizzato
    //                                                   LOOP_Clock()
    // 0-----------------------------------------------------X----------------------------> LOOP_Clock()
    // 0------------------0------------------0---------------X-0-----------------0--------> LOOP_normalized_time()
    //
    return LOOP_Clock() % LOOP_time;
}

// Converti un tempo normalizzato in tempo virtuale LOOP_Clock, a partire dal tempo virtuale attuale
unsigned long LOOP_Clock_time_from_virtual_time(int T_evento)
{
    if (T_evento < LOOP_normalized_time())
    {
        //                     T_evento                 LOOP_time
        //          0-------------*-------------------------|
        //
        //  LOOP_zero_time()      LOOP_normalized_time()
        //          |------------------------X--------------|-------------*-----------------------|--->
        //                                                             return
        return LOOP_zero_time() + LOOP_time + T_evento;
    }

    else
    {
        //                                               T_evento                    L_t
        //          0----------------------------------------*------------------------|
        //
        //  LOOP_zero_time()  LOOP_normalized_time()
        //          |------------------X---------------------*------------------------|----------------------------------------------------------|--->
        //                                                 return
        return LOOP_zero_time() + T_evento;
    }
}

// Define time ordering of events
void LOOP_set_time_order(int track)
{
    if (LOOP_events[track] == 1)
    {
        LOOP_time_order[track][0] = 0;
        return;
    }
    else if (LOOP_events[track] > 1)
    {
        // Trova l'indice associato al primo evento rispetto al tempo normalizzato
        uint32_t min_time_index = 0; // indice cercato
        int min_time = LOOP_element[track][0].time;

        for (uint32_t i = 1; i < LOOP_events[track]; ++i)
        {
            if (LOOP_element[track][i].time < min_time)
            {
                min_time_index = i;
                min_time = LOOP_element[track][i].time;
            }
        }

        /*
        Gli eventi sono così ordinati:
        LOOP_time_order[track][0] = evento con time minimo
        LOOP_time_order[track][0] = evento successivo
        */
        for (uint32_t i = 0; i < LOOP_events[track]; ++i)
        {
            LOOP_time_order[track][i] = (min_time_index + i) % LOOP_events[track];
        }

        return;
    }
    return;
}

void LOOP_restart_procedure(int track)
{
    bool found = false;

    // Procedura di ripartenza
    if (LOOP_events[track] == 1)
    {
        LOOP_play_event[track] = 0;
        LOOP_play_time[track] = LOOP_Clock_time_from_virtual_time(LOOP_element[track][0].time);
        LOOP_track_run[track] = true;
    }
    else
    {
        LOOP_clock_memo = LOOP_normalized_time();
        for (uint32_t i = 0; i < LOOP_events[track]; ++i)
        {
            if (LOOP_element[track][LOOP_time_order[track][i]].time >= LOOP_clock_memo)
            {
                LOOP_play_event[track] = LOOP_time_order[track][i];
                LOOP_play_time[track] = LOOP_Clock_time_from_virtual_time(LOOP_element[track][LOOP_time_order[track][i]].time);

                // Ferma la ricerca
                found = true;
                break;
            }
        }
        if (!found)
        {
            LOOP_play_event[track] = 0;
            LOOP_play_time[track] = LOOP_Clock_time_from_virtual_time(LOOP_element[track][0].time);
        }
        LOOP_track_run[track] = true;
    }
}

bool LOOP_Print_midi_loop_complete_data(int loop_id)
{
    if (false)
    {
        return false;
    }

    Serial.print("****  MIDI_LOOP complete data for loop: ");
    Serial.println(loop_id);
    Serial.println();

    // uint16_t LOOP_time;
    Serial.print("uint16_t LOOP_time: ");
    Serial.println(LOOP_time);

    // uint32_t LOOP_events[TRACKS]
    Serial.println("uint32_t LOOP_events[TRACKS]");
    for (auto i = 0; i < TRACKS; ++i)
    {
        Serial.println(LOOP_events[i]);
    }

    // int LOOP_slide[TRACKS]
    Serial.println("int LOOP_slide[TRACKS]");
    for (auto i = 0; i < TRACKS; ++i)
    {
        Serial.println(LOOP_slide[i]);
    }

    // int LOOP_pitch_int[TRACKS]
    Serial.println("int LOOP_pitch_int[TRACKS]");
    for (auto i = 0; i < TRACKS; ++i)
    {
        Serial.println(LOOP_pitch_int[i]);
    }

    // float LOOP_stretch
    Serial.print("int LOOP_stretch : ");
    Serial.println(LOOP_stretch_int);

    // LOOP_element[TRACKS][LOOP_EVENTS]
    for (byte track = 0; track < TRACKS; ++track)
    {
        for (uint32_t event = 0; event < LOOP_events[track]; ++event)
        {
            Serial.print("*** LOOP_element[");
            Serial.print(track);
            Serial.print("][");
            Serial.print(event);
            Serial.println("]:");
            Serial.print("time: ");
            Serial.println(LOOP_element[track][event].time);
            Serial.print("midi_channel: ");
            Serial.println(LOOP_element[track][event].midi_channel);
            Serial.print("note_number: ");
            Serial.println(LOOP_element[track][event].note_number);
            Serial.print("velocity: ");
            Serial.println(LOOP_element[track][event].velocity);
            Serial.print("note_on: ");
            Serial.println(LOOP_element[track][event].note_on);
        }
    }
    return true;
}

String LOOP_Filename_midi_loop(int loop_id)
{
    String filename = String(loop_id);
    return String(filename + ".loop");
}

// Loop SD v1: magic line, uint32_t track count, uint16_t duration, uint32_t event counts,
// int32_t slides/pitches/stretch, then 8-byte events. Little-endian bytes, one decimal byte per line.
// Legacy files omit magic/track count and use uint8_t event counts; malformed 24-byte arrays are rejected.
static constexpr char LOOP_SD_MAGIC[] = "LILLALOOP 1\r\n";
static_assert(sizeof(int) == 4 && sizeof(LOOP_struct) == 8 && offsetof(LOOP_struct, note_on) == 7, "Loop SD event layout changed");

FLASHMEM
bool LOOP_Read_bytes(FsFile &file, void *destination, size_t size)
{
    auto *bytes = static_cast<uint8_t *>(destination);
    for (size_t i = 0; i < size; ++i)
    {
        unsigned int value = 0;
        unsigned int digits = 0;
        int c = file.read();
        while (c >= '0' && c <= '9' && digits < 3)
        {
            value = value * 10 + c - '0';
            ++digits;
            c = file.read();
        }
        if (c == '\r')
        {
            c = file.read();
        }
        if (digits == 0 || value > 255 || c != '\n')
        {
            return false;
        }
        bytes[i] = static_cast<uint8_t>(value);
    }
    return true;
}

FLASHMEM
bool LOOP_Write_bytes(FsFile &file, const void *source, size_t size)
{
    const auto *bytes = static_cast<const uint8_t *>(source);
    for (size_t i = 0; i < size; ++i)
    {
        const size_t expected = (bytes[i] >= 100 ? 3 : (bytes[i] >= 10 ? 2 : 1)) + 2;
        if (file.println(bytes[i]) != expected)
        {
            return false;
        }
    }
    return true;
}

FLASHMEM
bool LOOP_Read_midi_loop_file(FsFile &file, bool load)
{
    const bool versioned = file.peek() == 'L';
    if (versioned)
    {
        for (size_t i = 0; i < sizeof(LOOP_SD_MAGIC) - 1; ++i)
        {
            if (file.read() != LOOP_SD_MAGIC[i])
            {
                return false;
            }
        }
        uint32_t tracks = 0;
        if (!LOOP_Read_bytes(file, &tracks, sizeof(tracks)) || tracks != TRACKS)
        {
            return false;
        }
    }
    uint16_t duration = 0;
    uint32_t counts[TRACKS] = {};
    int slides[TRACKS], pitches[TRACKS], stretch;
    if (!LOOP_Read_bytes(file, &duration, sizeof(duration)))
    {
        return false;
    }
    for (int track = 0; track < TRACKS; ++track)
    {
        // Keep this bound in both passes: never trust an SD count as an array bound.
        if (!LOOP_Read_bytes(file, &counts[track], versioned ? sizeof(uint32_t) : sizeof(uint8_t)) || counts[track] > LOOP_EVENTS)
        {
            return false;
        }
    }
    if (!LOOP_Read_bytes(file, slides, sizeof(slides)) || !LOOP_Read_bytes(file, pitches, sizeof(pitches)) || !LOOP_Read_bytes(file, &stretch, sizeof(stretch)))
    {
        return false;
    }
    if (!load)
    {
        if (duration == 0 || counts[MASTER_TRACK] == 0 || stretch < 1 || stretch > 198)
        {
            return false;
        }
        for (int track = 0; track < TRACKS; ++track)
        {
            if (slides[track] < 0 || slides[track] >= duration || pitches[track] < -24 || pitches[track] > 24)
            {
                return false;
            }
        }
    }
    for (int track = 0; track < TRACKS; ++track)
    {
        for (uint32_t event = 0; event < counts[track]; ++event)
        {
            uint8_t bytes[8];
            if (!LOOP_Read_bytes(file, bytes, sizeof(bytes)))
            {
                return false;
            }
            int time;
            memcpy(&time, bytes, sizeof(time));
            if (!load && (time < 0 || time > duration || bytes[4] > 15 || bytes[5] > 127 || bytes[6] > 127 || bytes[7] > 1))
            {
                return false;
            }
            if (load)
            {
                LOOP_element[track][event] = {time, bytes[4], bytes[5], bytes[6], bytes[7] != 0};
            }
        }
    }
    if (file.getError() || file.curPosition() != file.fileSize())
    {
        return false;
    }
    if (load)
    {
        LOOP_time = duration;
        memcpy(LOOP_events, counts, sizeof(counts));
        memcpy(LOOP_slide, slides, sizeof(slides));
        memcpy(LOOP_pitch_int, pitches, sizeof(pitches));
        LOOP_stretch_int = stretch;
        LOOP_stretch = static_cast<float>(stretch) / 100.0f;
    }
    return true;
}

FLASHMEM
bool LOOP_Compile_midi_loop_file(FsFile &file)
{
    const uint32_t tracks = TRACKS;
    if (file.write(LOOP_SD_MAGIC, sizeof(LOOP_SD_MAGIC) - 1) != sizeof(LOOP_SD_MAGIC) - 1 || !LOOP_Write_bytes(file, &tracks, sizeof(tracks)))
    {
        return false;
    }
    if (!LOOP_Write_bytes(file, &LOOP_time, sizeof(LOOP_time)) || !LOOP_Write_bytes(file, LOOP_events, sizeof(LOOP_events)) || !LOOP_Write_bytes(file, LOOP_slide, sizeof(LOOP_slide)) || !LOOP_Write_bytes(file, LOOP_pitch_int, sizeof(LOOP_pitch_int)) || !LOOP_Write_bytes(file, &LOOP_stretch_int, sizeof(LOOP_stretch_int)))
    {
        return false;
    }
    for (int track = 0; track < TRACKS; ++track)
    {
        if (LOOP_events[track] > LOOP_EVENTS)
        {
            return false;
        }
        for (uint32_t event = 0; event < LOOP_events[track]; ++event)
        {
            if (!LOOP_Write_bytes(file, &LOOP_element[track][event], sizeof(LOOP_struct)))
            {
                return false;
            }
        }
    }
    return true;
}

FLASHMEM
bool LOOP_Recover_SD_file(const String &path)
{
    const String backup = path + ".bak";
    return SD.exists(path.c_str()) || !SD.exists(backup.c_str()) || SD.rename(backup.c_str(), path.c_str());
}

FLASHMEM
bool LOOP_Copy_midi_loop_from_RAM_to_SD(int loop_id)
{
    if (loop_id < 0 || loop_id >= MIDI_LOOP_FILES || !SD.begin(BUILTIN_SDCARD))
    {
        return false;
    }
    if (!SD.exists("/LILLALOOP") && !SD.mkdir("/LILLALOOP"))
    {
        return false;
    }
    const String path = "/LILLALOOP/" + LOOP_Filename_midi_loop(loop_id);
    const String temporary = path + ".tmp";
    const String backup = path + ".bak";
    if (!LOOP_Recover_SD_file(path))
    {
        return false;
    }
    FsFile file = SD.sdfs.open(temporary.c_str(), O_WRONLY | O_CREAT | O_TRUNC);
    if (!file)
    {
        return false;
    }
    const bool written = LOOP_Compile_midi_loop_file(file) && file.sync();
    const bool closed = file.close();
    if (!written || !closed)
    {
        return false;
    }
    file = SD.sdfs.open(temporary.c_str(), O_RDONLY);
    const bool verified = file && LOOP_Read_midi_loop_file(file, false);
    file.close();
    if (!verified)
    {
        return false;
    }
    const bool had_previous = SD.exists(path.c_str());
    if (had_previous)
    {
        if (SD.exists(backup.c_str()) && !SD.remove(backup.c_str()))
        {
            return false;
        }
        if (!SD.rename(path.c_str(), backup.c_str()))
        {
            return false;
        }
    }
    if (!SD.rename(temporary.c_str(), path.c_str()))
    {
        if (had_previous && !SD.rename(backup.c_str(), path.c_str()))
        {
            Serial.println(F("Loop replacement failed; previous loop retained in .loop.bak."));
        }
        return false;
    }
    return true; // Keep .bak until the next successful replacement or explicit deletion.
}

FLASHMEM
bool LOOP_Copy_midi_loop_from_SD_to_RAM(int loop_id)
{
    if (loop_id < 0 || loop_id >= MIDI_LOOP_FILES || !SD.begin(BUILTIN_SDCARD))
    {
        return false;
    }
    const String path = "/LILLALOOP/" + LOOP_Filename_midi_loop(loop_id);
    if (!LOOP_Recover_SD_file(path))
    {
        return false;
    }
    FsFile file = SD.sdfs.open(path.c_str(), O_RDONLY);
    if (!file || !LOOP_Read_midi_loop_file(file, false) || !file.seekSet(0))
    {
        return false; // No active-loop data has been changed.
    }
    LOOP_stop_and_reset_runnig_loop_data();
    // Same reader, no second semantic validation and no full-loop staging buffer.
    const bool loaded = LOOP_Read_midi_loop_file(file, true);
    file.close();
    if (!loaded)
    {
        LOOP_reset_all_data(); // A read failure must never expose partially loaded events to playback.
        LOOP_id = NEW_LOOP;
        LOOP_original = false;
        return false;
    }
    LOOP_original = true;
    return true;
}

FLASHMEM
bool LOOP_Delete_midi_loop_from_SD(int loop_id)
{
    if (loop_id < 0 || loop_id >= MIDI_LOOP_FILES || !SD.begin(BUILTIN_SDCARD))
    {
        return false;
    }
    const String path = "/LILLALOOP/" + LOOP_Filename_midi_loop(loop_id);
    // Remove sidecars first so a deleted loop cannot be rediscovered through its backup.
    for (const char *suffix : {".tmp", ".bak", ""})
    {
        const String target = path + suffix;
        if (SD.exists(target.c_str()) && !SD.remove(target.c_str()))
        {
            return false;
        }
    }
    return true;
}

bool LOOP_Look_for_midi_loop_in_SD(int loop_id)
{
    if (SD.begin(BUILTIN_SDCARD))
    {
        if (!SD.exists("/LILLALOOP"))
        {
            Serial.println(F("LOOP_Look_for_midi_loop_in_SD(int loop_id) - file doesn't exist."));
            return false;
        }

        String filename = LOOP_Filename_midi_loop(loop_id);
        String full_path = String("/LILLALOOP/" + filename);
        const char *full_path_ = &full_path[0];

        if (SD.exists(full_path_) || SD.exists((full_path + ".bak").c_str()))
        {
            return true;
        }
        else
        {
            Serial.println(F("LOOP_Look_for_midi_loop_in_SD(int loop_id) - file doesn't exist."));
            return false;
        }
    }
    Serial.println(F("LOOP_Look_for_midi_loop_in_SD(int loop_id) - ERROR - SD not present!"));
    return false;
}

int LOOP_Get_first_loop_id_free(void)
{
    if (!SD.begin(BUILTIN_SDCARD))
    {
        Serial.println(F("LOOP_Get_first_loop_id_free(void) - ERROR - SD not present!"));
        return -1;
    }
    else if (!SD.exists("/LILLALOOP"))
    {
        Serial.println(F("LOOP_Get_first_loop_id_free(void) - no .loop file in SD."));
        return 0; // The export procedure creates the directory on the first save.
    }
    for (auto loop_id = 0; loop_id < MIDI_LOOP_FILES; ++loop_id)
    {
        String filename = LOOP_Filename_midi_loop(loop_id);
        String full_path = String("/LILLALOOP/" + filename);
        const char *full_path_ = &full_path[0];
        if (!SD.exists(full_path_) && !SD.exists((full_path + ".bak").c_str()))
        {
            Serial.print(F("LOOP_Get_first_loop_id_free(void) - loop_id: "));
            Serial.println(loop_id);
            return loop_id;
        }
    }
    Serial.println(F("LOOP_Get_first_loop_id_free(void) - Please delete one .loop file in SD."));
    return -3;
}

int LOOP_Get_next_loop_id_in_SD(int loop_id)
{
    if (!SD.begin(BUILTIN_SDCARD))
    {
        Serial.println(F("LOOP_Get_next_loop_id_in_SD(int loop_id) - ERROR - SD not present!"));
        return loop_id;
    }
    else if (!SD.exists("/LILLALOOP"))
    {
        Serial.println(F("LOOP_Get_next_loop_id_in_SD(int loop_id) - no .loop file in SD."));
        return loop_id;
    }
    for (auto next_loop = loop_id + 1; next_loop < MIDI_LOOP_FILES; ++next_loop)
    {
        String filename = LOOP_Filename_midi_loop(next_loop);
        String full_path = String("/LILLALOOP/" + filename);
        const char *full_path_ = &full_path[0];
        if (SD.exists(full_path_) || SD.exists((full_path + ".bak").c_str()))
        {
            return next_loop;
        }
    }

    Serial.println(F("LOOP_Get_next_loop_id_in_SD(int loop_id) - loop_id is the last .loop file in SD."));
    return loop_id;
}

int LOOP_Get_previous_loop_id_in_SD(int loop_id)
{
    if (loop_id <= 0)
    {
        Serial.println(F("LOOP_Get_previous_loop_id_in_SD(int loop_id) - no .loop file before this!"));
        return loop_id;
    }
    if (!SD.begin(BUILTIN_SDCARD))
    {
        Serial.println(F("LOOP_Get_previous_loop_id_in_SD(int loop_id) - ERROR - SD not present!"));
        return loop_id;
    }
    else if (!SD.exists("/LILLALOOP"))
    {
        Serial.println(F("LOOP_Get_previous_loop_id_in_SD(int loop_id) - no .loop file in SD."));
        return loop_id;
    }
    for (auto previous_loop = loop_id - 1; previous_loop >= 0; --previous_loop)
    {
        String filename = LOOP_Filename_midi_loop(previous_loop);
        String full_path = String("/LILLALOOP/" + filename);
        const char *full_path_ = &full_path[0];
        if (SD.exists(full_path_) || SD.exists((full_path + ".bak").c_str()))
        {
            return previous_loop;
        }
    }
    Serial.println(F("LOOP_Get_previous_loop_id_in_SD(int loop_id) - loop_id is the first .loop file in SD."));
    return loop_id;
}

// ***************************************************************************************************************
// **********************************            VIRTUAL FILE SYSTEM            **********************************
// ***************************************************************************************************************

void VFS_Make_VFS(void)
{
    char packet_filename[NAME_PACKET_SIZE];
    Display_Storage.VFS_Make_presentation();

    // calcola DS_packets Free space, in PACKET_DIM
    VFS_packets_max = (Get_flash_size() - Get_flash_occupation() - FLASH_FREE_SPACE) / PACKET_DIM;
    VFS_packets_max = constrain(VFS_packets_max, 0, VFS_PACKETS_MAX);

    if (VFS_packets_max > 40)
    {
        Display_Storage.VFS_Make_assignments();
        VFS_packets = VFS_packets_max / 3.0f; // questa proporzione puo' essere modifitata a piacere
        Display_Storage.VFS_show_packets();
        bool confirmation = false;

        Clear_UI_events();
        while (!confirmation)
        {
            Shifters_manager.Update();

            result = Read_encoder_simple(EN_PB_Value);
            if (result != 0)
            {
                if (result == +1)
                {
                    if (VFS_packets <= (VFS_packets_max - 2))
                    {
                        VFS_packets += 2;
                        Display_Storage.VFS_show_packets();
                    }
                }
                else
                {
                    if (VFS_packets >= 2)
                    {
                        VFS_packets -= 2;
                        Display_Storage.VFS_show_packets();
                    }
                }
            }

            if (Read_pushbutton(EN_PB_Select))
            {
                confirmation = true;
            }
        }
        Clear_UI_events();

        // create VFS
        for (auto i = 0; i < VFS_packets; ++i)
        {
            SerialFlash.createErasable(Get_packet_name(i, packet_filename), PACKET_DIM);
        }

        Serial.print(F("Created Virtual File System  - VFS_packets are "));
        Serial.println(VFS_packets);

        Display_Storage.VFS_Make_restart();
        delay(10000);
    }

    else
    {
        Display_Storage.VFS_Make_not_enough_memory_for_sampler();
        delay(10000);
    }
}

int VFS_Get_packets(void)
{
    char packet_filename[NAME_PACKET_SIZE];
    int value = 0;
    for (auto i = 0; i < VFS_PACKETS_MAX; ++i)
    {
        if (SerialFlash.exists(Get_packet_name(i, packet_filename)))
        {
            ++value;
        }
    }
    return value;
}

FLASHMEM
void VFS_Print_allocation(void)
{
    Serial.println();

    Serial.println(F("*** VFS_Print_allocation() *** "));
    Serial.print(F("VFS_packets: "));
    Serial.println(VFS_packets);

    Serial.print(F("DS_VFS_packets: "));
    Serial.println(DS_VFS_packets);

    Serial.print(F("DS_First_packet: "));
    Serial.print(DS_First_packet);
    Serial.print(F(" DS_Last_packet: "));
    Serial.println(DS_Last_packet);

    Serial.println(F("*** Finished *** "));
    Serial.println();
}

// VFS operations run without audio callbacks, which could otherwise access the same SPI Flash.
struct VFS_Audio_guard
{
    const bool enabled = NVIC_IS_ENABLED(IRQ_SOFTWARE) != 0;
    bool successful = false;
    VFS_Audio_guard() { AudioNoInterrupts(); }
    bool Complete()
    {
        successful = true;
        return true;
    }
    ~VFS_Audio_guard()
    {
        if (successful && enabled && SerialFlash.ready())
        {
            AudioInterrupts();
        }
    }
};

FLASHMEM
void Require_VFS(bool result)
{
    if (result)
    {
        return;
    }
    Serial.println(F("VFS operation failed; restart required. Incomplete recordings remain marked in FRAM."));
    AudioNoInterrupts();
    Trigger.Stop();
    Midi_reader.Stop();
    while (true)
    {
        delay(10);
    }
}

FLASHMEM
bool VFS_Valid_span(const VFS_Recording &entry)
{
    const int channels = entry.stereo ? 2 : 1;
    return entry.packets >= 0 && entry.packets <= DS_VFS_packets / channels && (entry.packets == 0 || (entry.first_packet >= DS_First_packet && entry.first_packet <= DS_VFS_packets - entry.packets * channels));
}

FLASHMEM
bool VFS_Compile_FAT_table(void)
{
    if (DS_First_packet < 0 || DS_VFS_packets < DS_First_packet || DS_VFS_packets > VFS_PACKETS_DS)
    {
        return false;
    }
    VFS_Reset_FAT_table();
    for (int id = 0; id < RECORDINGS; ++id)
    {
        const auto &entry = Recording[id];
        if (!VFS_Valid_span(entry))
        {
            return false;
        }
        if (entry.consistent)
        {
            const int end = entry.first_packet + entry.packets * (entry.stereo ? 2 : 1);
            for (int packet = entry.first_packet; packet < end; ++packet)
            {
                if (VFS_FAT_table[packet] != -1)
                {
                    return false; // Never erase or move recordings with overlapping ownership.
                }
                VFS_FAT_table[packet] = id;
            }
        }
    }
    return true;
}

void VFS_Reset_FAT_table(void)
{
    Serial.println(F("*** VFS_Reset_FAT_table() *** "));
    for (auto i = DS_First_packet; i < DS_VFS_packets; ++i)
    {
        VFS_FAT_table[i] = -1;
    }
}

int VFS_Get_first_packet_free(void)
{
    for (auto i = DS_First_packet; i < DS_VFS_packets; ++i)
    {
        if (VFS_FAT_table[i] == -1)
        {
            Serial.print(F("First packet free is: "));
            Serial.println(i);
            return i;
        }
    }
    return -1;
}

void VFS_Erase_all_packets(void)
{
    char packet_filename[NAME_PACKET_SIZE];
    Serial.println("*** Erase ALL Packets and VFS_FAT ***");
    for (auto i = 0; i < VFS_PACKETS_MAX; ++i) // for(auto i = 0; i < VFS_packets; ++i)
    {
        if (SerialFlash.exists(Get_packet_name(i, packet_filename)))
        {
            Require_VFS(VFS_Erase_packet(i));
        }
    }
    Serial.println(F("*** Finished *** "));
    Serial.println();
}

void VFS_Erase_all_packets_for_DS(void)
{
    Serial.println("*** Erase ALL Packets for Direct Sampling and VFS_FAT ***");
    for (auto i = DS_First_packet; i <= DS_Last_packet; ++i)
    {
        Require_VFS(VFS_Erase_packet(i));
    }
    Serial.println(F("*** Finished *** "));
    Serial.println();
}

static constexpr uint32_t VFS_FLASH_TIMEOUT_MS = 10000;
static constexpr uint32_t VFS_COPY_BYTES = 256;

FLASHMEM
bool VFS_Wait_flash(void)
{
    const uint32_t start = millis();
    while (!SerialFlash.ready())
    {
        if (static_cast<uint32_t>(millis() - start) >= VFS_FLASH_TIMEOUT_MS)
        {
            return false;
        }
        delay(1);
    }
    return true;
}

FLASHMEM
bool VFS_Open_packet(int id, SerialFlashFile &file)
{
    char packet_filename[NAME_PACKET_SIZE];
    if (id < 0 || id >= VFS_PACKETS_MAX || !VFS_Wait_flash())
    {
        return false;
    }
    file = SerialFlash.open(Get_packet_name(id, packet_filename));
    const uint32_t block = SerialFlash.blockSize();
    return file && file.size() == PACKET_DIM && block > 0 && PACKET_DIM % block == 0 && file.getFlashAddress() % block == 0;
}

FLASHMEM
bool VFS_Packet_is_blank(SerialFlashFile &file, bool &blank)
{
    uint8_t buffer[VFS_COPY_BYTES];
    blank = false;
    file.seek(0);
    for (uint32_t offset = 0; offset < PACKET_DIM; offset += sizeof(buffer))
    {
        if (!VFS_Wait_flash() || file.read(buffer, sizeof(buffer)) != sizeof(buffer))
        {
            return false;
        }
        for (uint8_t value : buffer)
        {
            if (value != 0xFF)
            {
                return true;
            }
        }
    }
    blank = true;
    return true;
}

FLASHMEM
bool VFS_Erase_packet(int value)
{
    VFS_Audio_guard audio_guard;
    SerialFlashFile file;
    if (!VFS_Open_packet(value, file))
    {
        return false;
    }
    // Issue one physical erase at a time so the driver's internal wait cannot hide a timeout.
    const uint32_t block = SerialFlash.blockSize();
    for (uint32_t offset = 0; offset < PACKET_DIM; offset += block)
    {
        SerialFlash.eraseBlock(file.getFlashAddress() + offset);
        if (!VFS_Wait_flash())
        {
            return false;
        }
    }
    bool blank = false;
    if (!VFS_Packet_is_blank(file, blank) || !blank)
    {
        return false;
    }
    if (value >= DS_First_packet && value < DS_VFS_packets)
    {
        VFS_FAT_table[value] = -1;
    }
    return audio_guard.Complete();
}

FLASHMEM
bool VFS_Save_recording_verified(int id)
{
    const VFS_Recording expected = Recording[id];
    if (Archive.Save_DS_Recording(id) != LillaFRAM_2x512::ERROR_0)
    {
        return false;
    }
    const byte result = Archive.Read_DS_Recording(id); // Uses the public CRC-checked reader.
    const auto &stored = Recording[id];
    const bool verified = result == LillaFRAM_2x512::ERROR_0 && stored.first_packet == expected.first_packet && stored.packets == expected.packets && stored.stereo == expected.stereo && stored.consistent == expected.consistent;
    Recording[id] = expected;
    return verified;
}

FLASHMEM
bool VFS_Clean_up_orphan_packets(void)
{
    VFS_Audio_guard audio_guard;
    if (!VFS_Compile_FAT_table())
    {
        return false;
    }
    for (int packet = DS_First_packet; packet < DS_VFS_packets; ++packet)
    {
        if (VFS_FAT_table[packet] == -1)
        {
            SerialFlashFile file;
            bool blank = false;
            if (!VFS_Open_packet(packet, file) || !VFS_Packet_is_blank(file, blank))
            {
                return false;
            }
            if (!blank && !VFS_Erase_packet(packet))
            {
                return false;
            }
        }
    }
    return audio_guard.Complete();
}

FLASHMEM
bool VFS_Clean_up_VFS(void)
{
    VFS_Audio_guard audio_guard;
    if (!VFS_Compile_FAT_table())
    {
        return false;
    }
    bool pending = false;
    for (int id = 0; id < RECORDINGS; ++id)
    {
        if (!Recording[id].consistent)
        {
            // Also covers runtime Delete: persist intent before the first Flash change.
            if (!VFS_Save_recording_verified(id))
            {
                return false;
            }
            pending = true;
        }
    }
    if (!pending)
    {
        return audio_guard.Complete();
    }
    // Keep every pending marker until originals AND orphaned destinations are erased.
    // A second power loss during cleanup will therefore cause cleanup to run again.
    if (!VFS_Clean_up_orphan_packets())
    {
        return false;
    }
    for (int id = 0; id < RECORDINGS; ++id)
    {
        if (!Recording[id].consistent)
        {
            const VFS_Recording previous = Recording[id];
            Recording[id] = {};
            Recording[id].consistent = true;
            if (!VFS_Save_recording_verified(id))
            {
                Recording[id] = previous;
                return false;
            }
        }
    }
    return audio_guard.Complete();
}

FLASHMEM
bool VFS_Packet_used_bytes(SerialFlashFile &file, uint32_t &used)
{
    uint8_t buffer[VFS_COPY_BYTES];
    for (uint32_t end = PACKET_DIM; end > 0; end -= sizeof(buffer))
    {
        file.seek(end - sizeof(buffer));
        if (!VFS_Wait_flash() || file.read(buffer, sizeof(buffer)) != sizeof(buffer))
        {
            return false;
        }
        for (int i = sizeof(buffer) - 1; i >= 0; --i)
        {
            if (buffer[i] != 0xFF)
            {
                used = (end - sizeof(buffer) + i + 2) & ~1U; // Preserve the complete final 16-bit sample.
                return true;
            }
        }
    }
    used = 0;
    return true;
}

FLASHMEM
bool VFS_Shift_file(int to_packet, int recording_id)
{
    VFS_Audio_guard audio_guard;
    if (recording_id < 0 || recording_id >= RECORDINGS)
    {
        return false;
    }
    const auto &entry = Recording[recording_id];
    if (!VFS_Valid_span(entry) || entry.consistent || entry.packets == 0 || to_packet < DS_First_packet || to_packet >= entry.first_packet)
    {
        return false;
    }
    const int from_packet = entry.first_packet;
    const int channels = entry.stereo ? 2 : 1;
    const int packets = entry.packets * channels;
    uint8_t source[VFS_COPY_BYTES], actual[VFS_COPY_BYTES];
    for (int i = 0; i < packets; ++i)
    {
        SerialFlashFile from, to;
        if (!VFS_Open_packet(from_packet + i, from) || !VFS_Open_packet(to_packet + i, to))
        {
            return false;
        }
        uint32_t used = PACKET_DIM;
        // Runtime byte counts are inferred, not stored. Inspect each channel's actual tail independently.
        if (i / channels == entry.packets - 1 && !VFS_Packet_used_bytes(from, used))
        {
            return false;
        }
        if (!VFS_Erase_packet(to_packet + i))
        {
            return false;
        }
        from.seek(0);
        to.seek(0);
        for (uint32_t offset = 0; offset < used; offset += VFS_COPY_BYTES)
        {
            const uint32_t count = used - offset < VFS_COPY_BYTES ? used - offset : VFS_COPY_BYTES;
            if (!VFS_Wait_flash() || from.read(source, count) != count || to.write(source, count) != count || !VFS_Wait_flash())
            {
                return false;
            }
            to.seek(offset);
            if (to.read(actual, count) != count || memcmp(source, actual, count) != 0)
            {
                return false;
            }
        }
    }
    // Overlapping source packets are already destinations: erase only the abandoned source tail.
    const int tail = from_packet > to_packet + packets ? from_packet : to_packet + packets;
    for (int packet = tail; packet < from_packet + packets; ++packet)
    {
        if (!VFS_Erase_packet(packet))
        {
            return false;
        }
    }
    return audio_guard.Complete();
}

FLASHMEM
bool VFS_Defragment(void)
{
    VFS_Audio_guard audio_guard;
    if (!VFS_Compile_FAT_table())
    {
        return false;
    }
    for (int id = 0; id < RECORDINGS; ++id)
    {
        if (!Recording[id].consistent)
        {
            return false; // Cleanup must precede compaction.
        }
    }
    int destination = DS_First_packet;
    int source = DS_First_packet;
    while (source < DS_VFS_packets)
    {
        const int id = VFS_FAT_table[source];
        if (id < 0)
        {
            ++source;
            continue;
        }
        auto &entry = Recording[id];
        const int packets = entry.packets * (entry.stereo ? 2 : 1);
        if (destination < source)
        {
            // Check file geometry before changing the persistent validity marker.
            for (int i = 0; i < packets; ++i)
            {
                SerialFlashFile from, to;
                if (!VFS_Open_packet(source + i, from) || !VFS_Open_packet(destination + i, to))
                {
                    return false;
                }
            }
            entry.consistent = false;
            if (!VFS_Save_recording_verified(id) || !VFS_Shift_file(destination, id))
            {
                return false;
            }
            entry.first_packet = destination;
            entry.consistent = true;
            if (!VFS_Save_recording_verified(id))
            {
                entry.first_packet = source;
                entry.consistent = false;
                return false;
            }
            for (int i = source; i < source + packets; ++i)
            {
                VFS_FAT_table[i] = -1;
            }
            for (int i = destination; i < destination + packets; ++i)
            {
                VFS_FAT_table[i] = id;
            }
        }
        destination += packets;
        source += packets;
    }
    return audio_guard.Complete();
}

void VFS_Print_FAT(void)
{
    Serial.println();
    for (auto i = DS_First_packet; i < DS_VFS_packets; ++i)
    {
        Serial.print("VFS_FAT_table[");
        Serial.print(i);
        Serial.print("] = ");
        Serial.println(VFS_FAT_table[i]);
    }
    Serial.println();
}

// ***************************************************************************************************************
// **********************************            STANDARD RAW FILES             **********************************
// ***************************************************************************************************************

// Complete SD backups: metadata v3 binds every audio channel by length and CRC.
// RAW files preserve full allocated packets, including erased tails, without trusting inferred runtime lengths.
static constexpr char BACKUP_ROOT[] = "/LILLABACKUP";
static constexpr char BACKUP_CONFIG[] = "LILLA_CONFIG.fram";

FLASHMEM
uint32_t BACKUP_Update_crc(uint32_t crc, const uint8_t *data, size_t size)
{
    for (size_t i = 0; i < size; ++i)
    {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; ++bit)
        {
            crc = (crc >> 1) ^ ((crc & 1U) ? 0xEDB88320UL : 0UL);
        }
    }
    return crc;
}

FLASHMEM
void BACKUP_Audio_path(char *path, size_t size, const char *directory, int id, int channel)
{
    snprintf(path, size, "%s/REC_%02d_%c.raw", directory, id, channel == 0 ? 'L' : 'R');
}

FLASHMEM
bool BACKUP_Verify_audio(const char *path, uint32_t bytes, uint32_t expected_crc)
{
    FsFile file = SD.sdfs.open(path, O_RDONLY);
    if (!file || file.fileSize() != bytes)
    {
        return false;
    }
    uint8_t buffer[VFS_COPY_BYTES];
    uint32_t crc = 0xFFFFFFFFUL;
    for (uint32_t offset = 0; offset < bytes; offset += sizeof(buffer))
    {
        const uint32_t count = bytes - offset < sizeof(buffer) ? bytes - offset : sizeof(buffer);
        if (file.read(buffer, count) != static_cast<int>(count))
        {
            return false;
        }
        crc = BACKUP_Update_crc(crc, buffer, count);
    }
    return !file.getError() && (crc ^ 0xFFFFFFFFUL) == expected_crc;
}

FLASHMEM
bool BACKUP_Copy_audio(FsFile &file, const VFS_Recording &entry, int channel, bool restore, uint32_t &crc)
{
    uint8_t buffer[VFS_COPY_BYTES], actual[VFS_COPY_BYTES];
    crc = 0xFFFFFFFFUL;
    for (int packet = 0; packet < entry.packets; ++packet)
    {
        SerialFlashFile flash;
        const int id = entry.first_packet + packet * (entry.stereo ? 2 : 1) + channel;
        if (!VFS_Open_packet(id, flash))
        {
            return false;
        }
        for (uint32_t offset = 0; offset < PACKET_DIM; offset += sizeof(buffer))
        {
            if (!VFS_Wait_flash())
            {
                return false;
            }
            if (restore)
            {
                if (file.read(buffer, sizeof(buffer)) != sizeof(buffer) || flash.write(buffer, sizeof(buffer)) != sizeof(buffer) || !VFS_Wait_flash())
                {
                    return false;
                }
                flash.seek(offset);
                if (flash.read(actual, sizeof(actual)) != sizeof(actual) || memcmp(buffer, actual, sizeof(buffer)) != 0)
                {
                    return false;
                }
            }
            else
            {
                if (flash.read(buffer, sizeof(buffer)) != sizeof(buffer) || file.write(buffer, sizeof(buffer)) != sizeof(buffer))
                {
                    return false;
                }
            }
            crc = BACKUP_Update_crc(crc, buffer, sizeof(buffer));
        }
    }
    crc ^= 0xFFFFFFFFUL;
    return !file.getError();
}

FLASHMEM
bool BACKUP_Verify_directory(const char *directory, ArchivingManager::Recording_backup_audio *audio, VFS_Recording *entries, int &capacity, bool discard_invalid_audio = false, bool *config_error = nullptr)
{
    char packet_filename[NAME_PACKET_SIZE];
    char path[64];
    snprintf(path, sizeof(path), "%s/%s", directory, BACKUP_CONFIG);
    File config = SD.open(path);
    if (!config || !Archive.Read_backup_audio(config, audio, entries))
    {
        if (config_error != nullptr)
        {
            *config_error = true;
        }
        return false;
    }
    config.close();
    // Recovery runs before runtime loading: probe physical packets, not the old FRAM addresses.
    capacity = 0;
    bool gap = false;
    for (int packet = 0; packet < VFS_PACKETS_MAX; ++packet)
    {
        if (!SerialFlash.exists(Get_packet_name(packet, packet_filename)))
        {
            gap = true;
            continue;
        }
        SerialFlashFile flash;
        if (gap || !VFS_Open_packet(packet, flash))
        {
            return false;
        }
        ++capacity;
    }
    uint32_t required = 0;
    for (int id = 0; id < RECORDINGS; ++id)
    {
        for (int channel = 0; channel < 2; ++channel)
        {
            if (audio[id].bytes[channel] > 0)
            {
                BACKUP_Audio_path(path, sizeof(path), directory, id, channel);
                if (!BACKUP_Verify_audio(path, audio[id].bytes[channel], audio[id].crc32[channel]))
                {
                    if (!discard_invalid_audio)
                    {
                        return false;
                    }
                    // Discard the whole Recording, including both stereo channels; never retry its audio.
                    entries[id] = {};
                    entries[id].consistent = true;
                    audio[id] = {};
                    Serial.print(F("Restore: cleared Recording with missing or invalid audio: "));
                    Serial.println(id);
                    break;
                }
            }
        }
        required += entries[id].packets * (entries[id].stereo ? 2U : 1U);
    }
    return required <= static_cast<uint32_t>(capacity);
}

FLASHMEM
bool BACKUP_Export_files(void)
{
    if (!SD.begin(BUILTIN_SDCARD) || Archive.Check_FRAM_archive() != LillaFRAM_2x512::ERROR_0)
    {
        return false;
    }
    if (!SD.exists(BACKUP_ROOT) && !SD.mkdir(BACKUP_ROOT))
    {
        return false;
    }
    uint32_t highest = 0;
    File directory = SD.open(BACKUP_ROOT);
    if (!directory || !directory.isDirectory())
    {
        return false;
    }
    while (true)
    {
        File entry = directory.openNextFile();
        if (!entry)
        {
            break;
        }
        const char *name = entry.name();
        const char *slash = strrchr(name, '/');
        name = slash ? slash + 1 : name;
        const size_t length = strlen(name);
        if (length == 6 || (length == 10 && strcmp(name + 6, ".tmp") == 0))
        {
            uint32_t number = 0;
            bool numeric = true;
            for (int i = 0; i < 6; ++i)
            {
                numeric &= name[i] >= '0' && name[i] <= '9';
                number = number * 10 + (numeric ? name[i] - '0' : 0);
            }
            if (numeric && number > highest)
            {
                highest = number;
            }
        }
        entry.close();
    }
    directory.close();

    if (highest >= 999999)
    {
        return false;
    }

    char temporary[40], destination[40], path[64];
    snprintf(temporary, sizeof(temporary), "%s/%06lu.tmp", BACKUP_ROOT, static_cast<unsigned long>(highest + 1));
    snprintf(destination, sizeof(destination), "%s/%06lu", BACKUP_ROOT, static_cast<unsigned long>(highest + 1));

    if (SD.exists(temporary) || SD.exists(destination) || !SD.mkdir(temporary))
    {
        return false;
    }
    ArchivingManager::Recording_backup_audio audio[RECORDINGS]{};
    VFS_Recording entries[RECORDINGS]{};

    for (int id = 0; id < RECORDINGS; ++id)
    {
        const VFS_Recording runtime = Recording[id];
        const byte result = Archive.Read_DS_Recording(id);
        entries[id] = Recording[id];
        Recording[id] = runtime;
        if (result != LillaFRAM_2x512::ERROR_0 || !entries[id].consistent || !VFS_Valid_span(entries[id]))
        {
            return false;
        }
        const auto &entry = entries[id];
        if (entry.packets == 0)
        {
            continue;
        }
        for (int earlier = 0; earlier < id; ++earlier)
        {
            const auto &other = entries[earlier];
            if (other.packets > 0 && entry.first_packet < other.first_packet + other.packets * (other.stereo ? 2 : 1) && other.first_packet < entry.first_packet + entry.packets * (entry.stereo ? 2 : 1))
            {
                return false;
            }
        }
        for (int channel = 0; channel < (entry.stereo ? 2 : 1); ++channel)
        {
            BACKUP_Audio_path(path, sizeof(path), temporary, id, channel);
            FsFile file = SD.sdfs.open(path, O_WRONLY | O_CREAT | O_EXCL);
            if (!file)
            {
                return false;
            }
            audio[id].bytes[channel] = static_cast<uint32_t>(entry.packets) * PACKET_DIM;
            const bool written = BACKUP_Copy_audio(file, entry, channel, false, audio[id].crc32[channel]) && file.sync();
            const bool closed = file.close();
            if (!written || !closed)
            {
                return false;
            }
        }
    }

    // Written last: an interrupted audio export never acquires a complete configuration file.
    snprintf(path, sizeof(path), "%s/%s", temporary, BACKUP_CONFIG);
    File config = SD.open(path, FILE_WRITE);
    const bool saved = config && Archive.Save_FRAM_backup(config, audio);
    config.close();
    int capacity = 0;

    if (!saved || !BACKUP_Verify_directory(temporary, audio, entries, capacity) || !SD.rename(temporary, destination))
    {
        return false;
    }

    Serial.print(F("Complete backup saved: "));
    Serial.println(destination);

    return true;
}

FLASHMEM
bool BACKUP_Export(void)
{
    VFS_Audio_guard guard;
    const bool saved = BACKUP_Export_files();
    guard.Complete(); // An SD export failure has not changed the live archive.
    return saved;
}

FLASHMEM
bool BACKUP_Restore_files(bool &changed, bool &config_error)
{
    if (!SD.begin(BUILTIN_SDCARD))
    {
        config_error = true;
        return false;
    }

    if (!VFS_Wait_flash())
    {
        return false;
    }

    ArchivingManager::Recording_backup_audio audio[RECORDINGS]{};
    VFS_Recording entries[RECORDINGS]{};
    int capacity = 0;

    if (!BACKUP_Verify_directory(BACKUP_ROOT, audio, entries, capacity, true, &config_error))
    {
        return false;
    }

    char path[64];
    snprintf(path, sizeof(path), "%s/%s", BACKUP_ROOT, BACKUP_CONFIG);
    File config = SD.open(path);

    if (!config || !Archive.Verify_FRAM_backup(config))
    {
        config_error = true;
        return false;
    }

    changed = true; // Even a torn state-marker write requires recovery rather than resuming playback.
    if (!Archive.Restore_FRAM_backup(config, false))
    {
        return false;
    }

    config.close();
    DS_First_packet = 0;
    DS_VFS_packets = capacity;
    DS_Last_packet = capacity - 1;
    VFS_packets = capacity;

    // Existing RAW library files are untouched; only the VFS packet area is replaced.
    for (int packet = 0; packet < capacity; ++packet)
    {
        if (!VFS_Erase_packet(packet))
        {
            return false;
        }
    }
    int destination = 0;
    for (int id = 0; id < RECORDINGS; ++id)
    {
        auto &entry = entries[id];
        entry.first_packet = entry.packets > 0 ? destination : 0;
        for (int channel = 0; channel < (entry.stereo ? 2 : 1) && entry.packets > 0; ++channel)
        {
            BACKUP_Audio_path(path, sizeof(path), BACKUP_ROOT, id, channel);
            FsFile file = SD.sdfs.open(path, O_RDONLY);
            uint32_t crc = 0;
            if (!file || file.fileSize() != audio[id].bytes[channel] || !BACKUP_Copy_audio(file, entry, channel, true, crc) || crc != audio[id].crc32[channel])
            {
                return false;
            }
        }
        Recording[id] = entry;
        if (!VFS_Save_recording_verified(id))
        {
            return false;
        }
        destination += entry.packets * (entry.stereo ? 2 : 1);
    }
    return Archive.Set_FRAM_archive_state(ArchivingManager::ARCHIVE_READY) == LillaFRAM_2x512::ERROR_0;
}

FLASHMEM
bool BACKUP_Restore(bool *config_error)
{
    VFS_Audio_guard guard;
    bool changed = false;
    bool invalid_config = false;
    const bool restored = BACKUP_Restore_files(changed, invalid_config);
    if (config_error != nullptr)
    {
        *config_error = invalid_config;
    }
    if (restored || !changed)
    {
        guard.Complete();
    }
    return restored;
}

// End complete SD backups.

int Get_next_raw_file_in_flash(int file)
{
    int value = file;
    do
    {
        ++value;
        if (value == FIRST_LIVE_SAMPLING_FILE)
        {
            Serial.println(F("Get_next_raw_file_in_flash() --> reached last file!"));
            return file;
        }
        if (Get_samples_in_raw_file(value) > 0)
        {
            Serial.print(F("Get_next_raw_file_in_flash() --> next file found : "));
            Serial.println(value);
            return value;
        }
    } while (1);
}

int Get_previous_raw_file_in_flash(int file)
{
    int value = file;
    do
    {
        --value;
        if (value == -1)
        {
            Serial.print(F("Get_previous_raw_file_in_flash() --> reached first file!"));
            return file;
        }
        if (Get_samples_in_raw_file(value) > 0)
        {
            Serial.print(F("Get_previous_raw_file_in_flash() --> previous file found: "));
            Serial.println(value);
            return value;
        }
    } while (1);
}

int Get_samples_in_raw_file(int file_id) // value is a file_id
{
    if (file_id < FIRST_RECORDING_FILE)
        return Info.Raw_file_samples(file_id);

    else if (file_id < FIRST_LIVE_SAMPLING_FILE)
    {
        int recording = (file_id - FIRST_RECORDING_FILE) / 2;
        bool file_L_flag = ((file_id - FIRST_RECORDING_FILE) % 2 == 0);
        if (!Recording[recording].stereo && !file_L_flag)
            return 0;
        else
            return Info.DS_recording_samples(recording); // DS_recording_samples(int first_packet, int packets)
    }

    else
        return 0;
}

bool Verify_space_on_flash(int value)
{
    return ((Get_flashchip_size() - Get_flash_occupation()) >= value ? true : false);
}

int Get_first_raw_file_available(int start_value)
{
    char audio_filename[NAME_FILE_SIZE];
    if (start_value >= 0)
    {
        for (auto i = start_value; i < FIRST_RECORDING_FILE; ++i)
        {
            if (FileNameRegistry::Numeric_available(i) && Capture_find(i) == nullptr && !SerialFlash.exists(Get_file_name(i, audio_filename)))
            {
                return i;
            }
        }
    }
    return -1; // nessun file .RAW
}

FLASHMEM
void Print_flash_file_list(void)
{
    uint32_t filesize = 0;
    int occupation = 0;
    Serial.println(F("All Files on SPI Flash chip:")); // Questo puo' esser fatto solo per i "Serial.print" che contengono stringhe di testo COSTANTI e non per i "Serial.print" che contengono variabili.
    SerialFlash.opendir();
    while (1)
    {
        char filename[64];
        // SerialFlash.readdir compila
        // - filename
        // - sizeof(filename)
        // - filesize
        // e restituisce "true" se il file esiste
        if (SerialFlash.readdir(filename, sizeof(filename), filesize))
        {
            occupation += filesize;
            Serial.print(F("filename:"));
            Serial.print(filename);
            Serial.print(F("  filesize:"));
            Serial.print(filesize);
            Serial.print(F(" bytes"));
            Serial.println();
        }
        else
            break; // no more files
    }
    Serial.println(F("---------------"));
    Serial.print(F("Total space occupied (kB): "));
    Serial.println(occupation >> 10);
    Serial.print(F("Total space free (kB): "));
    Serial.println((Get_flash_size() - occupation) >> 10);
    Serial.println(F("---------------"));
}

FLASHMEM
int Get_flashchip_size(void)
{
    unsigned char buf[256];
    unsigned long chipsize, blocksize;
    SerialFlash.readID(buf);
    chipsize = SerialFlash.capacity(buf);

    Serial.println();
    Serial.println(F("Read Chip Identification:"));
    Serial.print(F("  JEDEC ID:     "));
    Serial.print(buf[0], HEX);
    Serial.print(' ');
    Serial.print(buf[1], HEX);
    Serial.print(' ');
    Serial.println(buf[2], HEX);
    Serial.print(F("  Part Number: "));
    Serial.println(id2chip(buf));
    Serial.print(F("  Memory Size:  "));
    Serial.print(chipsize);
    Serial.println(F(" bytes"));

    if (chipsize == 0)
    {
        return false;
    }
    blocksize = SerialFlash.blockSize();

    Serial.print(F("  Block Size:   "));
    Serial.print(blocksize);
    Serial.println(F(" bytes"));
    return chipsize;
}

int Get_raw_files(void)
{
    char audio_filename[NAME_FILE_SIZE];
    int value = 0;
    for (auto i = 0; i < FIRST_RECORDING_FILE; ++i)
    {
        if (FileNameRegistry::Assigned(i) && SerialFlash.exists(Get_file_name(i, audio_filename)))
        {
            ++value;
        }
    }
    return value; // nessun file .RAW
}

int Get_raw_files_volume(void)
{
    char audio_filename[NAME_FILE_SIZE];
    unsigned long value = 0;
    for (auto i = 0; i < FIRST_RECORDING_FILE; ++i)
    {
        if (FileNameRegistry::Assigned(i) && SerialFlash.exists(Get_file_name(i, audio_filename)))
        {
            SerialFlashFile raw_file = SerialFlash.open(Get_file_name(i, audio_filename));
            value += raw_file.size();
            raw_file.close();
        }
    }
    return value;
}

FLASHMEM
const char *id2chip(const unsigned char *id)
{
    if (id[0] == 0xEF)
    {
        // Winbond
        if (id[1] == 0x40)
        {
            if (id[2] == 0x14)
            {
                return "W25Q80BV";
            }
            if (id[2] == 0x15)
            {
                return "W25Q16DV";
            }
            if (id[2] == 0x17)
            {
                return "W25Q64FV";
            }
            if (id[2] == 0x18)
            {
                return "W25Q128FV";
            }
            if (id[2] == 0x19)
            {
                return "W25Q256FV";
            }
            if (id[2] == 0x20) // aggiunto io
            {
                return "W25Q512FV";
            }
        }
    }

    if (id[0] == 0x01)
    {
        // Spansion
        if (id[1] == 0x02)
        {
            if (id[2] == 0x16)
            {
                return "S25FL064A";
            }
            if (id[2] == 0x19)
            {
                return "S25FL256S";
            }
            if (id[2] == 0x20)
            {
                return "S25FL512S";
            }
        }
        if (id[1] == 0x20)
        {
            if (id[2] == 0x18)
            {
                return "S25FL127S";
            }
        }
    }

    if (id[0] == 0xC2)
    {
        // Macronix
        if (id[1] == 0x20)
        {
            if (id[2] == 0x18)
            {
                return "MX25L12805D";
            }
        }
    }

    if (id[0] == 0x20)
    {
        // Micron
        if (id[1] == 0xBA)
        {
            if (id[2] == 0x20)
            {
                return "N25Q512A";
            }
            if (id[2] == 0x21)
            {
                return "N25Q00AA";
            }
        }
        if (id[1] == 0xBB)
        {
            if (id[2] == 0x22)
            {
                return "MT25QL02GC";
            }
        }
    }

    if (id[0] == 0xBF)
    {
        // SST
        if (id[1] == 0x25)
        {
            if (id[2] == 0x02)
            {
                return "SST25WF010";
            }
            if (id[2] == 0x03)
            {
                return "SST25WF020";
            }
            if (id[2] == 0x04)
            {
                return "SST25WF040";
            }
            if (id[2] == 0x41)
            {
                return "SST25VF016B";
            }
            if (id[2] == 0x4A)
            {
                return "SST25VF032";
            }
        }
        if (id[1] == 0x25)
        {
            if (id[2] == 0x01)
            {
                return "SST26VF016";
            }
            if (id[2] == 0x02)
            {
                return "SST26VF032";
            }
            if (id[2] == 0x43)
            {
                return "SST26VF064";
            }
        }
    }

    if (id[0] == 0x1F)
    {
        // Adesto
        if (id[1] == 0x89)
        {
            if (id[2] == 0x01)
            {
                return "AT25SF128A";
            }
        }
    }

    return "(unknown chip)";
}

// ***************************************************************************************************************
// **********************************                   NOCLICK                 **********************************
// ***************************************************************************************************************

uint16_t S_Calc_Noclick_max(bool use_Wavetable)
{
    if (use_Wavetable)
    {
        return 42;
    }
    else
    {
        return NOCLICK_DIM;
    }
}

// ***************************************************************************************************************
// **********************************                WAVETABLE                  **********************************
// ***************************************************************************************************************
// ***************************************************************************************************************
// **********************************               FACTORY SETUP               **********************************
// ***************************************************************************************************************
FLASHMEM
void Factory_setup_FRAM(void)
{
    Require_FRAM(Archive.Factory_reset_FRAM(false));

    // cancella gli array descrittivi di Patch e Sound
    P_Delete_all_Patches_and_Sounds();

    // Definisce e salva la Patch iniziale in FRAM.
    Patch[0].used = true;
    Patch[0].instruments = 1; // number of instruments in the patch_id
    Patch[0].Instrument[0].used = 1;
    Patch[0].Instrument[0].sound_id = 0;
    Patch[0].Instrument[0].root_key = 60;
    Patch[0].Instrument[0].from_note = 0;
    Patch[0].Instrument[0].to_note = 127;
    Patch[0].Instrument[0].precedence = 0;
    Patch[0].Instrument[0].lock = 0;

    Patch[0].Instrument[0].Filter.use = 0;            // yes/no
    Patch[0].Instrument[0].Filter.type = 1;           // lowpass, highpass, bandpass, notch
    Patch[0].Instrument[0].Filter.pivot = 20;         // 0 --> 100 filter frequency/note frequency
    Patch[0].Instrument[0].Filter.resonance = 7;      // 0 --> 40
    Patch[0].Instrument[0].Filter.modulation = 3;     // 0 --> 4 bit 1,2,3:modulation
    Patch[0].Instrument[0].Filter.index = 20;         // 1 --> 20 modulation_index
    Patch[0].Instrument[0].Filter.frequency_time = 5; // 0 --> 20

    Patch_id = 0;
    Require_FRAM(Archive.Save_Patch(Patch_id));
    Archive.Copy_Patch_from_RAM_to_SD(Patch_id);

    Serial.println(F("Patch[0] Saved"));

    // Inizializza e salva in FRAM i metadati del Direct Sampler.
    DS_seed_all_Recordings();

    // Definisce e salva il Sound iniziale in FRAM.
    Sound[0].used = true;
    Sound[0].file = 0;
    Sound[0].mode = 0;
    Sound[0].pitch = 0;
    Sound[0].A = 0;
    Sound[0].B = 40000;
    Sound[0].Noclick = 0;
    Sound[0].pan = 0;
    Sound[0].data = 0; // bit4-3-2-1: midi_channel bit0: Attack ramp ("0" Slow, "1" Fast)
    Sound[0].attack = 0;
    Sound[0].decay = 50;
    Sound[0].sustain = 50;
    Sound[0].release = 10;
    Sound[0].gain = 12; // 20 means gain = 1.0
    Serial.println(F("Saving Sound[0]"));
    Sound_id = 0;
    Require_FRAM(Archive.Save_Sound(Sound_id));

    // Salva in FRAM l'ottava del NoteNumber 0.
    Require_FRAM(Archive.Save_first_octave(-2));

    // Default: 12 file voices, pitch up to x16 from cache or x2.8 from Flash.

    // Assegna e salva in FRAM i parametri iniziali del Delay.
    Delay_data.samples = 24;                  // 35 ms.
    Delay_data.samples_LR = 0;                // value L/R ; -10 --> 10
    Delay_data.instrument_route = 0b00000000; // all Instruments are NOT routed to Delay
    Delay_data.modulation_source = 0;         // 0: none 1:LFO(sinus) 2:input_1
    Delay_data.modulation_depth = 30;         // 0 --> 40 modulation depth
    Delay_data.modulation_frequency = 12;     // 0 --> 40 only for waveform
    Delay_data.modulation_phase_LR = 0;       // 0 --> 359 only for waveform
    Delay_data.loop_gain = 65;
    Require_FRAM(Archive.Save_Delay(0, Delay_data));

    // cancella il contenute dei packet sulla Flash aggiuntiva
    VFS_Erase_all_packets();
    Require_FRAM(Archive.Set_FRAM_archive_state(ArchivingManager::ARCHIVE_READY));
}

FLASHMEM
void Print_Patch(int patch_id)
{
    Serial.print("Patch:");
    Serial.print(patch_id);
    if (true)
    {
        Serial.print(" used:");
        Serial.print(Patch[patch_id].used);
        Serial.print(" instruments:");
        Serial.println(Patch[patch_id].instruments);
        for (auto instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
        {
            if (!Patch[patch_id].Instrument[instrument_id].used)
            {
                Serial.print("* ");
                Serial.println(instrument_id);
            }
            else
            {
                Serial.print("* ");
                Serial.print(instrument_id);
                Print_Instrument(patch_id, instrument_id);
                Serial.println();
            }
        }
    }
    Serial.println();
}

FLASHMEM
void Print_Instrument(int patch_id, int instrument_id)
{
    Serial.print(" sound_id:");
    Serial.print(Get_sound_id(patch_id, instrument_id));
    Serial.print(" file:");
    char audio_filename[NAME_FILE_SIZE];
    Serial.print(Get_file_name(Sound[Get_sound_id(patch_id, instrument_id)].file, audio_filename));
    Serial.print(" root_key:");
    Serial.print(Patch[patch_id].Instrument[instrument_id].root_key);
    Serial.print(" from_note:");
    Serial.print(Patch[patch_id].Instrument[instrument_id].from_note);
    Serial.print(" to_note:");
    Serial.print(Patch[patch_id].Instrument[instrument_id].to_note);
    Serial.print(" precedence:");
    Serial.print(Patch[patch_id].Instrument[instrument_id].precedence);
    Serial.print(" lock:");
    Serial.println(Patch[patch_id].Instrument[instrument_id].lock);

    Serial.print("Filter  use:");
    Serial.print(Patch[patch_id].Instrument[instrument_id].Filter.use);
    Serial.print(" type:");
    Serial.print(Patch[patch_id].Instrument[instrument_id].Filter.type);
    Serial.print(" pivot:");
    Serial.print(Patch[patch_id].Instrument[instrument_id].Filter.pivot);
    Serial.print(" resonance:");
    Serial.print(Patch[patch_id].Instrument[instrument_id].Filter.resonance);
    Serial.print(" modulation:");
    Serial.print(Patch[patch_id].Instrument[instrument_id].Filter.modulation);
    Serial.print(" index:");
    Serial.print(Patch[patch_id].Instrument[instrument_id].Filter.index);
    Serial.print(" frequency_time:");
    Serial.println(Patch[patch_id].Instrument[instrument_id].Filter.frequency_time);
}

FLASHMEM
void Print_Sound(int sound_id)
{
    char audio_filename[NAME_FILE_SIZE];
    Serial.println();
    Serial.print("sound_id:");
    Serial.print(sound_id);
    Serial.print(" used:");
    Serial.println((Sound[sound_id].used ? "yes" : "no"));

    Serial.print("file_id:");
    Serial.print(Sound[sound_id].file);
    Serial.print(" file name: ");
    Serial.print(Get_file_name(Sound[sound_id].file, audio_filename));
    Serial.print(" samples:");

    // xxx.raw file (standard files coming from micro SD)
    if (sound_id < SOUNDS_MAX)
    {
        Serial.print(Get_samples_in_raw_file(Sound[sound_id].file));
    }

    // Rxxx.raw file (Recording)
    else if (sound_id >= SOUNDS_MAX)
    {
        if (Sound[sound_id].file >= FIRST_RECORDING_FILE)
        {
            int recording = ((Sound[sound_id].file - FIRST_RECORDING_FILE) / 2);
            Serial.print(DS_get_samples_in_Recording(recording));
        }
        else
        {
            Serial.print(Get_samples_in_raw_file(Sound[sound_id].file));
        }
    }

    Serial.print(" mode:");
    Serial.print(Sound[sound_id].mode);
    Serial.print(" pitch:");
    Serial.print(Sound[sound_id].pitch);
    Serial.print(" A:");
    Serial.print(Sound[sound_id].A);
    Serial.print(" B:");
    Serial.print(Sound[sound_id].B);
    Serial.print(" Noclick:");
    Serial.print(Sound[sound_id].Noclick);
    Serial.print(" Attack:");
    Serial.print(Sound[sound_id].attack);
    Serial.print(" attack_ramp:");
    Serial.print((Sound[sound_id].data & 1) ? "fast" : "slow"); // bit4-3-2-1: midi_channel bit0: Attack ramp ("0" Slow, "1" Fast)
    Serial.print(" Decay:");
    Serial.print(Sound[sound_id].decay);
    Serial.print(" Sustain:");
    Serial.print(Sound[sound_id].sustain);
    Serial.print(" Release:");
    Serial.print(Sound[sound_id].release);
    Serial.print(" volume:");
    Serial.print(Sound[sound_id].gain);
    Serial.print(" MIDI channel:");
    Serial.println(S_Get_midi_channel_from_Sound(sound_id) + 1);
    Serial.println();
}

FLASHMEM
void Print_Lilla_state(void)
{
    Serial.print("Displayed page is: ");
    switch (Lilla_state)
    {
    case 0:
        Serial.println("Performance");
        break;
    case 1:
        Serial.println("Sound edit");
        break;
    case 2:
        Serial.println("Setup");
        break;
    case 3:
        Serial.println("Midi monitor");
        break;
    case 4:
        Serial.println("Control Change Settings");
        break;
    case 5:
        Serial.println("Delay");
        break;
    case 6:
        Serial.println("Instrument Filter");
        break;
    case 7:
        Serial.println("Direct Sampling");
        break;
    case 8:
        Serial.println("Live Sampling");
        break;
    case 9:
        Serial.println("MIDI Loop");
        break;
    default:
        PRINT_ERROR(F("Switch MISSING! "));
        break;
    }
}

FLASHMEM
void Print_keyboard_state(int midi_channel, int from_key, int to_key)
{
    Serial.println();
    Serial.print(F("********  Print_keyboard_state - MIDI CH.: "));
    Serial.print(midi_channel + 1);
    Serial.println("   *********");
    for (auto key = from_key; key <= to_key; ++key)
    {
        Serial.print(key_state[midi_channel][key]);
        Serial.print("  ");
    }
    Serial.println();
    for (auto key = from_key; key <= to_key; ++key)
    {
        Serial.print(key);
        Serial.print("  ");
    }
    Serial.println();
}

FLASHMEM
void Print_map_instrument_for_note(int midi_channel)
{
    Serial.println();
    Serial.print(F("Map notes/Sound for MIDI channel:"));
    Serial.println(midi_channel + 1);
    for (auto Inst_id = (INSTRUMENTS - 1); Inst_id >= 0; --Inst_id)
    {
        for (auto note_number = 0; note_number < NOTE_NUMBERS; ++note_number)
        {
            Serial.print(bitRead(map_instrument_for_note[midi_channel][note_number], Inst_id));
            Serial.print("  ");
        }
        Serial.println();
    }
    Serial.println();

    for (auto note_number = 0; note_number < NOTE_NUMBERS; ++note_number)
    {
        Serial.print(note_number);
        Serial.print("  ");
    }
    Serial.println();
    Serial.println();
}

// ***************************************************************************************************************
// **********************************                LIVE_SAMPLING              **********************************
// ***************************************************************************************************************

// Live capture creates ordinary patch/Sound metadata; RAW storage is deferred until Save.
FLASHMEM void LS_Capture_notice(const char *message, const char *second_line)
{
    Show_popup_text(message, second_line, ILI9341_WHITE, ILI9341_RED, 0);
    const uint32_t started = millis();
    while (static_cast<uint32_t>(millis() - started) < 2000u)
    {
        Shifters_manager.Update();
    }
    Clear_UI_events();
}

FLASHMEM bool LS_Capture_confirm(void)
{
    int choice = 0;
    //                                  "REPLACE CAPTURE?"
    Show_popup_text("REPLACE CAPTURE?", "    NO  YES", ILI9341_WHITE, ILI9341_RED);
    Clear_UI_events();

    Display_LiveSampler.LS_Display_Confirm_capture_frame(0);
    do
    {
        Shifters_manager.Update();
        if (Read_encoder(EN_PB_Select, choice, 1, 0, 1))
        {
            Display_LiveSampler.LS_Display_Confirm_capture_frame(choice);
        }
    } while (!Read_pushbutton(EN_PB_Select));

    Clear_UI_events();
    return choice == 1;
}

FLASHMEM bool LS_Capture_root(uint8_t &root)
{
    LS_refresh_LS_page();
    //                                                "PRESS A KEY TO INSERT ROOT KEY"
    Show_popup_text("PRESS A KEY TO INSERT ROOT KEY", "           CANCEL", ILI9341_WHITE, ILI9341_RED);
    Frame_by_pixels_on_RED(138, 123, 6, true);

    Clear_UI_events();
    AudioNoInterrupts();
    Capture_learn_note = -1;
    Capture_learn_key = true;
    AudioInterrupts();
    int note = -1;
    while (note < 0)
    {
        Shifters_manager.Update();
        if (Read_pushbutton(EN_PB_Select))
        {
            Capture_learn_key = false;
            Clear_UI_events();
            return false;
        }
        AudioNoInterrupts();
        note = Capture_learn_note;
        Capture_learn_note = -1;
        AudioInterrupts();
    }
    Capture_learn_key = false;
    root = note;
    Clear_UI_events();
    return true;
}

FLASHMEM bool LS_Capture_drain(void)
{
    Midi_reader.Stop();

    AudioNoInterrupts();
    Players_Manager.Stop_all_players();
    AudioInterrupts();

    const uint32_t started = millis();
    bool playing;

    do
    {
        playing = false;
        AudioNoInterrupts();
        for (int i = 0; i < PLAYERS; ++i)
        {
            playing |= Player[i].isPlaying();
        }
        AudioInterrupts();
    } while (playing && static_cast<uint32_t>(millis() - started) < 100u);

    if (playing)
    {
        Midi_reader.Start();
    }
    return !playing;
}

FLASHMEM bool LS_Capture_referenced(int file, int except_sound)
{
    for (int id = 0; id < SOUNDS_MAX; ++id)
    {
        if ((id != except_sound && Sound[id].used && Sound[id].file == file) || (S_Sound_cache_P[id].used && S_Sound_cache_P[id].file == file))
        {
            return true;
        }
    }
    return false;
}

FLASHMEM void LS_Capture_collect(void)
{
    // Snapshots protect Discard; retiring players retain their cache until their release ends.
    AudioNoInterrupts();
    for (auto &source : Capture_sources)
    {
        if (source.audio.psram_ptr != nullptr && !LS_Capture_referenced(source.audio.file_id))
        {
            const int file = source.audio.file_id;
            source = {};
            PatchCache_Manager.Invalidate_file(file);
        }
    }
    PatchCache_Manager.Release_unreferenced_caches(Players_Manager.Get_cache_reference_mask());
    AudioInterrupts();
}

FLASHMEM void LS_Capture_sound(int selected)
{
    char audio_filename[NAME_FILE_SIZE];
    AudioNoInterrupts();
    const bool empty = LS_state == EMPTY || (LiveSampler.first_write_flag && LiveSampler.Q_sample < 0);
    AudioInterrupts();

    if (empty)
    {
        Display_LiveSampler.Update_no_recorded_audio(true);
        return;
    }
    if (LS_state == REC || LiveSampler.Is_writing() || LS_state != PLAYONLY || LS_mode < LOOP_FWD)
    {
        LS_Capture_notice("STOP REC AND SELECT LOOP MODE");
        LS_refresh_LS_page();
        return;
    }
    const uint32_t samples = LS_XY_delta + 1;
    if (samples == 0 || samples > PATCH_CACHE_ARRAY_SAMPLES || LS_buffer_dim <= 0)
    {
        LS_Capture_notice("LOOP TOO LONG FOR CACHE");
        LS_refresh_LS_page();
        return;
    }
    const bool creating = Capture_target < 0 || Capture_target != Patch_id_old || !Patch[Capture_target].used;
    const int target = creating ? S_Get_Patch_id_free() : Capture_target;

    if (target < 0 || (creating && !P_Verify_is_Patch_original(Patch_id_old)))
    {
        LS_Capture_notice(target < 0 ? "NO FREE PATCH" : "OPEN PERFORMANCE", target < 0 ? "" : "AND SAVE THE PREVIOUS PATCH");
        LS_refresh_LS_page();
        return;
    }

    int first = selected;
    int second = -1;
    const int partner = creating ? -1 : Capture_pair[selected];

    if (LS_stereo)
    {
        if (partner >= 0 && Patch[target].Instrument[partner].used && Patch[target].Instrument[selected].used)
        {
            first = selected < partner ? selected : partner;
            second = selected < partner ? partner : selected;
        }
        else if (selected + 1 < INSTRUMENTS && (creating || !Patch[target].Instrument[selected + 1].used))
        {
            second = selected + 1;
        }
    }

    if (!creating && Patch[target].Instrument[selected].used && !LS_Capture_confirm())
    {
        LS_refresh_LS_page();
        return;
    }

    uint8_t root = 60;

    if (!LS_Capture_root(root) || !LS_Capture_drain())
    {
        LS_refresh_LS_page();
        return;
    }

    LS_Capture_collect();
    const int channels = second < 0 ? 1 : 2;
    int sounds[2] = {-1, -1};
    int slots[2] = {-1, -1};
    int files[2] = {-1, -1};
    int16_t *destinations[2] = {};

    for (int channel = 0; channel < channels; ++channel)
    {
        const int instrument = channel == 0 ? first : second;

        if (!creating && Patch[target].Instrument[instrument].used)
        {
            sounds[channel] = Patch[target].Instrument[instrument].sound_id;
        }
        else
        {
            for (int id = 0; id < SOUNDS_MAX; ++id)
            {
                if (!Sound[id].used && id != sounds[0])
                {
                    sounds[channel] = id;
                    break;
                }
            }
        }

        for (int slot = 0; slot < CAPTURE_SOURCES; ++slot)
        {
            const auto &source = Capture_sources[slot];
            const bool reusable = !source.written && !LS_Capture_referenced(source.audio.file_id, sounds[channel]);
            if (slot != slots[0] && (source.audio.psram_ptr == nullptr || reusable))
            {
                slots[channel] = slot;
                break;
            }
        }

        if (slots[channel] >= 0 && Capture_sources[slots[channel]].audio.psram_ptr != nullptr)
        {
            files[channel] = Capture_sources[slots[channel]].audio.file_id;
        }
        else
        {
            for (int file = 1; file < FIRST_RECORDING_FILE; ++file)
            {
                if (file != files[0] && FileNameRegistry::Numeric_available(file) && Capture_find(file) == nullptr && !SerialFlash.exists(Get_file_name(file, audio_filename)))
                {
                    files[channel] = file;
                    break;
                }
            }
        }

        if (sounds[channel] < 0 || slots[channel] < 0 || files[channel] < 0)
        {
            Midi_reader.Start();
            LS_Capture_notice("NO FREE SOUND / CACHE / FILE");
            LS_refresh_LS_page();
            return;
        }
    }

    for (int channel = 0; channel < channels; ++channel)
    {
        destinations[channel] = PatchCache_Manager.Reserve_capture(slots[channel], files[channel], samples);

        if (destinations[channel] == nullptr)
        {
            Midi_reader.Start();
            LS_Capture_notice("CAPTURE CACHE UNAVAILABLE");
            LS_refresh_LS_page();
            return;
        }
    }

    if (creating)
    {
        Capture_return_patch = Patch_id_old;
        Capture_target = target;
        Capture_new_patch = target;
        Capture_patch_delay = Delay_data;
        Patch_cache_P = Patch[target];
        S_Copy_all_Sound_to_Sound_cache_P();
        Patch[target] = {};
        for (auto &pair : Capture_pair)
        {
            pair = -1;
        }
    }

    int position = LS_X_sample % LS_buffer_dim;
    if (position < 0)
    {
        position += LS_buffer_dim;
    }

    for (int channel = 0; channel < channels; ++channel)
    {
        const int instrument_id = channel == 0 ? first : second;
        int16_t *destination = destinations[channel];

        for (uint32_t i = 0; i < samples; ++i)
        {
            const int index = (position + i) % LS_buffer_dim;
            destination[i] = !LS_stereo ? LS_buffer_mono_ptr[index] : (second < 0 ? static_cast<int16_t>((static_cast<int32_t>(LS_buffer_L_ptr[index]) + LS_buffer_R_ptr[index]) / 2) : (channel == 0 ? LS_buffer_L_ptr[index] : LS_buffer_R_ptr[index]));
        }

        Capture_sources[slots[channel]] = {{static_cast<int16_t>(files[channel]), Psram, destination, samples, static_cast<int8_t>(slots[channel])}, false};
        auto &instrument = Patch[target].Instrument[instrument_id];
        instrument = {};
        instrument.used = true;
        instrument.sound_id = sounds[channel];
        instrument.from_note = instrument.root_key = instrument.to_note = root;
        instrument.Filter = Patch[PATCHES_MAX].Instrument[0].Filter;
        auto &sound = Sound[sounds[channel]];
        sound = Sound[SOUNDS_MAX + channel];
        sound.used = true;
        sound.file = files[channel];
        sound.A = 0;
        sound.B = samples - 1;
        sound.Noclick = LS_mode == LOOP_FWD ? 128 : 0;
        sound.pan = second < 0 ? 0 : (channel == 0 ? -16 : 16);
        const int previous_partner = Capture_pair[instrument_id];

        if (previous_partner >= 0)
        {
            Capture_pair[previous_partner] = -1;
        }

        Capture_pair[instrument_id] = -1;
    }

    if (second >= 0)
    {
        Capture_pair[first] = second;
        Capture_pair[second] = first;
    }

    Patch[target].used = true;
    Patch[target].instruments = 0;

    for (const auto &instrument : Patch[target].Instrument)
    {
        Patch[target].instruments += instrument.used;
    }

    Patch_id_old = target;
    P_Update_Patches_number();
    Midi_reader.Start();

    LS_refresh_LS_page();
}

FLASHMEM bool LS_Capture_write(CaptureSource &source)
{
    char audio_filename[NAME_FILE_SIZE];
    if (source.written)
    {
        return true; // A retry after another channel failed must not allocate again.
    }

    if (!FileNameRegistry::Bind_numeric(source.audio.file_id) || !FileNameRegistry::Save())
    {
        return false;
    }
    const char *name = Get_file_name(source.audio.file_id, audio_filename);
    const uint32_t bytes = source.audio.samples * sizeof(int16_t);

    if (SerialFlash.exists(name) || !SerialFlash.create(name, bytes))
    {
        return false;
    }

    SerialFlashFile file = SerialFlash.open(name);
    bool complete = static_cast<bool>(file);
    const auto *data = reinterpret_cast<const uint8_t *>(source.audio.psram_ptr);

    for (uint32_t offset = 0; complete && offset < bytes; offset += 256u)
    {
        const uint32_t count = bytes - offset < 256u ? bytes - offset : 256u;
        complete = file.write(data + offset, count) == count;
        Shifters_manager.Update();
    }

    SerialFlash.wait();
    file.seek(0);
    uint8_t verify[256];

    for (uint32_t offset = 0; complete && offset < bytes; offset += sizeof(verify))
    {
        const uint32_t count = bytes - offset < sizeof(verify) ? bytes - offset : sizeof(verify);
        complete = file.read(verify, count) == count && memcmp(verify, data + offset, count) == 0;
    }

    file.close();

    if (!complete)
    {
        SerialFlash.remove(name); // Hide incomplete data; SerialFlash cannot reclaim its allocation.
        return false;
    }

    source.written = true;
    return true;
}

FLASHMEM bool LS_Capture_materialize(void)
{
    bool needed = false;

    for (int id = 0; id < SOUNDS_MAX; ++id)
    {
        needed |= Sound[id].used && Capture_pending(Sound[id].file);
    }

    if (!needed)
    {
        return true;
    }

    if (!LS_Capture_drain())
    {
        LS_Capture_notice("SAVE BUSY - TRY AGAIN");
        return false;
    }

    Show_popup_text("SAVING RAW FILES...", ILI9341_WHITE, ILI9341_RED, 0);
    const uint32_t popup_started = millis();

    bool complete = true;
    for (int id = 0; complete && id < SOUNDS_MAX; ++id)
    {
        auto *source = Sound[id].used ? Capture_find(Sound[id].file) : nullptr;
        if (source != nullptr)
        {
            complete = LS_Capture_write(*source);
        }
    }

    File_scanner.Read_all_file_data();

    while (static_cast<uint32_t>(millis() - popup_started) < 2000u)
    {
        Shifters_manager.Update();
    }

    Midi_reader.Start();

    Clear_UI_events();
    if (!complete)
    {
        LS_Capture_notice("RAW SAVE FAILED - RETRY");
    }
    return complete;
}

FLASHMEM void LS_Capture_finish_save(void)
{
    AudioNoInterrupts();
    for (auto &source : Capture_sources)
    {
        if (source.written)
        {
            source = {}; // Its ordinary cache remains valid and may now be reclaimed normally.
        }
    }
    PatchCache_Manager.Set_required_files(Preset);
    AudioInterrupts();
}

void LS_refresh_LS_page(void)
{
    // Discard notices from the previous page before entering Live Sampler.

    AudioNoInterrupts();
    AudioPlayer::Take_live_unrecorded_notice();
    AudioInterrupts();

    Lilla_state = LIVE_SAMPLING;

    Display_LiveSampler.Page();

    // restore LEDs
    Performance_led_set.Restore_all_LED();

    LS_update_menu_elements();
    Display_LiveSampler.Menu();

    if (!LS_XY_lock)
    {
        LS_update_both_X_Y_samples();
    }
    else // altrimenti e' gia' stato calcolato
    {
        LS_update_Q_sample(); // Usato da LS_wave_color
    }
    Display_LiveSampler.Show_wave(LS_sound_id);

    Clear_UI_events();
}

bool LS_ask_if_exit_from_LS(void)
{
    confirmation = false;
    int action = 0;
    Display_LiveSampler.Confirm_EXIT_from_LS();
    Display_Common.Confirm_no_yes_popup_frame(0);
    delay(200);

    Clear_UI_events();
    while (!confirmation)
    {
        Shifters_manager.Update();

        if (Read_encoder(EN_PB_Select, action, 1, 0, 1))
        {
            Display_Common.Confirm_no_yes_popup_frame(action);
        }
        if (Read_pushbutton(EN_PB_Select))
        {
            confirmation = true;
        }
    }
    Clear_UI_events();

    return (action == 0 ? false : true);
}

void LS_update_menu_elements(void)
{
    // voices that can be displayed
    Menu_LS[0] = true; // Open
    Menu_LS[1] = true; // Close
    Menu_LS[2] = true; // Mono/Stereo
    Menu_LS[3] = true; // Erase FIFO

    if (LS_state == EMPTY) // nessuna registrazione
    {
        Menu_LS[1] = false; // Close
        Menu_LS[3] = false; // Erase FIFO
    }

    if (LS_state == REC) // fase di registrazione
    {
        Menu_LS[0] = false; // Open
        Menu_LS[2] = false; // Mono/Stereo
        Menu_LS[3] = false; // Erase FIFO
    }

    if (LS_state == PLAYONLY) // registrazione disponibile
    {
        Menu_LS[1] = false; // Close
        Menu_LS[2] = false; // Mono/Stereo
    }

    LS_menu_max = Menu_LS[0] + Menu_LS[1] + Menu_LS[2] + Menu_LS[3] - 1;
}

void LS_lock_X_sample(void)
{
    AudioNoInterrupts();
    LS_Q_sample = LiveSampler.Q_sample;
    LS_X_sample = LS_constrain_position(LS_Q_sample + LS_X_delta);
    LS_Y_sample = LS_X_sample + LS_XY_delta;
    LS_XY_lock = true;
    AudioInterrupts();

    // Serial.print(F("LS_Q_sample: "));
    // Serial.println(LS_Q_sample);
    // Serial.print(F("LS_X_sample: "));
    // Serial.println(LS_X_sample);
}

void LS_update_both_X_Y_samples(void)
{
    AudioNoInterrupts();
    LS_Q_sample = LiveSampler.Q_sample;
    LS_X_sample = LS_constrain_position(LS_Q_sample + LS_X_delta);
    LS_Y_sample = LS_X_sample + LS_XY_delta;
    AudioInterrupts();

    // Serial.print(F("LS_Q_sample: "));
    // Serial.println(LS_Q_sample);
    // Serial.print(F("LS_X_sample: "));
    // Serial.println(LS_X_sample);
}

void LS_update_Q_sample(void)
{
    AudioNoInterrupts();
    LS_Q_sample = LiveSampler.Q_sample;
    AudioInterrupts();
}

FLASHMEM
void LS_Reset_buffer(void)
{
    memset(LS_buffer_storage, 0, sizeof(LS_buffer_storage));

    LiveSampler.LS_buffer_mono_ptr = LS_buffer_mono_ptr;
    LiveSampler.LS_buffer_L_ptr = LS_buffer_L_ptr;
    LiveSampler.LS_buffer_R_ptr = LS_buffer_R_ptr;

    Info.LS_buffer_mono_ptr = LS_buffer_mono_ptr;
    Info.LS_buffer_L_ptr = LS_buffer_L_ptr;
    Info.LS_buffer_R_ptr = LS_buffer_R_ptr;

    Players_Manager.Broadcast_FIFO_mono(LS_buffer_mono_ptr);
    Players_Manager.Broadcast_FIFO_stereo(LS_buffer_L_ptr, LS_buffer_R_ptr);

    LiveSampler.Reset();
    LS_Q_sample = -1;
}

FLASHMEM
void LS_setup_LS_Patch(bool stereo)
{
    // set Sampler Patch and Sounds
    Patch[PATCHES_MAX].used = true;
    if (stereo)
    {
        // Patch
        Patch[PATCHES_MAX].instruments = 2;

        // Left channel recording
        Patch[PATCHES_MAX].Instrument[0].used = true;
        Patch[PATCHES_MAX].Instrument[0].sound_id = SOUNDS_MAX;
        Patch[PATCHES_MAX].Instrument[0].root_key = 60;
        Patch[PATCHES_MAX].Instrument[0].from_note = 0;
        Patch[PATCHES_MAX].Instrument[0].to_note = 127;
        Patch[PATCHES_MAX].Instrument[0].precedence = false;
        Patch[PATCHES_MAX].Instrument[0].lock = false;

        Patch[PATCHES_MAX].Instrument[0].Filter.use = false;        // yes/no
        Patch[PATCHES_MAX].Instrument[0].Filter.type = 0;           // lowpass, highpass, bandpass, notch
        Patch[PATCHES_MAX].Instrument[0].Filter.pivot = 10;         // 0 --> 30 filter frequency/note frequency
        Patch[PATCHES_MAX].Instrument[0].Filter.resonance = 7;      // 0 --> 40
        Patch[PATCHES_MAX].Instrument[0].Filter.modulation = 3;     // 0 --> 4 bit 1,2,3:modulation
        Patch[PATCHES_MAX].Instrument[0].Filter.index = 10;         // 1 --> 20 modulation_index
        Patch[PATCHES_MAX].Instrument[0].Filter.frequency_time = 5; // 0 --> 20

        // Right channel recording
        Patch[PATCHES_MAX].Instrument[1].used = true;
        Patch[PATCHES_MAX].Instrument[1].sound_id = SOUNDS_MAX + 1;
        Patch[PATCHES_MAX].Instrument[1].root_key = 60;
        Patch[PATCHES_MAX].Instrument[1].from_note = 0;
        Patch[PATCHES_MAX].Instrument[1].to_note = 127;
        Patch[PATCHES_MAX].Instrument[1].precedence = false;
        Patch[PATCHES_MAX].Instrument[1].lock = false;

        Patch[PATCHES_MAX].Instrument[1].Filter.use = false;        // yes/no
        Patch[PATCHES_MAX].Instrument[1].Filter.type = 0;           // lowpass, highpass, bandpass, notch
        Patch[PATCHES_MAX].Instrument[1].Filter.pivot = 10;         // 0 --> 30 filter frequency/note frequency
        Patch[PATCHES_MAX].Instrument[1].Filter.resonance = 7;      // 0 --> 40
        Patch[PATCHES_MAX].Instrument[1].Filter.modulation = 3;     // 0 --> 4 bit 1,2,3:modulation
        Patch[PATCHES_MAX].Instrument[1].Filter.index = 10;         // 1 --> 20 modulation_index
        Patch[PATCHES_MAX].Instrument[1].Filter.frequency_time = 5; // 0 --> 20

        // Other instruments
        Patch[PATCHES_MAX].Instrument[2].used = false;
        Patch[PATCHES_MAX].Instrument[3].used = false;
        Patch[PATCHES_MAX].Instrument[4].used = false;
        Patch[PATCHES_MAX].Instrument[5].used = false;
        Patch[PATCHES_MAX].Instrument[6].used = false;
        Patch[PATCHES_MAX].Instrument[7].used = false;

        // Left Sound
        Sound[SOUNDS_MAX].used = true;
        Sound[SOUNDS_MAX].mode = LS_mode;
        Sound[SOUNDS_MAX].file = FIRST_LIVE_SAMPLING_FILE + 1;
        Sound[SOUNDS_MAX].pitch = 0;                           // -128 + 127
        Sound[SOUNDS_MAX].A = LS_X_sample;                     // non utilizzato;
        Sound[SOUNDS_MAX].B = LS_X_sample + LS_buffer_dim - 1; // non utilizzato;
        Sound[SOUNDS_MAX].Noclick = 0;
        Sound[SOUNDS_MAX].pan = -16; // full Left
        Sound[SOUNDS_MAX].data = 0;  // bit4-3-2-1: midi_channel bit0: Attack ramp ("0" Slow, "1" Fast)
        Sound[SOUNDS_MAX].attack = 0;
        Sound[SOUNDS_MAX].decay = 50;
        Sound[SOUNDS_MAX].sustain = 50;
        Sound[SOUNDS_MAX].release = 10;
        Sound[SOUNDS_MAX].gain = 20;

        // Right Sound
        Sound[SOUNDS_MAX + 1].used = true;
        Sound[SOUNDS_MAX + 1].mode = LS_mode;
        Sound[SOUNDS_MAX + 1].file = FIRST_LIVE_SAMPLING_FILE + 2;
        Sound[SOUNDS_MAX + 1].pitch = 0;                           // -128 + 127
        Sound[SOUNDS_MAX + 1].A = LS_X_sample;                     // non utilizzato;
        Sound[SOUNDS_MAX + 1].B = LS_X_sample + LS_buffer_dim - 1; // non utilizzato;
        Sound[SOUNDS_MAX + 1].Noclick = 0;
        Sound[SOUNDS_MAX + 1].pan = 16; // full Right
        Sound[SOUNDS_MAX + 1].data = 0; // bit4-3-2-1: midi_channel bit0: Attack ramp ("0" Slow, "1" Fast)
        Sound[SOUNDS_MAX + 1].attack = 0;
        Sound[SOUNDS_MAX + 1].decay = 50;
        Sound[SOUNDS_MAX + 1].sustain = 50;
        Sound[SOUNDS_MAX + 1].release = 10;
        Sound[SOUNDS_MAX + 1].gain = 20;
    }

    else // mono
    {
        // Patch
        Patch[PATCHES_MAX].instruments = 1;
        Patch[PATCHES_MAX].Instrument[0].used = true;
        Patch[PATCHES_MAX].Instrument[0].sound_id = SOUNDS_MAX;
        Patch[PATCHES_MAX].Instrument[0].root_key = 60;
        Patch[PATCHES_MAX].Instrument[0].from_note = 0;
        Patch[PATCHES_MAX].Instrument[0].to_note = 127;
        Patch[PATCHES_MAX].Instrument[0].precedence = false;
        Patch[PATCHES_MAX].Instrument[0].lock = false;

        Patch[PATCHES_MAX].Instrument[0].Filter.use = false;        // yes/no
        Patch[PATCHES_MAX].Instrument[0].Filter.type = 0;           // lowpass, highpass, bandpass, notch
        Patch[PATCHES_MAX].Instrument[0].Filter.pivot = 10;         // 0 --> 30 filter frequency/note frequency
        Patch[PATCHES_MAX].Instrument[0].Filter.resonance = 7;      // 0 --> 40
        Patch[PATCHES_MAX].Instrument[0].Filter.modulation = 3;     // 0 --> 4 bit 1,2,3:modulation
        Patch[PATCHES_MAX].Instrument[0].Filter.index = 20;         // 1 --> 20 modulation_index
        Patch[PATCHES_MAX].Instrument[0].Filter.frequency_time = 5; // 0 --> 20

        Patch[PATCHES_MAX].Instrument[1].used = false;
        Patch[PATCHES_MAX].Instrument[2].used = false;
        Patch[PATCHES_MAX].Instrument[3].used = false;
        Patch[PATCHES_MAX].Instrument[4].used = false;
        Patch[PATCHES_MAX].Instrument[5].used = false;
        Patch[PATCHES_MAX].Instrument[6].used = false;
        Patch[PATCHES_MAX].Instrument[7].used = false;

        // Mono Sound
        Sound[SOUNDS_MAX].used = true;
        Sound[SOUNDS_MAX].mode = LS_mode;
        Sound[SOUNDS_MAX].file = FIRST_LIVE_SAMPLING_FILE;
        Sound[SOUNDS_MAX].pitch = 0;                           // -128 + 127
        Sound[SOUNDS_MAX].A = LS_X_sample;                     // non utilizzato;
        Sound[SOUNDS_MAX].B = LS_X_sample + LS_buffer_dim - 1; // non utilizzato;
        Sound[SOUNDS_MAX].Noclick = 0;
        Sound[SOUNDS_MAX].pan = 0;  // full Left
        Sound[SOUNDS_MAX].data = 0; // bit4-3-2-1: midi_channel bit0 (ch.1 in questo caso) : Attack ramp ("0" Slow, "1" Fast)
        Sound[SOUNDS_MAX].attack = 0;
        Sound[SOUNDS_MAX].decay = 50;
        Sound[SOUNDS_MAX].sustain = 50;
        Sound[SOUNDS_MAX].release = 10;
        Sound[SOUNDS_MAX].gain = 28;

        Sound[SOUNDS_MAX + 1].used = false;
    }
}

// ***************************************************************************************************************
// **********************************                   MIXER                   **********************************
// ***************************************************************************************************************

void Switch_to_MIXER()
{
    // Se non si sta editando, si parte dal primo Instrument esistente
    if (Lilla_state_0 != PERFORMANCE && Lilla_state_0 != SOUND_EDIT && Lilla_state_0 != INSTRUMENT_VCF && Lilla_state_0 != LIVE_SAMPLING)
    {
        for (auto instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
        {
            if (Patch[Patch_id].Instrument[instrument_id].used)
            {
                Instrument_id = instrument_id;
                break;
            }
        }
        Sound_id = Get_sound_id(Patch_id, Instrument_id);
    }

    else if (Lilla_state == LIVE_SAMPLING)
    {
        Instrument_id = LS_instrument;
        Sound_id = Get_sound_id(Patch_id, Instrument_id);
    }

    Golive_MIXER();
}

void Golive_MIXER(void)
{
    Lilla_state = MIXER;

    Display_Mixer.MX_page();

    Clear_UI_events();

    for (auto source = 0; source < MX_sources; ++source)
    {
        Display_Mixer.MX_source_values(source, (source == 0 ? true : false));
    }

    Pointer_Mixer.Set_pointer_to_source(0);
    MX_local_pointer = Pointer_Mixer.Get_pointer();

    Instrument_id = 0;
    Sound_id = Get_sound_id(Patch_id, Instrument_id);
}

// ***************************************************************************************************************
// ****************************                         SETTINGS                        **************************
// ***************************************************************************************************************

byte CC_Save_settings(void)
{
    Midi_reader.Stop();
    const byte result = Archive.Save_CC_settings(CC_Sound_gain, CC_lowpass_filter_value);

    Players_Manager.Stop_all_players();
    Midi_reader.Start();
    return result;
}

void SET_Ask_if_IMPORT_EXPORT_setup(void)
{
    confirmation = false;
    result = 0;

    Clear_UI_events();
    while (!confirmation)
    {
        Shifters_manager.Update();

        if (Read_encoder(EN_PB_Select, result, 1, 0, 1))
        {
            Display_Storage.Confirm_config_import_frame(result);
        }
        if (Read_pushbutton(EN_PB_Select))
        {
            confirmation = true;
        }
    }

    Clear_UI_events();
}

void SET_Ask_if_FACTORY_RESET(void)
{
    confirmation = false;
    result = 0;

    Clear_UI_events();
    while (!confirmation)
    {
        Shifters_manager.Update();

        if (Read_encoder(EN_PB_Select, result, 1, 0, 1))
        {
            Display_Storage.Confirm_config_import_frame(result);
        }
        if (Read_pushbutton(EN_PB_Select))
        {
            confirmation = true;
        }
    }
    Clear_UI_events();
}

// ***************************************************************************************************************
// ****************************                 CONTROL CHANGE ASSIGNENT                **************************
// ***************************************************************************************************************

byte CC_Read_all_Sound_gain()
{
    return Archive.Read_CC_settings(CC_Sound_gain, CC_lowpass_filter_value);
}

// ***************************************************************************************************************
// ****************************   COPY RAW FILES FROM SD/LILLA_AUDIO TO FLASH MEMORY CHIP  **************************
// ***************************************************************************************************************

static constexpr uint32_t SET_AUDIO_MAX_RAW_BYTES = 3U * 1024U * 1024U;

enum class SET_Audio_format : uint8_t
{
    Raw,
    Wav,
    Aiff,
    Mp3
};

FLASHMEM
static bool SET_WAV_raw_data(File &file, uint32_t &data_offset, uint32_t &data_length, uint16_t &channels)
{
    const auto read16 = [&file](uint16_t &value) -> bool
    {
        uint8_t bytes[2];
        if (file.read(bytes, sizeof(bytes)) != sizeof(bytes))
        {
            return false;
        }
        value = static_cast<uint16_t>(bytes[0]) | static_cast<uint16_t>(bytes[1]) << 8;
        return true;
    };
    const auto read32 = [&file](uint32_t &value) -> bool
    {
        uint8_t bytes[4];
        if (file.read(bytes, sizeof(bytes)) != sizeof(bytes))
        {
            return false;
        }
        value = static_cast<uint32_t>(bytes[0]) | static_cast<uint32_t>(bytes[1]) << 8 | static_cast<uint32_t>(bytes[2]) << 16 | static_cast<uint32_t>(bytes[3]) << 24;
        return true;
    };
    char id[4];
    uint32_t riff_length;
    if (!file.seek(0) || file.read(id, sizeof(id)) != sizeof(id) || memcmp(id, "RIFF", 4) != 0 || !read32(riff_length) || file.read(id, sizeof(id)) != sizeof(id) || memcmp(id, "WAVE", 4) != 0)
    {
        return false;
    }
    const uint64_t file_length = file.size();
    if (riff_length < 4 || riff_length > UINT32_MAX - 8 || static_cast<uint64_t>(riff_length) + 8 > file_length)
    {
        return false;
    }
    const uint32_t riff_end = riff_length + 8;
    bool format_found = false;
    bool data_found = false;
    uint32_t position = 12;
    while (position <= riff_end && riff_end - position >= 8)
    {
        uint32_t chunk_length;
        if (!file.seek(position) || file.read(id, sizeof(id)) != sizeof(id) || !read32(chunk_length))
        {
            return false;
        }
        const uint32_t chunk_start = position + 8;
        if (chunk_length > riff_end - chunk_start || (chunk_length & 1U) > riff_end - chunk_start - chunk_length)
        {
            return false;
        }
        if (memcmp(id, "fmt ", 4) == 0)
        {
            uint16_t encoding;
            uint32_t sample_rate;
            uint32_t byte_rate;
            uint16_t block_align;
            uint16_t bits;
            if (format_found || chunk_length < 16 || !read16(encoding) || !read16(channels) || !read32(sample_rate) || !read32(byte_rate) || !read16(block_align) || !read16(bits) || encoding != 1 || (channels != 1 && channels != 2) || sample_rate != 44100 || byte_rate != 88200U * channels || block_align != 2U * channels || bits != 16)
            {
                return false;
            }
            format_found = true;
        }
        else if (memcmp(id, "data", 4) == 0)
        {
            if (data_found || chunk_length < 2 || (chunk_length & 1U) != 0)
            {
                return false;
            }
            data_offset = chunk_start;
            data_length = chunk_length;
            data_found = true;
        }
        position = chunk_start + chunk_length + (chunk_length & 1U);
    }
    if (!format_found || !data_found || data_length % (sizeof(int16_t) * channels) != 0)
    {
        return false;
    }
    // Report the mono output size for Flash allocation and the import summary.
    data_length /= channels;
    return true;
}

FLASHMEM
static bool SET_AIFF_raw_data(File &file, uint32_t &data_offset, uint32_t &data_length, uint16_t &channels)
{
    const auto read16 = [&file](uint16_t &value) -> bool
    {
        uint8_t bytes[2];
        if (file.read(bytes, sizeof(bytes)) != sizeof(bytes))
        {
            return false;
        }
        value = static_cast<uint16_t>(bytes[0]) << 8 | static_cast<uint16_t>(bytes[1]);
        return true;
    };
    const auto read32 = [&file](uint32_t &value) -> bool
    {
        uint8_t bytes[4];
        if (file.read(bytes, sizeof(bytes)) != sizeof(bytes))
        {
            return false;
        }
        value = static_cast<uint32_t>(bytes[0]) << 24 | static_cast<uint32_t>(bytes[1]) << 16 | static_cast<uint32_t>(bytes[2]) << 8 | static_cast<uint32_t>(bytes[3]);
        return true;
    };
    char id[4];
    uint32_t form_length;
    if (!file.seek(0) || file.read(id, sizeof(id)) != sizeof(id) || memcmp(id, "FORM", 4) != 0 || !read32(form_length) || file.read(id, sizeof(id)) != sizeof(id) || memcmp(id, "AIFF", 4) != 0)
    {
        return false;
    }
    if (form_length < 4 || form_length > UINT32_MAX - 8 || static_cast<uint64_t>(form_length) + 8 > file.size())
    {
        return false;
    }
    const uint32_t form_end = form_length + 8;
    bool format_found = false;
    bool data_found = false;
    uint32_t frames = 0;
    uint32_t position = 12;
    while (position <= form_end && form_end - position >= 8)
    {
        uint32_t chunk_length;
        if (!file.seek(position) || file.read(id, sizeof(id)) != sizeof(id) || !read32(chunk_length))
        {
            return false;
        }
        const uint32_t chunk_start = position + 8;
        if (chunk_length > form_end - chunk_start || (chunk_length & 1U) > form_end - chunk_start - chunk_length)
        {
            return false;
        }
        if (memcmp(id, "COMM", 4) == 0)
        {
            uint16_t bits;
            uint8_t sample_rate[10];
            // Canonical 80-bit extended representation of 44100 Hz.
            static constexpr uint8_t rate_44100[10] = {0x40, 0x0E, 0xAC, 0x44, 0, 0, 0, 0, 0, 0};
            if (format_found || chunk_length != 18 || !read16(channels) || !read32(frames) || !read16(bits) || file.read(sample_rate, sizeof(sample_rate)) != sizeof(sample_rate) || (channels != 1 && channels != 2) || bits != 16 || frames == 0 || memcmp(sample_rate, rate_44100, sizeof(sample_rate)) != 0)
            {
                return false;
            }
            format_found = true;
        }
        else if (memcmp(id, "SSND", 4) == 0)
        {
            uint32_t offset;
            uint32_t block_size;
            if (data_found || chunk_length < 8 || !read32(offset) || !read32(block_size) || offset > chunk_length - 8)
            {
                return false;
            }
            // Alignment bytes precede the first frame; trailing block padding is not audio.
            data_offset = chunk_start + 8 + offset;
            data_length = chunk_length - 8 - offset;
            data_found = true;
        }
        position = chunk_start + chunk_length + (chunk_length & 1U);
    }
    if (position != form_end || !format_found || !data_found || static_cast<uint64_t>(frames) * channels * sizeof(int16_t) > data_length)
    {
        return false;
    }
    data_length = frames * sizeof(int16_t);
    return true;
}

FLASHMEM
static bool SET_Audio_raw_data(File &file, SET_Audio_format format, uint32_t &data_offset, uint32_t &data_length, uint16_t &channels)
{
    data_offset = 0;
    const uint64_t file_length = file.size();
    data_length = file_length < SET_AUDIO_MAX_RAW_BYTES ? static_cast<uint32_t>(file_length) : SET_AUDIO_MAX_RAW_BYTES;
    channels = 1;
    if (format == SET_Audio_format::Mp3)
    {
        Mp3Import mp3;
        return mp3.Open(file, data_length, channels, SET_AUDIO_MAX_RAW_BYTES);
    }
    if (format == SET_Audio_format::Wav && !SET_WAV_raw_data(file, data_offset, data_length, channels))
    {
        return false;
    }
    if (format == SET_Audio_format::Aiff && !SET_AIFF_raw_data(file, data_offset, data_length, channels))
    {
        return false;
    }
    // Apply the same mono output limit to the summary, Flash allocation and copy loop.
    if (data_length > SET_AUDIO_MAX_RAW_BYTES)
    {
        data_length = SET_AUDIO_MAX_RAW_BYTES;
    }
    return true;
}

FLASHMEM
bool SET_Copy_audio_files_from_SD_to_Flash(bool &flash_changed)
{
    LS_Capture_collect();
    for (const auto &source : Capture_sources)
    {
        if (source.audio.psram_ptr != nullptr)
        {
            LS_Capture_notice("SAVE OR DISCARD CAPTURES FIRST");
            flash_changed = false;
            return false;
        }
    }
    flash_changed = false;
    int row;

    Display_Storage.Copy_audio_files_SD_to_Flash_chip_titolo();

    // Wait for SD card
    while (!SD.begin(BUILTIN_SDCARD))
    {
        Display_Storage.Copy_raw_files_SD_to_Flash_chip_waiting_for_SD();
        delay(10000);
        return false;
    }
    Delete_text_row(3);

    // Check if LILLA_AUDIO directory exists
    if (!SD.exists("/LILLA_AUDIO"))
    {
        Display_Storage.Copy_raw_files_SD_to_Flash_chip_lilla_audio_missing();
        delay(4000);
        return false;
    }

    // Preserve the basename and normalize the audio extension for Flash.
    const auto raw_filename = [](const char *source, char *destination, size_t capacity, SET_Audio_format &format) -> bool
    {
        const char *extension = strrchr(source, '.');
        if (extension == nullptr || extension == source)
        {
            return false;
        }
        const size_t basename_length = extension - source;
        if (basename_length + 5 > capacity)
        {
            return false;
        }
        if (strcasecmp(extension, ".raw") == 0)
        {
            format = SET_Audio_format::Raw;
        }
        else if (strcasecmp(extension, ".wav") == 0)
        {
            format = SET_Audio_format::Wav;
        }
        else if (strcasecmp(extension, ".aif") == 0 || strcasecmp(extension, ".aiff") == 0)
        {
            format = SET_Audio_format::Aiff;
        }
        else if (strcasecmp(extension, ".mp3") == 0)
        {
            format = SET_Audio_format::Mp3;
        }
        else
        {
            return false;
        }
        memcpy(destination, source, basename_length);
        memcpy(destination + basename_length, ".raw", 5);
        return true;
    };

    // SD card info
    unsigned long SD_raw_volume = 0;
    int SD_raw_files = 0;
    bool SD_has_zero_raw = false;
    File rootdir = SD.open("/LILLA_AUDIO");
    if (!rootdir || !rootdir.isDirectory())
    {
        rootdir.close();
        Display_Storage.Copy_raw_files_SD_to_Flash_chip_lilla_audio_missing();
        delay(4000);
        return false;
    }
    while (1)
    {
        // open a file from the SD card
        File f = rootdir.openNextFile();
        if (!f)
        {
            break;
        }

        char filename[256];
        SET_Audio_format format;
        if (!f.isDirectory() && raw_filename(f.name(), filename, sizeof(filename), format))
        {
            uint32_t offset = 0;
            uint32_t length = f.size();
            uint16_t channels = 1;
            if (SET_Audio_raw_data(f, format, offset, length, channels))
            {
                SD_raw_volume += length;
                ++SD_raw_files;
                if (strcmp(filename, "0.raw") == 0 && length >= sizeof(int16_t) && length % sizeof(int16_t) == 0)
                {
                    SD_has_zero_raw = true;
                }
            }
        }
        f.close();
    }
    rootdir.close();

    Display_Storage.Copy_raw_files_SD_to_Flash_chip_files_report(SD_raw_volume, SD_raw_files, Get_raw_files_volume(), Get_raw_files());

    unsigned char id[3];
    SerialFlash.readID(id);
    Serial.println();
    Serial.printf("Flash chip Identity: %02X %02X %02X\n", id[0], id[1], id[2]);

    /*
    Chip      Uniform Sector Erase
              20/21   52    D8/DC
              -----   --    -----
    W25Q64CV      4   32    64
    W25Q128FV     4   32    64
    S25FL127S               64
    N25Q512A      4         64
    N25Q00AA      4         64
    S25FL512S               256
    SST26VF032    4
    AT25SF128A   32         64
    */

    //                      size  sector         busy pgm/erase chip
    // Part                 Mbyte kbyte ID bytes  cmd suspend   erase
    // ----                 ----  ----- --------  --- -------   -----
    // Winbond W25Q64CV     8     64    EF 40 17
    // Winbond W25Q128FV    16    64    EF 40 18  05  single    60 & C7
    // Winbond W25Q256FV    32    64    EF 40 19
    // Winbond W25Q512FV    64    64    EF 40 20
    // Spansion S25FL064A   8 ?         01 02 16
    // Spansion S25FL127S   16    64    01 20 18  05
    // Spansion S25FL128P   16    64    01 20 18
    // Spansion S25FL256S   32    64    01 02 19  05            60 & C7
    // Spansion S25FL512S   64   256    01 02 20
    // Macronix MX25L12805D 16     ?    C2 20 18
    // Macronix MX66L51235F 64          C2 20 1A
    // Numonyx M25P128      16     ?    20 20 18
    // Micron M25P80         1     ?    20 20 14
    // Micron N25Q128A      16    64    20 BA 18
    // Micron N25Q512A      64     ?    20 BA 20  70  single    C4 x2
    // Micron N25Q00AA     128    64    20 BA 21      single    C4 x4
    // Micron MT25QL02GC   256    64    20 BA 22  70            C4 x2
    // SST SST25WF010      1/8     ?    BF 25 02
    // SST SST25WF020      1/4     ?    BF 25 03
    // SST SST25WF040      1/2     ?    BF 25 04
    // SST SST25VF016B       1     ?    BF 25 41
    // SST26VF016            ?          BF 26 01
    // SST26VF032            ?          BF 26 02
    // SST25VF032            4    64    BF 25 4A
    // SST26VF064            8     ?    BF 26 43
    // LE25U40CMC          1/2    64    62 06 13
    // Adesto AT25SF128A    16          1F 89 01

    float erasing_time_ms = Get_flash_size() / SET_eraseBytesPerSecond(id) * 1000;
    const uint32_t erasing_time_ms_step = static_cast<uint32_t>(erasing_time_ms / 100.0f);
    Display_Storage.Copy_audio_files_SD_to_Flash_chip_last_warning(erasing_time_ms);

    // Confirmation
    bool confirm = false;
    uint8_t action = 0;
    Display_Storage.Import_raw_files_frame(action);

    Clear_UI_events();
    while (!confirm)
    {
        Shifters_manager.Update();

        result = Read_encoder_simple(EN_PB_Select);
        if (result == +1)
        {
            if (action == 0)
            {
                action = 1;
                Display_Storage.Import_raw_files_frame(action);
            }
        }
        else if (result == -1)
        {
            if (action == 1)
            {
                action = 0;
                Display_Storage.Import_raw_files_frame(action);
            }
        }

        if (Read_pushbutton(EN_PB_Select))
        {
            confirm = true;
        }
    }
    Clear_UI_events();

    if (action == 0)
    {
        return false;
    }

    // Stage and commit every accepted name before erasing audio. Missing names retain their IDs.
    rootdir = SD.open("/LILLA_AUDIO");
    bool registry_ok = rootdir && rootdir.isDirectory();
    const char *registry_error = "CANNOT READ AUDIO DIRECTORY";
    while (registry_ok)
    {
        File source = rootdir.openNextFile();
        if (!source)
        {
            break;
        }
        char filename[256];
        SET_Audio_format format;
        if (!source.isDirectory() && raw_filename(source.name(), filename, sizeof(filename), format))
        {
            uint32_t offset = 0;
            uint32_t length = source.size();
            uint16_t channels = 1;
            if (SET_Audio_raw_data(source, format, offset, length, channels))
            {
                registry_error = FileNameRegistry::Valid_name(filename) ? "FILE NAME TABLE FULL" : "NAME TOO LONG OR RESERVED";
                registry_ok = FileNameRegistry::Add(filename) >= 0;
            }
        }
        source.close();
    }
    rootdir.close();
    if (!registry_ok)
    {
        FileNameRegistry::Load();
        Show_popup_text(registry_error, ILI9341_WHITE, ILI9341_RED, 75);
        return false;
    }
    if (!FileNameRegistry::Save())
    {
        Show_popup_text("CANNOT SAVE FILE NAMES", ILI9341_WHITE, ILI9341_RED, 75);
        return false;
    }

    // Start erasing flash chip
    Display_Storage.Copy_raw_files_SD_to_Flash_chip_job_start();

    flash_changed = true;
    SerialFlash.eraseAll(); // uint32_t size = Get_flash_size(); // SerialFlash.capacity(id);
    elapsedMillis dotMillis = 0;
    int percentage = 0;

    Display_Storage.Update_raw_copy_progress(percentage);

    while (SerialFlash.ready() == false)
    {
        if (dotMillis > erasing_time_ms_step)
        {
            dotMillis = 0;
            ++percentage;
            Display_Storage.Update_raw_copy_progress(percentage);
        }
    }

    // Create the fallback first so other imports cannot consume its space.
    Display_Storage.Copy_raw_files_SD_to_Flash_chip_popup_landscape();
    row = 2;
    if (!SD_has_zero_raw)
    {
        Display_Storage.Copy_raw_files_SD_to_Flash_chip_files_to_copy(++row, "0.raw", sizeof(zeroraw));
        if (!ZeroRaw_ensure_file())
        {
            Display_Storage.Copy_raw_files_SD_to_Flash_chip_flash_error();
            delay(4000);
            return false;
        }
        Serial.println(F("0.raw reconstructed from firmware: 43996 samples, 87992 bytes."));
    }

    // Import audio as mono RAW files from SD to Flash chip.
    rootdir = SD.open("/LILLA_AUDIO");
    if (!rootdir || !rootdir.isDirectory())
    {
        rootdir.close();
        Display_Storage.Copy_raw_files_SD_to_Flash_chip_lilla_audio_missing();
        delay(4000);
        return false;
    }
    while (1)
    {
        File f = rootdir.openNextFile();
        if (!f)
        {
            break;
        }
        char filename[256];
        SET_Audio_format format;
        if (f.isDirectory() || !raw_filename(f.name(), filename, sizeof(filename), format))
        {
            f.close();
            continue;
        }

        const bool is_zero_raw = strcmp(filename, "0.raw") == 0;
        Mp3Import mp3;
        uint32_t offset = 0;
        uint32_t length = f.size();
        uint16_t channels = 1;
        if (!(format == SET_Audio_format::Mp3 ? mp3.Open(f, length, channels, SET_AUDIO_MAX_RAW_BYTES) : SET_Audio_raw_data(f, format, offset, length, channels)))
        {
            ++row;
            if (row > 14)
            {
                Display_Storage.Copy_raw_files_SD_to_Flash_chip_popup_landscape();
                row = 3;
            }
            Display_Storage.Copy_audio_files_SD_to_Flash_chip_invalid_audio(row, f.name());
            f.close();
            continue;
        }
        if (is_zero_raw && (length < sizeof(int16_t) || length % sizeof(int16_t) != 0))
        {
            f.close();
            continue; // An empty or truncated PCM sample cannot replace the fallback.
        }

        if (FileNameRegistry::Find(filename) < 0)
        {
            f.close();
            rootdir.close();
            Show_popup_text("AUDIO DIRECTORY CHANGED", ILI9341_WHITE, ILI9341_RED, 75);
            return false;
        }

        if (SerialFlash.exists(filename))
        {
            ++row;
            if (row > 14)
            {
                Display_Storage.Copy_raw_files_SD_to_Flash_chip_popup_landscape();
                row = 3;
            }
            Display_Storage.Copy_raw_files_SD_to_Flash_chip_duplicate(row, f.name());
            f.close();
            continue;
        }

        if (format != SET_Audio_format::Raw && format != SET_Audio_format::Mp3 && !f.seek(offset))
        {
            f.close();
            rootdir.close();
            Display_Storage.Copy_raw_files_SD_to_Flash_chip_flash_error();
            return false;
        }
        ++row;
        if (row > 14)
        {
            Display_Storage.Copy_raw_files_SD_to_Flash_chip_popup_landscape();
            row = 3;
        }
        Display_Storage.Copy_raw_files_SD_to_Flash_chip_files_to_copy(row, filename, length);

        if (!SerialFlash.create(filename, length))
        {
            f.close();
            rootdir.close();
            Display_Storage.Copy_raw_files_SD_to_Flash_chip_flash_full_error();
            delay(4000);
            return false;
        }

        SerialFlashFile ff = SerialFlash.open(filename);
        bool copied = static_cast<bool>(ff);
        unsigned long count = 0;
        while (copied && count < length)
        {
            int16_t buf[128];
            const unsigned long remaining = length - count;
            const unsigned int capacity = sizeof(buf) / channels;
            const unsigned int bytes = remaining < capacity ? remaining : capacity;
            const unsigned int input_bytes = bytes * channels;
            if (!(format == SET_Audio_format::Mp3 ? mp3.Read(buf, bytes / sizeof(int16_t)) : f.read(buf, input_bytes) == input_bytes))
            {
                copied = false;
                break;
            }
            if (format == SET_Audio_format::Aiff)
            {
                // AIFF PCM is big endian; Flash RAW and the Teensy use little endian.
                uint8_t *pcm = reinterpret_cast<uint8_t *>(buf);
                for (unsigned int index = 0; index < input_bytes; index += 2)
                {
                    const uint8_t first = pcm[index];
                    pcm[index] = pcm[index + 1];
                    pcm[index + 1] = first;
                }
            }
            if (channels == 2)
            {
                // Average stereo pairs in place, using 32 bits to avoid overflow.
                for (unsigned int sample = 0; sample < bytes / sizeof(int16_t); ++sample)
                {
                    buf[sample] = static_cast<int16_t>((static_cast<int32_t>(buf[2 * sample]) + static_cast<int32_t>(buf[2 * sample + 1])) / 2);
                }
            }
            if (ff.write(buf, bytes) != bytes)
            {
                copied = false;
                break;
            }
            count += bytes;
        }
        SerialFlash.wait();
        ff.close();
        if (!copied)
        {
            SerialFlash.remove(filename);
            f.close();
            rootdir.close();
            Serial.println(F("RAW import failed while reading SD or writing Flash."));
            Display_Storage.Copy_raw_files_SD_to_Flash_chip_flash_error();
            delay(4000);
            return false;
        }
        f.close();
    }
    rootdir.close();

    // The required file must exist before reporting success and rebuilding the VFS.
    if (!ZeroRaw_ensure_file())
    {
        Display_Storage.Copy_raw_files_SD_to_Flash_chip_flash_error();
        delay(4000);
        return false;
    }
    delay(10);

    // Display RAW files list
    Display_Storage.Copy_raw_files_SD_to_Flash_chip_job_done();
    row = 2;
    SerialFlash.opendir();

    char filename[64];
    uint32_t filesize;
    while (SerialFlash.readdir(filename, sizeof(filename), filesize))
    {
        ++row;
        if (row > 14)
        {
            delay(4000);
            Display_Storage.Copy_raw_files_SD_to_Flash_chip_list_landscape();
            row = 3;
        }

        Display_Storage.Copy_raw_files_SD_to_Flash_chip_file_copied(row, filename, filesize);
    }
    delay(6000);
    return true;
}

FLASHMEM
float SET_eraseBytesPerSecond(const unsigned char *id)
{
    if (id[0] == 0x20) // Micron
    {
        return 152000.0;
    }
    if (id[0] == 0x01) // Spansion
    {
        return 500000.0;
    }
    if (id[0] == 0xEF) // 419430.0 Winbond
    {
        return 512281.4;
    }
    if (id[0] == 0xC2) // Macronix
    {
        return 279620.0;
    }
    return 320000.0; // guess?
}

// ***************************************************************************************************************
// **********************************                 SOUND_EDIT                 *********************************
// ***************************************************************************************************************

FLASHMEM
void S_Select_menu_elements(void)
{
    // voices of instrument_edit_menu that can be displayed
    S_Menu[value_S_Return] = true; // RETURN
    S_Menu[value_S_Clone] = true;  // CLONE
    S_Menu[value_S_Drop] = true;   // DELETE

    if (Patch[Patch_id].instruments == INSTRUMENTS)
    {
        S_Menu[value_S_Clone] = false; // CLONE
    }

    if (S_Get_sounds_free() < 1)
    {
        S_Menu[value_S_Clone] = false; // CLONE
    }

    if (Patch[Patch_id].instruments == 1)
    {
        S_Menu[value_S_Drop] = false; // DELETE
    }

    if (Lilla_state_0 == MIDI_LOOP)
    {
        S_Menu[value_S_Clone] = false;
        S_Menu[value_S_Drop] = false;
    }

    S_menu_max = S_Menu[value_S_Return] + S_Menu[value_S_Clone] + S_Menu[value_S_Drop] - 1;
}

bool S_Fill_tables(uint8_t instrument_id)
{
    return S_Rebuild_audio_tables(instrument_id);
}

bool S_Fill_all_tables(void)
{
    return S_Rebuild_audio_tables();
}

bool S_Rebuild_audio_tables(uint8_t)
{
    Preset_struct next_presets[INSTRUMENTS] = {};
    uint16_t tables_mask = 0;
    if (!P_Prepare_audio_tables(Patch_id, Patch_volume_gain(volume_patch), next_presets, tables_mask, true))
    {
        audio_tables_error_pending = true;
        return false;
    }
    if (!Players_Manager.Activate_prepared_presets(next_presets))
    {
        Audio_tables.Cancel_prepare();
        audio_tables_error_pending = true;
        return false;
    }
    return true;
}

// ***************************************************************************************************************
// **********************************        INSTRUMENT_VCF FUNCTIONS           **********************************
// ***************************************************************************************************************

void Macro_VCF_filter_on_none(void)
{
    if (!Patch[Patch_id].Instrument[Instrument_id].Filter.use)
    {
        Patch[Patch_id].Instrument[Instrument_id].Filter.use = true;
    }
    else
    {
        Patch[Patch_id].Instrument[Instrument_id].Filter.use = false;
    }

    AudioNoInterrupts();
    Players_Manager.Update_Preset_IF(Patch_id, Instrument_id);
    Players_Manager.Multicast_IF_update_filter_type(Instrument_id);

    if (Lilla_state_0 == LIVE_SAMPLING)
    {
        Patch[Patch_id].Instrument[1].Filter.use = Patch[Patch_id].Instrument[0].Filter.use;
        Instrument_id = 1;

        Players_Manager.Update_Preset_IF(Patch_id, Instrument_id);
        Players_Manager.Multicast_IF_update_filter_type(Instrument_id);

        Instrument_id = 0;
    }
    AudioInterrupts();
}

void Macro_VCF_modulation_none(void)
{
    Patch[Patch_id].Instrument[Instrument_id].Filter.modulation = 0;

    AudioNoInterrupts();
    Players_Manager.Update_Preset_IF(Patch_id, Instrument_id);

    if (Lilla_state_0 == LIVE_SAMPLING)
    {
        Patch[Patch_id].Instrument[1].Filter.modulation = Patch[Patch_id].Instrument[0].Filter.modulation;
        Instrument_id = 1;

        Players_Manager.Update_Preset_IF(Patch_id, Instrument_id);

        Instrument_id = 0;
    }
    AudioInterrupts();
}

// ***************************************************************************************************************
// ******************************          ENCODERS PUSHBUTTONS FUNCTIONS           ******************************
// ***************************************************************************************************************

bool Read_pushbutton(int element)
{
    return (Pushbuttons_manager.Get_change(element));
}
bool Read_pushbutton_fast(int element)
{
    return (Pushbuttons_manager.Get_value(element));
}
int Read_encoder_simple(int element)
{
    auto R = Encoders_manager.Get_rotation(element);
    if (R == 0)
    {
        return 0;
    }
    else if (R == 1)
    {
        return 1;
    }
    else
    {
        return -1;
    }
}
bool Read_encoder_fast(int element)
{
    auto R = Encoders_manager.Get_rotation(element);
    if (R == 0)
    {
        return false;
    }
    else
    {
        return true;
    }
}

void Clear_UI_events(void)
{
    Encoders_manager.Clear_rotation_all_encoders();
    Pushbuttons_manager.Clear_change_all_pushbuttons();
}

// **************************************************************************************************************
// *************************************            BOOTSTRAP             ***************************************
// **************************************************************************************************************

bool P_Prepare_audio_tables(int patch_id, float patch_volume, Preset_struct (&presets)[INSTRUMENTS], uint16_t &tables_mask, bool allow_retiring_fade)
{
    const bool audio_interrupts_enabled = NVIC_IS_ENABLED(IRQ_SOFTWARE) != 0;
    AudioNoInterrupts();
    const bool trigger_running = Trigger.Is_running();
    Trigger.Stop();

    // Players and the finalizer may run while the model is being prepared; MIDI, filter and delay control callbacks must see only committed state.
    const bool snapshot_ready = Players_Manager.Build_presets_snapshot(patch_id, patch_volume, presets, tables_mask);
    Audio_tables.Release_unreferenced_banks(Players_Manager.Refresh_audio_table_references());
    bool preparation_started = snapshot_ready && Audio_tables.Begin_prepare(tables_mask);
    const uint32_t wait_started_us = micros();
    uint8_t reclaimed_banks = 0;
    uint16_t stopped_players = 0;

    // Allow pending edits and restarts to finish before fading any remaining old-bank voices.
    while (snapshot_ready && !preparation_started && __get_primask() == 0 && static_cast<uint32_t>(micros() - wait_started_us) < 20000u)
    {
        const uint8_t retiring_banks = Audio_tables.Get_retiring_banks_mask();
        if (retiring_banks == 0)
        {
            break;
        }
        if (allow_retiring_fade && static_cast<uint32_t>(micros() - wait_started_us) >= 6000u)
        {
            reclaimed_banks |= retiring_banks;
            stopped_players |= Players_Manager.Fast_stop_players_using_tables(retiring_banks);
        }
        AudioInterrupts();
        delayMicroseconds(50);
        AudioNoInterrupts();
        Audio_tables.Release_unreferenced_banks(Players_Manager.Refresh_audio_table_references());
        preparation_started = Audio_tables.Begin_prepare(tables_mask);
    }

    bool prepared = false;
    if (preparation_started)
    {
        // Protect the shared SPI-use counter, then leave audio processing enabled during Flash reads.
        AudioStartUsingSPI();
        AudioInterrupts();
        prepared = Audio_tables.Prepare_all(presets);
        AudioNoInterrupts();
        AudioStopUsingSPI();
    }
    if (trigger_running)
    {
        Trigger.Start();
    }
    if (audio_interrupts_enabled)
    {
        AudioInterrupts();
        if (reclaimed_banks != 0)
        {
            Serial.print(F("AudioTables bank reclaim, banks: 0x"));
            Serial.print(reclaimed_banks, HEX);
            Serial.print(F(", stopped players: 0x"));
            Serial.print(stopped_players, HEX);
            Serial.print(F(", ready: "));
            Serial.println(prepared);
        }
    }
    return prepared;
}

bool P_Quiesce_audio_players(void)
{
    const bool audio_interrupts_enabled = NVIC_IS_ENABLED(IRQ_SOFTWARE) != 0;
    AudioNoInterrupts();
    Trigger.Stop();
    Midi_reader.Stop();
    const uint32_t wait_started_us = micros();
    // At first startup all players are idle and delay buffers are not connected yet: do not enable audio just to discover that nothing needs draining.
    bool playing = false;
    for (uint8_t player_id = 0; player_id < PLAYERS; ++player_id)
    {
        playing |= Player[player_id].isPlaying();
    }
    while (playing && __get_primask() == 0 && static_cast<uint32_t>(micros() - wait_started_us) < 20000u)
    {
        Players_Manager.Stop_all_players();
        AudioInterrupts();
        delayMicroseconds(50);
        AudioNoInterrupts();
        playing = false;
        for (uint8_t player_id = 0; player_id < PLAYERS; ++player_id)
        {
            playing |= Player[player_id].isPlaying();
        }
    }
    const uint8_t referenced_banks = Players_Manager.Refresh_audio_table_references();
    const bool ready = !playing && Audio_tables.Reset(referenced_banks);
    if (ready)
    {
        PatchCache_Manager.Begin();
    }
    if (audio_interrupts_enabled)
    {
        AudioInterrupts();
    }
    return ready;
}

bool Startup_mode(void)
{
    const bool enabled = NVIC_IS_ENABLED(IRQ_SOFTWARE) != 0;
    AudioNoInterrupts();
    Lilla_state = PERFORMANCE;
    Lilla_state_0 = Lilla_state;
    Patch_id_old = Patch_id;
    Patch_cache_P = Patch[Patch_id];
    const auto startup_mode = Switches_manager.Get_value(SwitchModes);
    bool ready = false;

    if (startup_mode == SwModesSampler)
    {
        ready = DS_setup_DIRECT_SAMPLING_Patch_and_Preset();
    }
    else
    {
        if (startup_mode == SwModesLiveSampler)
        {
            Patch_id = PATCHES_MAX;
            LS_setup_LS_Patch(LS_stereo);
        }
        ready = S_Fill_all_tables();
        if (ready)
        {
            P_Update_all_maps_Instrument_for_notes();
        }
    }
    uint16_t tables_mask = 0;
    if (ready)
    {
        for (uint8_t instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
        {
            if (Patch[Patch_id].Instrument[instrument_id].used && Preset[instrument_id].file < FIRST_LIVE_SAMPLING_FILE)
            {
                tables_mask |= static_cast<uint16_t>(1u << instrument_id);
            }
        }
    }
    if (enabled)
    {
        AudioInterrupts();
    }
    if (!ready)
    {
        Serial.println(F("AudioTables startup preparation failed"));
        return false;
    }
    switch (startup_mode)
    {
    case SwModesSampler:
        Golive_DIRECT_SAMPLING();
        break;
    case SwModesLiveSampler:
        Golive_with_LIVE_SAMPLING();
        break;
    case SwModesMidiLoop:
        Golive_with_MIDI_LOOP(true);
        break;
    default:
        Golive_with_PERFORMANCE(Patch_id);
        break;
    }
    Serial.print(F("AudioTables startup ready, mask: 0x"));
    Serial.println(tables_mask, HEX);
    return true;
}

FLASHMEM
void Startup_hardware_and_objects(void)
{
    Serial.begin(115200);

    // Value tables
    Compile_tables();

    // audioControlSGTL5000 Audio_shield - Audio Adaptor inizialization
    /*
        lineInLevel(both) adjust the sensitivity of the line-level inputs. Fifteen settings are possible:
        0: 3.12 Volts p-p
        1: 2.63 Volts p-p
        2: 2.22 Volts p-p
        3: 1.87 Volts p-p
        4: 1.58 Volts p-p
        5: 1.33 Volts p-p  (default)
        6: 1.11 Volts p-p
        7: 0.94 Volts p-p
        8: 0.79 Volts p-p
        9: 0.67 Volts p-p
        10: 0.56 Volts p-p
        11: 0.48 Volts p-p
        12: 0.40 Volts p-p
        13: 0.34 Volts p-p
        14: 0.29 Volts p-p
        15: 0.24 Volts p-p
    */
    Audio_shield.lineInLevel(Line_in_gain);

    /*
        lineOutLevel(both) adjust the line level output voltage range. The following settings are possible:
        13: 3.16 Volts p-p
        14: 2.98 Volts p-p
        15: 2.83 Volts p-p
        16: 2.67 Volts p-p
        17: 2.53 Volts p-p
        18: 2.39 Volts p-p
        19: 2.26 Volts p-p
        20: 2.14 Volts p-p
        21: 2.02 Volts p-p
        22: 1.91 Volts p-p
        23: 1.80 Volts p-p
        24: 1.71 Volts p-p
        25: 1.62 Volts p-p
        26: 1.53 Volts p-p
        27: 1.44 Volts p-p
        28: 1.37 Volts p-p
        29: 1.29 Volts p-p  (default)
        30: 1.22 Volts p-p
        31: 1.16 Volts p-p
    */
    Line_out_level = 13;
    Audio_shield.lineOutLevel(Line_out_level);

    Audio_shield.enable();
    Audio_shield.volume(0.8); // Set the headphone volume level. Range is 0 to 1.0, but 0.8 corresponds to the maximum undistorted output for a full scale signal. Usually 0.5 is a comfortable listening level. The line level outputs are not changed by this function.
    Audio_shield.inputSelect(myInput);

    // Audio_shield.audioPostProcessorEnable();
    Audio_shield.eqSelect(0);                // 0=NONE, 1=PEQ (7 IIR Biquad filters), 2=TONE (tone), 3=GEQ (5 band EQ)
    Audio_shield.adcHighPassFilterDisable(); // noise reduction: https://openaudio.blogspot.com/2017/03/teensy-audio-board-self-noise.html

    // Start SPI communication with W25Q512 Flash memory chip
    SerialFlash.begin();
    delay(100);

    // Start Gate IN/OUT
    Setup_GATE_pins();

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //   ***********    INIZIALIZZAZIONE OGGETTI    *************
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    // Get Vibrato pointers
    Vibrato_array_pointer = Vibrato.Get_vibrato_array_pointer();
    Vibrato_array_last_element = Vibrato.Get_vibrato_array_last_element();
    Vibrato.Make_vibrato_table();

    // PlayerManager
    Players_Manager.Set_ADSR_ptr(&ADSR[0]);

    // AudioADSR
    for (auto i = 0; i < PLAYERS; ++i)
    {
        ADSR[i].Set_identity(i);
    }

    // Setup Player and LFO
    for (auto player = 0; player < PLAYERS; ++player)
    {
        Player[player].Set_identity(player);
        Player[player].Set_vibrato_pointers(Vibrato_array_pointer, Vibrato_array_last_element); // void Set_vibrato_pointers(float *p_vibrato_array_in, uint8_t *p_vibrato_array_last_element_in)
        Player[player].VCF_ptr = &VCF[player];
        Player[player].LFO_ptr = &LFO_P0[player];
        Player[player].LiveSampler_ptr = &LiveSampler;
        Player[player].Players_statistics_ptr = &Players_statistics;
        Player[player].Set_ADSR_ptr(&ADSR[player]);
        LFO_P0[player].identity = player;
    }

    for (auto instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
    {
        CC_Sound_gain_cache[instrument_id] = -1;
    }

    // Setup Midi_reader
    Midi_reader.Vibrato = &Vibrato;
    Midi_reader.Tone_generator = &Tone_generator;
    Midi_reader.Players_Manager = &Players_Manager;

    // Setup Execute_Commands
    Filter_Biquad_Manager.biquad_L_ptr = &biquad_L;
    Filter_Biquad_Manager.biquad_R_ptr = &biquad_R;

    // Setup Trigger
    Trigger.Players_Manager_ptr = &Players_Manager; // Budget every rendering block, independently of MIDI activity.
    Trigger.Midi_reader_ptr = &Midi_reader;
    Trigger.Filter_Biquad_Manager_ptr = &Filter_Biquad_Manager;
    Trigger.Delay_Manager_ptr = &Delay_manager;

    // Setup Delays
    Delay_L.LFO_ptr = &LFO_D[0];
    Delay_R.LFO_ptr = &LFO_D[1];
    LFO_D[0].identity = 88;
    LFO_D[1].identity = 99;

    // Setup Infotest
    Info.LiveSampler_ptr = &LiveSampler;

    // Setup Wavetable-s

    // Setup stereo Live Sampler feedback and compressor (initially bypassed).
    LS_Compressor.Recorder_ptr = &LiveSampler;
    LS_Compressor.Set_feedback(0);

    // Setup Display (module)
    tft.begin();
    tft.setRotation(3);
    tft.setTextWrap(false);
    tft.fillScreen(ILI9341_BLACK);
    canvas.setTextWrap(false);

    // DelayManager
    Delay_manager.Delay_L_ptr = &Delay_L;
    Delay_manager.Delay_R_ptr = &Delay_R;
    Delay_manager.LFO_D_ptr[0] = &LFO_D[0];
    Delay_manager.LFO_D_ptr[1] = &LFO_D[1];
    Delay_manager.D_gain_L_feedback_ptr = &D_gain_L_feedback;
    Delay_manager.D_gain_R_feedback_ptr = &D_gain_R_n;
    Delay_manager.Players_Manager_ptr = &Players_Manager;

    // Setup PlayersStatistics
    Players_statistics.Loop_led_set_ptr = &Loop_led_set;
    Players_statistics.Performance_led_set_ptr = &Performance_led_set;

    // Gate
    Gate_out.Reset();

    // DirectSampler and LiveSampler inputs are not used
    MAIN_mixer_out_L.gain(2, 0.0); // Direct Samp
    MAIN_mixer_out_R.gain(2, 0.0); // Live Sampler

    // mutes LINE_IN to MAIN (Audio Board)
    MAIN_mixer_out_L.gain(1, 0.0);
    MAIN_mixer_out_R.gain(1, 0.0);

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // *******************       PSRAM       *********************
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    // PatchCacheManager
    for (auto i = 0; i < PATCH_CACHE_ARRAY_COUNT; ++i)
    {
        PatchCache_Manager.Set_cache_pointer(i, &patch_cache_array[i][0]);
    }

    // CacheCycleFinalizer
    CacheCycle_finalizer.Begin(&Player[0], &PatchCache_Manager, &Audio_tables, &Players_Manager);

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // *******************        FRAM       **********************
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // I2C n.2 bus inizialization.
    // Bus I2C n.2 is:
    // SCL2: 24
    // SDA2: 25
    Wire2.begin();
    Wire2.setClock(1000000); // Wire2.setClock(400000);

    const byte result = LillaFram.begin();

    if (result == LillaFRAM_2x512::ERROR_0)
    {
        Serial.println(F("FRAM bank check: OK"));
    }
    else
    {
        Serial.print(F("FRAM bank check failed, error: "));
        Serial.println(result);
    }

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // *******************   SHIFTERS DATA   **********************
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    Shifters_manager.Monitor_all_controllers();
    Shifters_manager.Update();

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // *******************   FILE SCANNER   **********************
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // Install the bundled middle-C sine before building file inventory and caches.
    if (!ZeroRaw_ensure_file(true))
    {
        Serial.println(F("ERROR: cannot install sine 0.raw"));
        while (true)
        {
            delay(1000);
        }
    }
    // File inventory is built after validating/loading the persistent name registry.

    // Note-to-pitch conversion array
    key_step = 0;
    Calc_pitch_from_note(key_step);
}

FLASHMEM
void Reload_system_state(void)
{
    if (!P_Quiesce_audio_players())
    {
        Serial.println(F("AudioTables reload stopped: players still active"));
        return;
    }

    Capture_new_patch = -1;
    Capture_target = -1;
    Capture_learn_key = false;
    Capture_learn_note = -1;
    const bool audio_interrupts_enabled = NVIC_IS_ENABLED(IRQ_SOFTWARE) != 0;
    AudioNoInterrupts();
    for (auto &source : Capture_sources)
    {
        source = {};
    }
    PatchCache_Manager.Begin();
    if (audio_interrupts_enabled)
    {
        AudioInterrupts();
    }

    if (Archive.Check_FRAM_archive() != LillaFRAM_2x512::ERROR_0)
    {
        Serial.println(F("FRAM archive unavailable: interrupted restore, invalid header or I/O error. Automatic repair is blocked."));
        Serial.println(F("Place LILLA_CONFIG.fram and its REC files in /LILLABACKUP. Press Select to retry the complete restore."));
        Display_Storage.FRAM_recovery_popup();
        bool recovered = false;
        while (!recovered)
        {
            Shifters_manager.Update();
            if (Read_pushbutton(EN_PB_Select))
            {
                bool config_error = false;
                recovered = BACKUP_Restore(&config_error);
                if (!config_error)
                {
                    Serial.println(recovered ? F("Configuration and audio restored.") : F("Restore failed. Check the backup files and press Select to retry."));
                }
            }
            delay(10);
        }
    }

    File_scanner.Read_all_file_data();

    // ***************   DIRECT SAMPLING AND VFS   ******************
    // Flash memory dimension MB
    verified_flash_memory_MB = Get_flash_size() / 1048576;

    // Prints Flash chip file list, occupation and available space.
    Print_flash_file_list();

    VFS_packets = VFS_Get_packets();
    DS_VFS_packets = VFS_packets; // Packets dedicated to Direct Sampling; CAN be an ODD value

    DS_First_packet = 0;
    DS_Last_packet = VFS_packets - 1;

    Serial.print(F("At startup there are: "));
    Serial.print(VFS_packets);
    Serial.println(F(" VFS packets (64KB --> 32K 16bit-samples)."));
    VFS_Print_allocation();

    // Free space on flash chip for new .raw files
    Serial.print(F("Flash occupation (kB): "));
    Serial.println(Get_flash_occupation() / 1024);
    Serial.print(F("Flash available for more .raw files (kB): "));
    Serial.println((Get_flash_size() - Get_flash_occupation() - FLASH_FREE_SPACE) / 1024);
    Serial.println();

    // |||||||||||||||||        TOOLS         |||||||||||||||||||
    // Resets all Recording and erase all DS_VFS_Packets
    if (false)
    {
        Serial.println(F("RESET_all_Recording -> SEED_all_Recording"));
        DS_seed_all_Recordings();

        Serial.println(F("RESET_all_Recording -> VFS_Erase_all_Packet"));
        VFS_Erase_all_packets_for_DS();
        Serial.println();
    }
    // |||||||||||||||||       END TOOLS      ||||||||||||||||||||

    ArchivingManager::FRAM_System_repair_report system_repair_report;
    const byte system_repair_result = Archive.Repair_System_in_FRAM(system_repair_report);

    if (system_repair_result != LillaFRAM_2x512::ERROR_0)
    {
        Serial.print(F("FRAM System repair failed, error "));
        Serial.println(system_repair_result);
        while (true)
        {
            delay(1000);
        }
    }

    Serial.print(F("FRAM System repair: defaulted="));
    Serial.println(system_repair_report.defaulted ? F("yes") : F("no"));

    ArchivingManager::FRAM_Recording_repair_report recording_repair_report;
    const byte recording_repair_result = Archive.Repair_Recordings_in_FRAM(recording_repair_report);

    if (recording_repair_result != LillaFRAM_2x512::ERROR_0)
    {
        Serial.print(F("FRAM Recording repair failed: Recording "));
        Serial.print(recording_repair_report.failed_id);
        Serial.print(F(", error "));
        Serial.println(recording_repair_result);
        while (true)
        {
            delay(1000);
        }
    }

    Serial.print(F("FRAM Recording repair: cleared Recordings="));
    Serial.println(recording_repair_report.cleared_recordings);

    const byte recording_load_result = DS_read_all_Recordings();
    if (recording_load_result != LillaFRAM_2x512::ERROR_0)
    {
        Serial.print(F("FRAM Recording load failed, error "));
        Serial.println(recording_load_result);
        while (true)
        {
            delay(1000);
        }
    }

    Require_VFS(VFS_Clean_up_VFS());
    // CRC repair also leaves a durable inconsistent marker; cleanup handles every orphan before clearing it.
    Require_VFS(VFS_Defragment());
    DS_update_recordings();

    // Print VFS FAT table
    VFS_Print_FAT();

    // Setup Input Gain
    LINE_IN_amplifier.Set_gain(1.0f);

    // *******************   CORE ARRAYS  ************************
    ArchivingManager::FRAM_Repair_report repair_report;

    const byte repair_result = Archive.Repair_Patch_Sound_in_FRAM(repair_report);

    Serial.print(F("FRAM repair: cleared Patches="));
    Serial.print(repair_report.cleared_patches);
    Serial.print(F(", cleared Sounds="));
    Serial.print(repair_report.cleared_sounds);
    Serial.print(F(", defaulted Sounds="));
    Serial.println(repair_report.defaulted_sounds);

    if (repair_result != LillaFRAM_2x512::ERROR_0)
    {
        Serial.print(F("FRAM repair failed: "));
        Serial.print(repair_report.failed_sound ? F("Sound ") : F("Patch "));
        Serial.print(repair_report.failed_id);
        Serial.print(F(", error "));
        Serial.println(repair_result);
        while (true)
        {
            delay(1000);
        }
    }
    P_Delete_all_Patches_and_Sounds();

    uint16_t failed_metadata_id = 0;
    bool failed_sound = false;
    const byte metadata_result = Archive.Load_Patch_Sound_from_FRAM(failed_metadata_id, failed_sound);
    if (metadata_result != LillaFRAM_2x512::ERROR_0)
    {
        Serial.print(F("FRAM metadata load failed: "));
        Serial.print(failed_sound ? F("Sound ") : F("Patch "));
        Serial.print(failed_metadata_id);
        Serial.print(F(", error "));
        Serial.println(metadata_result);
        while (true)
        {
            delay(1000);
        }
    }
    S_Copy_all_Sound_to_Sound_cache_P();
    Serial.println(F("FRAM -> RAM2: 200 Patches and 800 Sounds loaded, CRC verified"));

    byte system_settings_result = Archive.Read_first_octave(first_octave);

    uint8_t stored_key_step = 0;
    if (system_settings_result == LillaFRAM_2x512::ERROR_0)
    {
        system_settings_result = Archive.Read_key_step(stored_key_step);
    }
    if (system_settings_result == LillaFRAM_2x512::ERROR_0)
    {
        system_settings_result = CC_Read_all_Sound_gain();
    }
    if (system_settings_result != LillaFRAM_2x512::ERROR_0)
    {
        Serial.print(F("FRAM System settings load failed, error "));
        Serial.println(system_settings_result);
        while (true)
        {
            delay(1000);
        }
    }

    key_step = stored_key_step;
    Calc_pitch_from_note(key_step);

    P_Update_Patches_number(); // aggiorna patches_number (numero di patchi disponibili)
    Patch_id = P_Get_first_Patch_id_existing();

    // *****************      DELAY AND LFO    ********************
    // Delay arrays (FIFO)
    memset(DELAY_fifo_L, 0, sizeof(DELAY_fifo_L));
    memset(DELAY_fifo_R, 0, sizeof(DELAY_fifo_R));

    Serial.print("indirizzo DELAY_fifo_L: ");
    Serial.println((unsigned long)DELAY_fifo_L, HEX);
    Serial.print("decimale: ");
    Serial.println((unsigned long)DELAY_fifo_L);
    Serial.print("indirizzo DELAY_fifo_R: ");
    Serial.println((unsigned long)DELAY_fifo_R, HEX);
    Serial.print("decimale: ");
    Serial.println((unsigned long)DELAY_fifo_R);

    // Delay objects setup
    Delay_L.DELAY_fifo = DELAY_fifo_L;
    Delay_R.DELAY_fifo = DELAY_fifo_R;

    // |||||||||||||||||        TOOLS         |||||||||||||||||||
    if (false)
    {
        Delay_data.samples = 24;
        Delay_data.samples_LR = 0;
        Delay_data.instrument_route = 0b11111111;
        Delay_data.modulation_source = 0;
        Delay_data.modulation_depth = 30;
        Delay_data.modulation_frequency = 12;
        Delay_data.modulation_phase_LR = 0;
        Delay_data.loop_gain = 65;

        if (Patch_id < PATCHES_MAX)
        {
            Require_FRAM(Archive.Save_Delay(Patch_id, Delay_data));
        }
    }
    // |||||||||||||||||       END TOOLS      ||||||||||||||||||||

    Require_FRAM(Archive.Read_Delay(Patch_id, Delay_data));
    Calc_Delay_values(Delay_data);

    // Transmits data to Delay objects
    Delay_L.Setup_delay(Delay_values.samples + (Delay_values.samples_LR > 0 ? Delay_values.samples_LR : 0)); // Restore the same signed left offset used by runtime parameter changes.
    Delay_R.Setup_delay(Delay_values.samples - (Delay_values.samples_LR < 0 ? Delay_values.samples_LR : 0)); // Restore the same signed right offset used by runtime parameter changes.
    Delay_L.Set_delay_modulation_source(Delay_values.modulation_source);                                     // 0:none 1:LFO  2:input_1
    Delay_R.Set_delay_modulation_source(Delay_values.modulation_source);                                     // 0:none 1:LFO  2:input_1
    Delay_L.Set_delay_modulation_gain(Delay_values.modulation_depth);
    Delay_R.Set_delay_modulation_gain(Delay_values.modulation_depth);
    D_gain_L_feedback.Set_gain(Delay_values.loop_gain);
    D_gain_R_n.Set_gain(Delay_values.loop_gain);

    LFO_D[0].Set_amplitude(1000); // notice: modulation depth is set in Delay(0), not here.
    LFO_D[1].Set_amplitude(1000); // notice: modulation depth is set in Delay(1), not here.
    LFO_D[0].Set_frequency(Delay_values.modulation_frequency);
    LFO_D[1].Set_frequency(Delay_values.modulation_frequency);
    LFO_D[0].Set_phase(Delay_values.modulation_phase_LR);

    // *******************   LIVE SAMPLING  **********************
    LS_stereo = false;
    LS_buffer_dim = (LS_stereo ? LS_CACHE_STEREO_SAMPLES : LS_CACHE_MONO_SAMPLES);

    LS_state = EMPTY;
    LS_sound_id = SOUNDS_MAX;
    LS_instrument = 0;
    LS_X_delta = 0;
    LS_X_sample = 0;
    LS_window_width = LS_buffer_dim;
    LS_window_step = LS_window_width / 8;
    LS_XY_lock = true; // LS_X_sample blocked on FIFO; LS_X_delta is useless
    LS_XY_delta = AUDIO_SAMPLE_RATE;
    LS_Y_sample = LS_X_sample + LS_XY_delta;
    LS_X_step = LS_window_width / LS_COMB;
    LS_feedback = 0;
    LS_Reset_buffer();

    // * LPF final filter Output Butterworth filters, 12 db/octave *
    biquad_L.setLowpass(0, 20000, 0.707);
    biquad_R.setLowpass(0, 20000, 0.707);

    // **************            FLAGS            ****************
    display_instrument_volume_flag = false;
    instrument_volume_changed = 0;

    // **************     MIDI CONTROL CHANGE     ****************
    CC_lowpass_filter_value = 0;
    CC_midi_controller = 0;

    // **************  RESOLUTION  DOWNSAMPLING   ****************
    lowpass_flag = false;
    lowpass_direction = false;
    lowpass = LPF_MAX;
    lowpass_target = LPF_MAX;
    display_lowpass_flag = false;
    resolution = 0;   // [0, RES_MAX] 0: risoluzione 16bit
    downsampling = 1; // n. of repeated samples  1 = 44.1ksps

    // *******************    MIDI LOOP   ************************
    LOOP_id = -1;
    LOOP_reset_all_data();

    // *******************    COVER PAGE    **********************
    Display_Startup.Lilla_cover_slow();

    // ****************    DEFINE STARTUP MODE     ************
    if (!Startup_mode())
    {
        return;
    }

    // *******************    START MIDI   ************************
    Midi_reader.Begin();
    Midi_reader.Start();
    delay(10);

    // *******************  START TRIGGERS  ***********************
    Trigger.Start();
    delay(20);
}

bool TEST_Current_Patch_SD_round_trip(void)
{
    const int test_patch_id = Patch_id;

    Serial.println();
    Serial.println(F("*** PATCH SD ROUND-TRIP TEST ***"));
    Serial.print(F("Patch id: "));
    Serial.println(test_patch_id);

    // Conserva lo stato originale della Patch in uso.
    const Patch_struct Original_Patch = Patch[test_patch_id];

    // 1. Salvataggio su SD
    if (!Archive.Save_Patch_from_RAM_to_SD(test_patch_id))
    {
        PRINT_ERROR(F("TEST FAILED: unable to save Patch"));
        return false;
    }

    Serial.println(F("Patch successfully saved"));

    // 2. Alterazione intenzionale della Patch in RAM.
    // Serve a dimostrare che Resume non ÃƒÆ’Ã‚Â¨ un semplice no-op.
    Patch[test_patch_id].used = !Original_Patch.used;

    if (Patch[test_patch_id] == Original_Patch)
    {
        PRINT_ERROR(F("TEST FAILED: RAM Patch was not altered"));
        Patch[test_patch_id] = Original_Patch;
        return false;
    }

    Serial.println(F("RAM Patch intentionally altered"));

    // 3. Recupero dalla SD
    const bool resume_success =
        Archive.Resume_Patch_from_SD_to_RAM(test_patch_id);

    if (!resume_success)
    {
        PRINT_ERROR(F("TEST FAILED: unable to resume Patch"));

        // Mantiene invariato lo stato applicativo anche in caso di errore.
        Patch[test_patch_id] = Original_Patch;
        return false;
    }

    // 4. Confronto campo per campo tramite operator==
    const bool data_match = Patch[test_patch_id] == Original_Patch;

    // Ripristino finale garantito.
    // In caso di successo l'assegnazione ÃƒÆ’Ã‚Â¨ ridondante ma innocua.
    Patch[test_patch_id] = Original_Patch;

    if (!data_match)
    {
        PRINT_ERROR(F("TEST FAILED: restored Patch does not match"));
        return false;
    }

    Serial.println(F("TEST PASSED: saved and restored Patch match"));
    Serial.println(F("*** END PATCH SD ROUND-TRIP TEST ***"));
    Serial.println();

    return true;
}

void P_Invalidate_file_cache(int file_id)
{
    const bool enabled = NVIC_IS_ENABLED(IRQ_SOFTWARE) != 0;
    AudioNoInterrupts();
    PatchCache_Manager.Invalidate_file(file_id);
    Players_Manager.Refresh_cache_sources();
    if (enabled)
    {
        AudioInterrupts();
    }
}

void P_Invalidate_recording_cache(int recording_id)
{
    const bool enabled = NVIC_IS_ENABLED(IRQ_SOFTWARE) != 0;
    AudioNoInterrupts();
    PatchCache_Manager.Invalidate_file(FIRST_RECORDING_FILE + 2 * recording_id);
    PatchCache_Manager.Invalidate_file(FIRST_RECORDING_FILE + 2 * recording_id + 1);
    Players_Manager.Refresh_cache_sources();
    if (enabled)
    {
        AudioInterrupts();
    }
}

void P_Service_patch_cache(void) // Coordinate bounded cache loading and own both audio critical sections from the main loop.
{
    // Paused control callbacks indicate metadata replacement; Direct Sampling owns Flash while recording or converting.
    const bool copying_allowed = Trigger.Is_running() && !(Lilla_state == DIRECT_SAMPLING && DS_state != DS_waiting_state);
    if (!copying_allowed || NVIC_IS_ENABLED(IRQ_SOFTWARE) == 0)
    {
        return;
    }

    // Entry requires IRQ_SOFTWARE to be enabled, so each critical-section exit restores that state.
    static constexpr unsigned int CYCLE_TIME_LIMIT = 1700; // Latest permitted copy start within the audio cycle, in microseconds.
    static uint32_t last_cycle = 0;                        // Last audio cycle in which background loading attempted work.
    static uint32_t blocked_since_ms = 0;                  // Start of the current cache-reclamation grace period.
    static uint16_t blocked_mask = 0;                      // Retiring caches currently blocking a pending load.
    PatchCacheManager::CopyJob job;

    AudioNoInterrupts(); // Take a coherent snapshot while the audio callback cannot change player references.
    const uint32_t cycle = audio_update_cycle;

    // loop() can run many times per audio block: allow at most one attempt per block, and skip late attempts.
    // The CYCLE_TIME_LIMIT threshold leaves a conservative margin for up to 512 samples before the next audio deadline.
    if (cycle == last_cycle || audio_update_time_micros > CYCLE_TIME_LIMIT)
    {
        AudioInterrupts();
        return;
    }

    last_cycle = cycle;
    PatchCache_Manager.Release_unreferenced_caches(Players_Manager.Get_cache_reference_mask()); // Reuse retired buffers only after every player has released them.
    const bool copying = PatchCache_Manager.Prepare_copy(job);                                  // Reserve the next chunk; incomplete files remain unavailable to players.
    const uint16_t reclaim_mask = copying ? 0 : PatchCache_Manager.Get_reclaim_mask();          // Request space only when pending work cannot obtain a free cache.

    if (reclaim_mask != blocked_mask)
    {
        blocked_mask = reclaim_mask;
        blocked_since_ms = millis(); // Start a new grace period whenever the blocking cache selection changes.
    }
    if (reclaim_mask != 0 && static_cast<uint32_t>(millis() - blocked_since_ms) >= 20u)
    {
        Players_Manager.Fast_stop_players_using_cache(reclaim_mask); // After 20 ms, fade eligible old readers; their buffers are not freed until references disappear.
    }
    if (copying)
    {
        AudioStartUsingSPI(); // Register Flash bus use before allowing audio callbacks to run again.
    }
    AudioInterrupts(); // Perform the Flash transfer outside the audio critical section.

    if (!copying)
    {
        return;
    }
    const bool success = LillaSerialFlashFile::Read_audio_samples_background(job.file_id, job.destination, job.first_sample, job.samples); // Subdivide the reserved copy so pending audio runs between short Flash transactions.

    AudioNoInterrupts();
    AudioStopUsingSPI();                                               // Balance the SPI reservation even when the Flash read fails.
    const bool ready = PatchCache_Manager.Complete_copy(job, success); // Publish only a fully copied file; failed reads leave playback on Flash.

    if (ready)
    {
        Players_Manager.Refresh_cache_sources(); // Promote current and queued matching voices atomically to the completed PSRAM source.
    }
    AudioInterrupts(); // Restore audio processing
}

void Print_player_read_diagnostics(void)
{
    DMAMEM static PlayersManager::ReadDiagnosticsSnapshot snapshots[4];
    const bool audio_enabled = NVIC_IS_ENABLED(IRQ_SOFTWARE) != 0;

    AudioNoInterrupts();
    const auto budget = Players_Manager.Get_read_budget_status(); // Coherent scheduler snapshot while the audio IRQ is suspended.
    const bool enabled = Players_Manager.Copy_read_diagnostics(snapshots[0], snapshots[1], snapshots[2], snapshots[3]);

    if (audio_enabled)
    {
        AudioInterrupts();
    }

    Serial.printf("READ_BUDGET: limit_us=%.1f,reserved_us=%.3f,crossfade_us=%.3f,rejected_notes=%lu,retired_players=%lu,forced_protected=%lu\n", PlayerReadBudget::Limit_us, budget.reserved_us, budget.crossfade_us, static_cast<unsigned long>(budget.rejected_notes), static_cast<unsigned long>(budget.retired_players), static_cast<unsigned long>(budget.forced_protected));
    Serial.printf("READ_DIAG: %s; observed_blocks=%lu\n", enabled ? "enabled" : "disabled", static_cast<unsigned long>(snapshots[0].blocks));

    if (snapshots[0].blocks == 0)
    {
        Serial.println(F("READ_DIAG: send d, play notes, then p (report); D disables"));
        return;
    }

    Serial.println(F("READ_DIAG: estimates describe reads performed in each block at actual pitch, NOT a maximum-bend reservation or full Player CPU cost"));
    Serial.println(F("READ_DIAG: harvest_us includes assembly/reversal, counter overhead and interruptions; estimated_us models source transfers only"));
    Serial.println(F("READ_DIAG: max_underestimate is the largest positive harvest-minus-estimate gap, including assembly/diagnostic overhead; not pure memory-model error"));
    Serial.println(F("READ_DIAG: flags 1=RAM loop proxy, 2=Live PSRAM copy without zero-fill (proxy), 4=padding, 8=packet open unmodelled, 16=pending restart/edit, 32=restart executed"));
    Serial.println(F("READ_DIAG: READ_DIAG_BUDGET belongs to each snapshot cycle; headroom values are before crossfades, pre_players_us includes scheduling; minimum_us is the rejected 16-sample cost"));
    Serial.println(F("READ_DIAG: transition_kind 0=none, 1=restart, 2=edit, 3=both; mix_samples is assigned, harvests/flags describe actual execution; READ_BUDGET counters are since boot"));
    Serial.println(F("snapshot,cycle,player,harvests,max_actual_pitch,flash_reads,flash_samples,psram_reads,psram_samples,ram_reads,ram_samples,estimated_us,harvest_us,flags,uncovered_reads,transition_kind,mix_samples,available_before_us,assigned_us,minimum_us"));

    const float cycles_per_us = static_cast<float>(F_CPU_ACTUAL) / 1000000.0f;
    const char *snapshot_names[] = {"last", "peak_estimate", "restart_peak", "max_underestimate"};

    for (uint8_t snapshot = 0; snapshot < 4; ++snapshot)
    {
        const auto &data = snapshots[snapshot];
        const char *name = snapshot_names[snapshot];

        if (data.blocks == 0)
        {
            Serial.printf("READ_DIAG: %s not observed in this window\n", name);
            continue;
        }

        const auto &scheduled = data.budget; // Historical allocation travels with the same last/peak/restart/gap snapshot.
        Serial.printf("READ_DIAG_BUDGET,%s,cycle=%lu,valid=%u,limit_us=%.1f,reserved_us=%.3f,read_headroom_us=%.3f,deadline_headroom_us=%.3f,scheduler_elapsed_us=%.3f,pre_players_us=%.3f,available_us=%.3f,crossfade_us=%.3f,first_player=%u\n", name, static_cast<unsigned long>(data.cycle), static_cast<unsigned int>(scheduled.valid), PlayerReadBudget::Limit_us, scheduled.reserved_us, scheduled.read_headroom_us, scheduled.deadline_headroom_us, scheduled.scheduler_elapsed_us, scheduled.pre_players_us, scheduled.available_us, scheduled.crossfade_us, static_cast<unsigned int>(scheduled.first_player));

        for (uint8_t player = 0; player < PLAYERS; ++player)
        {
            const auto &usage = data.players[player];
            const auto &flash = usage.sources[0];
            const auto &psram = usage.sources[1];
            const auto &ram = usage.sources[2];
            const uint32_t uncovered = flash.uncovered_operations + psram.uncovered_operations + ram.uncovered_operations;
            Serial.printf("%s,%lu,%u,%u,%.4f,%lu,%lu,%lu,%lu,%lu,%lu,%.3f,%.3f,%u,%lu,%u,%u,%.3f,%.3f,%.3f\n", name, static_cast<unsigned long>(data.cycle), static_cast<unsigned int>(player), static_cast<unsigned int>(usage.harvests), static_cast<double>(usage.maximum_pitch), static_cast<unsigned long>(flash.operations), static_cast<unsigned long>(flash.samples), static_cast<unsigned long>(psram.operations), static_cast<unsigned long>(psram.samples), static_cast<unsigned long>(ram.operations), static_cast<unsigned long>(ram.samples), static_cast<double>(data.estimated_us[player]), static_cast<double>(usage.harvest_cycles / cycles_per_us), static_cast<unsigned int>(usage.flags), static_cast<unsigned long>(uncovered), static_cast<unsigned int>(scheduled.transition[player]), static_cast<unsigned int>(scheduled.mix_samples[player]), scheduled.available_before_us[player], scheduled.assigned_us[player], scheduled.minimum_us[player]);
        }

        Serial.printf("READ_DIAG_TOTAL,%s,cycle=%lu,estimated_us=%.3f,harvest_us=%.3f,uncovered_reads=%lu,restarted_players=%u,harvest_minus_estimate_us=%.3f\n", name, static_cast<unsigned long>(data.cycle), static_cast<double>(data.total_estimated_us), static_cast<double>(data.total_harvest_us), static_cast<unsigned long>(data.uncovered_operations), static_cast<unsigned int>(data.restarted_players), static_cast<double>(data.total_harvest_us - data.total_estimated_us));
    }
}

void Require_FRAM(byte result)
{
    if (result == LillaFRAM_2x512::ERROR_0)
    {
        return;
    }

    Serial.print(F("FRAM operation failed, error "));
    Serial.println(result);

    P_Quiesce_audio_players();
    AudioNoInterrupts();
    Display_Storage.FRAM_io_error_popup();
    // Do not run subsequent save, erase or cache-publication steps after a failed access.
    while (true)
    {
        delay(10);
    }
}
