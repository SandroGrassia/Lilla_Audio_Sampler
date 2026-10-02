# LILLA User Guide

For LILLA Audio Sampler 2026, PCB2026_R1, firmware 7.0.0 dated 29 September 2026.

This guide describes the controls and workflows implemented in the firmware in this repository. Screen labels are shown in `UPPERCASE`. Procedures have been checked against the firmware; they have not yet been verified on the physical instrument.

## Contents

- [Getting started](#getting-started)
- [Controls and navigation](#controls-and-navigation)
- [Files, sounds and patches](#files-sounds-and-patches)
- [Performance](#performance)
- [Editing a sound](#editing-a-sound)
- [Importing audio](#importing-audio)
- [Recording with Sampler](#recording-with-sampler)
- [Live Sampler](#live-sampler)
- [MIDI Loop](#midi-loop)
- [Mixer, delay and filters](#mixer-delay-and-filters)
- [Setup and MIDI controls](#setup-and-midi-controls)
- [Backup and restore](#backup-and-restore)
- [Troubleshooting](#troubleshooting)
- [Quick reference](#quick-reference)

## Getting started

LILLA is a polyphonic, multitimbral hardware sampler with up to 16 playback voices. A performance patch can contain up to eight sounds, with individual MIDI channels and keyboard ranges. You can play imported audio, record the line input, work with a temporary live buffer, and record four-track MIDI loops.

### Connect and play

1. Connect a MIDI controller to LILLA's MIDI input.
2. Connect the stereo line output to your mixer, amplifier or audio interface. Start with low listening levels.
3. Power on LILLA and wait for startup to finish.
4. Set the Modes selector to **Performance**.
5. Turn **Select** to highlight the patch number, then turn **Value** to choose an existing patch.
6. Set your controller to the MIDI channel shown for a sound in the patch.
7. Play notes within that sound's `FROM K` and `TO K` range.
8. Raise **Line Out Vol** gradually.

If there is no suitable audio loaded, follow [Importing audio](#importing-audio). Importing replaces the Flash audio library and deletes existing recordings, so back up your work first.

## Controls and navigation

The same controls perform different tasks depending on the active page. Follow the highlighted field and the options currently visible on the screen.

| Control | Main use |
| --- | --- |
| Modes selector | Choose Sampler, Live Sampler, Performance or MIDI Loop. |
| Tools selector | Choose Mixer, Delay, Setup or Test. |
| Tools button | Open the selected tool; the Tools LED indicates tool access. |
| Select, turn | Move the highlight between fields and menu items. |
| Select, press | Execute a menu command, confirm a choice, or enter/leave a group of fields. |
| Value, turn | Change the highlighted parameter. |
| S1-S8 | Open the corresponding active sound for editing in Performance; capture into sound slots in Live Sampler. |
| From / To | Adjust sample boundaries in Sound Edit; adjust the live playback region in Live Sampler. |
| Step | Change editing increments; its push function depends on the page. |
| Line Out Vol | Adjust patch playback volume on performance-related pages. |
| Pre Listen Vol | Adjust pre-listening level. |
| Resolution / Downsampling | Change playback character through bit reduction and sample repetition. |
| Cutoff | Adjust the common low-pass filter; press to return it to its maximum cutoff. |
| Tuning Tone | Enable the tuning reference; turn to adjust its level when enabled. |
| Loop / Tempo | Select and control MIDI loops and their playback timing. |
| Track 1-4 / Rec 1-4 | Control individual MIDI-loop tracks and their recording. |

To open a tool, set the Tools selector to the required position and press **Tools**. Test opens the MIDI monitor. Use the Tools button to return from a tool where supported, or select the required operating mode.

In confirmation dialogs, turn **Select** to choose an option and press it to confirm. Read the dialog before confirming: save, discard, erase and restore have different consequences.

Menus are dynamic. A command may be hidden when the current state does not allow it, for example when there is no recording to export.

## Files, sounds and patches

| Term | Meaning |
| --- | --- |
| Audio file | The source sample used for playback. Imported audio is converted to mono RAW audio in Flash. |
| Recording | Audio recorded by the Sampler into its Flash recording area; it can be mono or stereo. |
| Sound | A source file plus playback settings such as trimming, pitch, envelope, pan and gain. |
| Instrument / sound slot | One of up to eight positions in a patch, with MIDI mapping, root key, keyboard range and filter settings. |
| Patch | The group of sounds and settings used for a performance. |
| MIDI loop | Recorded MIDI events, arranged into four tracks. It does not contain the audio samples played by those events. |
| Live buffer | Temporary audio held in PSRAM for Live Sampler. |

The firmware provides storage for up to 200 patches and 800 sound records. These are storage capacities; the instrument has up to 16 simultaneous playback voices. Available polyphony also depends on playback workload.

Flash holds the imported audio library and Sampler recordings. PSRAM holds live audio. The microSD card is used for import, WAV export, backups and MIDI-loop files.

## Performance

### Choose a patch

Highlight the patch number with **Select** and turn **Value** to browse existing patches.

When the current patch has changes, LILLA asks what to do before switching. You can stay in the current patch, discard its changes, or save them and continue. Use the displayed choices rather than switching away without checking the prompt.

### Map sounds to your controller

1. Highlight a sound row.
2. Press **Select** to enter its fields.
3. Turn **Select** to move between fields.
4. Turn **Value** to edit the selected field.
5. Press **Select** to leave the row's fields.

| Field | Purpose |
| --- | --- |
| `SOUND` | Sound slot in the patch, numbered 1-8. |
| `LOCK` | Protects the sound from selected performance controls, including pitch bend, resolution and downsampling. It also affects note-release handling. |
| `P` | Playback precedence: gives the sound priority in voice allocation. It does not guarantee unlimited voices. |
| `MIDI` | Receiving MIDI channel, displayed as 1-16. |
| `ROOT K` | Reference key used for the sound's pitch mapping. |
| `FROM K` / `TO K` | Inclusive keyboard range that triggers this sound. |
| `PAN` | Stereo position; press Value on this field to centre it. |
| `GAIN` | Sound level. |

For a keyboard split, put two sounds on the same MIDI channel with separate key ranges. For a layer, give them overlapping ranges. For multitimbral playback, assign different MIDI channels.

### Save or duplicate a patch

Use the commands shown in the Performance menu:

- `SAVE`: store the current patch and edited sounds.
- `CLONE`: create a copy in another available patch slot.
- `SAVE_AS_NEW`: store the edited result as a new patch.
- `EXIT`: discard the current edits through the Performance exit workflow.
- `DROP`: delete the patch, after the deletion confirmation.

A sound's `RETURN` command keeps edits in the current session. Save the patch to make those edits persistent.

## Editing a sound

From Performance, press **S1-S8** for an active sound to open Sound Edit. An unused slot displays `SOUND ... IS NOT USED`; pressing it does not create a new sound.

### Choose and trim the source

1. Highlight the file field with **Select**.
2. Turn **Value** to choose an available source.
3. Play the sound from your MIDI controller.
4. Turn **From** and **To** to adjust the playback region.
5. Turn **Step** to select a suitable trimming increment: use larger steps to find the region, then smaller steps to refine it.

Changing the source file resets the trim region to the new file's full length and resets its pitch adjustment. Recheck the boundaries after changing files.

### Shape playback

Use Select to highlight a parameter and Value to change it:

| Parameter | Purpose |
| --- | --- |
| Pitch | Tune the sample. |
| Gain / Pan | Set level and stereo position. |
| Attack | Set how quickly the sound reaches its initial level. |
| Decay | Set the transition to the sustain level. |
| Sustain | Set the held envelope level. |
| Release | Set the fade after release. |
| Play mode | Choose playback direction and one-shot or looping behavior. |
| Noclick | Adjust loop-boundary smoothing where available. |

Playback modes include forward and reverse one-shot, forward and reverse loop, and alternating forward/reverse loop directions. Choose a one-shot mode for a sample that should play through once, or a loop mode for a sustained repeating region.

For a clean loop, refine its start and end points before increasing Noclick. The available Noclick range depends on the selected region.

### Return, clone or remove a sound

- `RETURN` keeps the edits and returns to the previous performance page.
- `CLONE` copies the instrument into a free slot in the current patch. Adjust the copy's key range, root key or source as needed.
- `DROP` removes the instrument from the current patch.

Save the patch after finishing. Removing a sound slot is different from deleting its source audio file.

## Importing audio

> **Import replaces the audio library.** Confirming import erases the previous Flash audio files and Sampler recordings. Create a backup and keep your source audio on your computer before proceeding.

### Prepare the microSD card

Create `/LILLA_AUDIO` at the card's root and put your audio files directly inside it.

| Format | Import requirements |
| --- | --- |
| `.raw` | Headerless mono, signed 16-bit little-endian PCM at 44.1 kHz. |
| `.wav` | Uncompressed PCM, 16-bit, 44.1 kHz, mono or stereo. |
| `.aif` / `.aiff` | Uncompressed AIFF, 16-bit, 44.1 kHz, mono or stereo. |
| `.mp3` | Mono or stereo at standard MP3 rates from 8 to 48 kHz; converted to 44.1 kHz. |

Stereo imports are averaged to mono. To keep a stereo source as two independently playable files, prepare separate left and right files with different names before import.

Imported audio is stored as `<basename>.raw`. Use distinct basenames: `piano.wav` and `piano.mp3` both target `piano.raw` and are treated as duplicates.

Each imported file is limited to 3 MiB of decoded mono PCM, approximately 35.7 seconds. Longer files are truncated. MP3 decoding and rate conversion can take longer than PCM import.

### Import the files

1. Insert the prepared microSD card.
2. Open **Tools > Setup**.
3. Select `IMPORT AUDIO FILES FROM /LILLA_AUDIO` and press Select.
4. Review the import report and final erase warning.
5. Confirm only when you are ready to replace the current library and recordings.
6. Wait for copying, memory configuration and restart to complete.
7. Open Sound Edit and select the imported source you want to use.

The import screen reports invalid files, duplicates and Flash capacity problems. If too little memory remains for sampling, prepare a smaller library or shorter files.

## Recording with Sampler

Sampler records the line input into Flash and lets you audition, convert or export the result.

### Make a recording

1. Connect your audio source to the stereo line input.
2. Set the Modes selector to **Sampler**.
3. Select `PAUSE+REC` to monitor the input before recording.
4. Adjust the displayed line-input gain with Value when the gain field is selected. Watch both level meters and avoid persistent red peaks.
5. Select `MONO_REC` or `STEREO_REC`.
6. Start your source and watch the elapsed time and available recording memory.
7. Select `STOP` when finished.
8. Select the recording you want to audition and adjust its playback volume as needed.

The memory display distinguishes space available for recordings from space available for RAW files. A recording can fit even when there is insufficient room to convert it to RAW.

### Make a playable RAW file

1. Select the recording.
2. Choose `MAKE_RAW` and confirm the conversion.
3. Choose the available output: `MAKE_MONO`, `MAKE_LEFT`, `MAKE_RIGHT` or `MAKE_BOTH`.
4. Wait for the conversion to finish.
5. Open Sound Edit and choose the generated RAW source.
6. Save the patch that uses it.

For stereo recordings, left and right can become separate RAW sources. `CANCEL` exits the conversion choices. If LILLA reports that it cannot create a RAW file, check free RAW memory and available filenames.

### Export a WAV file

Insert a microSD card, select a recording and choose `EXPORT_WAV_TO_SD`. The exported WAV preserves the recording's mono or stereo layout and uses 16-bit PCM at 44.1 kHz.

Files are written to `/LILLAWAV_EXPORT`, with names such as `0M.wav` for mono or `0S.wav` for stereo. Wait for the success message before removing the card.

`CANCEL_RECORDING` deletes the selected recording. Export anything you want to retain first.

## Live Sampler

Live Sampler records into a circular PSRAM buffer. It offers approximately 40 seconds in mono or 20 seconds in stereo. As recording continues, new audio replaces older buffer content.

The live buffer is temporary and is lost at power-off. Use the capture-to-patch workflow below to preserve a selected loop.

### Record and explore

1. Set the Modes selector to **Live Sampler**.
2. Choose `MONO/STEREO` before recording. Changing this setting clears the buffer.
3. Select `CAPTURE` to start recording the input into the live buffer.
4. Select `STOP` to freeze recording and work with the recorded audio.
5. Choose a playback mode and play from your MIDI controller.
6. Turn From to adjust the region's start and To to adjust its length.
7. Turn Step to change the editing increment.

The page also provides gain, feedback and waveform-window controls. Feedback changes how earlier material contributes during live recording. Start with a modest setting and adjust while listening.

Pressing Step switches the live start-point lock behavior. Use the waveform and start-point display to follow the current region. `ERASE` clears the recorded buffer.

### Capture a live loop into a patch

1. Save any pending edits to the performance patch before starting this workflow.
2. Record some live audio, then select `STOP`.
3. Select a **loop** playback mode and refine the region.
4. Press the desired **S1-S8** slot button.
5. When prompted, play a MIDI key to set the captured sound's root key, or cancel.
6. If replacing an occupied capture slot, confirm `REPLACE CAPTURE?` only when intended.
7. Review the resulting patch and save it from Performance.

Stereo captures use a pair of sound slots. The selected region must fit the capture cache, and free patch, sound and file resources must be available.

Captured audio initially remains in PSRAM. Saving the patch writes its pending captured audio as RAW files to Flash. Wait for saving to finish; a failed save must be retried before powering off.

If the screen says `STOP REC AND SELECT LOOP MODE`, stop live recording and choose a looping mode before pressing a sound-slot button again. If it says `LOOP TOO LONG FOR CACHE`, shorten the selected region.

## MIDI Loop

MIDI Loop records MIDI events into four tracks and plays them through the current patch. Track 1 is the master track and establishes the loop duration.

### Record your first loop

1. Insert a microSD card for saving loops.
2. Choose a patch and check that your controller plays the intended sounds.
3. Set the Modes selector to **MIDI Loop**.
4. Press **Rec 1**, play your phrase, and press Rec 1 again to close recording.
5. Listen to the repeating master track.
6. Press Rec 2, Rec 3 or Rec 4 to record another track; press that same Rec button again to finish.
7. Use the menu to save the loop.

Recording into an occupied track replaces its events. **Recording Track 1 again also clears the other tracks**, because it creates a new master loop. Save an existing loop before replacing its master track.

A recording armed without any events is cancelled after approximately 20 seconds.

### Play and edit

- Turn **Loop** to browse saved loops.
- Press Loop to stop or restart the group of tracks.
- Press a **Track** encoder to stop or start that individual track.
- Turn **Tempo** to change playback timing; press it to reset the timing adjustment.
- Use Select to choose the track parameter row, then turn each Track encoder to change that track's level, pitch or time shift.

### Save loops

Use `SAVE` to update a saved loop and `SAVE_AS_NEW` to keep another version. `NEW` starts a new loop; `DELETE` removes a saved loop.

Loop files are stored in `/LILLALOOP`. Keep a copy of this folder when archiving your work. Loop files contain MIDI data, so preserve the required patch and source audio as well.

## Mixer, delay and filters

### Mixer

Open **Tools > Mixer** to adjust source gain or mute, pan, and routing to line and monitor outputs. Select the field with Select and change it with Value.

Use the separate line and monitor routes to decide what your audience hears and what you hear while monitoring. If a source is silent, check its mute/gain and output route as well as the patch volume.

### Delay

Open **Tools > Delay** to adjust feedback, delay time, the left/right time relationship, modulation source, modulation frequency, modulation depth and left/right modulation phase.

Start with low feedback, then increase it while listening. Check the instrument's delay routing if you hear no delayed signal. Save the patch to retain its delay settings.

### Filters and sound character

The common **Cutoff** control changes the low-pass filter. Press it to restore maximum cutoff. **Resolution** and **Downsampling** add digital coloration.

The instrument VCF page provides filter type, cutoff, resonance and modulation settings. Adjust these while playing the selected sound so you can hear how they interact with its sample and envelope.

A locked instrument is protected from selected performance changes. Check `LOCK` if pitch bend, resolution or downsampling appears to have no effect on that sound.

## Setup and MIDI controls

Open **Tools > Setup** for:

- `KEY STEP`: pitch mapping increments of one semitone, half a semitone, quarter or eighth of a semitone.
- `FIRST OCTAVE`: the octave-number convention used for note display.
- `CONTROL CHANGE ASSIGNMENT`: MIDI CC assignments for gains of Sound 1-8 and the low-pass cutoff.
- Audio import, backup, restore and factory reset.

In Control Change Assignment, choose each destination and set its CC number. A dash indicates no assignment. Configure the MIDI controller to transmit the chosen CC on the channel appropriate to the sound.

Open **Tools > Test** to monitor incoming MIDI message types. This is useful when checking whether the controller is sending notes, pitch bend, aftertouch or control changes.

## Backup and restore

### Create a backup

1. Save your current patch edits.
2. Insert a microSD card with sufficient free space.
3. Open Tools > Setup.
4. Select `NEW NUMBERED BACKUP IN /LILLABACKUP` and confirm.
5. Wait for the backup success message.
6. Copy the backup folder to your computer for safekeeping.

Backups are created in numbered directories such as `/LILLABACKUP/000001`. They contain configuration and Sampler recording audio.

Keep your imported source library separately. The backup operation does not copy the entire imported RAW library, the temporary live buffer or the MIDI-loop directory. Copy `/LILLALOOP` separately and retain your import files. Save live captures into a patch before archiving them; also preserve their generated RAW audio through an appropriate audio export/archive workflow.

### Restore a backup

1. On your computer, choose the numbered backup you want to restore.
2. Copy **the contents** of that directory into `/LILLABACKUP` on the card, keeping its configuration and recording files together.
3. Check that `/LILLABACKUP/LILLA_CONFIG.fram` exists. Leaving it only inside `000001`, for example, is not enough.
4. Insert the card and open Tools > Setup.
5. Select `RESTORE CONFIG + AUDIO FROM /LILLABACKUP ROOT` and confirm.
6. Wait for restoration and any requested restart to finish.
7. Check the restored patches and recordings, and that their imported source files are available.

Restoration replaces configuration and restores recording audio. Back up the current state before restoring a different one. Keep the original backup intact: invalid or missing recording audio can prevent a recording from being recovered.

### Factory reset

`FACTORY RESET` deletes patches, sounds and recordings. Make a backup before confirming. Wait for the reset and restart to complete.

## Troubleshooting

| Problem | What to check |
| --- | --- |
| No sound from the controller | MIDI connection, instrument MIDI channel, keyboard range, source availability, gain, patch volume and Mixer output routing. |
| A slot will not open for editing | The slot may be unused. Edit an active slot or clone an existing instrument into a free one. |
| Sample sounds too high or too low | Root key, sound pitch, controller pitch bend and Setup key step. |
| Audio clips or distorts | Reduce recording input gain or playback gain. Check Mixer levels, resolution and downsampling. |
| Clicks at a loop boundary | Refine From/To and adjust Noclick where available. |
| Fewer notes play than expected | LILLA has up to 16 voices; layers consume multiple voices and heavier playback can reduce availability. Review precedence and arrangement density. |
| SD import cannot find files | Check the card and the exact `/LILLA_AUDIO` folder at its root. |
| Import rejects a WAV or AIFF | Use uncompressed 16-bit PCM at 44.1 kHz with one or two channels. |
| Import reports duplicate files | Give each source a distinct basename, including files in different formats. |
| An imported sample ends early | Check the approximately 35.7-second import limit. |
| RAW conversion fails | Check free RAW-file memory and available filenames. |
| WAV export fails | Check that the SD card is present, writable and has free space. |
| Live audio disappears | The live buffer is temporary. Capture the desired loop into a patch and save it before power-off. |
| Live capture is refused | Stop recording, select loop mode, shorten the region if necessary, and save existing patch changes. |
| Tracks disappear after re-recording Track 1 | Track 1 establishes a new loop and clears the other tracks. |
| Restore cannot find the backup | Place `LILLA_CONFIG.fram` and its matching recording files directly in `/LILLABACKUP`. |
| Restored patch cannot play its source | Restore or re-import the required source library; configuration backup does not include all imported audio. |

## Quick reference

| Task | Starting point |
| --- | --- |
| Play an existing patch | Performance > patch field > Value. |
| Edit a sound | Performance > S1-S8 for an active slot. |
| Add another instrument using an existing sound | Sound Edit > CLONE, then edit the new slot. |
| Keep sound edits after power-off | Return to Performance > SAVE. |
| Import computer audio | SD `/LILLA_AUDIO` > Tools > Setup > import. |
| Record line input | Sampler > PAUSE+REC > MONO_REC or STEREO_REC > STOP. |
| Turn a recording into a source | Sampler > MAKE_RAW. |
| Export a recording | Sampler > EXPORT_WAV_TO_SD > SD `/LILLAWAV_EXPORT`. |
| Record temporary live audio | Live Sampler > CAPTURE > STOP. |
| Preserve a live loop | Stop live recording > loop mode > S1-S8 > set root key > save patch. |
| Record a MIDI loop | MIDI Loop > Rec 1 > play > Rec 1 again. |
| Back up configuration and recordings | Tools > Setup > new numbered backup. |
| Restore a numbered backup | Copy its contents to SD `/LILLABACKUP` > Tools > Setup > restore. |

---

Guide prepared from the firmware and repository documentation. Before publication, verify panel labels, monitoring behavior and each recording/save procedure on the target hardware.
