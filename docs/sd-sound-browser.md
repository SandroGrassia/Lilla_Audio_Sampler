# SD Sound browser (development)

At startup, LILLA scans files directly inside `/LILLA_AUDIO` and writes a case-insensitive alphabetical index to `/LILLA_AUDIO/.lilla-index` on the SD card. The index supports 1,000 files and is rebuilt on startup using a temporary file; its 64 KB sorting workspace is released afterwards. Subdirectories are not scanned. The existing Patch, Sound, RAW-name and Recording records retain their addresses.

## Controls

- Press an unused Sound button in Performance to prepare a new Instrument. If SD audio is indexed, browsing starts at its first filename. Otherwise the first external Flash sample is selected when available.
- `BROWSE_FLASH` appears in Sound Edit when external RAW files are present. It selects the first Flash file; the existing File control browses Flash sources. The firmware fallback alone does not make this command appear.
- `BROWSE_SD` appears when the SD index is nonempty. Value selects files in alphabetical order. Playback goes to the pre-listening output directly from SD through independent RAM2 ring buffers allocated for the selected format; it does not write Flash or load the complete file into PSRAM.
- MIDI audition is available immediately after opening a candidate, before import: up to **16 simultaneous notes for RAW/WAV/AIFF**, or **2 for MP3**, each limited to **4x pitch**. It uses the current Instrument MIDI channel and root key (channel 1 and C4 for a new Instrument), responds to velocity and Note Off, and replaces a voice when all voices are occupied. The first MIDI Note On ends automatic preview; held notes repeat the selected range. MIDI controls only publish requests; file reads and seeks run outside audio interrupts.
- From and To select inclusive sample boundaries, with the existing minimum of 100 samples. Step selects 1, 10, 100, 1,000, 10,000 or 44,100 samples. Preview repeats the selected range. Press Value to restart the current selection after stopping. After an unavailable-file warning, start browsing Flash or SD again to dismiss it. Panic also stops SD preview.
- `IMPORT` copies only the selected range into a PSRAM cache and assigns it to the Instrument. The range must fit one existing patch-cache buffer; its maximum duration is displayed. Stereo files are mixed to mono, matching the existing Flash importer.
- `RETURN` leaves SD browsing without assigning the candidate. A new Instrument is created only by successful import or selection from Flash. Existing Sounds remain unchanged while browsing SD.

Save the Patch from **PERFORMANCE** using the normal Save command to retain its Instrument assignment. Import stores the original SD filename and range in CRC-protected FRAM records. Saved Sounds reload their cropped source into PSRAM on startup and Patch changes; Instrument playback becomes available when the cache is complete. Keep the source files on the SD card with the same names and sizes. Missing sources retain their saved metadata and stay inactive at startup. In Sound Edit, a missing source displays **AUDIO FILE UNAVAILABLE** on red at the center of the waveform area. This warning remains latched until Flash or SD browsing begins; other parameter edits do not clear it. The selected source is checked every 500 ms while its page is open, and SD streaming errors also latch the warning. A source already imported into PSRAM can continue sounding from its cache even if the SD is removed.

## Formats and bounds

RAW is signed 16-bit little-endian mono PCM at 44.1 kHz. WAV and AIFF support uncompressed 16-bit mono/stereo at 44.1 kHz. MP3 uses the existing decoder and resampler. SD preview and trimming are not limited by the Flash importer's 3 MiB cap. MP3 opening and seeking require decoding and can take longer than PCM, especially for long files. The two MP3 decoder states share an immutable resampling filter and use 16 KB compressed-input buffers to bound memory use. The current voice limit is displayed in the browser. Real-time performance with simultaneous voices at high pitch depends on decoder workload and SD latency; verify this on hardware.

FRAM retains 20,800 bytes for 260 source records; the index no longer occupies FRAM. The SD index contains up to 1,000 filenames of at most 63 bytes including their extension. An overflow, overlong name or index I/O failure produces an explicit error and disables the incomplete index. SD imports share the existing 259 user audio IDs with Flash samples and Live captures.

Configuration backup versions 8 and 9 include SD source references. Older backups remain readable and clear these references when restored. Source audio remains in `/LILLA_AUDIO`; it must be retained separately from the configuration backup. Factory reset clears source references. The alphabetical index is rebuilt and is not backed up.

## Validation

Host regressions exercise alphabetical ordering, FRAM separation, trimmed reads, persistence, absent SD, interrupted metadata writes, preview buffering, simultaneous notes at different pitches, the 4x pitch cap, voice replacement, Note Off and conditional menus. MP3 seek tests compare output against the normal decoder stream at all supported resampling rates. Firmware compilation checks RAM1, RAM2 and PSRAM capacity.

Listening, display behavior, SD-card latency, rapid encoder changes, Patch changes and power-cycle recovery still require verification on the physical Teensy sampler before release.
