# LILLA Audio Sampler 7.2.7

## New SD folder layout

Audio input, exported audio, backups and restore files now have dedicated folders. Firmware-managed data is grouped under `/LILLA_PRIVATE`.

```text
microSD root/
  SD_AUDIO_FILES/        Originals for the SD audio library
  FLASH_AUDIO_FILES/     Originals for internal Flash import
  EXPORTED_AUDIO_FILES/  Exported Sampler WAV recordings
  DATA_BACKUPS/          Numbered configuration/recording backups
  RESTORE/               Contents of the backup chosen for restore
  LILLA_PRIVATE/
    MIDI_LOOPS/         Saved MIDI loops
    AUDIO_FILES/        Prepared RAW files, index and source references
    PATCHES/            Patch parameter copies
```

## Update an existing SD card

Rename or move the existing folders as follows before using firmware 7.2.7. Migration is manual; the firmware uses the new paths.

| Previous folder | New location |
| --- | --- |
| `/EXTERNAL_AUDIO` | `/SD_AUDIO_FILES` |
| `/FLASH_AUDIO` | `/FLASH_AUDIO_FILES` |
| `/LILLAWAV_EXPORT` | `/EXPORTED_AUDIO_FILES` |
| `/LILLABACKUP` | `/DATA_BACKUPS` |
| `/LILLALOOP` | `/LILLA_PRIVATE/MIDI_LOOPS` |
| `/LILLA_AUDIO` | `/LILLA_PRIVATE/AUDIO_FILES` |
| `/LILLAPATCH` | `/LILLA_PRIVATE/PATCHES` |

Keep all hidden files and subdirectories when moving the audio library, especially `.lilla-index` and `.lilla-sources`. Audio and saved-data formats are unchanged.

New backups are written to numbered folders inside `/DATA_BACKUPS`. To restore one, copy the **contents** of the chosen numbered folder into `/RESTORE`, including `LILLA_CONFIG.fram` and its matching `REC_*.raw` files. The firmware no longer restores from the backup root.

The unused `/LILLASOUND` folder is no longer used by the firmware. Output folders are created when needed; prepare the input folders and `/RESTORE` yourself.
