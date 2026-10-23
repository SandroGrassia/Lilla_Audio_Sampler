# LILLA Audio Sampler 7.2.8

## Expanded WAV import

WAV files can now be imported with a wider range of resolutions and sample rates:

- PCM: 8, 16, 24 or 32 bits.
- Floating point: 32 or 64 bits.
- Sample rates: 8 to 192 kHz.
- Mono or stereo, with standard or extensible WAV headers.

Both **READ SD AUDIO FILES** and **IMPORT AUDIO FILES FROM SD** convert supported WAV files to mono, signed 16-bit RAW at 44.1 kHz. Sample-rate conversion uses antialias filtering and preserves pitch and duration to within one output sample.

Place sources in `/AUDIO_FILES` for SD preparation or `/UPLOAD_AUDIO_FILES` for Flash import. See the [User Guide](../User%20Guide.md#importing-audio) for supported formats and the [SD folder layout](../User%20Guide.md#sd-folder-layout).
