# Pitch-dependent TONE compensation

The instrument TONE field selects OFF or an endpoint from 3.0 to 8.0 kHz in 0.1 kHz steps. The Performance encoder steps from OFF to 3.0, 3.1 and onward to 8.0, and reverses the same way. The three-character value shows the selected frequency in kHz, with one decimal place.

Each Player applies one low-pass biquad after resampling and before its VCF and stereo panning. Q is 0.7071, matching the existing global 12 dB/octave LPF. Pitch at or below unity bypasses compensation; above unity the cutoff follows a logarithmic sweep from 20 kHz to the selected endpoint at +11 semitones (pitch 1.887749), remaining at that endpoint for higher pitches. Every sample restarts at 20 kHz (bypass) when its playback pitch is 1, independently of its root-note octave.

The effective playback pitch includes tuning, pitch bend and vibrato. Coefficients are cached and cutoff changes move 25 percent toward the target per audio block. Timbre compensation enabling/disabling uses a one-block wet/dry ramp; source-domain anti-aliasing switches at the next harvest. Live TONE edits update both active and pending notes belonging to the edited patch/instrument. A change at unchanged pitch refreshes the target cutoff while retaining smoothing.

Any enabled TONE setting also enables the fourth-order Butterworth anti-alias filter before resampling above unity: cutoff is 0.4/pitch cycles per source sample, with a gradual blend from 1x to 1.25x. Source history reconstructs filter state for each segment, including loops, reverse playback and crossfades. OFF bypasses both upward-pitch filters. Low-pitch anti-imaging remains independent of TONE.

FRAM and SD retain the one-byte field and patch layout. Runtime and new saves encode OFF as 0 and frequencies as 30..80 tenths of kHz. On loading, historical HIFI levels 1..5 convert to 50, 45, 42, 40 and 38 respectively. Saving a converted patch writes the frequency encoding, preventing repeated conversion. Unknown stored values are rejected at import; the audio path receives validated settings.

The read budget reserves source history and an initial CPU allowance only when TONE is enabled. Hardware timing and listening tests remain separate from the host regressions and Teensy build. Host tests cover all endpoints, frequency response, unity/OFF bypass, live edits, reset and FRAM/SD round trips and migration.

Inside Performance (SESSION), Select rotation traverses TONE through GAIN, then TONE of the next active Instrument, wrapping from last to first. Reverse rotation traverses the same sequence backwards. Pressing Select exits onto the whole current Instrument. Performance remembers the last selected instrument field during the session. Leaving MIDI, selecting another instrument and entering it returns to MIDI. Initial entry selects TONE; explicit RootKey shortcuts select and remember RootKey.
