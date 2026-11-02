# LILLA Audio Sampler 7.3.0

## Four layers per MIDI Loop track

Build richer arrangements without replacing the parts you have already played. Each of the four MIDI Loop tracks now supports **four independent layers**, for up to **16 layers** in one loop. Each layer can hold **1,000 NoteOn/NoteOff events**.

- Record master layer 0 with Rec 1 as before: the first note starts timing, and the next Rec 1 press establishes the loop length.
- During playback, press any track's REC to arm its next free layer. Recording begins on the first NoteOn and closes automatically at the next master restart, switching off the REC LED.
- Press REC again while waiting or recording to discard only the new layer. Existing layers keep playing and are preserved.
- No first note within 20 seconds cancels the armed layer. Reaching 1,000 events closes and keeps it.
- LEVEL, SHIFT and TRANSP affect all layers of a track together. Notes of the same pitch in different layers have independent NoteOff handling.
- The recording display identifies the layer, for example `1-R1` for Track 1, layer 1. `ALL LAYERS USED` indicates that a track is full.

## Saving and compatibility

All four tracks and their layers are saved together on microSD. **The new loop format is not compatible with MIDI loop files from earlier firmware.** Keep older loop files separately if you need them with an older firmware version.

Selective deletion of completed layers is not included in this release. Save an arrangement before using NEW to start again.

## Documentation and firmware

The README, firmware architecture, implementation notes and English User Guide (Markdown and PDF) describe the new layer workflow. Install `Lilla_v7_3_0.hex` using the firmware-update instructions in the guide.
