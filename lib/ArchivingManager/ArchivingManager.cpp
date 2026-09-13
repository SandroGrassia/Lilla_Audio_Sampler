/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "ArchivingManager.h"
#include "GlobalInfoMaster.h"

#include <string.h>

namespace
{
    // Keep each transfer within Wire2's 136-byte buffer, including the two address bytes.
    constexpr uint32_t FRAM_TRANSFER_BYTES = 128;

    uint32_t FRAM_Calculate_crc32(const uint8_t *bytes, uint32_t count)
    {
        uint32_t crc = 0xFFFFFFFFUL;
        for (uint32_t offset = 0; offset < count; ++offset)
        {
            crc ^= bytes[offset];
            for (uint8_t bit = 0; bit < 8; ++bit)
            {
                crc = (crc >> 1) ^ ((crc & 1U) != 0 ? 0xEDB88320UL : 0UL);
            }
        }
        return crc ^ 0xFFFFFFFFUL;
    }

    byte FRAM_Write_bytes(uint32_t address, const uint8_t *bytes, uint32_t size)
    {
        uint8_t buffer[FRAM_TRANSFER_BYTES];

        for (uint32_t offset = 0; offset < size;)
        {
            const uint32_t remaining = size - offset;
            const uint32_t count = remaining < FRAM_TRANSFER_BYTES ? remaining : FRAM_TRANSFER_BYTES;
            memcpy(buffer, bytes + offset, count);
            const byte result = LillaFram.writeArray(address + offset, count, buffer);
            if (result != LillaFRAM_2x512::ERROR_0)
            {
                return result; // Earlier blocks may already have been written.
            }
            offset += count;
        }
        return LillaFRAM_2x512::ERROR_0;
    }

    byte FRAM_Read_bytes(uint32_t address, uint8_t *bytes, uint32_t size)
    {
        for (uint32_t offset = 0; offset < size;)
        {
            const uint32_t remaining = size - offset;
            const uint32_t count = remaining < FRAM_TRANSFER_BYTES ? remaining : FRAM_TRANSFER_BYTES;
            const byte result = LillaFram.readArray(address + offset, count, bytes + offset);
            if (result != LillaFRAM_2x512::ERROR_0)
            {
                return result;
            }
            offset += count;
        }
        return LillaFRAM_2x512::ERROR_0;
    }

    template <class T>
    byte FRAM_Write_record(uint32_t address, const T &source)
    {
        return FRAM_Write_bytes(address, reinterpret_cast<const uint8_t *>(&source), sizeof(T));
    }

    template <class T>
    byte FRAM_Read_record(uint32_t address, T &destination)
    {
        T buffer{};
        uint8_t *bytes = reinterpret_cast<uint8_t *>(&buffer);

        for (uint32_t offset = 0; offset < sizeof(T);)
        {
            const uint32_t remaining = sizeof(T) - offset;
            const uint32_t count = remaining < FRAM_TRANSFER_BYTES ? remaining : FRAM_TRANSFER_BYTES;
            const byte result = LillaFram.readArray(address + offset, count, bytes + offset);
            if (result != LillaFRAM_2x512::ERROR_0)
            {
                return result;
            }
            offset += count;
        }

        memcpy(&destination, &buffer, sizeof(T)); // Publish only a complete record.
        return LillaFRAM_2x512::ERROR_0;
    }
}

byte ArchivingManager::Factory_reset_FRAM()
{
    const FRAM_Patch_struct empty_patch{};
    for (uint16_t id = 0; id < FRAM_PATCHES; ++id)
    {
        const byte result = FRAM_Write_patch(id, empty_patch);
        if (result != LillaFRAM_2x512::ERROR_0)
        {
            return result;
        }
    }

    const FRAM_Sound_struct empty_sound{};
    for (uint16_t id = 0; id < FRAM_SOUNDS; ++id)
    {
        const byte result = FRAM_Write_sound(id, empty_sound);
        if (result != LillaFRAM_2x512::ERROR_0)
        {
            return result;
        }
    }

    FRAM_Recording_struct empty_recording{};
    empty_recording.consistent = 1;
    for (uint8_t id = 0; id < RECORDINGS; ++id)
    {
        const byte result = FRAM_Write_recording(id, empty_recording);
        if (result != LillaFRAM_2x512::ERROR_0)
        {
            return result;
        }
    }

    FRAM_System_struct system{};
    system.first_octave = -2;
    return FRAM_Write_system(system);
}

bool ArchivingManager::Save_FRAM_backup(File &file)
{
    FRAM_Backup_header_struct header{};
    const uint8_t magic[8] = {'L', 'I', 'L', 'L', 'A', 'F', 'R', 'M'};
    memcpy(header.magic, magic, sizeof(magic));
    header.version = FRAM_BACKUP_VERSION;
    header.header_bytes = sizeof(header);
    header.payload_bytes = FRAM_FIRST_FREE_ADDRESS;

    uint8_t buffer[128];
    uint32_t crc = 0xFFFFFFFFUL;
    for (uint32_t address = 0; address < header.payload_bytes; address += sizeof(buffer))
    {
        const uint32_t remaining = header.payload_bytes - address;
        const uint32_t count = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
        if (FRAM_Read_bytes(address, buffer, count) != LillaFRAM_2x512::ERROR_0)
        {
            return false;
        }
        for (uint32_t offset = 0; offset < count; ++offset)
        {
            crc ^= buffer[offset];
            for (uint8_t bit = 0; bit < 8; ++bit)
            {
                crc = (crc >> 1) ^ ((crc & 1U) != 0 ? 0xEDB88320UL : 0UL);
            }
        }
    }
    header.payload_crc32 = crc ^ 0xFFFFFFFFUL;
    if (file.write(reinterpret_cast<const uint8_t *>(&header), sizeof(header)) != sizeof(header))
    {
        return false;
    }
    for (uint32_t address = 0; address < header.payload_bytes; address += sizeof(buffer))
    {
        const uint32_t remaining = header.payload_bytes - address;
        const uint32_t count = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
        if (FRAM_Read_bytes(address, buffer, count) != LillaFRAM_2x512::ERROR_0 || file.write(buffer, count) != count)
        {
            return false;
        }
    }
    return true;
}

bool ArchivingManager::Restore_FRAM_backup(File &file)
{
    FRAM_Backup_header_struct header{};
    const uint8_t magic[8] = {'L', 'I', 'L', 'L', 'A', 'F', 'R', 'M'};
    if (file.read(reinterpret_cast<uint8_t *>(&header), sizeof(header)) != sizeof(header))
    {
        return false;
    }
    if (memcmp(header.magic, magic, sizeof(magic)) != 0 || header.version != FRAM_BACKUP_VERSION || header.header_bytes != sizeof(header) || header.payload_bytes != FRAM_FIRST_FREE_ADDRESS)
    {
        return false;
    }

    uint8_t buffer[128];
    uint32_t crc = 0xFFFFFFFFUL;
    uint32_t remaining_payload = header.payload_bytes;
    while (remaining_payload > 0)
    {
        const uint32_t count = remaining_payload < sizeof(buffer) ? remaining_payload : sizeof(buffer);
        if (file.read(buffer, count) != count)
        {
            return false;
        }
        for (uint32_t offset = 0; offset < count; ++offset)
        {
            crc ^= buffer[offset];
            for (uint8_t bit = 0; bit < 8; ++bit)
            {
                crc = (crc >> 1) ^ ((crc & 1U) != 0 ? 0xEDB88320UL : 0UL);
            }
        }
        remaining_payload -= count;
    }
    if ((crc ^ 0xFFFFFFFFUL) != header.payload_crc32 || !file.seek(header.header_bytes))
    {
        return false;
    }

    for (uint32_t address = 0; address < header.payload_bytes; address += sizeof(buffer))
    {
        const uint32_t remaining = header.payload_bytes - address;
        const uint32_t count = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
        if (file.read(buffer, count) != count || FRAM_Write_bytes(address, buffer, count) != LillaFRAM_2x512::ERROR_0)
        {
            return false;
        }
    }
    return true;
}

