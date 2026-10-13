# LILLA Audio Sampler 7.1.1

Date: 13 October 2026. Firmware: `Lilla_v7_1_1.hex` for Teensy 4.1.

## Mixer and pre-listening

- MUTE and GAIN now have separate fields and selection frames. MUTE, LINEOUT and PRE_LIST accept both a Value click and rotation: clockwise enables, counter-clockwise disables.
- PRE_LIST replaces MONITOR and is exclusive: one of the eight Sounds or L_IN can be selected, or none. Changing the selection does not turn on the previous source's LINEOUT.
- A Sound in SOLO and the temporary Sampler patch's Sound sources cannot be muted. L_IN remains independently controllable.
- Refined MUTE, LINEOUT and PRE_LIST frames and L_IN gain alignment improve readability.

## Sound Edit

- CLONE preserves the original gain and sends the new Sound only to pre-listen, unmuted.
- ONLY_PRE_LISTEN and TO_LINE_OUT stay aligned with Mixer routing. TO_LINE_OUT preserves an active PRE_LIST route. If neither output is enabled, both commands are available.
- SOLO and RESET_SOLO are menu commands. SOLO is no longer toggled from the Sound fields and stays active when opening Mixer from Sound Edit in Performance.
- Long menus follow the cursor so every enabled command remains reachable.

## Playback and page feedback

- PAN edits and return-to-centre now reach notes already playing, including reversals during a smooth transition. Starting a new note clears any previous PAN ramp.
- Performance shows a green PATCH UPLOADING message while patch audio moves from Flash to PSRAM.
- Destructive commands use DELETE in Performance, Sampler and Live Sampler. The RAW conversion submenu uses RETURN to leave without deleting the recording.

## Documentation and validation

README and the English User Guide (Markdown and PDF) describe the new controls and routing states. The release includes the compiled Teensy 4.1 firmware and printable guide. The Teensy 4.1 build succeeded and all 46 host regression suites passed. Local documentation links, CRLF line endings and the rendered PDF were checked. Listening and control validation on the physical instrument remain the final user check.
