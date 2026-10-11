# Experimental pitch-dependent piano filter

The instrument HIFI field temporarily enables timbre compensation and source-domain anti-alias filtering instead of its previous pitch-bend/effects lock. HIFI is a numeric instrument setting from 0 to 5. Stored numeric values are retained without migration: old level 1 now selects 5 kHz, and 4.5 kHz is level 2; FRAM and SD retain the same one-byte field and patch layout. The Performance encoder selects the level and displays its number.

| HIFI | Cutoff at +11 semitones | Anti-aliasing above unity |
| --- | --- | --- |
| 0 | Bypassed | Off |
| 1 | 5000 Hz | On |
| 2 | 4500 Hz | On |
| 3 | 4200 Hz | On |
| 4 | 4000 Hz | On |
| 5 | 3800 Hz | On |

Each Player applies one low-pass biquad after resampling and before its VCF and stereo panning. Q is 0.7071, matching the existing global 12 dB/octave LPF. Pitch at or below unity bypasses compensation; above unity the cutoff follows a logarithmic sweep from 20 kHz to the selected endpoint at +11 semitones (pitch 1.887749), remaining at that endpoint for higher pitches. Every sample restarts at 20 kHz (bypass) when its playback pitch is 1, independently of its root-note octave. This is an initial listening-test curve, not a measured instrument model.

The effective playback pitch includes tuning, pitch bend and vibrato. Coefficients are cached and cutoff changes are smoothed across blocks. Timbre compensation enabling/disabling uses a one-block wet/dry ramp; source-domain anti-aliasing switches at the next harvest. Filter state belongs to each Player and resets when a new note starts. Live HIFI edits update both active and pending notes belonging to the edited patch/instrument. Changing level at unchanged pitch also refreshes the target cutoff, retaining smoothing.

For listening tests, map each octave sample from its root C through B and leave the global LPF open (20 kHz), so its manual attenuation does not compound the new correction. Compare HIFI off/on. Anti-imaging remains available below unity. With HIFI set to 1-5, the previously tested fourth-order Butterworth anti-alias filter runs before resampling above unity: cutoff is 0.4/pitch cycles per source sample, with a gradual blend from 1x to 1.25x. Source history reconstructs filter state for each segment, including loops, reverse playback and crossfades. HIFI 0 bypasses both upward-pitch filters.

Timbre compensation covers the 128 output samples regardless of pitch; anti-alias processing scales with source samples and pitch. The read budget reserves source history and an initial CPU allowance only when HIFI is enabled. Hardware timing remains to be measured. The host regressions check all five Butterworth responses, cutoff endpoints, exact disabled/unity bypass, live level edits, state reset and FRAM/SD round trips; the Teensy build checks integration.

Performance remembers the last selected instrument field during the session. Leaving MIDI, selecting another instrument and entering it returns to MIDI. Initial entry selects HIFI; explicit RootKey shortcuts select and remember RootKey.
