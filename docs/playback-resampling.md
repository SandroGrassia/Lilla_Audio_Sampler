# Playback resampling

Below unity pitch, the player uses the committed eight-tap Hann-windowed sinc interpolator. Its contribution is full at pitch <= 0.75 and decreases continuously to zero at unity. The source-normalized design cutoff is 0.45 cycles/sample. This path removes spectral images; it does not restore the original spectral envelope of a transposed instrument.

Above unity pitch, two source-domain biquads form a fourth-order Butterworth low-pass. The design cutoff is 0.4 / pitch cycles per source sample, corresponding to 17.64 kHz after transposition at 44.1 kHz. The filtered contribution rises continuously from zero at 1x to full at 1.25x, then remains full through the supported 35x maximum. Linear interpolation follows the source filter. There is no high-pitch bypass.

Each logical segment gathers 64 + ceil(12 * pitch) preceding source samples to reconstruct IIR state before emitting output. This avoids reusing state from a different note, seek, reverse segment, or outgoing crossfade, and avoids resetting at each output-block boundary without source context. Context follows the same logical loop order and A/B limits as playback. Only recorded Live Sampler memory is read. The output playhead and end-of-note decisions exclude this context.

IIR processing and interpolation use floating point until the existing player gain/saturation stage. Only the output block is retained in a shared float buffer; the complete source basket remains int16. Source windows fit in 5000 samples, including the maximum pitch and context. Transfers above the calibrated 4500-sample limit are split. Scalar RAM-table diagnostic proxies are grouped at that same limit.

The admission estimate includes source context and an initial CPU allowance of 0.10 microseconds per source sample plus 8 microseconds per harvest, before the existing margin. This allowance is an engineering estimate, not a measured hardware timing result. Source-read diagnostic estimates still describe transfers; harvest elapsed cycles include the IIR work as well.

`tests/player_antialias_regression.py` checks DC gain, output bounds, passband and alias tones through 35x, and block/seek agreement within two PCM units against an independent continuously running double-precision reference. The reader regression covers context at loop, reverse, one-shot and Live boundaries. Hardware listening and timing remain separate checks.
