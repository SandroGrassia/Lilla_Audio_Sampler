# LILLA Audio Sampler 7.2.7

## New SD folder layout

Audio input, exported audio, backups and restore files now have dedicated folders. Firmware-managed data is grouped under `/LILLA_PRIVATE`.

```text
microSD root/
  AUDIO_FILES/          SD audio and converted RAW files (visible)
  UPLOAD_AUDIO_FILES/   Sources for Flash import (visible)
  EXPORTED_AUDIO_FILES/ Exported Sampler WAV recordings
  DATA_BACKUPS/         Numbered configuration/recording backups
  RESTORE_FROM_BACKUP/  Contents of the selected backup
  LILLA_PRIVATE/        Firmware metadata (Hidden/System)
    .lilla-index        SD audio catalog
    .lilla-sources/     Saved Sound source references
    MIDI_LOOPS/         Saved MIDI loops
    PATCHES/            Patch parameter copies
```

## Update an existing SD card

Rename or move the existing folders as follows before using firmware 7.2.7. Migration is manual; the firmware uses the new paths.

| Previous folder | New location |
| --- | --- |
| `/EXTERNAL_AUDIO` | `/AUDIO_FILES` |
| `/FLASH_AUDIO` | `/UPLOAD_AUDIO_FILES` |
| `/LILLAWAV_EXPORT` | `/EXPORTED_AUDIO_FILES` |
| `/LILLABACKUP` | `/DATA_BACKUPS` |
| `/LILLALOOP` | `/LILLA_PRIVATE/MIDI_LOOPS` |
| `/LILLA_AUDIO` | `/AUDIO_FILES` |
| `/LILLAPATCH` | `/LILLA_PRIVATE/PATCHES` |

Move the RAW audio files into visible `/AUDIO_FILES`, and keep `.lilla-index` and `.lilla-sources` directly in hidden `/LILLA_PRIVATE`. Audio and saved-data formats are unchanged. Recent `/LILLA_HIDDEN` layouts and the `/SD_AUDIO_FILES` and `/FLASH_AUDIO_FILES` names are migrated automatically when the destinations are absent; existing destinations and conflicting legacy trees are preserved.

New backups are written to numbered folders inside `/DATA_BACKUPS`. To restore one, copy the **contents** of the chosen numbered folder into `/RESTORE_FROM_BACKUP`, including `LILLA_CONFIG.fram` and its matching `REC_*.raw` files. The firmware no longer restores from the backup root.

The unused `/LILLASOUND` folder is no longer used by the firmware. Output folders are created when needed; prepare the input folders and `/RESTORE_FROM_BACKUP` yourself.
