# User audio preparation on SD (development)

This feature adds **IMPORT AUDIO FILES FROM /USER_AUDIO TO /LILLA_AUDIO** to SETUP. Put source files directly in `/USER_AUDIO`, insert the SD, select the command and confirm YES. NO is selected initially. The red confirmation warns that files with the same destination name will be overwritten.

MIDI and Trigger are stopped, active Players are drained, and audio processing is suspended until preparation ends. The previous audio/MIDI state is restored afterwards. The screen asks the user to wait; the Serial terminal lists each source and destination, successful copies/conversions, skipped files, errors, and the index result.

## Files and conversion

- RAW: signed 16-bit little-endian mono PCM at 44.1 kHz; copied without changing its samples.
- WAV: uncompressed 16-bit PCM, mono or stereo, at 44.1 kHz.
- AIFF/AIF: uncompressed 16-bit PCM, mono or stereo, at 44.1 kHz; converted from big endian.
- MP3: decoded with the existing importer and resampled to 44.1 kHz when needed.
- Stereo is averaged to mono with the existing conversion rules. All outputs are signed 16-bit little-endian mono RAW at 44.1 kHz.

The destination keeps the source stem and uses `.raw`: `Piano.wav` becomes `/LILLA_AUDIO/Piano.raw`. Originals in `/USER_AUDIO` are retained. Subdirectories and unsupported extensions are skipped and reported. Files already in `/LILLA_AUDIO` whose names are not replaced are retained.

Files with the same stem map to the same RAW name. When multiple inputs have that stem, the last successfully processed one wins; processing follows SD directory order and every overwrite is reported. Use unique stems when all source files must be retained as separate RAWs.

The SD operation copies/converts complete files rather than applying the existing Flash import's 3 MiB limit. Empty or odd-length RAWs and unsupported/corrupt audio are reported as errors. Conversion writes a temporary file first. An existing destination is replaced only after that write is complete; a failed rename attempts to restore the previous file. An interrupted replacement may leave `.user-import.previous`; subsequent replacement is blocked and the terminal reports the recovery requirement. The original source remains available in `/USER_AUDIO`.

## Alphabetical index

After processing, `/LILLA_AUDIO/.lilla-index` describes all valid RAW files currently in the destination, including previously existing files. Capacity is **1,024 entries**. Ordering is case-insensitive by filename, with the original spelling retained. Filenames can use up to 255 bytes including `.raw`.

The binary index uses little-endian fields. Its 24-byte header contains `magic=0x4C524157`, version 1, entry count, entry size 264, sample rate 44100, channel count 1 and bit depth 16. Each 264-byte entry contains a zero-terminated, zero-padded filename in 256 bytes, a 32-bit byte count and a 32-bit sample count. Entry N starts at byte `24 + N * 264`.

These records allow later code to obtain a filename and sample length without rescanning the directory or opening each audio file. They do not store physical SD block addresses. No Player or SOUND_EDIT integration is introduced by this first part.

The previous index is invalidated before file changes. A complete replacement is published after scanning and sorting the destination. If capacity is exceeded, a destination RAW is malformed, memory is insufficient or an index write fails, no partial index is published. Conversion errors can coexist with a valid index describing the files actually present; the terminal reports both outcomes.

The temporary sorting allocation is 270,336 bytes in RAM2 and is released after index creation. Conversion and index allocation happen sequentially. Nothing is stored in FRAM or copied into the patch's PSRAM cache by this command.

## Verification

Host tests cover mono/stereo WAV and AIFF, actual MP3 conversion at 44.1/48/22.05 kHz, complete RAW copies larger than 3 MiB, overwrites, write/rename failures, preservation of originals, alphabetic metadata, 1,024 entries and overflow. They also exercise NO/YES, suspension during preparation, failed Player draining and restoration of the original audio/MIDI state. Physical SD timing, display appearance and removal during operation still need hardware verification.
