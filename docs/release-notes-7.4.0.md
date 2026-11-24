# Firmware 7.4.0 - Sliding loop windows

## Scope

This release compares firmware **7.4.0** with **7.3.1** and includes SLIDE FWD/REV, pitch compensation, instrument-selection correction and continuous From/To editing. Release date: **24 November 2026**.

## Changes from 7.3.1

| Area | 7.3.1 | Current 7.4.0 implementation |
| --- | --- | --- |
| File loop windows | Repeat the selected From/To region. | SLIDE moves the window between cycles in LOOP FWD and LOOP REV. |
| Motion control | No Sound SLIDE parameter. | Signed value limited to +/-min(2000, window length), referenced to pitch 1. |
| Pitch relationship | No moving-window trajectory. | Effective step is SLIDE divided by actual playback pitch, including tuning, bend and vibrato. Fractional steps accumulate. |
| Loop junctions | Stationary endpoint smoothing uses prepared tables. | Moving junctions blend the outgoing window with the next window directly from the source. |
| Active SLIDE changes | Not applicable. | Preserve reached windows and already rendered junctions; zero freezes motion and a sign change reverses it from the reached position. |
| From/To and Slice edits | Existing stationary-reader edit handling. | Retain accumulated motion and current read position when valid in the resized window, with the existing outgoing crossmix. |
| SOUND selection | No conditional SLIDE field. | Show SLIDE for FWD/REV; fall back to Play Mode when the selected instrument does not expose SLIDE. |
| Persistence | Reserved Sound bytes unused for SLIDE. | Store signed SLIDE in reserved bytes 0..1; zero preserves stationary behavior. |

For example, SLIDE 100 produces steps of 200, 100 and 50 source samples per cycle at pitches 0.5, 1 and 2 respectively. The cycle rate changes inversely, maintaining the same average window motion per unit of output time for an unchanged loop length. This does not smooth motion continuously within a cycle or normalize speed across different loop lengths.

## Implementation and invariants

- `PlayerLoopSlide` owns the trajectory: current phase, fractional progress, bounded step history, source bounds and playback direction. Fixed-point window positions use 24 fractional bits. The window retains its length while sliding and stops at source boundaries.
- `AudioPlayer::Harvest_slide_samples()` reconstructs samples in the shared basket. A temporary 32-sample array supplies the second crossfade endpoint; there are no per-voice persistent audio buffers for SLIDE.
- Interpolation lookahead protects already prepared junctions. Step edits and pitch changes take effect at the first available junction, without resetting the note.
- From/To, Slice and NoClick edits capture the current position before reading the outgoing crossmix. `Reframe()` applies the new geometry to the reached window, retaining fractional motion and source position where possible. If the new range excludes that position, it clamps to a valid point. This also works after setting SLIDE to zero.
- A new Note On resets to the edited preset's From/To. Playback-mode changes follow the existing geometry transition/reset path.
- Nonzero SLIDE bypasses wavetables. Sliding RAW playback needs the full source rather than just a cached initial slice; partial cache coverage can cause Flash fallback and its lower pitch ceiling. Source descriptors bound the available motion. Live Sampler circular recording playback is excluded.
- Read-budget estimates include dynamic endpoint reads and chunked blending, including edits to a frozen sliding window. Control-side edit preparation (`Main_settings_editing()` and `Apply_preset_edit()`) resides in Flash to keep rendering within the available RAM1 banks.
- The SOUND page uses the shorter `NOCLK` label, aligned with REL. SLIDE navigation follows display order; switching instruments validates the retained field. Stereo Direct Sampler edits propagate SLIDE to the other channel.

The detailed algorithms and source references are in [Firmware Implementation Notes](firmware-implementation-notes.md#sliding-loop-windows-740). The [architecture overview](../FIRMWARE_ARCHITECTURE.md) describes the surrounding audio system.

## Compatibility

The persistent Sound record retains its size. SLIDE occupies reserved bytes 0 and 1 as a signed little-endian 16-bit value; existing records with zero in those bytes remain stationary. The displayed value remains referenced to pitch 1 when saved; pitch compensation is applied at runtime per voice.

The `LILLALOOP 3` MIDI Loop format introduced in 7.3.0 is unchanged. MIDI Loop track time shifting is independent of Sound SLIDE. Earlier incompatible MIDI Loop file versions remain unsupported. The Live Sampler partial canvas updates introduced in 7.3.1 remain in place.

## Validation

The current implementation passed the PlatformIO `teensy41` build and all **67 host regression scripts** on 24 November 2026. The generated binary is `Lilla_v7_4_0.hex`.

Coverage includes FWD/REV rendering, positive and negative motion, source bounds, fractional pitch, equal-duration displacement across pitches, interpolation history and lookahead, active step changes, freezing and reversing, From/To/Slice/NoClick edits, instrument cursor restoration, persistence, cache selection and read-budget accounting. Edit tests exercise outgoing crossmix lengths of 0, 32 and 128 samples.

The final build reported 22,432 bytes free for local variables in RAM1 and 155,456 bytes free for malloc/new in RAM2. These build figures are not runtime stack or timing measurements. Listening tests and worst-case timing checks on Teensy remain necessary; host tests do not establish acoustic quality or real-time headroom.
