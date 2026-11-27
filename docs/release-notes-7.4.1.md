# Firmware 7.4.1 - MIDI Line Out Volume control

Release date: **28 November 2026**. Changes from **7.4.0**.

## Changes for users

- **LINE OUT VOLUME MIDI assignment:** a new destination below LPF CUTOFF in **Tools > Setup > CONTROL CHANGE ASSIGNMENT**. Choose a CC with the Value encoder or learn it by moving an external controller. Press Value to clear it.
- **Global volume control:** incoming values 0-127 map to volume 0-100 using the existing front-panel volume curve. All instrument presets and active playback voices are updated. This destination responds regardless of the incoming MIDI channel.
- **Direct exit from Tools:** pressing TOOLS on the assignment page saves the assignments and returns to the operating mode selected by the mode switch. RETURN saves and returns to Setup.
- **Clearer save dialog:** SAVE CHANGES? now offers CANCEL / NO / YES. CANCEL remains the initial choice and keeps the current edits. Text and selection frames have been aligned with the popup.
- **Updated guide:** the Markdown and PDF editions describe the new assignment, MIDI learning, exit behavior and save choices.

## Compatibility

The new assignment occupies a formerly reserved, zero-filled byte in the FRAM system settings. Record sizes and existing assignments are unchanged; existing zero-filled settings leave LINE OUT VOLUME unassigned until configured. The patch, Sound and MIDI Loop formats are unchanged from 7.4.0.

CC 0 means unassigned. CC 1, 123 and 126 retain their existing modulation and note-stop functions. Use different CC numbers for global controls and sound gains: LINE OUT VOLUME takes precedence over LPF CUTOFF, which takes precedence over sound gains when assignments overlap.

## Maintenance

Purpose comments and blank-line separation have been added throughout main.cpp. The comment-only pass preserved the executable token sequence. FRAM regression coverage includes persistence of the new assignment and preservation during other settings writes.

## Validation

- PlatformIO `teensy41` release build completed successfully on 28 November 2026.
- All **67 host regression scripts passed**.
- The 42-page PDF guide was regenerated and visually reviewed.
- Build memory report: 22,400 bytes free for local variables in RAM1 and 155,456 bytes free for malloc/new in RAM2.

Hardware operation, listening and timing checks are separate from these build and host-test results.

## Download

Use [Lilla_v7_4_1.hex](https://github.com/SandroGrassia/Lilla_Audio_Sampler/releases/download/v7.4.1/Lilla_v7_4_1.hex). See the [User Guide](../User%20Guide.md) or its [PDF edition](../User%20Guide.pdf) for upload instructions.