uint32_t ArchivingManager::FRAM_Get_patch_address(uint8_t patch_id)
{
    return FRAM_PATCH_ADDRESS + static_cast<uint32_t>(patch_id) * sizeof(FRAM_Patch_struct);
}

uint32_t ArchivingManager::FRAM_Get_instrument_address(uint8_t patch_id, uint8_t instrument_id)
{
    return FRAM_Get_patch_address(patch_id) + offsetof(FRAM_Patch_struct, Instrument) + static_cast<uint32_t>(instrument_id) * sizeof(FRAM_Instrument_struct);
}

uint32_t ArchivingManager::FRAM_Get_filter_address(uint8_t patch_id, uint8_t instrument_id)
{
    return FRAM_Get_instrument_address(patch_id, instrument_id) + offsetof(FRAM_Instrument_struct, Filter);
}

uint32_t ArchivingManager::FRAM_Get_delay_address(uint8_t patch_id)
{
    return FRAM_Get_patch_address(patch_id) + offsetof(FRAM_Patch_struct, Delay);
}

uint32_t ArchivingManager::FRAM_Get_sound_address(uint16_t sound_id)
{
    return FRAM_SOUND_ADDRESS + static_cast<uint32_t>(sound_id) * sizeof(FRAM_Sound_struct);
}

uint32_t ArchivingManager::FRAM_Get_recording_address(uint8_t recording_id)
{
    return FRAM_RECORDING_ADDRESS + static_cast<uint32_t>(recording_id) * sizeof(FRAM_Recording_struct);
}

uint32_t ArchivingManager::FRAM_Get_CC_settings_address()
{
    return FRAM_SYSTEM_ADDRESS + offsetof(FRAM_System_struct, CC_settings);
}

byte ArchivingManager::FRAM_Write_patch(uint8_t patch_id, const FRAM_Patch_struct &source)
{
    if (patch_id >= FRAM_PATCHES)
    {
        return LillaFRAM_2x512::ERROR_11;
    }

    const uint32_t address = FRAM_Get_patch_address(patch_id);
    const uint32_t payload_bytes = offsetof(FRAM_Patch_struct, crc32);
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&source);
    const uint32_t crc = FRAM_Calculate_crc32(bytes, payload_bytes);
    const byte result = FRAM_Write_bytes(address, bytes, payload_bytes);

    if (result != LillaFRAM_2x512::ERROR_0)
    {
        return result;
    }

    return FRAM_Write_record(address + payload_bytes, crc);
}

byte ArchivingManager::FRAM_Read_patch(uint8_t patch_id, FRAM_Patch_struct &destination)
{
    if (patch_id >= FRAM_PATCHES)
    {
        return LillaFRAM_2x512::ERROR_11;
    }

    FRAM_Patch_struct patch{};
    const byte result = FRAM_Read_record(FRAM_Get_patch_address(patch_id), patch);

    if (result != LillaFRAM_2x512::ERROR_0)
    {
        return result;
    }
    if (patch.crc32 != FRAM_Calculate_crc32(reinterpret_cast<const uint8_t *>(&patch), offsetof(FRAM_Patch_struct, crc32)))
    {
        return FRAM_ERROR_CRC;
    }

    memcpy(&destination, &patch, sizeof(patch));
    return LillaFRAM_2x512::ERROR_0;
}

byte ArchivingManager::FRAM_Write_instrument(uint8_t patch_id, uint8_t instrument_id, const FRAM_Instrument_struct &source)
{
    if (patch_id >= FRAM_PATCHES || instrument_id >= INSTRUMENTS)
    {
        return LillaFRAM_2x512::ERROR_11;
    }

    FRAM_Patch_struct patch{};
    const byte result = FRAM_Read_patch(patch_id, patch);

    if (result != LillaFRAM_2x512::ERROR_0)
    {
        return result;
    }

    patch.Instrument[instrument_id] = source;
    return FRAM_Write_patch(patch_id, patch);
}

byte ArchivingManager::FRAM_Read_instrument(uint8_t patch_id, uint8_t instrument_id, FRAM_Instrument_struct &destination)
{
    if (patch_id >= FRAM_PATCHES || instrument_id >= INSTRUMENTS)
    {
        return LillaFRAM_2x512::ERROR_11;
    }

    FRAM_Patch_struct patch{};
    const byte result = FRAM_Read_patch(patch_id, patch);

    if (result != LillaFRAM_2x512::ERROR_0)
    {
        return result;
    }

    destination = patch.Instrument[instrument_id];
    return LillaFRAM_2x512::ERROR_0;
}

byte ArchivingManager::FRAM_Write_filter(uint8_t patch_id, uint8_t instrument_id, const FRAM_Instrument_filter_struct &source)
{
    if (patch_id >= FRAM_PATCHES || instrument_id >= INSTRUMENTS)
    {
        return LillaFRAM_2x512::ERROR_11;
    }

    FRAM_Patch_struct patch{};
    const byte result = FRAM_Read_patch(patch_id, patch);

    if (result != LillaFRAM_2x512::ERROR_0)
    {
        return result;
    }

    patch.Instrument[instrument_id].Filter = source;
    return FRAM_Write_patch(patch_id, patch);
}

byte ArchivingManager::FRAM_Read_filter(uint8_t patch_id, uint8_t instrument_id, FRAM_Instrument_filter_struct &destination)
{
    if (patch_id >= FRAM_PATCHES || instrument_id >= INSTRUMENTS)
    {
        return LillaFRAM_2x512::ERROR_11;
    }

    FRAM_Patch_struct patch{};
    const byte result = FRAM_Read_patch(patch_id, patch);

    if (result != LillaFRAM_2x512::ERROR_0)
    {
        return result;
    }

    destination = patch.Instrument[instrument_id].Filter;
    return LillaFRAM_2x512::ERROR_0;
}

byte ArchivingManager::FRAM_Write_delay(uint8_t patch_id, const FRAM_Patch_delay_struct &source)
{
    if (patch_id >= FRAM_PATCHES)
    {
        return LillaFRAM_2x512::ERROR_11;
    }

    FRAM_Patch_struct patch{};
    const byte result = FRAM_Read_patch(patch_id, patch);

    if (result != LillaFRAM_2x512::ERROR_0)
    {
        return result;
    }

    patch.Delay = source;
    return FRAM_Write_patch(patch_id, patch);
}

byte ArchivingManager::FRAM_Read_delay(uint8_t patch_id, FRAM_Patch_delay_struct &destination)
{
    if (patch_id >= FRAM_PATCHES)
    {
        return LillaFRAM_2x512::ERROR_11;
    }

    FRAM_Patch_struct patch{};
    const byte result = FRAM_Read_patch(patch_id, patch);

    if (result != LillaFRAM_2x512::ERROR_0)
    {
        return result;
    }

    destination = patch.Delay;
    return LillaFRAM_2x512::ERROR_0;
}

byte ArchivingManager::FRAM_Write_sound(uint16_t sound_id, const FRAM_Sound_struct &source)
{
    if (sound_id >= FRAM_SOUNDS)
    {
        return LillaFRAM_2x512::ERROR_11;
    }

    const uint32_t address = FRAM_Get_sound_address(sound_id);
    const uint32_t payload_bytes = offsetof(FRAM_Sound_struct, crc32);
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&source);
    const uint32_t crc = FRAM_Calculate_crc32(bytes, payload_bytes);
    const byte result = FRAM_Write_bytes(address, bytes, payload_bytes);

    if (result != LillaFRAM_2x512::ERROR_0)
    {
        return result;
    }

    return FRAM_Write_record(address + payload_bytes, crc);
}

byte ArchivingManager::FRAM_Read_sound(uint16_t sound_id, FRAM_Sound_struct &destination)
{
    if (sound_id >= FRAM_SOUNDS)
    {
        return LillaFRAM_2x512::ERROR_11;
    }

    FRAM_Sound_struct sound{};
    const byte result = FRAM_Read_record(FRAM_Get_sound_address(sound_id), sound);

    if (result != LillaFRAM_2x512::ERROR_0)
    {
        return result;
    }
    if (sound.crc32 != FRAM_Calculate_crc32(reinterpret_cast<const uint8_t *>(&sound), offsetof(FRAM_Sound_struct, crc32)))
    {
        return FRAM_ERROR_CRC;
    }

    memcpy(&destination, &sound, sizeof(sound));
    return LillaFRAM_2x512::ERROR_0;
}

