# SD card formats and files

LILLA uses the Teensy 4.1 built-in microSD interface and the Teensy SD/SdFat `SdFs` filesystem implementation. The supported card filesystems are **FAT16, FAT32 and exFAT**. NTFS, APFS and Linux filesystems such as ext4 are not supported. This describes filesystem support in the firmware; individual card performance and reliability still depend on the medium.

## Directory layout

Folder names below are relative to the card root. Place input audio files directly in the appropriate folder; audio preparation does not scan subfolders.

```text
microSD root/
  AUDIO_FILES/             SD audio library: source files and converted RAWs
  UPLOAD_AUDIO_FILES/      Source files for import into internal Flash
  EXPORTED_AUDIO_FILES/    WAV files exported by the Sampler
  DATA_BACKUPS/            Numbered configuration and recording backups
  RESTORE_FROM_BACKUP/     Selected backup contents, ready to restore
  LILLA_PRIVATE/           Firmware-managed metadata (Hidden/System)
    .lilla-index           Alphabetical SD RAW catalog
    .lilla-sources/        Persistent SD audio references used by saved Sounds
    MIDI_LOOPS/            Saved MIDI loop event files
    PATCHES/               Saved patch parameter copies
```

| Folder or file | Use |
| --- | --- |
| `/AUDIO_FILES` | Add source audio here, then run SETUP > READ SD AUDIO FILES. Supported source files produce mono RAW files with the same basename beside the originals. The command builds the catalog used by BROWSE_SD. Selected audio regions are loaded into PSRAM for playback. |
| `/UPLOAD_AUDIO_FILES` | Input for SETUP > IMPORT AUDIO FILES FROM SD. This workflow imports audio into Flash; it is separate from preparing the SD library. |
| `/EXPORTED_AUDIO_FILES` | Destination for Sampler WAV exports. Copy these files to a computer to use the recordings elsewhere. |
| `/DATA_BACKUPS` | Destination for numbered backups containing configuration and recorded audio. Preserve the selected backup's contents together. |
| `/RESTORE_FROM_BACKUP` | Copy the contents of one selected backup directly here, including `LILLA_CONFIG.fram` and its `REC_*.raw` files. Do not put the enclosing numbered backup folder here. RESTORE CONFIGURATION reads this location. |
| `/LILLA_PRIVATE` | Firmware-managed metadata. Hidden/System attributes or dot-prefixed filenames may conceal it in a computer's file browser. Preserve it when copying the SD library. |
| `/LILLA_PRIVATE/.lilla-index` | Generated catalog of valid RAW files in `/AUDIO_FILES`. Run READ SD AUDIO FILES after adding or removing audio to rebuild it. It is a binary index, not a text file. |
| `/LILLA_PRIVATE/.lilla-sources` | Source references needed to reload saved SD Sounds. Keep this folder together with the referenced RAW audio; rebuilding the catalog does not replace these references. |
| `/LILLA_PRIVATE/MIDI_LOOPS` | Saved MIDI events, not rendered audio. Playback also needs the corresponding patches and samples. |
| `/LILLA_PRIVATE/PATCHES` | Patch parameter copies; these do not replace the referenced audio files. |

Normal configuration/recording backups do not include the SD audio library or MIDI loops. To preserve a complete SD setup, copy the card contents, including hidden files and folders, separately. Keep referenced RAW filenames and files available to saved Sounds.

## Compatible audio formats

| Input | Supported representation |
| --- | --- |
| RAW | Signed 16-bit little-endian mono PCM at 44.1 kHz, without a header. |
| WAV | PCM 8/16/24/32-bit or IEEE float 32/64-bit, mono/stereo, 8-192 kHz. Standard RIFF/WAVE and PCM/float extensible headers are supported. |
| AIFF/AIF | Uncompressed 16-bit PCM, mono/stereo, at 44.1 kHz. |
| MP3 | Decoded by the MP3 importer, with sample-rate conversion to 44.1 kHz when needed; see [MP3 import](docs/mp3-import.md). |

Conversion produces signed 16-bit little-endian mono RAW at 44.1 kHz. Stereo channels are averaged to mono; non-native WAV sample rates use filtered resampling. Unsupported codecs, multichannel WAV files and invalid audio are rejected.

For example, `/AUDIO_FILES/Piano.wav` produces `/AUDIO_FILES/Piano.raw`. Originals are retained and existing RAW destinations are not overwritten. Use distinct basenames for distinct sounds: `Piano.wav` and `Piano.mp3` would both target `Piano.raw`. To regenerate a RAW after changing its source, remove the old RAW before running preparation again.

The SD catalog supports up to 4,096 valid RAW files, with filenames up to 255 bytes including the extension. RAW files must be nonempty and contain a whole number of 16-bit samples. See [SD audio preparation](docs/user-audio-preparation.md) for conversion, indexing and browsing details, and the [User Guide](User%20Guide.md) for operating instructions.
