# MP3 import

The SD import accepts `.mp3` (case insensitive) in `/LILLA_AUDIO`, alongside RAW, WAV and AIFF. Files must use 44100 Hz and one or two channels. Stereo is averaged to mono; the output is signed 16-bit little-endian PCM, stored as `<basename>.raw` in SerialFlash. Other sample rates are rejected, with details on Serial; no resampling is performed.

The existing limit is 3 MiB of decoded mono PCM per file (approximately 35.7 seconds). Longer files are truncated. Names shared by different input formats produce the same RAW filename, so the existing duplicate handling applies. The import confirmation still erases the previous Flash audio files and recordings.

The decoder is `dr_mp3` 0.7.4 from https://github.com/mackron/dr_libs, pinned in `platformio.ini` to commit `dfe8377631000664666519fdb83da193fd8037f4`. The upstream source includes its public-domain/MIT-0 licensing terms. Only the MP3 implementation is compiled. PlatformIO downloads the dependency during the first build.

Decoding uses SD callbacks and small PCM blocks, without loading the whole file into RAM. The scan decodes up to the output limit to determine the Flash allocation size; the copy pass opens and scans the stream again, then rewinds and writes it. Consequently, MP3 inventory and import are slower than PCM import. CBR, VBR and ID3 metadata are handled by the decoder; malformed streams may be resynchronized by the library rather than rejected in full.

`scripts/mp3_flash.py` derives the build's linker script from the installed Teensy script and places `Mp3Import.cpp` code and constant tables in program Flash. This preserves the fast RAM used for playback. The generated script stays under `.pio/build`; the framework installation is unchanged.

Build: `platformio run -e teensy41`.

Host regression: `python tests/zero_raw_regression.py --mp3`. Requires a C++20 compiler, the installed PlatformIO dependency and FFmpeg on PATH (or under `.pio/test-tools`). The test generates synthetic mono/stereo CBR/VBR MP3s, compares imported PCM with a separate in-memory decode, and exercises metadata, missing Xing tags, the size limit, unsupported rates, invalid files and I/O failures. The same run checks existing RAW/WAV/AIFF imports.

For hardware testing, place MP3s at 44.1 kHz with distinct basenames in `/LILLA_AUDIO`, load the new firmware and run the existing import command. Check the imported lengths, listen to mono and stereo sources, and try a file longer than 36 seconds. Include a 48 kHz file to verify rejection. Host tests cannot verify physical SD/Flash timing or playback quality.