byte ArchivingManager::FRAM_Write_recording(uint8_t recording_id, const FRAM_Recording_struct &source)
{
    if (recording_id >= RECORDINGS)
    {
        return LillaFRAM_2x512::ERROR_11;
    }

    const uint32_t address = FRAM_Get_recording_address(recording_id);
    const uint32_t payload_bytes = offsetof(FRAM_Recording_struct, crc32);
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&source);
    const uint32_t crc = FRAM_Calculate_crc32(bytes, payload_bytes);
    const byte result = FRAM_Write_bytes(address, bytes, payload_bytes);

    if (result != LillaFRAM_2x512::ERROR_0)
    {
        return result;
    }

    return FRAM_Write_record(address + payload_bytes, crc);
}

byte ArchivingManager::FRAM_Read_recording(uint8_t recording_id, FRAM_Recording_struct &destination)
{
    if (recording_id >= RECORDINGS)
    {
        return LillaFRAM_2x512::ERROR_11;
    }

    FRAM_Recording_struct recording{};
    const byte result = FRAM_Read_record(FRAM_Get_recording_address(recording_id), recording);

    if (result != LillaFRAM_2x512::ERROR_0)
    {
        return result;
    }
    if (recording.crc32 != FRAM_Calculate_crc32(reinterpret_cast<const uint8_t *>(&recording), offsetof(FRAM_Recording_struct, crc32)))
    {
        return FRAM_ERROR_CRC;
    }

    memcpy(&destination, &recording, sizeof(recording));
    return LillaFRAM_2x512::ERROR_0;
}

byte ArchivingManager::Repair_Recordings_in_FRAM(FRAM_Recording_repair_report &report)
{
    report = {};
    bool clear_recording[RECORDINGS] = {};

    // Complete the assessment before writing: an I/O failure is not evidence of corruption.
    for (uint8_t id = 0; id < RECORDINGS; ++id)
    {
        report.failed_id = id;
        FRAM_Recording_struct recording{};
        const byte result = FRAM_Read_recording(id, recording);

        if (result == FRAM_ERROR_CRC)
        {
            clear_recording[id] = true;
            continue;
        }
        if (result != LillaFRAM_2x512::ERROR_0)
        {
            return result;
        }
    }

    for (uint8_t id = 0; id < RECORDINGS; ++id)
    {
        if (!clear_recording[id])
        {
            continue;
        }

        report.failed_id = id;
        FRAM_Recording_struct expected{};
        expected.consistent = 1;
        byte result = FRAM_Write_recording(id, expected);

        if (result != LillaFRAM_2x512::ERROR_0)
        {
            return result;
        }

        FRAM_Recording_struct actual{};
        result = FRAM_Read_recording(id, actual);

        if (result != LillaFRAM_2x512::ERROR_0)
        {
            return result;
        }
        if (memcmp(&expected, &actual, offsetof(FRAM_Recording_struct, crc32)) != 0)
        {
            return FRAM_ERROR_VERIFY;
        }

        ++report.cleared_recordings;
    }

    report.failed_id = UINT8_MAX;
    return LillaFRAM_2x512::ERROR_0;
}

byte ArchivingManager::FRAM_Write_CC_settings(const FRAM_CC_settings_struct &source)
{
    FRAM_System_struct system{};
    const byte result = FRAM_Read_system(system);

    if (result != LillaFRAM_2x512::ERROR_0)
    {
        return result;
    }

    system.CC_settings = source;
    return FRAM_Write_system(system);
}

byte ArchivingManager::FRAM_Read_CC_settings(FRAM_CC_settings_struct &destination)
{
    FRAM_System_struct system{};
    const byte result = FRAM_Read_system(system);

    if (result != LillaFRAM_2x512::ERROR_0)
    {
        return result;
    }

    destination = system.CC_settings;
    return LillaFRAM_2x512::ERROR_0;
}

byte ArchivingManager::FRAM_Write_system(const FRAM_System_struct &source)
{
    const uint32_t payload_bytes = offsetof(FRAM_System_struct, crc32);
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&source);
    const uint32_t crc = FRAM_Calculate_crc32(bytes, payload_bytes);
    const byte result = FRAM_Write_bytes(FRAM_SYSTEM_ADDRESS, bytes, payload_bytes);

    if (result != LillaFRAM_2x512::ERROR_0)
    {
        return result;
    }

    return FRAM_Write_record(FRAM_SYSTEM_ADDRESS + payload_bytes, crc);
}

byte ArchivingManager::FRAM_Read_system(FRAM_System_struct &destination)
{
    FRAM_System_struct system{};
    const byte result = FRAM_Read_record(FRAM_SYSTEM_ADDRESS, system);

    if (result != LillaFRAM_2x512::ERROR_0)
    {
        return result;
    }
    if (system.crc32 != FRAM_Calculate_crc32(reinterpret_cast<const uint8_t *>(&system), offsetof(FRAM_System_struct, crc32)))
    {
        return FRAM_ERROR_CRC;
    }

    memcpy(&destination, &system, sizeof(system));
    return LillaFRAM_2x512::ERROR_0;
}

byte ArchivingManager::Repair_System_in_FRAM(FRAM_System_repair_report &report)
{
    report = {};
    FRAM_System_struct system{};
    byte result = FRAM_Read_system(system);

    if (result == LillaFRAM_2x512::ERROR_0)
    {
        return result;
    }
    if (result != FRAM_ERROR_CRC)
    {
        return result;
    }

    FRAM_System_struct expected{};
    expected.optimization = 0;
    expected.first_octave = -2;
    expected.key_step = 0;
    result = FRAM_Write_system(expected);

    if (result != LillaFRAM_2x512::ERROR_0)
    {
        return result;
    }

    FRAM_System_struct actual{};
    result = FRAM_Read_system(actual);

    if (result != LillaFRAM_2x512::ERROR_0)
    {
        return result;
    }
    if (memcmp(&expected, &actual, offsetof(FRAM_System_struct, crc32)) != 0)
    {
        return FRAM_ERROR_VERIFY;
    }

    report.defaulted = true;
    return LillaFRAM_2x512::ERROR_0;
}

