# LILLA Audio Sampler 7.2.3

Released 17 October 2026.

## Stereo compressor for Delay feedback

The Delay feedback path now includes a stereo compressor that controls the sum of routed audio and feedback before it reaches the delay buffers. Both channels share the same gain reduction to preserve stereo balance and contain peaks as echoes accumulate.

- Select **FEED COMPRESSOR** on the Delay page and turn or press Value to choose ON or OFF.
- Save the patch to retain the setting. Existing patches load with the compressor OFF; the persistent FRAM record size is unchanged.
- The left column now reads **FEEDBACK**, **FEED LPF**, **FEED COMPRESSOR**, **DELAY TIME**, **DELAY TIME L/R**. Routing from Live Sampler is labeled **LIVE SAMPLER**.
- Delay and Live Sampler share `LillaCompressor`, with independent state and a 32-sample lookahead (about 0.73 ms), including in bypass. The Live Sampler lookahead was reduced from 128 samples.
- Compression begins around -6 dBFS and limits sample peaks to approximately -1 dBFS once fully enabled. It does not boost quiet signals or repair clipping earlier in the audio path.

## Other improvements

- MIDI Loop initially selects the lowest saved loop ID, or displays the first free ID when no saved loops exist.
- NEW loop navigation starts from the displayed free ID. While stopped, recording is allowed only on Track 1, which clears the current loop and starts a new master recording.
- MIDI Loop recording LEDs use a smooth 4-second brightness cycle, with a 40% minimum, and the sound/track LEDs are restored when returning from Tools.
- Player volume transitions use fixed 0.00005 gain steps per sample with exact endpoints. Idle playback and pan/volume ramp handling were improved.

The User Guide and firmware architecture documentation have been updated. The README now focuses on the current release and core project information.
