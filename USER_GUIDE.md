# LILLA User Guide

For **LILLA Audio Sampler 2026 | PCB2026_R1 | firmware 7.0.2**

Guide edition: **8 October 2026**

Printable edition: [User Guide PDF for firmware 7.0.2](LILLA_User_Guide_v7.0.2.pdf).

<img src="doc/assets/images/0.jpg" alt="LILLA startup screen with firmware version and memory information" width="37%">

*Welcome screen. The photographs in this guide show a working instrument; patch numbers, file names and values are examples.*

LILLA brings sample playback, line-input recording, live sampling and MIDI looping into one instrument. This guide takes you from your first playable patch to creating your own sounds and preserving a complete session.

Start with **Getting started** if you want to play immediately. Read **Files, sounds and patches** before building a library: understanding what each Save operation preserves will make the rest of the instrument easier to use.

**How to read this guide:** bold names such as **Select** refer to physical controls; `UPPERCASE` text refers to screen labels or messages. **Tools > Setup** means set the Tools selector to Setup, then press Tools. A numbered procedure describes the order of operations. Photographs illustrate the page layout; the accompanying instructions describe the repository firmware, including changes made after a photograph was taken.

Procedures have been reviewed against the firmware and supplied display photographs. A complete walkthrough on the physical instrument remains to be performed.

## Contents