byte ArchivingManager::Repair_Patch_Sound_in_FRAM(FRAM_Repair_report &report)
{
    report = {};
    bool clear_patch[FRAM_PATCHES] = {};
    bool referenced[FRAM_SOUNDS] = {};
    uint8_t sound_action[FRAM_SOUNDS] = {}; // 0: keep, 1: clear, 2: default.

    // Complete the read-only assessment first. An I/O failure is not evidence of corruption.
    for (uint16_t id = 0; id < FRAM_PATCHES; ++id)
    {
        report.failed_id = id;
        FRAM_Patch_struct patch{};
        const byte result = FRAM_Read_patch(id, patch);

        if (result == FRAM_ERROR_CRC)
        {
            clear_patch[id] = true;
            continue; // Never use links from a record with an invalid CRC.
        }

        if (result != LillaFRAM_2x512::ERROR_0)
        {
            return result;
        }
        if (patch.used > 1)
        {
            return FRAM_ERROR_SOURCE;
        }
        if (!patch.used)
        {
            continue;
        }

        uint8_t count = 0;
        for (const auto &instrument : patch.Instrument)
        {
            if (instrument.used > 1)
            {
                return FRAM_ERROR_SOURCE;
            }
            if (!instrument.used)
            {
                continue;
            }
            if (instrument.sound_id >= FRAM_SOUNDS)
            {
                return FRAM_ERROR_SOURCE;
            }
            referenced[instrument.sound_id] = true;
            ++count;
        }

        if (count != patch.instruments)
        {
            return FRAM_ERROR_SOURCE;
        }
    }

    report.failed_sound = true;
    const FRAM_Sound_struct empty_sound{};

    for (uint16_t id = 0; id < FRAM_SOUNDS; ++id)
    {
        report.failed_id = id;
        FRAM_Sound_struct sound{};
        const byte result = FRAM_Read_sound(id, sound);
        if (result != LillaFRAM_2x512::ERROR_0 && result != FRAM_ERROR_CRC)
        {
            return result;
        }
        if (!referenced[id])
        {
            if (result == FRAM_ERROR_CRC || memcmp(&sound, &empty_sound, offsetof(FRAM_Sound_struct, crc32)) != 0)
            {
                sound_action[id] = 1;
            }
        }
        else if (result == FRAM_ERROR_CRC)
        {
            sound_action[id] = 2;
        }
        else if (sound.used != 1)
        {
            return FRAM_ERROR_SOURCE;
        }
    }

    // Persist and verify each repair. A later startup can safely resume after interruption.
    report.failed_sound = false;
    for (uint16_t id = 0; id < FRAM_PATCHES; ++id)
    {
        if (!clear_patch[id])
        {
            continue;
        }

        report.failed_id = id;
        const FRAM_Patch_struct empty{};
        byte result = FRAM_Write_patch(id, empty);

        if (result != LillaFRAM_2x512::ERROR_0)
        {
            return result;
        }

        FRAM_Patch_struct actual{};
        result = FRAM_Read_patch(id, actual);

        if (result != LillaFRAM_2x512::ERROR_0)
        {
            return result;
        }
        if (memcmp(&empty, &actual, offsetof(FRAM_Patch_struct, crc32)) != 0)
        {
            return FRAM_ERROR_VERIFY;
        }

        ++report.cleared_patches;
    }
    report.failed_sound = true;
    for (uint16_t id = 0; id < FRAM_SOUNDS; ++id)
    {
        if (!sound_action[id])
        {
            continue;
        }

        report.failed_id = id;
        FRAM_Sound_struct expected{};

        if (sound_action[id] == 2)
        {
            // Same defaults as the factory Sound in main; all remaining fields stay zero.
            expected.used = 1;
            expected.B = 1000;
            expected.decay = 50;
            expected.sustain = 50;
            expected.release = 10;
            expected.gain = 12;
        }

        byte result = FRAM_Write_sound(id, expected);
        if (result != LillaFRAM_2x512::ERROR_0)

        {
            return result;
        }

        FRAM_Sound_struct actual{};
        result = FRAM_Read_sound(id, actual);

        if (result != LillaFRAM_2x512::ERROR_0)
        {
            return result;
        }
        if (memcmp(&expected, &actual, offsetof(FRAM_Sound_struct, crc32)) != 0)
        {
            return FRAM_ERROR_VERIFY;
        }
        if (sound_action[id] == 2)
        {
            ++report.defaulted_sounds;
        }
        else
        {
            ++report.cleared_sounds;
        }
    }

    report.failed_id = UINT16_MAX;
    return LillaFRAM_2x512::ERROR_0;
}

byte ArchivingManager::Load_Patch_Sound_from_FRAM(uint16_t &failed_id, bool &failed_sound)
{
    static_assert(PATCHES_MAX == FRAM_PATCHES && SOUNDS_MAX == FRAM_SOUNDS);

    memset(Patch, 0, sizeof(Patch));
    memset(Sound, 0, sizeof(Sound));
    byte result = LillaFRAM_2x512::ERROR_0;
    failed_sound = true;

    for (failed_id = 0; failed_id < SOUNDS_MAX; ++failed_id)
    {
        FRAM_Sound_struct source{};
        result = FRAM_Read_sound(failed_id, source);

        if (result != LillaFRAM_2x512::ERROR_0)
        {
            break;
        }
        if (source.used > 1)
        {
            result = FRAM_ERROR_SOURCE;
            break;
        }
        if (!source.used)
        {
            continue;
        }

        auto &target = Sound[failed_id];
        target.used = true;
        target.file = source.file;
        target.mode = source.mode;
        target.pitch = source.pitch;
        target.A = source.A;
        target.B = source.B;
        target.Noclick = source.Noclick;
        target.pan = source.pan;
        target.data = (source.midi_channel << 1) | (source.attack_type & 1);
        target.attack = source.attack;
        target.decay = source.decay;
        target.sustain = source.sustain;
        target.release = source.release;
        target.gain = source.gain;
    }
    if (result == LillaFRAM_2x512::ERROR_0)
    {
        failed_sound = false;

        for (failed_id = 0; failed_id < PATCHES_MAX; ++failed_id)
        {
            FRAM_Patch_struct source{};
            result = FRAM_Read_patch(failed_id, source);

            if (result != LillaFRAM_2x512::ERROR_0)
            {
                break;
            }
            if (source.used > 1)
            {
                result = FRAM_ERROR_SOURCE;
                break;
            }
            if (!source.used)
            {
                continue;
            }

            auto &target = Patch[failed_id];
            target.used = true;
            target.instruments = source.instruments;
            uint8_t count = 0;

            for (uint8_t id = 0; id < INSTRUMENTS; ++id)
            {
                const auto &input = source.Instrument[id];
                if (input.used > 1)
                {
                    result = FRAM_ERROR_SOURCE;
                    break;
                }
                if (!input.used)
                {
                    continue;
                }
                if (input.sound_id >= SOUNDS_MAX || !Sound[input.sound_id].used)
                {
                    result = FRAM_ERROR_SOURCE;
                    break;
                }

                ++count;
                auto &output = target.Instrument[id];
                output.used = true;
                output.sound_id = input.sound_id;
                output.root_key = input.root_key;
                output.from_note = input.from_note;
                output.to_note = input.to_note;
                output.precedence = input.precedence;
                output.lock = input.lock;
                output.Filter.use = input.Filter.use;
                output.Filter.type = input.Filter.type;
                output.Filter.pivot = input.Filter.pivot;
                output.Filter.resonance = input.Filter.resonance;
                output.Filter.modulation = input.Filter.modulation;
                output.Filter.index = input.Filter.index;
                output.Filter.frequency_time = input.Filter.frequency_time;
            }

            if (count != source.instruments)
            {
                result = FRAM_ERROR_SOURCE;
            }
            if (result != LillaFRAM_2x512::ERROR_0)
            {
                break;
            }
        }
    }

    if (result != LillaFRAM_2x512::ERROR_0)
    {
        memset(Patch, 0, sizeof(Patch));
        memset(Sound, 0, sizeof(Sound));
    }
    else
    {
        failed_id = UINT16_MAX;
    }
    
    return result;
}

byte ArchivingManager::Save_CC_lowpass_filter(const int CC_lowpass_filter)
{
    FRAM_System_struct system{};
    const byte result = FRAM_Read_system(system);

    if (result != LillaFRAM_2x512::ERROR_0)
    {
        return result;
    }

    system.CC_settings.lowpass_filter = static_cast<uint8_t>(CC_lowpass_filter);
    return FRAM_Write_system(system);
}

byte ArchivingManager::Read_CC_lowpass_filter(uint8_t &CC_lowpass_filter)
{
    FRAM_System_struct system{};
    const byte result = FRAM_Read_system(system);

    if (result == LillaFRAM_2x512::ERROR_0)
    {
        CC_lowpass_filter = system.CC_settings.lowpass_filter;
    }

    return result;
}

byte ArchivingManager::Save_CC_settings(const uint8_t sound_gain[INSTRUMENTS], uint8_t lowpass_filter)
{
    FRAM_System_struct system{};
    const byte result = FRAM_Read_system(system);

    if (result != LillaFRAM_2x512::ERROR_0)
    {
        return result;
    }

    memcpy(system.CC_settings.sound_gain, sound_gain, sizeof(system.CC_settings.sound_gain));
    system.CC_settings.lowpass_filter = lowpass_filter;
    return FRAM_Write_system(system);
}

byte ArchivingManager::Read_CC_settings(uint8_t sound_gain[INSTRUMENTS], uint8_t &lowpass_filter)
{
    FRAM_System_struct system{};
    const byte result = FRAM_Read_system(system);

    if (result == LillaFRAM_2x512::ERROR_0)
    {
        memcpy(sound_gain, system.CC_settings.sound_gain, sizeof(system.CC_settings.sound_gain));
        lowpass_filter = system.CC_settings.lowpass_filter;
    }

    return result;
}

