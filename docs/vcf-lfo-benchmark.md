# VCF and LFO hardware timing

Build with `pio run -e teensy41_vcf_benchmark`. The diagnostic HEX is `.pio/build/teensy41_vcf_benchmark/firmware.hex`; this environment does not replace the release HEX. Upload with `pio run -e teensy41_vcf_benchmark -t upload`.

Only this environment enables the Serial diagnostic commands and removes read-budget admission/retirement limits. All 16 Players remain available. Invalid source geometry is still rejected, and the 2700 us Player deadline remains active to keep overload recoverable. Normal firmware retains its budget enforcement.

## PWM interrupt comparison

The diagnostic environment currently defines `LILLA_VCF_BENCHMARK_DISABLE_PWM`. With this flag, PWM initialization returns before starting the hardware or DMA interrupts; the two output objects still drain incoming audio blocks. I2S output remains available, but PWM pre-listening is unavailable. The `c` report explicitly prints `PWM=OFF`. Remove this flag from the diagnostic environment and rebuild to restore PWM; the report then prints `PWM=ON`. Normal firmware is unaffected because the bypass also requires `LILLA_VCF_BENCHMARK`.

Compare the same held note at the root key, LOWPASS, pivot 2000 Hz, Q 2.4 and the same periodic LFO settings as the previous measurement. Send `v`, hold for 20 seconds, then send `c` while the note remains held. Compare stage peaks, inactive Player peaks, total CPU peak and deadline stops. Repeat with the same 16-note set only if needed. A reduction in peaks implicates the disabled PWM path but does not by itself measure the exact ISR duration; the change also removes its DMA traffic and cache maintenance.

## Commands and reports

- `v`: reset CPU peaks, VCF statistics and the emergency-stop baseline; disable source-read instrumentation for a cleaner VCF measurement.
- `c`: print CPU and per-Player VCF results without resetting the measurement window.
- `d`, `p`, `D`: existing source-read diagnostics; use only in a separate investigation because collection adds overhead.

`VCF_PLAYER` reports rendered VCF blocks, mean and maximum times in microseconds. Stages 0/1/2/3 are LFO generation, frequency mapping plus the first coefficient update, VCF processing (including the remaining three coefficient updates when modulated), and the complete chain. The complete-chain maximum is measured directly; it is not the sum of independent maxima. Clock reads and branch overhead are included. Statistics accumulation and Serial printing are outside the measured chain but do contribute to total CPU load. This measures steady audio rendering, not note-start setup such as Start_VCF.

The peak record includes filter type (0 lowpass, 1 highpass, 2 bandpass, 3 notch), modulation, periodic flag, effective pitch, pivot frequency and Q. These settings describe the block with the largest complete-chain time. A report containing `INVALID_FULL_POLYPHONY_TEST` means the emergency deadline stopped voices during that window. Do not use it to claim stable 16-voice operation. Individual voice peaks can occur in different audio blocks; use AUDIO_COST peak for the whole chain.

## Test matrix

Use a long PSRAM-cached sound, a high sustain, no NoClick or short loops, TONE OFF and delay bypass initially. Confirm the cache upload has finished. Start with one held note at the root key, then repeat with 8 and 16 held voices. Send `v` after setting up the case, play for 10 seconds, and send `c` while notes are held. Capture the output before releasing notes. Repeat a case with repeated note attacks to include voice startup in the total CPU peak.

1. VCF bypass: capture CPU baseline; no VCF block samples are expected.
2. VCF enabled, modulation NONE, fixed cutoff: measure the steady biquad path.
3. VCF enabled, modulation NONE, rotate cutoff continuously: exercise the four coefficient updates during cutoff ramps.
4. Periodic LFO: test minimum and maximum rate, low and maximum depth, and simultaneous cutoff/depth edits.
5. Repeat with each aperiodic modulation shape. Retrigger notes during the window so the envelope remains active; also measure its completed state separately.
6. Repeat 2-5 for LOWPASS, HIGHPASS, BANDPASS and NOTCH, at minimum/mid/maximum pivot and minimum/maximum resonance.
7. Repeat the heaviest cases at +12, +24 and +36 semitones and during bend. Restore TONE and optional A afterwards to measure the complete user configuration.

Keep each window restricted to a documented case; reset with v between cases. First determine the worst mean and maximum chain time per Player. Then use 16 voices at modest source pitch to validate simultaneous load. High source pitch can hit the emergency deadline because of PSRAM reads even if VCF itself is inexpensive. Record voice count, filter type, modulation shape/rate/depth, cutoff, resonance, pitch, TONE/A, mean3_us, peak3_us, CPU peak and deadline stops. Physical measurements must be collected from the instrument; host tests do not provide hardware timings.
