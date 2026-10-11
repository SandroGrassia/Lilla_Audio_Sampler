# Experimental pitch-dependent piano filter

The instrument LOCK field temporarily enables timbre compensation instead of its previous pitch-bend/effects lock. Existing saved LOCK values now select this filter. The UI and storage format retain the LOCK name.

Each Player applies one low-pass biquad after resampling and before its VCF and stereo panning. Q is 0.7071, matching the existing global 12 dB/octave LPF. Pitch at or below unity bypasses compensation; above unity the cutoff follows a logarithmic sweep from 20 kHz to 4.5 kHz at +11 semitones (pitch 1.887749), remaining at 4.5 kHz for higher pitches. This is an initial listening-test curve, not a measured instrument model.

The effective playback pitch includes tuning, pitch bend and vibrato. Coefficients are cached and cutoff changes are smoothed across blocks. Enabling/disabling uses a one-block wet/dry ramp. Filter state belongs to each Player and resets when a new note starts. Live LOCK edits update both active and pending notes belonging to the edited patch/instrument.

For listening tests, map each octave sample from its root C through B and leave the global LPF open (20 kHz), so its manual attenuation does not compound the new correction. Compare LOCK off/on. Anti-imaging remains available below unity; this output filter does not prevent resampling aliasing.

Processing covers the 128 output samples regardless of pitch. Hardware timing remains to be measured. The host regression checks the Butterworth response, cutoff endpoints, exact disabled bypass and state reset; the Teensy build checks integration.