byte ArchivingManager::Save_optimization(const uint8_t optimization)
{
    FRAM_System_struct system{};
    const byte result = FRAM_Read_system(system);

    if (result != LillaFRAM_2x512::ERROR_0)
    {
        return result;
    }

    system.optimization = Normalize_optimization(optimization);
    return FRAM_Write_system(system);
}

byte ArchivingManager::Read_optimization(uint8_t &optimization)
{
    FRAM_System_struct system{};
    byte result = FRAM_Read_system(system);

    if (result != LillaFRAM_2x512::ERROR_0)
    {
        return result;
    }

    const uint8_t valid = Normalize_optimization(system.optimization);
    optimization = valid;

    if (valid != system.optimization)
    {
        system.optimization = valid;
        result = FRAM_Write_system(system);
    }

    return result;
}

byte ArchivingManager::Save_key_step(const uint8_t key_step)
{
    FRAM_System_struct system{};
    const byte result = FRAM_Read_system(system);

    if (result != LillaFRAM_2x512::ERROR_0)
    {
        return result;
    }

    system.key_step = key_step;
    return FRAM_Write_system(system);
}

byte ArchivingManager::Read_key_step(uint8_t &key_step)
{
    FRAM_System_struct system{};
    const byte result = FRAM_Read_system(system);

    if (result == LillaFRAM_2x512::ERROR_0)
    {
        key_step = system.key_step;
    }

    return result;
}

byte ArchivingManager::Save_CC_Sound_gain(const uint8_t instrument_id, const uint8_t CC_Sg_instrument)
{
    if (instrument_id >= INSTRUMENTS)
    {
        return LillaFRAM_2x512::ERROR_11;
    }

    FRAM_System_struct system{};
    const byte result = FRAM_Read_system(system);

    if (result != LillaFRAM_2x512::ERROR_0)
    {
        return result;
    }

    system.CC_settings.sound_gain[instrument_id] = CC_Sg_instrument;
    return FRAM_Write_system(system);
}

byte ArchivingManager::Read_CC_Sound_gain(const uint8_t instrument_id, uint8_t &CC_Sg_instrument)
{
    if (instrument_id >= INSTRUMENTS)
    {
        return LillaFRAM_2x512::ERROR_11;
    }

    FRAM_System_struct system{};
    const byte result = FRAM_Read_system(system);

    if (result == LillaFRAM_2x512::ERROR_0)
    {
        CC_Sg_instrument = system.CC_settings.sound_gain[instrument_id];
    }

    return result;
}

