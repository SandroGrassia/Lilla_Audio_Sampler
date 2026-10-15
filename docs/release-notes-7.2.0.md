# LILLA Audio Sampler 7.2.0

Date: 15 October 2026. Firmware: `Lilla_v7_2_0.hex` for Teensy 4.1.

## SD audio library

- Prepare an SD library from RAW, WAV, AIFF/AIF and MP3 files with SETUP > CONVERT AUDIO FILES STORED IN SD /EXTERNAL_AUDIO. Conversion starts immediately and pauses audio/MIDI until complete.
- Originals remain in `/EXTERNAL_AUDIO`; converted mono RAW files are stored in `/LILLA_AUDIO`. Existing destination files are preserved. Serial reports conversions, skipped files and errors.
- An alphabetical index supports up to 1,024 audio files. SD conversion handles complete files rather than truncating to the Flash import limit.

## Sound Edit and playback

- BROWSE_FLASH and BROWSE_SD provide alphabetical source selection when the corresponding library contains audio.
- Audition an SD source through the existing Players from a window limited by the per-source PSRAM cache. Adjust From, To and Step, then SAVE the selected portion and save the patch in Performance.
- Saved SD Sounds retain a persistent source-and-trim reference and reload from SD after startup. Keep `/LILLA_AUDIO`, including its hidden `.lilla-sources` directory, backed up together. Saving an SD selection does not copy its audio into Flash.
- PATCH UPLOADING now also indicates SD-to-PSRAM loading. Missing sources and card removal produce explicit feedback while preserving the previous selection where possible.

## Folder changes

- `/FLASH_AUDIO`: source files for direct import into Flash, which replaces the Flash library and recordings after confirmation.
- `/EXTERNAL_AUDIO`: source files for SD conversion.
- `/LILLA_AUDIO`: prepared SD RAW library, index and saved source references.

## Release files

The repository root contains only the current firmware HEX; older binaries remain available in their GitHub Releases and Git history. README and the English User Guide (Markdown and PDF) describe the new workflows. Physical SD timing, listening and card-removal behavior still require verification on the instrument.

## Validation

The Teensy 4.1 firmware build succeeded. All 48 host regression suites passed, including optional MP3 conversion cases; the restore-menu fixture was updated for the new Setup entry position and rerun successfully. The rendered 44-page PDF and release file consistency were checked.
