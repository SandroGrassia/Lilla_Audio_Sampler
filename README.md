<p align="center">
	<img width="500" alt="LILLA Audio Sampler" src="doc/assets/images/logo_0.jpg">
</p>

# LILLA Audio Sampler 2026

This repository contains the firmware project for the LILLA Audio Sampler, a Teensy 4.1 based hardware sampler with up to 16 playback voices, designed and assembled in Italy.

The codebase targets a Teensy 4.1 running at 600 MHz and is organized as a standalone PlatformIO project with custom audio, display, storage, MIDI, and user-interface modules.

<p align="center">
    <img width="1000" alt="LILLA Audio Sampler in blue" src="doc/assets/images/lilla_blue_1.jpg">
</p>

Read the illustrated [User Guide](USER_GUIDE.md) for connections, controls and operating procedures.

## What This Project Does

LILLA is a polyphonic, multitimbral, multi-MIDI audio sampler designed to work with imported audio, self-recorded audio, and live input.

| Mode | Features |
| --- | --- |
| Performance | Up to 16 playback voices and 200 patches, each with up to eight sounds, individual MIDI channels and keyboard ranges. |
| Sampler | Mono/stereo recording into Flash, conversion into playable sounds and WAV export to microSD. |
| Live Sampler | Circular PSRAM recording with approximately 40 seconds in mono or 20 seconds in stereo, adjustable feedback and capture of selected regions into patches. |
| MIDI Loop | Four tracks of MIDI events, played through the current patch and stored on microSD. |

Sound shaping includes sample trimming, forward/reverse and loop playback, envelopes, instrument filters and modulation, a common low-pass filter, stereo delay, bit reduction and downsampling. Separate main and pre-listening routes support auditioning and performance.

## Operating Modes

The **Modes** selector chooses Performance, Sampler, Live Sampler or MIDI Loop. Each mode offers a different way to use the same sound engine: play prepared patches, record a take, explore incoming audio or build a repeating MIDI arrangement.

### Performance

Performance is the main page for playing and organizing patches. Each patch contains up to eight sound slots with their own source, MIDI channel, root key, keyboard range, gain and pan. Assign overlapping ranges to build layers, separate ranges to create keyboard splits, or different MIDI channels to play several parts from a controller or sequencer. Up to 16 playback voices are shared between the active sounds, including layers and release tails.

The root key establishes the reference pitch of each sound. The keyboard range determines which notes trigger it, while precedence and lock settings provide additional control over voice allocation and selected performance adjustments. Patch volume, the common low-pass cutoff, resolution and downsampling are available for shaping the performance as a whole.

Press **S1-S8** to edit an active sound. Sound Edit provides source selection, waveform trimming, pitch, gain, pan, ADSR envelope and playback direction. One-shot modes suit hits and natural decays; forward, reverse and alternating loops extend a selected region into a sustained sound. Noclick provides boundary smoothing where supported. Auto Tune analyses the selected region and adjusts its pitch toward a chromatic note; the result can then be refined by ear.