byte ArchivingManager::Read_Delay(uint8_t patch_id, Delay_data_struct &delay_data)
{
    FRAM_Patch_delay_struct source{};
    const byte result = FRAM_Read_delay(patch_id, source);
    if (result != LillaFRAM_2x512::ERROR_0)
    {
        return result;
    }

    delay_data.samples = source.samples;
    delay_data.samples_LR = source.samples_LR;
    delay_data.instrument_route = 0;
    for (uint8_t instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
    {
        if (source.instrument_route[instrument_id] > 1)
        {
            return FRAM_ERROR_SOURCE;
        }
        bitWrite(delay_data.instrument_route, instrument_id, source.instrument_route[instrument_id]);
    }
    delay_data.modulation_source = source.modulation_source;
    delay_data.modulation_depth = source.modulation_depth;
    delay_data.modulation_frequency = source.modulation_frequency;
    delay_data.modulation_phase_LR = source.modulation_phase_LR;
    delay_data.loop_gain = source.loop_gain;
    return LillaFRAM_2x512::ERROR_0;
}

byte ArchivingManager::Save_Delay(uint8_t patch_id, const Delay_data_struct &delay_data)
{
    FRAM_Patch_delay_struct destination{};
    destination.samples = delay_data.samples;
    destination.samples_LR = delay_data.samples_LR;
    for (uint8_t instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
    {
        destination.instrument_route[instrument_id] = bitRead(delay_data.instrument_route, instrument_id);
    }
    destination.modulation_source = delay_data.modulation_source;
    destination.modulation_depth = delay_data.modulation_depth;
    destination.modulation_frequency = delay_data.modulation_frequency;
    destination.modulation_phase_LR = delay_data.modulation_phase_LR;
    destination.loop_gain = delay_data.loop_gain;
    return FRAM_Write_delay(patch_id, destination);
}

byte ArchivingManager::Read_first_octave(int8_t &first_octave)
{
    FRAM_System_struct system{};
    const byte result = FRAM_Read_system(system);

    if (result == LillaFRAM_2x512::ERROR_0)
    {
        first_octave = system.first_octave;
    }

    return result;
}

byte ArchivingManager::Save_first_octave(const int8_t first_octave)
{
    FRAM_System_struct system{};
    const byte result = FRAM_Read_system(system);

    if (result != LillaFRAM_2x512::ERROR_0)
    {
        return result;
    }

    system.first_octave = first_octave;
    return FRAM_Write_system(system);
}

byte ArchivingManager::Save_Sound(const int sound_id)
{
    if (sound_id < 0 || sound_id >= SOUNDS_MAX)
    {
        return LillaFRAM_2x512::ERROR_11;
    }

    const auto &runtime = Sound[sound_id];
    FRAM_Sound_struct destination{};
    destination.used = runtime.used ? 1 : 0;
    destination.file = runtime.file;
    destination.mode = runtime.mode;
    destination.pitch = runtime.pitch;
    destination.A = runtime.A;
    destination.B = runtime.B;
    destination.Noclick = runtime.Noclick;
    destination.pan = runtime.pan;
    destination.midi_channel = (runtime.data >> 1) & 15;
    destination.attack_type = runtime.data & 1;
    destination.attack = runtime.attack;
    destination.decay = runtime.decay;
    destination.sustain = runtime.sustain;
    destination.release = runtime.release;
    destination.gain = runtime.gain;
    return FRAM_Write_sound(static_cast<uint16_t>(sound_id), destination);
}

bool ArchivingManager::Validate_Sound_AB_file_raw(uint32_t sound_id)
{
    if (sound_id >= SOUNDS_MAX)
    {
        PRINT_ERROR(F("ERROR: invalid sound_id - "));
        return false;
    }

    int sample_count = 0;

    if (Sound[sound_id].file < FIRST_RECORDING_FILE)
    {
        sample_count = Info.Raw_file_samples(Sound[sound_id].file);
    }

    const bool invalid = sample_count <= 0 || Sound[sound_id].A >= static_cast<uint32_t>(sample_count) || Sound[sound_id].B >= static_cast<uint32_t>(sample_count) || Sound[sound_id].B < Sound[sound_id].A;

    if (!invalid)
    {
        return true;
    }

    const int fallback_samples = Info.Raw_file_samples(0);

    // La mancanza di 0.raw è un errore di sistema non recuperabile automaticamente.
    if (fallback_samples <= 0)
    {
        PRINT_ERROR(F("ERROR: required 0.raw missing or empty - "));
        return false;
    }

    Sound[sound_id].file = 0;
    Sound[sound_id].A = 0;
    Sound[sound_id].B = static_cast<uint32_t>(fallback_samples - 1);

    Save_Sound(sound_id);
    return true;
}

byte ArchivingManager::Read_Sound(const int sound_id)
{
    if (sound_id < 0 || sound_id >= SOUNDS_MAX)
    {
        return LillaFRAM_2x512::ERROR_11;
    }

    FRAM_Sound_struct source{};
    const byte result = FRAM_Read_sound(static_cast<uint16_t>(sound_id), source);
    if (result != LillaFRAM_2x512::ERROR_0)
    {
        return result;
    }
    if (source.used > 1)
    {
        return FRAM_ERROR_SOURCE;
    }

    auto &runtime = Sound[sound_id];
    runtime.used = source.used != 0;
    runtime.file = source.file;
    runtime.mode = source.mode;
    runtime.pitch = source.pitch;
    runtime.A = source.A;
    runtime.B = source.B;
    runtime.Noclick = source.Noclick;
    runtime.pan = source.pan;
    runtime.data = (source.midi_channel << 1) | (source.attack_type & 1);
    runtime.attack = source.attack;
    runtime.decay = source.decay;
    runtime.sustain = source.sustain;
    runtime.release = source.release;
    runtime.gain = source.gain;
    return Validate_Sound_AB_file_raw(sound_id) ? LillaFRAM_2x512::ERROR_0 : FRAM_ERROR_SOURCE;
}

byte ArchivingManager::Save_Patch(const int patch_id)
{
    if (patch_id < 0 || patch_id >= PATCHES_MAX)
    {
        return LillaFRAM_2x512::ERROR_11;
    }

    FRAM_Patch_struct destination{};
    byte result = FRAM_Read_patch(static_cast<uint8_t>(patch_id), destination);
    if (result != LillaFRAM_2x512::ERROR_0)
    {
        return result;
    }

    const auto &runtime = Patch[patch_id];
    if (runtime.instruments > INSTRUMENTS)
    {
        return FRAM_ERROR_SOURCE;
    }
    destination.used = runtime.used ? 1 : 0;
    destination.instruments = runtime.instruments;
    memset(destination.Instrument, 0, sizeof(destination.Instrument));
    uint8_t instruments = 0;
    for (uint8_t instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
    {
        const auto &input = runtime.Instrument[instrument_id];
        if (input.used && input.sound_id >= SOUNDS_MAX)
        {
            return FRAM_ERROR_SOURCE;
        }
        if (input.used)
        {
            ++instruments;
        }

        auto &output = destination.Instrument[instrument_id];
        output.used = input.used ? 1 : 0;
        output.sound_id = input.sound_id;
        output.root_key = input.root_key;
        output.from_note = input.from_note;
        output.to_note = input.to_note;
        output.precedence = input.precedence ? 1 : 0;
        output.lock = input.lock ? 1 : 0;
        output.Filter.use = input.Filter.use;
        output.Filter.type = input.Filter.type;
        output.Filter.pivot = input.Filter.pivot;
        output.Filter.resonance = input.Filter.resonance;
        output.Filter.modulation = input.Filter.modulation;
        output.Filter.index = input.Filter.index;
        output.Filter.frequency_time = input.Filter.frequency_time;
    }
    if (instruments != runtime.instruments)
    {
        return FRAM_ERROR_SOURCE;
    }

    result = FRAM_Write_patch(static_cast<uint8_t>(patch_id), destination);
    return result;
}

byte ArchivingManager::Read_Patch(const int patch_id)
{
    if (patch_id < 0 || patch_id >= PATCHES_MAX)
    {
        return LillaFRAM_2x512::ERROR_11;
    }

    FRAM_Patch_struct source{};
    const byte result = FRAM_Read_patch(static_cast<uint8_t>(patch_id), source);
    if (result != LillaFRAM_2x512::ERROR_0)
    {
        return result;
    }
    if (source.used > 1 || source.instruments > INSTRUMENTS)
    {
        return FRAM_ERROR_SOURCE;
    }

    Patch_struct destination{};
    destination.used = source.used != 0;
    destination.instruments = source.instruments;
    uint8_t instruments = 0;
    for (uint8_t instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
    {
        const auto &input = source.Instrument[instrument_id];
        if (input.used > 1)
        {
            return FRAM_ERROR_SOURCE;
        }
        if (!input.used)
        {
            continue;
        }
        if (input.sound_id >= SOUNDS_MAX)
        {
            return FRAM_ERROR_SOURCE;
        }

        auto &output = destination.Instrument[instrument_id];
        output.used = true;
        output.sound_id = input.sound_id;
        output.root_key = input.root_key;
        output.from_note = input.from_note;
        output.to_note = input.to_note;
        output.precedence = input.precedence != 0;
        output.lock = input.lock != 0;
        output.Filter.use = input.Filter.use;
        output.Filter.type = input.Filter.type;
        output.Filter.pivot = input.Filter.pivot;
        output.Filter.resonance = input.Filter.resonance;
        output.Filter.modulation = input.Filter.modulation;
        output.Filter.index = input.Filter.index;
        output.Filter.frequency_time = input.Filter.frequency_time;
        ++instruments;
    }
    if (instruments != source.instruments)
    {
        return FRAM_ERROR_SOURCE;
    }

    Patch[patch_id] = destination;
    return LillaFRAM_2x512::ERROR_0;
}

byte ArchivingManager::Save_DS_Recording(const int recording)
{
    if (recording < 0 || recording >= RECORDINGS)
    {
        return LillaFRAM_2x512::ERROR_11;
    }

    const auto &runtime = Recording[recording];
    if (runtime.first_packet < 0 || runtime.first_packet > UINT16_MAX || runtime.packets < 0 || runtime.packets > UINT16_MAX)
    {
        return FRAM_ERROR_SOURCE;
    }

    FRAM_Recording_struct destination{};
    destination.first_packet = static_cast<uint16_t>(runtime.first_packet);
    destination.packets = static_cast<uint16_t>(runtime.packets);
    destination.stereo = runtime.stereo ? 1 : 0;
    destination.consistent = runtime.consistent ? 1 : 0;
    return FRAM_Write_recording(static_cast<uint8_t>(recording), destination);
}

byte ArchivingManager::Read_DS_Recording(const int recording)
{
    if (recording < 0 || recording >= RECORDINGS)
    {
        return LillaFRAM_2x512::ERROR_11;
    }

    FRAM_Recording_struct source{};
    const byte result = FRAM_Read_recording(static_cast<uint8_t>(recording), source);

    if (result != LillaFRAM_2x512::ERROR_0)
    {
        return result;
    }
    if (source.stereo > 1 || source.consistent > 1)
    {
        return FRAM_ERROR_SOURCE;
    }
    const uint32_t packet_span = static_cast<uint32_t>(source.packets) * (source.stereo ? 2U : 1U);
    if (source.packets > 0 && (source.first_packet >= VFS_PACKETS_MAX || packet_span > static_cast<uint32_t>(VFS_PACKETS_MAX - source.first_packet)))
    {
        return FRAM_ERROR_SOURCE;
    }

    Recording[recording].first_packet = source.first_packet;
    Recording[recording].packets = source.packets;
    Recording[recording].stereo = source.stereo != 0;
    Recording[recording].consistent = source.consistent != 0;
    return LillaFRAM_2x512::ERROR_0;
}

String ArchivingManager::Filename_Patch(const int patch_id)
{
    String filename = String(patch_id);
    return String(filename + ".patch");
}

// Filename sound description
String ArchivingManager::Filename_Sound(const int patch_id, const int instrument_id)
{
    // String filename = String(patch_id + "_" + instrument_id);
    String filename = String(patch_id) + "_" + instrument_id;
    return String(filename + ".sound");
}

bool ArchivingManager::Copy_Patch_from_RAM_to_SD(const int patch_id) // public
{
    String filename = Filename_Patch(patch_id);
    String full_path = String("/LILLAPATCH/" + filename);
    auto *full_path_ptr = full_path.c_str();

    if (SD.begin(BUILTIN_SDCARD))
    {
        if (!SD.exists("/LILLAPATCH"))
        {
            SD.mkdir("/LILLAPATCH");
            Serial.println(F("/LILLAPATCH directory created"));
        }

        else if (SD.exists(full_path_ptr))
        {
            SD.remove(full_path_ptr);

            Serial.print(F("ArchivingManager::Copy_Patch_from_RAM_to_SD - existing "));
            Serial.print(full_path_ptr);
            Serial.println(" has been deleted.");
        }

        File file = SD.open(full_path_ptr, FILE_WRITE);
        if (file)
        {
            Copy_Patch_from_RAM_to_SD(patch_id, file);
            file.close();

            Serial.print(F("ArchivingManager::Copy_Patch_from_RAM_to_SD - Patch saved in "));
            Serial.print(full_path_ptr);
            Serial.println(" in SD.");

            return true;
        }

        else
        {
            return false;
        }
    }

    else
    {
        Serial.println(F("ArchivingManager::Copy_Patch_from_RAM_to_SD - SD not present!"));
        return false;
    }
}

bool ArchivingManager::Save_Patch_from_RAM_to_SD(const int patch_id)
{
    if (patch_id < 0 || patch_id >= PATCHES_MAX)
    {
        PRINT_ERROR(F("ERROR: patch_id out of range"));
        return false;
    }

    char full_path[PATCH_PATH_SIZE];
    const int length = snprintf(full_path, sizeof(full_path), "/LILLAPATCH/%d.bin.patch", patch_id);

    if (length < 0 || static_cast<size_t>(length) >= sizeof(full_path))
    {
        PRINT_ERROR(F("ERROR: Patch path too long"));
        return false;
    }

    if (!Is_SD_inserted())
    {
        PRINT_ERROR(F("ERROR: SD not inserted"));
        return false;
    }

    else
    {
        if (!SD.exists("/LILLAPATCH"))
        {
            SD.mkdir("/LILLAPATCH");
            Serial.println(F("/LILLAPATCH directory created"));
        }

        if (SD.exists(full_path) && !SD.remove(full_path))
        {
            PRINT_ERROR(F("ERROR: impossible to remove previous Patch"));
            return false;
        }

        File file = SD.open(full_path, FILE_WRITE);

        if (file)
        {
            FileHeader header = {FILEHEADER_VERSION, static_cast<uint16_t>(sizeof(Patch_struct))};

            size_t headerWritten = file.write(reinterpret_cast<const uint8_t *>(&header), sizeof(header));
            size_t payloadWritten = file.write(reinterpret_cast<const uint8_t *>(&Patch[patch_id]), sizeof(Patch[patch_id]));

            bool success = (headerWritten == sizeof(header)) && (payloadWritten == sizeof(Patch[patch_id]));

            if (success)
            {
                Serial.print(F("ArchivingManager::Save_Patch_from_RAM_to_SD - Patch saved in "));
                Serial.println(full_path);

                file.close();
                return success;
            }
            else
            {
                PRINT_ERROR(F("ERROR: incomplete file writing on SD"));

                file.close();
                return false;
            }
        }
        else
        {
            PRINT_ERROR(F("ERROR: file not created on SD"));
            return false;
        }
    }

    PRINT_ERROR(F("ERROR: SD not present for saving Patch"));
    return false;
}

bool ArchivingManager::Resume_Patch_from_SD_to_RAM(const int patch_id)
{
    if (patch_id < 0 || patch_id >= PATCHES_MAX)
    {
        PRINT_ERROR(F("ERROR: patch_id out of range"));
        return false;
    }

    char full_path[PATCH_PATH_SIZE];

    const int length = snprintf(full_path, sizeof(full_path), "/LILLAPATCH/%d.bin.patch", patch_id);

    if (length < 0 || static_cast<size_t>(length) >= sizeof(full_path))
    {
        PRINT_ERROR(F("ERROR: Patch path too long"));
        return false;
    }

    if (!Is_SD_inserted())
    {
        PRINT_ERROR(F("ERROR: SD not inserted"));
        return false;
    }

    if (!SD.exists(full_path))
    {
        PRINT_ERROR(F("Patch not present in SD"));
        return false;
    }

    // Copy data
    File file = SD.open(full_path, FILE_READ);

    if (!file)
    {
        PRINT_ERROR(F("ERROR: unable to open Patch file"));
        return false;
    }

    FileHeader header{};
    const size_t headerRead = file.read(reinterpret_cast<uint8_t *>(&header), sizeof(header));

    if (headerRead != sizeof(header))
    {
        PRINT_ERROR(F("ERROR: incomplete Patch header"));
        file.close();
        return false;
    }

    if (header.version != FILEHEADER_VERSION)
    {
        PRINT_ERROR(F("ERROR: unsupported Patch file version"));
        file.close();
        return false;
    }

    if (header.payloadSize != sizeof(Patch_struct))
    {
        PRINT_ERROR(F("ERROR: incompatible Patch payload size"));
        file.close();
        return false;
    }

    const uint64_t expectedFileSize = sizeof(FileHeader) + sizeof(Patch_struct);

    if (file.size() != expectedFileSize)
    {
        PRINT_ERROR(F("ERROR: invalid Patch file size"));
        file.close();
        return false;
    }

    Patch_struct temporaryPatch{};

    const size_t payloadRead = file.read(reinterpret_cast<uint8_t *>(&temporaryPatch), sizeof(temporaryPatch));

    if (payloadRead != sizeof(temporaryPatch))
    {
        PRINT_ERROR(F("ERROR: incomplete Patch payload"));
        file.close();
        return false;
    }

    file.close();

    // Modifica la Patch attiva soltanto dopo una lettura completa.
    Patch[patch_id] = temporaryPatch;

    Serial.print(F("ArchivingManager::Resume_Patch_from_SD_to_RAM - Patch restored from "));
    Serial.println(full_path);

    return true;
}

void ArchivingManager::Copy_Patch_from_RAM_to_SD(const int patch_id, File &file)
{
    const auto *data = (const byte *)(const void *)&Patch[patch_id];

    for (auto i = 0; i < SIZE_OF_PATCH; ++i)
    {
        file.println(*(data + i));
    }
}

bool ArchivingManager::Copy_Sound_from_RAM_to_SD(const int patch_id, const int instrument_id)
{
    String filename = Filename_Sound(patch_id, instrument_id);
    String full_path = String("/LILLASOUND/" + filename);
    auto *full_path_ptr = full_path.c_str();

    if (SD.begin(BUILTIN_SDCARD))
    {
        if (!SD.exists("/LILLASOUND"))
        {
            SD.mkdir("/LILLASOUND");
            Serial.println(F("/LILLASOUND directory created"));
        }

        else if (SD.exists(full_path_ptr))
        {
            SD.remove(full_path_ptr);

            Serial.print(F("ArchivingManager::Copy_Sound_from_RAM_to_SD - existing "));
            Serial.print(full_path);
            Serial.println(" has been deleted.");
        }

        File file = SD.open(full_path_ptr, FILE_WRITE); // creazione del file vuoto
        if (file)
        {
            Copy_Sound_from_RAM_to_SD(patch_id, instrument_id, file);
            file.close();

            Serial.print(F("ArchivingManager::Copy_Sound_from_RAM_to_SD - Sound saved in "));
            Serial.print(full_path_ptr);
            Serial.println(" in SD.");

            return true;
        }

        else
        {
            return false;
        }
    }

    else
    {
        Serial.println(F("ArchivingManager::Copy_Sound_from_RAM_to_SD - SD not present!"));
        return false;
    }
}

void ArchivingManager::Copy_Sound_from_RAM_to_SD(const int patch_id, const int instrument_id, File &file) // private
{
    const auto *data = (const byte *)(const void *)&Sound[Get_sound_id(patch_id, instrument_id)];

    for (auto i = 0; i < SIZE_OF_SOUND; ++i)
    {
        file.println(*(data + i));
    }
}

bool ArchivingManager::Copy_Patch_from_SD_to_RAM(const int patch_id)
{
    String filename = Filename_Patch(patch_id);
    String full_path = String("/LILLAPATCH/" + filename);
    auto *full_path_ptr = full_path.c_str();

    if (SD.begin(BUILTIN_SDCARD))
    {
        if (SD.exists(full_path_ptr))
        {
            Serial.print(F("ArchivingManager::Copy_Patch_from_SD_to_RAM - Patch file "));
            Serial.print(full_path);
            Serial.println(F(" found; now starts Patch import."));

            File file = SD.open(full_path_ptr);
            if (file)
            {
                Copy_Patch_from_SD_to_RAM(patch_id, file);
                file.close();
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            Serial.println(F("ArchivingManager::Copy_Patch_from_SD_to_RAM - Patch not found on SD!"));
            return false;
        }
    }

    else
    {
        Serial.println(F("ArchivingManager::Copy_Patch_from_SD_to_RAM - SD not present!"));
        return false;
    }
}

void ArchivingManager::Copy_Patch_from_SD_to_RAM(const int patch_id, File &file) // private
{
    uint8_t value;
    String value_txt;
    uint8_t value_LSB;
    uint8_t value_MSB;

    /*
    struct Patch_struct
    {
        bool used;
        uint8_t instruments;
        Instrument_struct Instrument[8];
    }
    */

    value_txt = file.readStringUntil('\n'); // restituisce String - es: x_txt = "230" ossia i char "2" "3" "0" "\n"
    value = value_txt.toInt();              // toInt() conversione da String a long es: x = 230
    Patch[patch_id].used = (value++ != 0);

    value_txt = file.readStringUntil('\n');
    value = value_txt.toInt();
    Patch[patch_id].instruments = value;

    /*
    struct Instrument_struct
    {
        bool used;
        uint8_t sound_id;
        uint8_t root_key;
        uint8_t from_note;
        uint8_t to_note;
        bool precedence;
        bool lock;
        Instrument_filter_data_struct Filter;
    };
    */

    for (auto instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
    {
        value_txt = file.readStringUntil('\n');
        value = value_txt.toInt();
        Patch[patch_id].Instrument[instrument_id].used = (value != 0);

        value_txt = file.readStringUntil('\n');
        value_LSB = value_txt.toInt();
        value_txt = file.readStringUntil('\n');
        value_MSB = value_txt.toInt();
        Patch[patch_id].Instrument[instrument_id].sound_id = value_MSB << 8 | value_LSB;

        value_txt = file.readStringUntil('\n');
        value = value_txt.toInt();
        Patch[patch_id].Instrument[instrument_id].root_key = value;

        value_txt = file.readStringUntil('\n');
        value = value_txt.toInt();
        Patch[patch_id].Instrument[instrument_id].from_note = value;

        value_txt = file.readStringUntil('\n');
        value = value_txt.toInt();
        Patch[patch_id].Instrument[instrument_id].to_note = value;

        value_txt = file.readStringUntil('\n');
        value = value_txt.toInt();
        Patch[patch_id].Instrument[instrument_id].precedence = (value != 0);

        value_txt = file.readStringUntil('\n');
        value = value_txt.toInt();
        Patch[patch_id].Instrument[instrument_id].lock = (value != 0);

        /*
        struct Instrument_filter_data_struct
        {
        uint8_t use;            // yes/no
        uint8_t type;           // filter type 0 --> 3
        uint8_t pivot;          // 0 --> 100 filter frequency/note frequency
        uint8_t resonance;      // 0 --> 40
        uint8_t modulation;     // waveform 0 -> 3
        uint8_t index;          // 1 --> 20 modulation_index
        uint8_t frequency_time; // 0 --> 20
        };
        */

        value_txt = file.readStringUntil('\n');
        value = value_txt.toInt();
        Patch[patch_id].Instrument[instrument_id].Filter.use = value;

        value_txt = file.readStringUntil('\n');
        value = value_txt.toInt();
        Patch[patch_id].Instrument[instrument_id].Filter.type = value;

        value_txt = file.readStringUntil('\n');
        value = value_txt.toInt();
        Patch[patch_id].Instrument[instrument_id].Filter.pivot = value;

        value_txt = file.readStringUntil('\n');
        value = value_txt.toInt();
        Patch[patch_id].Instrument[instrument_id].Filter.resonance = value;

        value_txt = file.readStringUntil('\n');
        value = value_txt.toInt();
        Patch[patch_id].Instrument[instrument_id].Filter.modulation = value;

        value_txt = file.readStringUntil('\n');
        value = value_txt.toInt();
        Patch[patch_id].Instrument[instrument_id].Filter.index = value;

        value_txt = file.readStringUntil('\n');
        value = value_txt.toInt();
        Patch[patch_id].Instrument[instrument_id].Filter.frequency_time = value;
    }

    if (true)
    {
        Serial.println(F("ArchivingManager::Copy_Patch_from_SD_to_RAM(int patch_id, File &file)- Done."));
    }
}

bool ArchivingManager::Copy_Sound_from_SD_to_RAM(const int patch_id, const int instrument_id, const int sound_id) // public
{
    String filename = Filename_Sound(patch_id, instrument_id);
    String full_path = String("/LILLASOUND/" + filename);
    auto *full_path_ptr = full_path.c_str();

    if (SD.begin(BUILTIN_SDCARD))
    {
        if (SD.exists(full_path_ptr))
        {
            Serial.print(F("ArchivingManager::Copy_Sound_from_SD_to_RAM - Sound file "));
            Serial.print(full_path);
            Serial.println(F(" found; now starts Sound import."));

            File file = SD.open(full_path_ptr);
            if (file)
            {
                Copy_Sound_from_SD_to_RAM(patch_id, instrument_id, sound_id, file);
                file.close();
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            Serial.println(F("ArchivingManager::Copy_Sound_from_SD_to_RAM - Sound not found on SD!"));
            return false;
        }
    }

    else
    {
        Serial.println(F("ArchivingManager::Copy_Sound_from_SD_to_RAM - SD not present!"));
        return false;
    }
}

void ArchivingManager::Copy_Sound_from_SD_to_RAM(const int patch_id, const int instrument_id, const int sound_id, File &file) // private
{
    uint8_t value;
    String value_txt;
    uint8_t value_1;
    uint8_t value_2;
    uint8_t value_3;
    uint8_t value_4;

    /*
    struct Sound_struct // 22 bytes
    {
        bool used;
        uint16_t file;
        uint8_t mode;
        int8_t pitch; // -128 + 127
        uint32_t A;
        uint32_t B;
        uint16_t Noclick;
        int8_t pan;
        uint8_t data; // bit4-3-2-1: midi_channel bit0: Attack ramp ("0" Slow, "1" Fast)
        uint8_t attack;
        uint8_t decay;
        uint8_t sustain;
        uint8_t release;
        uint8_t gain;
    }
    */

    value_txt = file.readStringUntil('\n'); // restituisce String - es: x_txt = "230" ossia i char "2" "3" "0" "\n"
    value = value_txt.toInt();              // toInt() conversione da String a long es: x = 230
    Sound[sound_id].used = (value != 0);

    value_txt = file.readStringUntil('\n');
    value_1 = value_txt.toInt();
    value_txt = file.readStringUntil('\n');
    value_2 = value_txt.toInt();
    Sound[sound_id].file = value_2 << 8 | value_1;

    value_txt = file.readStringUntil('\n');
    value = value_txt.toInt();
    Sound[sound_id].pitch = static_cast<int8_t>(value);

    value_txt = file.readStringUntil('\n');
    value_1 = value_txt.toInt();
    value_txt = file.readStringUntil('\n');
    value_2 = value_txt.toInt();
    value_txt = file.readStringUntil('\n');
    value_3 = value_txt.toInt();
    value_txt = file.readStringUntil('\n');
    value_4 = value_txt.toInt();
    Sound[sound_id].A = value_4 << 24 | value_3 << 16 | value_2 << 8 | value_1;

    value_txt = file.readStringUntil('\n');
    value_1 = value_txt.toInt();
    value_txt = file.readStringUntil('\n');
    value_2 = value_txt.toInt();
    value_txt = file.readStringUntil('\n');
    value_3 = value_txt.toInt();
    value_txt = file.readStringUntil('\n');
    value_4 = value_txt.toInt();
    Sound[sound_id].B = value_4 << 24 | value_3 << 16 | value_2 << 8 | value_1;

    value_txt = file.readStringUntil('\n');
    value = value_txt.toInt();
    Sound[sound_id].Noclick = value;

    value_txt = file.readStringUntil('\n');
    value = value_txt.toInt();
    Sound[sound_id].pan = static_cast<int8_t>(value);

    value_txt = file.readStringUntil('\n');
    value = value_txt.toInt();
    Sound[sound_id].data = value;

    value_txt = file.readStringUntil('\n');
    value = value_txt.toInt();
    Sound[sound_id].attack = value;

    value_txt = file.readStringUntil('\n');
    value = value_txt.toInt();
    Sound[sound_id].decay = value;

    value_txt = file.readStringUntil('\n');
    value = value_txt.toInt();
    Sound[sound_id].sustain = value;

    value_txt = file.readStringUntil('\n');
    value = value_txt.toInt();
    Sound[sound_id].release = value;

    value_txt = file.readStringUntil('\n');
    value = value_txt.toInt();
    Sound[sound_id].gain = value;

    Serial.println(F("ArchivingManager::Copy_Sound_from_SD_to_RAM - Done."));
}

bool ArchivingManager::Is_SD_inserted(void)
{
    if (SD.begin(BUILTIN_SDCARD))
    {
        return true;
    }
    return false;
}