- [Getting started](#getting-started)
- [I/O connections and buttons](#io-connections-and-buttons)
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
- [Updating the firmware on Windows](#updating-the-firmware-on-windows)
- [Updating the firmware on Mac](#updating-the-firmware-on-mac)
- [Troubleshooting](#troubleshooting)
- [Practical projects](#practical-projects)
- [Quick reference](#quick-reference)
- [Glossary](#glossary)

**Popular workflows:** [Keyboard split](#build-a-keyboard-split) | [Auto Tune](#tune-a-sound-and-use-auto-tune) | [Capture a live loop](#capture-a-live-loop-into-a-patch) | [Complete archive](#plan-a-complete-archive)

## Getting started

LILLA is a polyphonic, multitimbral hardware sampler with up to 16 playback voices. A performance patch can contain up to eight sounds, with individual MIDI channels and keyboard ranges. You can play imported audio, record the line input, work with a temporary live buffer, and record four-track MIDI loops.

### Choose the right mode

| You want to... | Choose | What you work with |
| --- | --- | --- |
| Play a keyboard split, layered instrument or multitimbral setup | **Performance** | A patch with up to eight sound slots. |
| Record a take, keep it in recording memory, or export a WAV | **Sampler** | A mono or stereo audio recording in Flash. |
| Explore incoming audio and capture a selected fragment | **Live Sampler** | A temporary circular audio buffer. |
| Record and replay phrases played on your MIDI controller | **MIDI Loop** | Four tracks of MIDI events using the current patch. |

Use Sampler for a take you intend to audition, convert or export. Use Live Sampler when the interesting sound is something you want to find inside an ongoing stream of audio.

### Connect and play

1. Connect a MIDI controller to LILLA's MIDI input.
2. Connect the stereo line output to your mixer, amplifier or audio interface. Start with low listening levels.
3. Power on LILLA and wait for startup to finish.
4. Set the Modes selector to **Performance**.
5. Turn **Select** to highlight the patch number, then turn **Value** to choose an existing patch.
6. Set your controller to the MIDI channel shown for a sound in the patch.
7. Play notes within that sound's `FROM K` and `TO K` range.
8. Raise **Line Out Vol** gradually.

**What you should see:** the Performance page shows a patch number and its active sound rows. The MIDI channel and keyboard range on each row determine which notes can trigger that sound. An eight-slot patch does not have to use every slot.

**What you should hear:** a sound when you play a note inside an active row's range on its assigned MIDI channel. If you hear nothing, start with the channel and range before changing the sample or its envelope.

### Your first edit

1. Press **S1** if Sound 1 is active, or press another active sound's button.
2. Turn Select to highlight `GAIN`, then turn Value a small amount.
3. Play a few notes and listen to the change.
4. Select `RETURN` and press Select.
5. In Performance, choose `SAVE` when it is available.

This introduces the normal editing cycle: **open a sound → adjust it → return to Performance → save the patch**. Returning from a page keeps your working edits, but does not replace the Save step.

### Finish a session

Before switching off, save edited patches, save any MIDI loop you want to keep, and complete any pending Live Sampler capture saves. Wait until writing or export operations finish. The live buffer itself does not survive power-off.

If there is no suitable audio loaded, follow [Importing audio](#importing-audio). Importing replaces the Flash audio library and deletes existing recordings, so back up your work first.

## I/O connections and buttons

| Connection or button | Connector | Description |
| --- | --- | --- |
| Line in | 3.5 mm jack | Stereo line input / stereo dynamic microphone input. |
| Line out | 3.5 mm jack | Stereo line output, 3.1 Vpp. |
| Phones line | 3.5 mm jack | Main headphone output. |
| Phones pre-listen | 3.5 mm jack | Pre-listening headphone output. |
| MIDI IN | 3.5 mm jack | MIDI input. |
| MIDI OUT | 3.5 mm jack | MIDI output. |
| Gate IN | 3.5 mm jack | Gate input, +5 V. |
| Gate OUT | 3.5 mm jack | Gate output, +5 V. |
| USB-C | USB-C | +5 V DC power and programming. |
| Firmware_upload mode button | Button | Enter firmware upload mode. |
| On/off button | Button | Switch the instrument on or off. |

**Future development:** MIDI OUT, Gate IN and Gate OUT are physically available and accessible through classes already included in the firmware codebase. No user-facing features currently use these connections; they are available for future development.

## Controls and navigation

<img src="doc/assets/images/top.jpg" alt="LILLA top panel showing the display, encoders, mode selectors and sound buttons" width="100%">

*Top panel overview: physical controls and their positions.*

The white selection frame identifies the field or command that will respond to the navigation controls. In the supplied photographs, labels are generally cyan, editable values and commands yellow, and page headings red.

**Turning and pressing an encoder are separate actions.** For example, turning Value edits a highlighted parameter, while pressing Value can reset or toggle that parameter on particular pages.

The same controls perform different tasks depending on the active page. Follow the highlighted field and the options currently visible on the screen.

| Control | Main use |
| --- | --- |
| Modes selector | Choose Sampler, Live Sampler, Performance or MIDI Loop. |
| Tools selector | Choose Mixer, Delay, Setup or Test. |
| Tools button | Open the selected tool; the Tools LED indicates tool access. |
| Select, turn | Move the highlight between fields and menu items. |
| Select, press | Execute a menu command, confirm a choice, or enter/leave a group of fields. |
| Value, turn | Change the highlighted parameter. |
| S1-S8 | Open active sounds in Performance; S1 opens a mono or left-channel recording and S2 the right channel in Sampler; capture into sound slots in Live Sampler. |
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

Menus are dynamic. A command may be hidden when the current state does not allow it, for example when there is no recording to export. A saved, unmodified patch may show fewer commands than a patch with pending edits.

### Three navigation patterns

**Menu commands:** turn Select until the frame surrounds the command, then press Select. If a confirmation appears, select the required answer and press Select again.

**Parameter editing:** turn Select until the parameter is highlighted, then turn Value. Listen while you adjust; many changes are immediately audible.

**Tables of sounds or sources:** first select the row or source, then press Select to enter its editable fields. Press Select again to leave that group where the page supports this pattern.

### Useful shortcuts

| Where | Action | Result |
| --- | --- | --- |
| Performance | Press an active S1-S8 button | Open that sound's editor. |
| Sound Edit from Performance | Press the same sound button again | Open its VCF page. |
| Sound Edit from Sampler | Press S1, or S2 for stereo | Select the recording channel; the same button keeps that channel in Sound Edit. |
| Sound Edit, `PITCH` selected | Press Value | Reset the pitch adjustment. |
| Sound Edit, `PITCH` selected | Press Select | Run Auto Tune on the selected audio region. |
| Sound Edit | Press From | Move the region's start to the beginning of the source. |
| Sound Edit | Press To | Toggle the boundary/slice editing behavior. |
| Sound Edit | Press Step | Select the region-relative trim step. |
| Performance or Sound Edit, `PAN` selected | Press Value | Centre the pan. |
| Live Sampler | Press Step | Toggle the start-point lock behavior. |
| Common playback controls | Press Cutoff | Restore maximum common low-pass cutoff. |
| Common playback controls | Press Line Out Vol | Stop active players; also clear delay feedback outside the Direct Sampler delay context. |

Use the last shortcut when you need to stop sounding notes quickly. In MIDI Loop it also stops track playback.

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

### Follow the sound from source to keyboard

A typical setup has three layers:

**Audio file → Sound settings → Instrument in a patch**

For example, `piano.wav` is imported as `piano.raw`. A Sound selects that source and defines its trim, envelope and tuning. An instrument slot assigns it a root key, a MIDI channel and a playable keyboard range.

Changing the trim adjusts which part of the source is played. It does not cut the original source file. Dropping an instrument removes its place in the patch; it does not erase the source audio.

An on-screen label such as `SOUND 1` means the first slot of the current patch. It is not the same thing as audio file 1, recording 1 or patch 1.

### What survives power-off?

| Material | How to keep it |
| --- | --- |
| Edits to a patch and its sounds | Save from Performance. |
| A completed Sampler recording | Finish recording with `STOP`; use backup or WAV export for an external copy. |
| Sampler recording A/B boundaries | Saved automatically while editing; recalled with the recording for keyboard playback and RAW/WAV export. Other recording playback edits remain in the current session. |
| A Sampler recording converted with `MAKE_RAW` | Keep the generated Flash source and save the patch that uses it. |
| Audio still in the Live Sampler buffer | Capture the desired region into a sound slot and save the resulting patch. |
| A newly captured Live Sampler sound | Save its patch so the pending audio is written to Flash. |
| A MIDI loop | Save it to microSD from MIDI Loop. |
| A portable copy of your work | Preserve configuration, recording audio, source library and MIDI-loop files as described in Backup and restore. |

### Patch numbers and the temporary session

Normal patches use **IDs 0-199**. **Patch 200** is the temporary sampling workspace; it is not an extra normal patch slot.

The first successful Live Sampler sound capture creates a normal patch using the **first available ID in 0-199**. Later captures can fill its remaining slots. Go to Performance to review and save that patch.

### File names matter

The Sound header displays only the first eight characters of the source basename and omits `.raw`. Two files with similar names may therefore look alike in that small field. Choose short, distinctive beginnings such as `BassDry` and `BassFX`.

LILLA retains a file's identity when the audio is missing. Reimporting the same basename can reconnect sounds that refer to it. Renaming a file creates a different identity; treat library names as part of your project.

The file-name table supports 260 RAW identities, including the fallback source, generated files and retained missing references. Free audio memory and free file identities are separate resources.

## Performance

<img src="doc/assets/images/1.jpg" alt="Performance page showing a seven-sound keyboard mapping" width="37%">

*An example patch spread across seven sound slots. Each row has its own root key, range and gain.*

Performance is the main page for assembling and saving a playable instrument. Read it from top to bottom: patch and volume at the top, common sound-character controls in the middle, then the active instrument rows.

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

### Build a keyboard split

A split lets one part of the keyboard play a bass sound and another play a pad, piano or lead.

1. Start with a patch containing two active sounds. If necessary, open an existing sound and use `CLONE` to add a slot.
2. Choose the appropriate source and envelope for each sound.
3. In Performance, give both slots the same MIDI channel.
4. Set Sound 1's `TO K` to the last key of the lower zone.
5. Set Sound 2's `FROM K` to the next key above it.
6. Test the two notes on either side of the split point.
7. Balance the gains and save the patch.

Ranges are inclusive. If Sound 1 ends on the same key where Sound 2 starts, that key triggers both sounds.

### Build a layer or multitimbral setup

For a **layer**, assign the same MIDI channel and overlapping ranges to two or more slots. Start with lower individual gains, then raise them while listening to the combined sound.

For a **multitimbral setup**, assign different channels to different slots. A sequencer or controller can then address each part separately. A MIDI channel selects which instrument responds; the slot number does not automatically establish that channel.

Each triggered layer uses playback voices. A four-note chord with two layers can require eight voices before release tails are counted.

### Set the root key

The root key is the keyboard reference for the sample. At the root key, the sample plays with the Sound's own pitch adjustment; notes above and below it transpose the source relative to that reference.

For a pitched sample, set the root to the note represented by the recording. For a single-key hit, set `FROM K` and `TO K` to the same trigger note and choose a root appropriate to the playback pitch you want.

Octave labels depend on Setup's `FIRST OCTAVE`. When comparing settings with another device, compare the actual MIDI key as well as its displayed octave name.

### Save or duplicate a patch

Use the commands shown in the Performance menu:

- `SAVE`: store the current patch and edited sounds.
- `CLONE`: create a copy in another available patch slot.
- `SAVE_AS_NEW`: store the edited result as a new patch.
- `EXIT`: discard the current edits through the Performance exit workflow.
- `DROP`: delete the patch, after the deletion confirmation.

A sound's `RETURN` command keeps edits in the current session. Save the patch to make those edits persistent.

Use `CLONE` when you want a second patch to develop from an existing one. Use `SAVE_AS_NEW` when you have edited a patch and want to preserve the result under another available patch ID. Read the displayed destination and confirmation before proceeding.

**Before moving into a sampling workflow, save the patch you were editing.** Live Sampler can keep the previous Performance patch in memory, but creating a new captured patch requires that previous patch to have no pending edits.

## Editing a sound

<img src="doc/assets/images/2.jpg" alt="Sound Edit page with envelope, playback mode and sample waveform" width="37%">

*The waveform belongs to the selected source. `FROM`, `TO` and `TOT` describe the playable region; `TRIM STEP` controls the editing increment.*

From Performance, press **S1-S8** for an active sound to open Sound Edit. An unused slot displays `SOUND ... IS NOT USED`; pressing it does not create a new sound.

### Choose and trim the source

1. Highlight the file field with **Select**.
2. Turn **Value** to choose an available source.
3. Play the sound from your MIDI controller.
4. Turn **From** and **To** to adjust the playback region.
5. Turn **Step** to select a suitable trimming increment: use larger steps to find the region, then smaller steps to refine it.

Changing the source file resets the trim region to the new file's full length and resets its pitch adjustment. Recheck the boundaries after changing files.

### Work from a rough cut to a precise region

Start with a large trim step to remove long silences or locate a phrase. Reduce the step near the desired attack and endpoint. Play the sample repeatedly as you refine it: the waveform helps locate an event, but listening tells you whether you have removed its attack or left an unwanted tail.

Pressing **From** returns the start to the source beginning. Pressing **To** switches between editing an endpoint and working with a slice. In slice behavior, moving From can move the region while retaining its length; watch the region display as you move it.

A short region is useful for a repeating texture. A longer region may preserve the natural attack and decay of an instrument. If you change the source afterward, repeat the trimming process because the source change resets its boundaries.

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

| Playback mode | How the region is read | Typical use |
| --- | --- | --- |
| Once FWD | From start to end once. | Hits, spoken words and natural decays. |
| Once REV | From end to start once. | Reverse impacts and swells. |
| Loop FWD | Repeats in the forward direction. | Sustained tones and repeating phrases. |
| Loop FWD/REV | Alternates direction, beginning forward. | Textures that turn around at the boundaries. |
| Loop REV/FWD | Alternates direction, beginning in reverse. | A reverse-starting variation of an alternating loop. |
| Loop REV | Repeats in the reverse direction. | Reversed repeating textures. |

Envelope and note-release behavior still affect what you hear. A looping region can fade away with the envelope; a one-shot source still has a finite end.

**A simple envelope starting point:** use a short attack for percussion, a slower attack for a pad, and a release long enough to avoid an abrupt ending. With a loop, raise sustain to hear the repeated region clearly before shaping decay and release.

For a clean loop, refine its start and end points before increasing Noclick. The available Noclick range depends on the selected region.

### Tune a sound and use Auto Tune

The `PITCH` value shows the playback ratio: **1.000** is the unadjusted source speed. Raising pitch also makes ordinary sample playback faster; lowering it makes playback slower.

1. Select `PITCH`.
2. Turn **Value** to tune by ear, or press **Value** to reset the adjustment.
3. Press **Select** to run **Auto Tune** on the chosen region.
4. Listen to the result at the root key and compare it with your other instruments.
5. Save the patch if you want to keep the adjustment.

Auto Tune is available for both one-shot and loop playback. It analyses the strongest frequency component and adjusts it toward the nearest chromatic note. A strong harmonic, noisy attack or unpitched sample may not correspond to the musical fundamental you expected. For a clearer result, select a stable pitched portion and check the result by ear.

If an `AUTO-TUNE` error appears, read its reason: an invalid region, unavailable source, no measurable signal or a busy audio operation needs a different response. Auto Tune does not replace choosing the correct root key and keyboard range.

The `MAX PITCH` display describes the available playback ceiling for the current source path. It can change with source caching and playback preparation; do not assume every source has the same transposition ceiling.

### Return, clone or remove a sound

The following commands apply to sounds opened from Performance. For a recording opened from Sampler, the only menu command is `RETURN`; see [Edit a recording before conversion or export](#edit-a-recording-before-conversion-or-export).

- `RETURN` keeps the edits and returns to the previous performance page.
- `CLONE` copies the instrument into a free slot in the current patch. Adjust the copy's key range, root key or source as needed.
- `DROP` removes the instrument from the current patch.

Save the patch after finishing. Removing a sound slot is different from deleting its source audio file.

## Importing audio

> **Import replaces the audio library.** Confirming import erases the previous Flash audio files and Sampler recordings. Create a backup and keep your source audio on your computer before proceeding.

<img src="doc/assets/images/12.jpg" alt="Audio import page showing source files, Flash capacity and the erase warning" width="37%">

*Check both the source report and destination capacity before importing. This photograph says 35 seconds; the current firmware limit is 3 MiB of decoded mono PCM, approximately 35.7 seconds.*

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

A simple card layout is:

```text
microSD root/
  LILLA_AUDIO/
    BassDry.wav
    Bell.aiff
    DrumLoop.mp3
    Texture.raw
```

Put the files directly in that folder. Keep their basenames distinct and no longer than 31 bytes. Plain ASCII names are an easy way to stay within that limit. Avoid names reserved for recording data, such as `P12.raw`.

A compressed MP3 may be small on your computer but much larger after conversion. Judge Flash requirements from the import report, which accounts for the decoded audio.

Each imported file is limited to 3 MiB of decoded mono PCM, approximately 35.7 seconds. Longer files are truncated. MP3 decoding and rate conversion can take longer than PCM import.

### Import the files

1. Insert the prepared microSD card.
2. Open **Tools > Setup**.
3. Select `IMPORT AUDIO FILES FROM /LILLA_AUDIO` and press Select.
4. Review the import report and final erase warning.
5. Confirm only when you are ready to replace the current library and recordings.
6. Wait for copying, memory configuration and restart to complete.
7. Open Sound Edit and select the imported source you want to use.

After import, audition a few sources in Sound Edit before rebuilding an entire patch. Confirm the attack, endpoint and pitch, especially for a long or converted source.

**For a stereo instrument:** export left and right as separate mono files, import both, assign them to two sound slots with matching root keys and ranges, then pan them left and right. The standard stereo-file import itself produces a mono source.

The import screen reports invalid files, duplicates and Flash capacity problems. If too little memory remains for sampling, prepare a smaller library or shorter files.

## Recording with Sampler

Sampler records the line input into Flash and lets you audition, convert or export the result.

<img src="doc/assets/images/6.jpg" alt="Sampler in PAUSE+REC with left and right input meters" width="37%">

*The input meters let you set recording gain before starting. Free recording time and free audio-file space are shown separately.*

### Understand the recording stages

**Monitor, record, stop, audition, then convert or export.**

Monitoring in `PAUSE+REC` lets you prepare the source and levels. `MONO_REC` or `STEREO_REC` begins the take. `STOP` ends it. Afterward, `MAKE_RAW` creates a source for a patch, while `EXPORT_WAV_TO_SD` creates a file for use outside LILLA.

Starting a take records audio for 20 ms before accepting the next control action, allowing the initial fade-in to finish. An immediate Stop is processed after this short pause and preserves the take. Stopping or leaving Sampler waits for recording to finish before saving it.

A Flash recording can be played from the keyboard and edited before conversion. Successful RAW conversion or WAV export removes the source recording and frees its recording space. Make a backup first if you need to preserve the full original take.

### Make a recording

1. Connect your audio source to the stereo line input.
2. Set the Modes selector to **Sampler**.
3. Select `PAUSE+REC` to monitor the input before recording.
4. Adjust the displayed line-input gain with Value when the gain field is selected. Watch both level meters and avoid persistent red peaks.
5. Select `MONO_REC` or `STEREO_REC`.
6. Start your source and watch the elapsed time and available recording memory.
7. Select `STOP` when finished.
8. Select the recording you want to audition and adjust its playback volume as needed.

**Set gain using the loudest part of the source.** A level that looks comfortable during a quiet passage can clip during an accent. Rehearse that loud section in `PAUSE+REC`, then record the take. Reducing playback volume afterward cannot undo distortion recorded at the input.

The memory display distinguishes space available for recordings from space available for RAW files. A recording can fit even when there is insufficient room to convert it to RAW.

### Edit a recording before conversion or export

1. Finish recording with `STOP`, or select an existing completed recording in Sampler.
2. Press **S1** for a mono recording. For stereo, press **S1** for the left channel or **S2** for the right channel.
3. In Sound Edit, play the recording from your MIDI keyboard while adjusting its playback settings.
4. Use **From** and **To** to set the first and last sample, shown as the **A/B** boundaries. Use Step to refine the region.
5. Adjust pitch, gain, pan, MIDI channel, attack curve, ADSR, playback mode or Noclick as needed.
6. Select `RETURN` and press Select to return to **SAMPLER**.

The source remains the selected recording in Flash; its audio is not rewritten when you trim or edit it, and the source selector cannot be changed in this editor. Keyboard playback uses the selected region and remains available during editing.

For a stereo recording, every edited playback parameter is automatically copied to the other channel, including A/B, tuning and Auto Tune, gain, pan, MIDI channel, attack curve, ADSR, playback mode and Noclick. Edits from either channel affect both. The original left/right pan positions remain until you edit pan; changing or centring pan applies the same value to both channels.

The Sampler editor offers only `RETURN`, with no separate save or discard choice. A/B boundaries are saved automatically and retained for later playback and export, including after selecting another recording or restarting LILLA. Leaving the editor also retains the boundaries. Other playback parameters remain active in the current editing session but are not stored as persistent recording settings.

RAW conversion and WAV export use the saved inclusive A/B region. Stereo exports use a common start and end for both channels, keeping their durations aligned. These operations copy the selected audio region; they do not render the editor's pitch, envelope, gain, pan or other playback effects into the exported audio. To export the full take, restore A/B to the full recording before exporting.

### Make a playable RAW file

1. Select the recording and, if needed, use S1/S2 to edit its A/B region, then `RETURN`.
2. Choose `MAKE_RAW` and confirm the conversion.
3. Choose the available output: `MAKE_MONO`, `MAKE_LEFT`, `MAKE_RIGHT` or `MAKE_BOTH`.
4. Wait for the conversion to finish.
5. Open Sound Edit and choose the generated RAW source.
6. Save the patch that uses it.

| Conversion choice | Result |
| --- | --- |
| `MAKE_MONO` | Create a mono source from the recording. |
| `MAKE_LEFT` | Create a source from the stereo recording's left channel. |
| `MAKE_RIGHT` | Create a source from its right channel. |
| `MAKE_BOTH` | Create separate left and right sources. |

The available choices depend on the selected recording. For stereo recordings, left and right can become separate RAW sources. `CANCEL` exits the conversion choices. If LILLA reports that it cannot create a RAW file, check free RAW memory and available filenames.

Conversion copies the saved A/B region. After all requested RAW files are created successfully, the original Sampler recording is deleted and its space is freed. A failed conversion retains the recording. Back up the original take before conversion if you want to keep it.

### Export a WAV file

Insert a microSD card, select a recording and choose `EXPORT_WAV_TO_SD`. The exported WAV contains its saved A/B region, preserves the recording's mono or stereo layout and uses 16-bit PCM at 44.1 kHz.

Files are written to `/LILLAWAV_EXPORT`, with names such as `0M.wav` for mono or `0S.wav` for stereo. Wait for the success message before removing the card.

After a successful WAV export, LILLA deletes the original Sampler recording and frees its recording slot and Flash packets. If the export fails, the original recording is retained. To keep the recording in LILLA as well as an external copy, use backup instead.

Sampler supports up to 30 recordings. When every slot is occupied, `PAUSE+REC` is hidden and a temporary notice asks you to delete or export a recording before recording another take.

WAV export is useful when you want to edit a take on a computer, share a recording, or keep an audio copy independent of LILLA's configuration. This command exports Sampler recordings; it is not a general export command for every RAW source in Flash.

`CANCEL_RECORDING` deletes the selected recording. Export anything you want to retain first.

## Live Sampler

<img src="doc/assets/images/4.jpg" alt="Live Sampler before recording, with playback and buffer controls" width="37%">

*The empty-buffer view shows capacity, input gain, playback mode, feedback and start-point controls.*

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

### Read and navigate the live waveform

<img src="doc/assets/images/5.jpg" alt="Live Sampler with a recorded waveform and a selected loop" width="37%">

*Here the display window is 1.3 seconds, while the selected loop is 0.46 seconds. Zooming the view and changing the loop length are separate operations.*

| Field | Meaning |
| --- | --- |
| `BUFFER` | Total live recording capacity for the selected mono/stereo layout. |
| `LINE IN GAIN` | Gain applied to the incoming recording signal. |
| `PLAY MODE` | Direction and looping behavior. Live capture into a sound requires a loop mode. |
| `FEEDBACK` | Amount of previous material fed back during live recording. |
| `WINDOW` | Amount of audio time visible in the waveform view. |
| `START POINT` | Start-position behavior and its displayed position or relationship to recording. |
| `LOOP` | Duration of the selected repeating region. |
| `STEP` | Increment used to move the region controls. |

A smaller Window helps inspect a transient or a short loop. It does not automatically shorten the selected audio. Use **To** to adjust the loop length, and **From** to move its start.

Start with modest feedback and adjust while listening. Feedback changes the recorded material, so compare the result before increasing it further.

Pressing Step switches the live start-point lock behavior. **FIXED** holds a location in the circular buffer. The unlocked behavior follows the recording position, and the page can show **SYNC**, **BEHIND** or another relative position according to the offset.

For your first capture, stop recording and work on a fixed region. Once you are comfortable finding and trimming material, experiment with a moving start point while recording. As the buffer wraps, old material is replaced.

`ERASE` clears the recorded buffer. Changing mono/stereo also resets it, so choose the layout before recording material you want to keep.

### Continuous FWD playback while recording

While Live Sampler is recording, held notes in **FWD** continue around the circular buffer instead of stopping after one buffer length. This applies to **SYNC**, relative start positions and **FIXED**, in mono and stereo. Lower notes can therefore remain active beyond 40 seconds in mono (80 seconds at half speed). Note-off and the sound envelope still control the voice.

During the first fill, reaching audio that has not yet been recorded still fades and stops the voice. Once recording stops, FWD resumes its normal one-shot behavior from the current playback position. Reverse and loop modes are unchanged.

### Stereo recording compressor

On the Live Sampler page, turn **Select** to highlight the yellow `ON`/`OFF` value beside `COMPRESSOR`, then press **Select** to toggle it. The control is on the `FEEDBACK` row, aligned with `LOOP`. It starts **off** at power-on; its setting is retained during the current session but is not saved in a patch. All sound buttons, **S1-S8**, remain available for capture into the corresponding patch slots.

The compressor acts on the sum of line input and feedback before writing to the live buffer. Audio processing runs only while Live Sampler is recording, including when its Mixer or Delay page is open; otherwise the block drains its inputs without allocating output blocks. The ON/OFF setting is retained, and lookahead history is cleared when recording resumes. Left and right share the same gain reduction, including when recording a mono mix. It begins reducing gain around -6 dBFS and limits sample peaks to approximately -1 dBFS once fully enabled. It does not repair clipping that has already occurred at the input or elsewhere in the feedback path, and does not process material already recorded.

A 128-sample lookahead adds approximately 2.9 ms to the recording path, including when the compressor is off. Switching uses a gradual 10 ms transition between equally delayed signals, so the recording timeline does not jump. Gain recovery takes approximately 100 ms per time constant. Strong compression can still change the sound and feedback behavior. During bypass or the transition to/from bypass, full peak protection is not guaranteed.

### Capture a live loop into a patch

1. Save any pending edits to the performance patch before starting this workflow.
2. Record some live audio, then select `STOP`.
3. Select a **loop** playback mode and refine the region.
4. Press the desired **S1-S8** slot button.
5. If replacing an occupied capture slot, answer `REPLACE CAPTURE?` before continuing.
6. When prompted, play a MIDI key to set the captured sound's root key, or choose Cancel.
7. Capture more regions into other slots if desired.
8. Switch to Performance, review the newly created patch and choose `SAVE`.
9. Wait for pending audio writes to complete before powering off.

The first capture uses the first free normal patch ID in **0-199**. Patch **200** remains the temporary sampling workspace.

Each captured sound initially plays on the key used for the root-key prompt: its lower and upper key limits are set to that same key. Expand the keyboard range in Performance if you want to play it melodically.

**Stereo slot allocation:** select a slot with its following slot free, such as S1 with S2 free, to capture left and right separately. The two sounds are panned left and right. Replacing an existing capture pair can reuse that pair. If a second slot is not available, including a new capture on S8, the selected region is captured as a mono mix of the two channels into one slot.

The selected region must fit the capture cache, and free patch, sound and file resources must be available. The live buffer can be longer than one captured sound; shorten the selected loop if it exceeds the capture limit.

Captured audio initially remains in PSRAM. Saving the patch writes its pending captured audio as RAW files to Flash. Wait for saving to finish; a failed save must be retried before powering off.

### If the previous patch has unsaved edits

The message:

> OPEN PERFORMANCE<br>
> AND SAVE THE PREVIOUS PATCH

refers to the Performance patch you were using before entering Live Sampler.

1. Stop live recording if it is still running.
2. Return to Performance and complete any exit confirmation.
3. Save the previous patch.
4. Return to Live Sampler.
5. Select the desired region and press the sound-slot button again.

You are not being asked to save patch 200. The warning protects edits to the previous normal patch before a new captured patch is created.

### Capture messages

| Message | Next action |
| --- | --- |
| `NO RECORDED AUDIO` | Record some input before playing or capturing it. |
| `STOP REC AND SELECT LOOP MODE` | Stop recording and select a looping playback mode. |
| `LOOP TOO LONG FOR CACHE` | Shorten the selected region. |
| `NO FREE PATCH` | Free a normal patch slot after preserving anything you need. |
| `NO FREE SOUND / CACHE / FILE` | Save pending work and review available sound, audio-cache and file resources. |
| `CAPTURE CACHE UNAVAILABLE` | The required audio memory could not be acquired; preserve pending work before retrying. |
| `SAVE BUSY - TRY AGAIN` | Let audio activity settle and retry Save. |
| `RAW SAVE FAILED - RETRY` | Retry saving and keep the instrument powered while captured audio remains pending. |

## MIDI Loop

<img src="doc/assets/images/8.jpg" alt="MIDI Loop page with four tracks, level, shift and transposition" width="37%">

*The four columns are MIDI tracks. The lower sound indicators show activity associated with the patch's sounds.*

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

### Add parts without replacing the master

Record the part that defines the phrase length on Track 1 first. Once it repeats correctly, add a second part on Track 2, then continue on Tracks 3 and 4. Use a different controller MIDI channel if the new part should address another sound in a multitimbral patch.

The recorded events trigger the patch's current sounds. Changing a source, keyboard range or MIDI assignment can therefore change how an existing loop sounds. Preserve the patch and audio library alongside the loop when you want to reproduce the arrangement later.

### Play and edit

- Turn **Loop** to browse saved loops.
- Press Loop to stop or restart the group of tracks.
- Press a **Track** encoder to stop or start that individual track.
- Turn **Tempo** to change playback timing; press it to reset the timing adjustment.
- Use Select to choose the track parameter row, then turn each Track encoder to change that track's level, pitch or time shift.

### Understand the track controls

| Row | What it changes | Starting point |
| --- | --- | --- |
| `LEVEL` | The playback level of that track. | 1.0 for an unadjusted level. |
| `SHIFT` | The track's timing offset within the loop. | 0.00 seconds for no shift. |
| `TRANSP` | The track's note transposition. | 0 keys for the original notes. |

Use a small Shift to move a part ahead of or behind the other tracks. Listen at the loop boundary as well as in the middle of the phrase. Use Transposition to try a different pitch for a part, then return it to zero to compare with the original.

Turning Tempo changes the timing of the MIDI sequence. It does not rewrite a sample's waveform or automatically time-stretch a recorded audio phrase.

### Save loops

Use `SAVE` to update a saved loop and `SAVE_AS_NEW` to keep another version. `NEW` starts a new loop; `DELETE` removes a saved loop.

Loop files are stored in `/LILLALOOP`. Keep a copy of this folder when archiving your work. Loop files contain MIDI data, so preserve the required patch and source audio as well.

## Mixer, delay and filters

### Mixer

<img src="doc/assets/images/7.jpg" alt="Mixer page with sound sources, line input and separate output routes" width="37%">

*The highlighted column is the selected source. LINEOUT and MONITOR are separate routes.*

Open **Tools > Mixer** to adjust source gain or mute, pan, and routing to line and monitor outputs. Turn Select to choose a source column, then press it to enter the fields. Turn Select to choose a field and Value to change its setting; press Select to return to source selection.

Use the separate line and monitor routes to decide what your audience hears and what you hear while monitoring. If a source is silent, check its mute/gain and output route as well as the patch volume.

### Delay

<img src="doc/assets/images/9.jpg" alt="Delay page with sound routing, feedback, time and stereo modulation" width="37%">

*The ROUTING row selects which sound slots feed the delay. The example values are not recommended defaults.*

Open **Tools > Delay** to adjust feedback, delay time, the left/right time relationship, modulation source, modulation frequency, modulation depth and left/right modulation phase.

Start with low feedback, then increase it while listening. Check the instrument's delay routing if you hear no delayed signal. Save the patch to retain its delay settings.

For a first delay sound, route one instrument, use a modest feedback level and choose a clearly audible delay time. Play short notes with gaps between them so you can hear the repeats. Then adjust the left/right time difference for stereo separation.

Modulation varies the delay over time. Introduce depth gradually, then adjust its rate and left/right phase while listening. Higher feedback lets repeats accumulate, so reduce it if the delayed signal overwhelms the dry sound.

### Filters and sound character

<img src="doc/assets/images/3.jpg" alt="Instrument VCF page with low-pass filtering and LFO modulation" width="37%">

*The individual VCF shapes one instrument. The common LPF cutoff remains visible above it.*

To reach the VCF from Performance, press the active sound's button to open Sound Edit, then press the same sound button again. Use Select to highlight a filter parameter and Value to edit it.

| Filter type | Audible effect |
| --- | --- |
| Lowpass | Reduces frequencies above the cutoff; useful for darkening a bright source. |
| Highpass | Reduces low frequencies; useful for thinning a sound or removing low-end weight. |
| Bandpass | Emphasizes a region between low and high frequencies. |
| Notch | Removes a band of frequencies. |
| None | Disables the instrument filter. |

Resonance emphasizes the filter's response around its characteristic frequency. Begin with a moderate setting, then listen while moving the cutoff. The modulation source and depth determine whether and how that setting moves over time; the frequency/time field follows the selected modulation type.



The common **Cutoff** control changes the low-pass filter. Press it to restore maximum cutoff. **Resolution** and **Downsampling** add digital coloration.

The instrument VCF page provides filter type, cutoff, resonance and modulation settings. Adjust these while playing the selected sound so you can hear how they interact with its sample and envelope.

A locked instrument is protected from selected performance changes. Check `LOCK` if pitch bend, resolution or downsampling appears to have no effect on that sound.

## Setup and MIDI controls

<img src="doc/assets/images/10.jpg" alt="Setup page with tuning conventions, MIDI assignment and storage operations" width="37%">

*Setup combines global playing preferences with audio-library and backup operations.*

Open **Tools > Setup** for:

- `KEY STEP`: pitch mapping increments of one semitone, half a semitone, quarter or eighth of a semitone.
- `FIRST OCTAVE`: the octave-number convention used for note display.
- `CONTROL CHANGE ASSIGNMENT`: MIDI CC assignments for gains of Sound 1-8 and the low-pass cutoff.
- Audio import, backup, restore and factory reset.

### Pitch steps and note names

With `KEY STEP` at one semitone, adjacent MIDI notes use the normal chromatic pitch spacing. Smaller steps spread a smaller pitch interval over each keyboard step, allowing half-, quarter- or eighth-semitone playing. Return to one semitone when checking a conventional keyboard mapping.

`FIRST OCTAVE` changes the octave numbering used in the display. It helps match your controller's naming convention; it should not be used as a substitute for setting the instrument's root key.

### Assign a controller knob

<img src="doc/assets/images/11.jpg" alt="Control Change Assignment page for eight sound gains and LPF cutoff" width="37%">

*Each destination can have a CC assignment. A dash means no assignment.*

1. Open `CONTROL CHANGE ASSIGNMENT`.
2. Select the gain destination or LPF cutoff.
3. Set the desired CC number.
4. Set your MIDI controller's knob or slider to transmit that CC.
5. For a sound gain, use the MIDI channel assigned to that sound.
6. Return and test the control while playing.

Use a dash to leave a destination unassigned. Check both the controller number and the transmitting channel if moving the external control has no effect.

### Check incoming MIDI

<img src="doc/assets/images/15.jpg" alt="MIDI monitor displaying a NoteOn message, channel, note and velocity" width="37%">

*This example confirms reception of a NoteOn on channel 1. The note name follows the current octave-display convention.*

Open **Tools > Test** to monitor incoming MIDI message types. This is useful when checking whether the controller is sending notes, pitch bend, aftertouch or control changes.

If the monitor receives notes but Performance is silent, the connection is working: check channel mapping, key ranges and audio routing next. If no message appears, check the controller output, cable and selected connection before editing the patch.

## Backup and restore

### Plan a complete archive

A configuration backup is one part of preserving a session. Keep the matching audio library and MIDI-loop files with it so the saved settings have the material they need.

| Item | Numbered configuration/recording backup | Additional action |
| --- | --- | --- |
| Saved patches, sounds and configuration | Included. | Save current edits before backing up. |
| File-name associations | Included. | Keep corresponding audio basenames unchanged. |
| Sampler recording audio | Included. | WAV export is also useful for computer access. |
| Sampler recording A/B boundaries | Included in current backups. | Older backups without trim metadata restore the full recording region. |
| Imported RAW library | Not included. | Retain the original import library separately. |
| RAW files generated from recordings or live captures | Not included as a complete RAW-library archive. | Keep an independent recoverable audio copy; saving to Flash alone is not an external backup. |
| MIDI-loop directory | Not included. | Copy `/LILLALOOP` from the card. |
| Temporary live buffer | Not included. | Capture and save useful material before power-off. |

For a Live Sampler capture, this firmware's Sampler WAV-export command does not provide a general RAW-library export. Do not assume that a numbered backup alone can restore every captured RAW source after that source is erased.

### Create a backup

1. Save your current patch edits.
2. Insert a microSD card with sufficient free space.
3. Open Tools > Setup.
4. Select `NEW NUMBERED BACKUP IN /LILLABACKUP` and confirm.
5. Wait for the backup success message.
6. Copy the backup folder to your computer for safekeeping.

Backups are created in numbered directories such as `/LILLABACKUP/000001`. They contain configuration and Sampler recording audio.

Keep your imported source library separately. The backup operation does not copy the entire imported RAW library, the temporary live buffer or the MIDI-loop directory. Copy `/LILLALOOP` separately and retain your import files. Save live captures into a patch before archiving their settings, and account for the RAW-audio limitation described above.

### Restore a backup

<img src="doc/assets/images/13.jpg" alt="Restore confirmation warning that patches, sounds and recordings will be replaced" width="37%">

*Choose YES only after preparing the intended backup in the card's backup root.*

1. On your computer, choose the numbered backup you want to restore.
2. Copy **the contents** of that directory into `/LILLABACKUP` on the card, keeping its configuration and recording files together.
3. Check that `/LILLABACKUP/LILLA_CONFIG.fram` exists. Leaving it only inside `000001`, for example, is not enough.
4. Insert the card and open Tools > Setup.
5. Select `RESTORE CONFIG + AUDIO FROM /LILLABACKUP ROOT` and confirm.
6. Wait for restoration and any requested restart to finish.
7. Check the restored patches and recordings, and that their imported source files are available.

The restore location should look like this:

```text
microSD root/
  LILLABACKUP/
    LILLA_CONFIG.fram
    [matching recording-audio files from the chosen backup]
```

The bracketed line above is a description, not a filename to create. Copy the actual recording files together with the configuration; do not mix files from different numbered backups.

Restoration replaces configuration and restores recording audio. Back up the current state before restoring a different one. Keep the original backup intact: invalid or missing recording audio can prevent a recording from being recovered.

### Factory reset

<img src="doc/assets/images/14.jpg" alt="Factory reset confirmation on the Setup page" width="37%">

*Factory reset is a destructive configuration operation, not a way to leave an editing page.*

`FACTORY RESET` deletes patches, sounds and recordings. Make a backup before confirming. Wait for the reset and restart to complete.

## Updating the firmware on Windows

The firmware is the program that runs LILLA. To load a compiled firmware on Windows 10 or 11, use **Teensy Loader (`teensy.exe`)**. It is a standalone application: download it and run it without an installer. **Teensyduino** is the Arduino development add-on; you do not need it, Arduino IDE or PlatformIO to upload the supplied HEX file. See the [PJRC download page](https://www.pjrc.com/teensy/td_download.html) for the distinction between development tools and the standalone loader.

### What you need

- Your LILLA instrument, which uses a Teensy 4.1.
- A Windows 10 or 11 computer.
- A USB **data** cable matching the computer and LILLA's USB-C connector. A charging-only cable cannot transfer firmware.
- The LILLA firmware file, for example `Lilla_v7_0_2.hex`.
- Teensy Loader, downloaded from PJRC.

### Download the firmware

1. Open the [main branch of the LILLA GitHub repository](https://github.com/SandroGrassia/Lilla_Audio_Sampler/tree/main). Confirm that the branch selector shows **main**.
2. In the project's top-level file list, open the published `.hex` file for your instrument, for example `Lilla_v7_0_2.hex`. This is the compiled firmware; **Code > Download ZIP** downloads the project sources instead.
3. On the HEX file page, click **Download raw file**. Download firmware only from **main**. The **develop** branch contains work in progress and may contain faulty or untested builds.
4. Save the file in a folder you can find easily, such as `Downloads/LILLA`. Check that its name ends in `.hex`, not `.html` or `.txt`.

The version numbers in `Lilla_v7_0_2.hex` identify the firmware version and revision. Keep the downloaded copy if you want to retain that exact build. If no HEX file is available on **main**, wait for the maintainer to publish it; do not substitute a file from **develop**.

### Download Teensy Loader

Open the [official PJRC Windows loader page](https://www.pjrc.com/teensy/loader_win10.html) and click **Teensy Loader Program**. Save `teensy.exe` and double-click it. The small Teensy Loader window should appear. You can keep the executable in the same folder as the HEX file. PJRC's [first-use instructions](https://www.pjrc.com/teensy/first_use.html) explain that programming mode uses the USB drivers built into Windows; no separate programming driver is required.

### Prepare LILLA

Save your current patch and any MIDI loops, and complete pending recording or export operations. Use [Backup and restore](#backup-and-restore) to preserve your work before changing firmware. Check any compatibility or migration instructions supplied with the new version.

Lower your amplifier or mixer level, then connect LILLA's USB-C connector to the computer with the data cable. Keep power and USB connected throughout programming.

### Upload and restart

1. In Teensy Loader, leave **Automatic Mode** off for this manual procedure.
2. Choose **File > Open HEX File** and select the downloaded `Lilla_v7_0_2.hex`. Confirm the filename shown in the loader.
3. Briefly press and release LILLA's **Firmware_upload mode** button. This is the programming button, not the On/off button. The instrument's current program stops and the loader should detect the Teensy.
4. Choose **Operations > Program**. Wait for **Download Complete** before disconnecting anything.
5. Choose **Operations > Reboot**. LILLA should restart.
6. Check the firmware version on LILLA's welcome screen. The welcome screen should show version 7.0.2. Load a familiar patch and check playback at a low listening level.

The manual controls above follow the [PJRC Windows loader instructions](https://www.pjrc.com/teensy/loader_win10.html). Updating firmware programs the Teensy's internal program memory; importing audio from the SD card is a separate operation.

### If the upload does not start

| Symptom | What to check |
| --- | --- |
| The loader does not detect LILLA | Press and release the Firmware_upload mode button after connecting USB. Try a known data cable and another computer USB port. |
| The HEX file cannot be opened | Download the raw `.hex` file again. Check that you did not save the GitHub web page or a source archive. |
| Programming finishes but LILLA does not start | Choose Operations > Reboot after Download Complete. If necessary, reconnect and repeat with the correct LILLA firmware. |
| The welcome screen shows the old version | Check which HEX file is open in the loader and repeat Program, followed by Reboot. |

## Updating the firmware on Mac

On macOS, use the standalone **Teensy Loader** application and the published LILLA `.hex` file. Arduino IDE, PlatformIO and the Teensyduino development add-on are not required to upload a compiled firmware. PJRC lists the standalone loader on its [download page](https://www.pjrc.com/teensy/td_download.html).

### What you need on Mac

- Your LILLA instrument, which uses a Teensy 4.1.
- A Mac compatible with the current Teensy Loader download.
- A USB **data** cable matching the Mac and LILLA's USB-C connector. If an adapter is necessary, it must support USB data.
- The published LILLA HEX file and the macOS Teensy Loader application.

### Download the firmware on Mac

1. In your browser, open the [main branch of the LILLA GitHub repository](https://github.com/SandroGrassia/Lilla_Audio_Sampler/tree/main). Confirm that the branch selector shows **main**.
2. Open the published `.hex` file in the project's top-level file list, for example `Lilla_v7_0_2.hex`.
3. Click **Download raw file** and save it in a convenient folder, such as `Downloads/LILLA`.
4. In Finder, confirm that the downloaded file ends in `.hex`. A GitHub web page or **Code > Download ZIP** source archive cannot be loaded as firmware.

Use firmware from **main** only. Files on **develop** are work in progress and may be faulty or untested. If main has no HEX file, wait for the maintainer to publish it. Keep the downloaded copy to preserve that exact build.

### Download and open Teensy Loader on Mac

1. Open the [official PJRC Mac loader page](https://www.pjrc.com/teensy/loader_mac.html) and download **Teensy Loader Disk Image**.
2. In Finder, open the downloaded `.dmg`. It contains the Teensy Loader application.
3. Copy the application to **Applications** for convenient reuse, then open it. Confirm **Open** if macOS asks about the downloaded application.

If macOS blocks an unverified application, first check that it came from the official PJRC download. After attempting to open it, use **Apple menu > System Settings > Privacy & Security > Open Anyway**, then confirm **Open**, if that option is available. Follow [Apple's instructions for opening downloaded apps](https://support.apple.com/en-us/102445); do not disable macOS security globally. If the loader reports an unsupported system, obtain a compatible version from PJRC.

### Prepare LILLA on Mac

Save your patch and MIDI loops, finish pending recording or export operations, and make a backup using [Backup and restore](#backup-and-restore). Read any compatibility or migration instructions accompanying the firmware.

Lower the listening level. Connect LILLA's USB-C connector to the Mac with the data cable, and keep power and USB connected throughout programming. Allow the USB accessory connection if your Mac asks for permission.

### Upload and restart on Mac

1. Leave Teensy Loader's **Automatic Mode** off.
2. Choose **File > Open HEX File** and select the downloaded LILLA HEX file.
3. Briefly press and release LILLA's **Firmware_upload mode** button, rather than On/off. The loader should detect the Teensy.
4. Choose **Operations > Program** and wait for **Download Complete**.
5. Choose **Operations > Reboot** to restart LILLA.
6. Check the version on the welcome screen and test a familiar patch at a low listening level. For this release, the welcome screen should show version 7.0.2.

These controls are described in the [PJRC Mac loader instructions](https://www.pjrc.com/teensy/loader_mac.html).

### If the Mac cannot upload

| Symptom | What to check |
| --- | --- |
| Teensy Loader will not open | Check the download source, macOS permission prompt and the loader's system requirements. |
| LILLA is not detected | Reconnect USB, allow the accessory if prompted, and briefly press Firmware_upload mode. Try another data cable, port or adapter. |
| The HEX cannot be opened | Download the raw HEX from main again and check its extension in Finder. |
| Programming completes but LILLA does not restart | Choose Operations > Reboot after Download Complete. |

## Troubleshooting

### Diagnose silence in a useful order

1. **MIDI:** does Tools > Test show incoming notes?
2. **Mapping:** is an active sound assigned to that channel and note range?
3. **Source:** is the expected audio source present and is the region valid?
4. **Envelope and level:** are gain and sustain sufficient, and is attack unusually long?
5. **Routing:** is the source unmuted and routed to the output you are using?
6. **Output:** are patch volume, external mixer level and the physical connection correct?

Change one thing at a time and retest with the same note. This makes it easier to identify the actual cause.

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
| Live Sampler asks to open Performance and save | The previous normal patch has unsaved edits. Return to Performance, save it, then retry capture. |
| Live capture is refused | Stop recording, select loop mode, shorten the region if necessary, and save existing patch changes. |
| Tracks disappear after re-recording Track 1 | Track 1 establishes a new loop and clears the other tracks. |
| Restore cannot find the backup | Place `LILLA_CONFIG.fram` and its matching recording files directly in `/LILLABACKUP`. |
| Restored patch cannot play its source | Restore or re-import the required source library; configuration backup does not include all imported audio. |

## Practical projects

### Make a playable instrument from a recorded note

**You need:** a source connected to line input and a MIDI controller.

1. In Sampler, use `PAUSE+REC` to set the input level.
2. Record a clean sustained note in mono, including its attack and decay.
3. Stop, press S1 to audition from the keyboard, trim A/B and select `RETURN`. Make a backup now if you want to retain the full original recording.
4. Use `MAKE_RAW` to create a playable source from the selected region; successful conversion removes the recording.
5. In Performance, clone a patch you can use as a starting point.
6. Open an active sound and choose the new source.
7. Trim unwanted silence, choose a playback mode and adjust the envelope.
8. Set the root key to match the recorded note, then set the keyboard range.
9. Play above and below the root to check the result.
10. Save the patch. Keep the backup made before conversion if you need the original recording; a converted take is no longer available for Sampler WAV export.

**Try next:** clone the sound into another slot, choose a different region of the same source and give the two slots separate keyboard zones.

### Turn live audio into a small playable kit

**You need:** several short events in the Live Sampler buffer and a previously saved Performance patch.

1. Record the source in Live Sampler, then stop.
2. Select loop mode and find the first useful event with From, To and Window.
3. Press S1 and play the desired trigger key when prompted.
4. Move the region to a second event.
5. Press another free slot and choose another trigger key.
6. Repeat for the remaining events, allowing pairs of slots for stereo captures.
7. Switch to Performance and check the trigger-key assignments.
8. Open each sound to choose its final one-shot or loop mode and envelope.
9. Save the patch and wait for its audio to be written to Flash.

**Result:** a normal patch whose slots contain different captured regions. The original live buffer remains a temporary workspace.

### Build a layered texture and animate it

1. Start from a saved patch and clone a sound into a free slot.
2. Give both slots the same MIDI channel and overlapping key ranges.
3. Use different trim regions or pitch adjustments for the two layers.
4. Reduce their individual gains before listening to the combination.
5. Open a layer's VCF and add a slow modulation with modest depth.
6. Route one or both layers to Delay and add a small amount of feedback.
7. Play sustained notes and listen to the release tails.
8. Save the patch when the balance works.

**Try next:** record a short phrase on MIDI Loop Track 1, add a contrasting part on Track 2, and experiment with a small timing Shift on the second track.

## Quick reference

| Task | Starting point |
| --- | --- |
| Play an existing patch | Performance > patch field > Value. |
| Edit a sound | Performance > S1-S8 for an active slot. |
| Add another instrument using an existing sound | Sound Edit > CLONE, then edit the new slot. |
| Keep sound edits after power-off | Return to Performance > SAVE. |
| Import computer audio | SD `/LILLA_AUDIO` > Tools > Setup > import. |
| Record line input | Sampler > PAUSE+REC > MONO_REC or STEREO_REC > STOP. |
| Edit a recording while playing it from the keyboard | Sampler > completed recording > S1 (mono/left) or S2 (stereo right) > Sound Edit > RETURN. |
| Keep recording trim points for playback and export | Adjust A/B in Sampler Sound Edit; saved automatically, with stereo channels linked. |
| Turn a recording into a source | Sampler > MAKE_RAW. |
| Export a recording | Sampler > EXPORT_WAV_TO_SD > SD `/LILLAWAV_EXPORT`. |
| Record temporary live audio | Live Sampler > CAPTURE > STOP. |
| Preserve a live loop | Stop live recording > loop mode > S1-S8 > set root key > save patch. |
| Record a MIDI loop | MIDI Loop > Rec 1 > play > Rec 1 again. |
| Back up configuration and recordings | Tools > Setup > new numbered backup. |
| Restore a numbered backup | Copy its contents to SD `/LILLABACKUP` > Tools > Setup > restore. |

## Glossary

| Term | In this guide |
| --- | --- |
| ADSR | Attack, decay, sustain and release: the stages that shape a sound's level over time. |
| Basename | A file name without its extension, such as `BassDry` in `BassDry.wav`. |
| Capture | On the Live Sampler menu, record into the live buffer; with S1-S8, copy a selected region into a patch sound. |
| Circular buffer | Recording memory that wraps around and overwrites older material. |
| Flash | Persistent audio storage inside LILLA. |
| FRAM | Persistent storage used for configuration and patch/sound metadata. |
| LFO | Low-frequency oscillator used to vary a parameter over time. |
| LPF | Low-pass filter, which reduces higher frequencies. |
| MIDI CC | A MIDI Control Change message used to control an assigned parameter. |
| Mono | One audio channel. |
| Noclick | Boundary smoothing used to reduce discontinuities in suitable loop regions. |
| PCM | Uncompressed digital sample data. |
| Polyphony | The number of simultaneous playback voices; layers and release tails use voices too. |
| PSRAM | Working audio memory; its contents are lost at power-off. |
| RAW | Audio sample data without a WAV or AIFF container header. |
| Root key | The reference MIDI key for a sound's pitch mapping. |
| Stereo | Two audio channels, left and right. |
| VCF | The instrument filter, with cutoff, resonance and modulation controls. |

---

*LILLA User Guide - English edition - 8 October 2026*

*Guide images: [doc/assets/images](doc/assets/images/). Preserve this relative folder path when sharing the illustrated guide.*