The instrument VCF adds low-pass, high-pass, band-pass or notch filtering, resonance and modulation. Save the patch to retain sound edits, or clone it and develop a variation without replacing the original. See [Performance](USER_GUIDE.md#performance) and [Editing a sound](USER_GUIDE.md#editing-a-sound) for the complete workflow.

### Sampler

Sampler records the line input as a mono or stereo take in Flash. Use **PAUSE+REC** to monitor the source and set the input level, start the take with **MONO_REC** or **STEREO_REC**, then press **STOP** to finish. The resulting recording can be auditioned and used as material for a playable sound.

**MAKE_RAW** converts recording material into the source format used by patches. **EXPORT_WAV_TO_SD** writes a 16-bit, 44.1 kHz WAV to microSD for editing, sharing or archiving on a computer. This makes Sampler useful for recording an instrument, voice, percussion hit or complete phrase before deciding how to map it to the keyboard.

Recordings occupy persistent Flash storage, while WAV exports are written to `/LILLAWAV_EXPORT`. Keep an external copy before replacing the audio library or performing destructive storage operations. See [Recording with Sampler](USER_GUIDE.md#recording-with-sampler).

### Live Sampler

Live Sampler records continuously into a circular PSRAM buffer: approximately **40 seconds in mono** or **20 seconds in stereo**. As recording proceeds, new material replaces the oldest audio. It is intended for playing and transforming an incoming source, finding useful moments in an ongoing recording, and turning those moments into new patch sounds.

The waveform view provides a window into the buffer. **From** sets the playback start, **To** adjusts the selected region, and **Step** changes the editing increment. The displayed window can be shortened to inspect a transient or expanded for a wider view of the recording. Playback modes let you audition a region once or repeat it as a loop.

Start-point behavior determines the relationship between playback and recording. **SYNC** starts playback close to the current write position; relative offsets move that start ahead of or behind it. **FIXED** anchors it to a location in the circular buffer. While recording, FWD playback continues across buffer wraps in both SYNC and FIXED, allowing held notes to continue beyond one buffer length. During the first fill, playback still stops with a fade if it reaches material that has not yet been recorded. After recording stops, normal one-shot FWD behavior resumes.

**Feedback** mixes the returning playback signal with the new input before recording, so earlier material can be recaptured and progressively transformed. Its level controls how strongly previous passes contribute to the next one.

A stereo-linked recording compressor controls the combined input and feedback level; toggle **COMPRESSOR ON/OFF** with Select.

To keep a region, stop recording, choose a loop mode, refine its boundaries and press **S1-S8** to select the destination slot. A MIDI key sets the captured sound's root key. Stereo captures can use adjacent slots for their left and right channels. Return to Performance to review the new patch and save it, which writes the captured audio to Flash. The live buffer itself is temporary and does not survive power-off. See [Live Sampler](USER_GUIDE.md#live-sampler).

### MIDI Loop

MIDI Loop records note events into **four tracks** and plays them through the current patch. It stores the performance as MIDI data, so changing the patch's sounds, filters or keyboard mapping can change the arrangement without recording the notes again.

Track 1 is the master and establishes the loop duration. Record the first phrase with **Rec 1**, close it by pressing Rec 1 again, then add parts using Rec 2, Rec 3 and Rec 4. Recording over an occupied track replaces its events; recording Track 1 again also clears the other tracks because it defines a new master loop.

Tracks can be started or stopped individually. Their **LEVEL**, **SHIFT** and **TRANSP** parameters adjust playback level, timing offset and note transposition. The Tempo control changes the timing of the MIDI sequence, while the Loop control selects saved loops and controls group playback. Tempo changes affect event timing rather than time-stretching sample audio.

Save an arrangement or save a new variation to microSD in `/LILLALOOP`. Preserve its patch and audio sources alongside the loop files when archiving a complete session. See [MIDI Loop](USER_GUIDE.md#midi-loop).

## Tools

Set the **Tools** selector to the desired position and press the Tools button to open its page. Mixer, Delay, Setup and Test provide routing, effects, global configuration and MIDI diagnostics.

### Mixer

Mixer controls how sound sources and the line input reach the main output and the separate monitoring output. Source gain or mute, pan and output routing let you balance the material that is heard in the performance and the material used for pre-listening.

Turn Select to choose a source column, press it to enter that source's fields, then use Select and Value to choose and adjust parameters. Press Select again to leave the fields. Separate **LINEOUT** and **MONITOR** routes are useful when auditioning material or checking a source before adding it to the main mix.

If a source is silent, check its output route and mute/gain settings as well as its MIDI channel, keyboard range and patch volume. See [Mixer](USER_GUIDE.md#mixer).

### Delay

Delay is a stereo effect with selectable sound routing, feedback, delay time and a left/right time relationship. Choose which patch sounds feed it, then set the repeat spacing and feedback to move from isolated echoes to longer repeating textures. Its settings are retained when the patch is saved.

Modulation varies the delay time. Source, frequency, depth and left/right phase controls shape how the repeats move, while the difference between the two channel times creates stereo separation. Start with one routed sound and low feedback, listen to short notes with gaps between them, then increase feedback or modulation to build the effect.

The Delay page controls the echo effect's feedback; the Live Sampler's Feedback control determines how much playback is mixed back into the recording. See [Delay](USER_GUIDE.md#delay).

### MIDI Monitor

The **Test** position opens the MIDI Monitor. It displays incoming MIDI messages and their associated data, making it possible to check reception of notes, pitch bend, aftertouch and Control Change messages from a connected controller.

Use it to verify that a keyboard or sequencer is transmitting and that the expected channel, note or controller data reaches LILLA. If note messages appear but no sound is heard, continue with the patch's MIDI assignments, keyboard ranges, audio sources and Mixer routes. See [Check incoming MIDI](USER_GUIDE.md#check-incoming-midi).

### Setup

Setup groups global playing preferences, MIDI Control Change assignments and storage operations. **FIRST OCTAVE** changes the octave-number convention used in note names. **CONTROL CHANGE ASSIGNMENT** maps external MIDI controls to the gains of Sound 1-8 and the common low-pass cutoff, allowing compatible controller knobs and sliders to adjust them while playing.

**KEY STEP** sets the pitch interval between adjacent MIDI keys. The normal setting is one semitone; smaller steps make the keyboard suitable for microtonal playing and finer pitch movement around a sound's root key:

| KEY STEP | Interval per MIDI key | MIDI-key steps for one octave |
| --- | --- | --- |
| 1 semitone | 100 cents | 12 |
| 1/2 semitone | 50 cents | 24 |
| 1/4 semitone | 25 cents | 48 |
| 1/8 semitone | 12.5 cents | 96 |

For example, moving 12 MIDI keys above the root raises pitch by an octave at the normal setting, but by only six semitones with KEY STEP set to 1/2. This changes the pitch spacing of keyboard playback; FIRST OCTAVE only changes the displayed octave names. KEY STEP is stored as a global setting, so return it to one semitone when conventional chromatic mapping is required.

Setup also provides audio import from `/LILLA_AUDIO`, numbered configuration and recording backups in `/LILLABACKUP`, restoration from the backup root, and factory reset. Import and restore can replace existing material: preserve the relevant audio library, patches and loop files before using those operations. See [Setup and MIDI controls](USER_GUIDE.md#setup-and-midi-controls) and [Backup and restore](USER_GUIDE.md#backup-and-restore).

## Audio Files and Storage

- Internal audio format: signed 16-bit PCM at 44.1 kHz.
- Import from microSD: mono RAW, mono/stereo 16-bit PCM WAV and AIFF at 44.1 kHz, and MP3 with conversion to 44.1 kHz. See [MP3 import](docs/mp3-import.md) for supported rates and implementation details.
- Import duration limit: approximately 35.7 seconds per source file; longer files are truncated.
- Flash stores imported audio and Sampler recordings; PSRAM provides live recording and playback caches; FRAM stores patch and configuration data.
- Configuration and recording backup/restore are available through microSD. A complete archive also requires separate copies of the imported audio library and MIDI-loop files; see [Backup and restore](USER_GUIDE.md#backup-and-restore).

## Hardware Summary

The firmware is written for a hardware platform built around:

- Teensy 4.1
- Teensy Audio Adaptor Rev D
- 64 MB SPI Flash memory (SPI)
- 32 MB total PSRAM (Quad-SPI)
- 128KB FRAM (I2C)
- ILI9341 display

## Connections and Buttons

| Connection | Specification |
| --- | --- |
| Line in | 3.5 mm jack; stereo line / stereo dynamic microphone input. |
| Line out | 3.5 mm jack; stereo line output, 3.1 Vpp. |
| Phones line | 3.5 mm jack; main stereo headphone output. |
| Phones pre-listen | 3.5 mm jack; stereo pre-listening headphone output. |
| MIDI IN / OUT | 3.5 mm jacks. |
| Gate IN / OUT | 3.5 mm jacks, +5 V. |
| USB-C | +5 V DC power and programming. |
| microSD | Audio import/export, backups and MIDI-loop storage. |

Dedicated buttons provide firmware upload mode and power on/off.

MIDI OUT and Gate IN/OUT are physically available and accessible through classes included in the codebase, but no user-facing features currently use them. They are available for future development.

## Repository Layout

- [src](src): application entry point and firmware sources
- [include](include): global headers and shared declarations
- [lib](lib): custom modules for audio, display, control, storage, and routing
- [doc/assets/images](doc/assets/images): images used by the README and User Guide
- [docs](docs): technical notes and implementation details
- [USER_GUIDE.md](USER_GUIDE.md): illustrated operating guide
- [platformio.ini](platformio.ini): PlatformIO build configuration

## Build

Build the Teensy 4.1 firmware with PlatformIO:

```sh
pio run -e teensy41
```

The project targets Teensy 4.1 at 600 MHz. Building does not upload firmware to the instrument.

## Links

- [www.lillasampler.it](https://www.lillasampler.it/)
- [Facebook page](https://www.facebook.com/Lilla.audio.sampler)
- [Tindie product page](https://www.tindie.com/products/lillasampler/lilla-audio-sampler-2/)

## Acknowledgements

Special thanks to:

- **Giuliano Cardinali**, for valuable suggestions on hardware and software design and implementation.
- **Andreas Huelsmann**, for suggestions on MIDI Loop functionality and for beta testing it.
- **Stefano Spada**, for contributing to the development of advanced features and improving usability.
- **Thomas Spada**, for helping define the core features and improve usability.
- **Andrea Lombardini**, for the C++ lessons.
- **[François Best](https://francoisbest.com/)**, for guidance on using the MIDI.h library within a class.

## License

This repository includes a [LICENSE](LICENSE) file. See it for the applicable terms.
