# User audio preparation on SD (firmware 7.2.4)

Select **CONVERT SD AUDIO FILES** in SETUP after putting source files directly in `/EXTERNAL_AUDIO` and inserting the SD. The green confirmation identifies the source and destination folders and explains that originals are retained and existing destination files are skipped. Choose GO to start or CANCEL to return. A green popup displays "FILE IMPORT IN PROGRESS" and "PLEASE DON'T POWER OFF LILLA" throughout indexing and import; SETUP is restored when the operation ends.

MIDI and Trigger are stopped, active Players are drained, and audio processing is suspended until preparation ends. The previous audio/MIDI state is restored afterwards. The Serial terminal lists each source and destination, successful copies/conversions, skipped files, errors, and the index result.

## Files and conversion

- RAW: signed 16-bit little-endian mono PCM at 44.1 kHz; copied without changing its samples.
- WAV: uncompressed 16-bit PCM, mono or stereo, at 44.1 kHz.
- AIFF/AIF: uncompressed 16-bit PCM, mono or stereo, at 44.1 kHz; converted from big endian.
- MP3: decoded with the existing importer and resampled to 44.1 kHz when needed.
- Stereo is averaged to mono with the existing conversion rules. All outputs are signed 16-bit little-endian mono RAW at 44.1 kHz.

The destination keeps the source stem and uses `.raw`: `Piano.wav` becomes `/LILLA_AUDIO/Piano.raw`. Originals in `/EXTERNAL_AUDIO` are retained. Subdirectories and unsupported extensions are skipped and reported. Existing destination files are never overwritten. If `Piano.raw` exists, `Piano.wav`, `Piano.aiff`, `Piano.mp3` and `Piano.raw` are skipped before conversion; matching follows the SD filesystem's case-insensitive filenames.

Files with the same stem map to the same RAW name. When multiple inputs have that stem, the first successfully processed one is retained; later inputs are skipped. Processing follows SD directory order and every skip is reported. Use unique stems when all source files must be retained as separate RAWs.

The SD operation copies/converts complete files rather than applying the existing Flash import's 3 MiB limit. Empty or odd-length RAWs and unsupported/corrupt audio are reported as errors. Conversion writes a temporary file first, then renames it to the new destination only after the write is complete. Failed writes or renames do not publish a partial audio file. Original source files remain available in `/EXTERNAL_AUDIO`.

## Alphabetical index

Before importing, the command creates `/LILLA_AUDIO/.lilla-index` if it is absent, describing the existing RAW files. If this initial creation fails, no audio is imported. An existing index is retained until the final update. After processing, the index is rebuilt and describes all valid RAW files currently in the destination, including previously existing files. Capacity is **4,096 entries**. Ordering is case-insensitive by filename, with the original spelling retained. Filenames can use up to 255 bytes including `.raw`.

The index contains one entry per file actually present, not 4,096 preallocated entries. It is a binary file, not a text listing; file browsers that hide dot-prefixed names may hide it. `INDEX ERROR` followed by `index=UNAVAILABLE` means no valid index was published, even if the RAW conversions succeeded.

The binary index uses little-endian fields. Its 24-byte header contains `magic=0x4C524157`, version 1, entry count, entry size 264, sample rate 44100, channel count 1 and bit depth 16. Each 264-byte entry contains a zero-terminated, zero-padded filename in 256 bytes, a 32-bit byte count and a 32-bit sample count. Entry N starts at byte `24 + N * 264`.

SOUND_EDIT uses these records to obtain filenames and sample lengths without rescanning the directory. They do not store physical SD block addresses.

The previous index is invalidated when rebuilding the final catalog. A complete replacement is published after scanning and sorting the destination. If capacity is exceeded, a destination RAW is malformed, memory is insufficient or an index write fails, no partial index is published. Conversion errors can coexist with a valid index describing the files actually present; the terminal reports both outcomes.

Sorting uses two temporary files on SD and merges progressively longer sorted runs. Only two index records (528 bytes total), plus file handles and filesystem buffers, are needed during each merge; there is no allocation proportional to the number of files. The binary index format and capacity remain unchanged. Nothing is stored in FRAM or copied into the patch's PSRAM cache by this command.

## Verification

Host tests cover mono/stereo WAV and AIFF, actual MP3 conversion at 44.1/48/22.05 kHz, complete RAW copies larger than 3 MiB, duplicate skipping, read/write/rename failures, preservation of originals, alphabetic metadata, 4,096 entries and overflow. Index tests enumerate files in reverse order, reject large nothrow array allocations, cover empty/single/odd-sized catalogs and verify recovery after failed sorting or publication. They also exercise the preparation workflow, initial/final indexing, suspension during preparation, failed Player draining and restoration of the original audio/MIDI state. Physical SD timing, display appearance and removal during operation still need hardware verification.

## Browsing and playing SD sounds

SOUND_EDIT offers BROWSE_FLASH and BROWSE_SD when their repositories are available. Flash RAW/REC files and indexed SD RAW files are browsed alphabetically. Existing Sound assignments are preserved when opening the editor. A new Sound using the firmware default starts with the first Flash file, or the first indexed SD file when Flash has no user audio.

Selecting an SD file loads its initial window into an existing PSRAM cache: the shorter of the file length and cache capacity. All 16 existing Players can audition this window through MIDI, with playback speed limited to 8x including pitch bend and vibrato. There are no continuous SD reads during notes. The waveform zooms to the From?To selection, matching Flash editing. During SD preview, SAVE FROM replaces FROM and precedes the starting sample number; TO marks the end of the portion to audition and save.

During preview, filename, resolution, downsampling, LPF cutoff, gain, pitch, MIDI channel, From, To and Step remain editable. Pan is centered, attack is FAST 0.00, decay is 1 second, sustain is 100%, release is 2 seconds, play mode is ONCE FWD and NOCLICK is zero. These locked controls become available after SAVE.

SAVE retains the selected portion in PSRAM and creates a persistent source-and-trim reference in `/LILLA_AUDIO/.lilla-sources`. It writes no audio to Flash. Keep this directory together with the original RAW files: committed Sounds use these references to reload their portions after startup. The alphabetical index may be rebuilt without changing saved Sound identities. Commit the definitive patch from PERFORMANCE as usual. Leaving preview with RETURN without SAVE restores the previous Sound.

Missing audio follows the Flash behavior: the Sound retains its reference and parameters, and remains inactive if no valid cached audio is available. SOUND_EDIT shows an empty waveform and the red message `AUDIO FILE UNAVAILABLE`. Audio already loaded in PSRAM can continue playing even after SD removal. When neither repository has audio, the editor displays the SETUP import instruction instead.

If the SD is removed while browsing, attempting to select another file displays `INSERT SD` for two seconds, then redraws the previous selection. The failed selection leaves its source reference, trim and cached audio unchanged.

For direct import into Flash, use SETUP > `IMPORT AUDIO FILES FROM SD` and place source files in `/FLASH_AUDIO`. SD library preparation instead reads `/EXTERNAL_AUDIO` and writes converted RAW files plus the index to `/LILLA_AUDIO`.
