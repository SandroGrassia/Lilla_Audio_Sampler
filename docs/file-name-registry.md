# Persistent RAW file identities

`Sound.file` remains a 16-bit ID. RAW IDs are 0..259; recording and live-sampler IDs retain their existing ranges. ID 0 is permanently associated with `0.raw`.

The registry stores one basename per RAW ID in FRAM. Names are case-sensitive, contain up to 31 bytes before `.raw`, and never contain path separators. Import normalizes the extension to `.raw`; names such as `P12.raw` are reserved for recording packets. Changing the basename creates a different identity.

## Storage and recovery

Two 8,332-byte banks occupy FRAM addresses `0x10000..0x14117`, leaving 48,872 bytes after the registry. Each bank contains a format marker, generation, 260 fixed 32-byte basename fields, and CRC32. The RAM copies occupy 16,664 bytes in RAM2; playback still uses the existing RAM address/length cache and performs no FRAM lookup.

A commit invalidates the inactive bank, writes its payload and checksum, then publishes its marker and verifies the complete bank. An interrupted write leaves the previous committed bank available. An archive with neither valid bank enters recovery instead of inventing new associations.

The archive header advances to version 2. Migration from version 1 reserves the original numeric IDs for all referenced RAW files, including missing files, and all numeric files present in Flash. Unreferenced arbitrary names already in Flash can be enrolled through the next audio import.

## Import and sampling

After user confirmation, import checks all accepted source names and stages any new identities. Invalid names, a full table, or a failed FRAM commit stop the operation before Flash erasure. The complete registry is committed before erasing audio, so loss of power during copying does not lose previous associations.

Missing files retain their names and IDs. Reimporting the same name reconnects its Sounds; IDs are never automatically recycled. The 260-slot limit includes missing files, the fallback file and generated sampler files. A full table therefore requires an explicit future cleanup policy, rather than silently reusing missing identities. Factory reset preserves an existing valid registry, as it also preserves the RAW library.

Direct and Live Sampler allocation excludes reserved IDs and numeric names already assigned elsewhere. Their generated numeric names are committed before writing audio. A failed audio write may leave a reserved identity, which is safer than reassigning it to unrelated audio.

The SOUND header uses its existing compact layout: it omits `.raw` when necessary and abbreviates basenames longer than eight characters with `~`. The complete name remains in FRAM and in import/diagnostic output. Missing audio displays the Sound's retained association rather than the inactive Preset's default file.

## Backup compatibility

Metadata backups use version 4; full configuration/Recording backups use version 5. Both include the two registry banks. Readers continue to accept versions 1, 2 and 3 and reconstruct their numeric identities. Full backup audio continues to cover Recordings; the RAW library must be supplied separately under the corresponding names, as before.

Individual `.sound` and `.patch` exports retain their existing numeric-reference format and do not carry an independent filename registry. Transferring them to another installation requires the matching registry/configuration backup.

Host regression coverage includes registry capacity and name validation, stable identities across deletion/reimport, torn bank writes, import failures before Flash erasure, sampler captures, legacy migration and full backup/restore.
