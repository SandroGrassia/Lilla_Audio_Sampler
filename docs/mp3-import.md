# MP3 import

The SD import accepts `.mp3` (case insensitive) in `/LILLA_AUDIO`, alongside RAW, WAV and AIFF. Mono and stereo MP3s at all standard MP3 sample rates (8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100 and 48000 Hz) are converted to 44100 Hz. Stereo is averaged to mono; the output is signed 16-bit little-endian PCM, stored as `<basename>.raw` in SerialFlash. Corrupt or undecodable MP3s can still be rejected; support for all sample rates does not make every damaged file readable. WAV and AIFF import requirements are unchanged.

Files already at 44100 Hz retain the original bit-exact decoding/downmix path. Other rates use a streaming 64-tap Blackman-windowed sinc filter with exact rational phases and a cutoff at 94% of the lower Nyquist frequency, providing a transition band for anti-aliasing and suppressing interpolation images. Integer source positions preserve duration and pitch across read blocks. Boundaries are zero-padded, output length is rounded up to the next sample, and the result is rounded and saturated to 16 bits. Filter history and coefficients use heap memory, with at most 441 phases (112896 bytes of coefficients), independent of file duration.

The existing limit is 3 MiB of decoded mono PCM per file (approximately 35.7 seconds). Longer files are truncated. Names shared by different input formats produce the same RAW filename, so the existing duplicate handling applies. The import confirmation still erases the previous Flash audio files and recordings.

The decoder is `dr_mp3` 0.7.4 from https://github.com/mackron/dr_libs, pinned in `platformio.ini` to commit `dfe8377631000664666519fdb83da193fd8037f4`. The upstream source includes its public-domain/MIT-0 licensing terms. Only the MP3 implementation is compiled. PlatformIO downloads the dependency during the first build.

Decoding uses SD callbacks and small PCM blocks, without loading the whole file into RAM. The scan decodes enough input frames to reach the output duration limit, plus filter lookahead, to determine the Flash allocation size; the copy pass opens and scans the stream again, then rewinds and writes it. Consequently, MP3 inventory and import are slower than PCM import, and resampling adds processing time. CBR, VBR and ID3 metadata are handled by the decoder; malformed streams may be resynchronized by the library rather than rejected in full.

`scripts/mp3_flash.py` derives the build's linker script from the installed Teensy script and places `Mp3Import.cpp` code and constant tables in program Flash. This preserves the fast RAM used for playback. The generated script stays under `.pio/build`; the framework installation is unchanged.

Build: `platformio run -e teensy41`.

Host regression: `python tests/zero_raw_regression.py --mp3`. Requires a C++20 compiler, the installed PlatformIO dependency and FFmpeg on PATH (or under `.pio/test-tools`). The test generates synthetic mono/stereo CBR/VBR MP3s, compares 44100 Hz imported PCM with a separate in-memory decode, and compares resampled output at all other standard rates against FFmpeg. It verifies exact output length, block-size independence, filter passband/stopband response, metadata, missing Xing tags, the output size limit, invalid files and I/O failures. The same run checks existing RAW/WAV/AIFF imports.

For hardware testing, place MP3s at different rates with distinct basenames in `/LILLA_AUDIO`, load the new firmware and run the existing import command. Check the imported lengths, listen to mono and stereo sources, and try a file longer than 36 seconds. Include 48 kHz and 22.05 kHz files to verify resampling without pitch/speed changes. Host tests cannot verify physical SD/Flash timing or playback quality.
