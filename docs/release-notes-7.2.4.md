# LILLA Audio Sampler 7.2.4

Released 20 October 2026.

## Adjustable compressors in Delay and Live Sampler

Choose how early compression acts on the combined input and feedback: both compressors now offer thresholds from **-24 to -3 dB**, in 1 dB steps, plus **OFF**.

- On the Delay page, select **COMPRESSOR** and turn **Value**. Save the patch to retain the threshold.
- On the Live Sampler page, select **FEEDBACK COMPRESSOR** and turn **Value**. This setting is retained during the session.
- Both channels share the same gain reduction to preserve stereo balance. Threshold changes are smoothed.
- OFF disables compression while retaining the 32-sample lookahead (about 0.73 ms), so switching does not change the signal timing.

## Clearer SETUP and guided operations

The SETUP menu groups audio preparation, playing preferences and backup operations with shorter, clearer labels.

Confirmation dialogs explain the source and destination folders, what a backup includes, and what destructive operations replace or preserve. Conversion and audio import begin with a **CANCEL/GO** choice; Flash erasure has a separate final warning. Backup and restore instructions identify `/LILLABACKUP` and make clear that audio libraries and MIDI loops must be kept separately.

## Revised Delay and Live Sampler layouts

The Delay page groups echo controls and compression on the left and modulation on the right. Live Sampler places feedback and its compressor together above the playback controls. Its STEP value is displayed in seconds.

The illustrated User Guide includes the new display photographs and updated operating instructions. The release includes the Teensy 4.1 firmware and printable English guide.
