# LILLA Audio Sampler 7.1.0

Date: 12 October 2026. Firmware: `Lilla_v7_1_0.hex` for Teensy 4.1.

## Pitch and timbre

- TONE replaces the former LOCK/HIFI field. Each Instrument selects OFF or a cutoff endpoint from 3.0 to 8.0 kHz in 0.1 kHz steps.
- With TONE enabled and pitch above unity, source-domain anti-aliasing reduces folding artifacts before resampling. A dedicated post-resampling low-pass filter (LPF) progressively compensates excessive brightness as pitch rises: from 20 kHz at pitch 1 to the selected endpoint at +11 semitones, held above that interval. This is targeted timbre compensation, separate from the instrument VCF and LFO.
- At pitch 1 or below, the TONE timbre LPF is bypassed. Anti-imaging remains automatically available for reduced pitch, independently of TONE and also with TONE OFF, to reduce resampling images and artifacts.
- TONE edits update sounding notes with smoothing. Legacy HIFI levels 1-5 load as 5.0, 4.5, 4.2, 4.0 and 3.8 kHz; saving writes the new setting. The patch storage layout is unchanged.

## Instrument navigation and VCF

- In Performance (SESSION), the cursor traverses TONE, P, MIDI, ROOT K, FROM K, TO K, PAN and GAIN, then continues on the next active Instrument and wraps around. Reverse rotation follows the reverse sequence. Press Select to leave the fields and select the whole current Instrument; re-entry remembers the last field.
- The PAN pointer frame and value clearing now accommodate three characters such as L16 and R16.
- Manual VCF CUTOFF changes reach notes already playing, with or without LFO, over about 29 ms. Simultaneous LFO index edits no longer postpone the cutoff ramp. Outside a cutoff ramp, the established VCF pitch reference is retained.

## Documentation

The English manual describes TONE, its purpose, and cyclic Instrument navigation. The printable English guide is updated to 7.1.0. Translated manuals have been removed so that one maintained guide remains the reference.

Hardware listening tests remain necessary to assess timbre, transitions and real-time headroom on the instrument. This file describes the local 7.1.0 build; it does not imply that a GitHub release has been published.

## Validation

The Teensy 4.1 PlatformIO build succeeded. All 43 host regression suites passed. Local documentation links, CRLF line endings and the rendered English PDF were checked.
