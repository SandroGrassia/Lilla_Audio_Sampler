# Audio IRQ timing monitor

Load the normal `teensy41` firmware. Serial commands:

1. Send `D` to disable the separate read-diagnostics instrumentation.
2. Select the stress patch with the effects and filters enabled.
3. Send `t` to enable/reset timing capture.
4. Exercise all 16 players, high pitch, modulation, repeated notes and crossfades; include cache loading if relevant.
5. Send `T` to freeze capture, then `q` to print it. Audio keeps running.

The module wraps the existing Teensy software audio interrupt vector; it does not modify the framework. It measures the complete original handler, including all AudioStream objects after the cache finalizer, not just the Player chain. Enable/reset and report snapshots preserve the previous interrupt mask.

`irq_us` includes the original ISR body and cycle initialization. Exception entry/exit and the monitor's report-aggregation tail are excluded. `monitor_tail_max_us` reports the largest measured aggregation tail separately; allow additional margin for instrumentation and IRQ entry/exit. `irq_body_overruns` compares the measured body against the audio block period, not the shorter Player deadline.

`residual_us = irq_us - flash_us - psram_copy_us - zero_us`, using measurements from the same block. Transfer scopes enclose actual Flash seek/read, PSRAM memcpy and PSRAM destination memset. Packet opens, address calculations, read-diagnostic counters, loop reversal, scalar RAM/wavetable copying, interpolation and DSP stay in residual. This deliberately retains scalar RAM assembly rather than treating it as a calibrated bulk RAM copy. Transfer barriers wait for memory operations; measurement overhead is not calibrated away.

`controls_us` covers Trigger_0, including MIDI, control updates and scheduling. `players_us` sums every Player update, including source transfers. `outside_controls_players_us` covers the remaining original IRQ work plus wrapper initialization and scope bookkeeping. Do not add overlapping categories to the IRQ duration.

The report includes last, peak IRQ, peak residual and peak residual among complete 16-player blocks. Each row is a coherent single-cycle snapshot. `full_16_blocks=0` means the test did not observe a complete full-polyphony block. A full block has 16 rendered players and no cached tails, allocation failures, deadline stops or budget retirements. Other snapshots may contain incomplete work; event columns and cumulative totals identify it. `tail_blocks` counts individual Player tail updates, not distinct IRQs.

Higher-priority interrupts remain enabled. Their time is included wherever they occur; interruptions during transfer scopes are consequently subtracted with those scopes. Results are empirical wall-time observations, not worst-case guarantees. For budget changes consider the full-polyphony residual, total IRQ peaks, monitor overhead, incomplete-work counters and an additional safety margin. The source-read budget remains 1800 microseconds.
