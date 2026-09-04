/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "PatchCacheManager.h"
#include "Functions.h"
#include "GlobalInfoMaster.h"
#include <spi_interrupt.h>

void PatchCacheManager::Begin(void)
{
    for (auto i = 0; i < PATCH_CACHE_ARRAY_COUNT; ++i)
    {
        Cache[i].state = Free;
        Cache[i].file_id = -1;
    }
}

void PatchCacheManager::Set_cache_pointer(uint8_t cache_id, int16_t *pointer)
{
    cache_pointer[cache_id] = pointer;
}

uint16_t PatchCacheManager::Get_copy_samples(uint16_t read_time_micros)
{
    // non fornire samples_per_cycle > SAMPLES_MAX
    return (SAMPLES_MAX * read_time_micros) / COPY_TIME_MAX;
}

int8_t PatchCacheManager::Get_cache_free(void)
{
    // Prefer a Free slot whose previous contents have already been invalidated.
    for (auto i = 0; i < PATCH_CACHE_ARRAY_COUNT; ++i)
    {
        if (Cache[i].state == Free && Cache[i].file_id == -1)
        {
            return i;
        }
    }

    // Otherwise reuse any Free slot containing an unreferenced cached file.
    for (auto i = 0; i < PATCH_CACHE_ARRAY_COUNT; ++i)
    {
        if (Cache[i].state == Free)
        {
            return i;
        }
    }
    return -1;
}

int8_t PatchCacheManager::Get_cache_id_from_file_id(uint16_t file_id)
{
    for (auto cache_id = 0; cache_id < PATCH_CACHE_ARRAY_COUNT; ++cache_id)
    {
        if (Cache[cache_id].file_id == file_id)
        {
            return cache_id;
        }
    }
    return -1;
}

int8_t PatchCacheManager::Get_cache_id_ready_from_file_id(uint16_t file_id)
{
    for (auto cache_id = 0; cache_id < PATCH_CACHE_ARRAY_COUNT; ++cache_id)
    {
        if (Cache[cache_id].file_id == file_id && Cache[cache_id].state == Ready)
        {
            return cache_id;
        }
    }
    return -1;
}

bool PatchCacheManager::Load_audio_file(int16_t file_id_old, uint16_t file_id)
{
    int8_t cache_id = -1;

    if (file_id >= FIRST_RECORDING_FILE)
    {
        PRINT_ERROR(F("ERROR: PatchCacheManager supports only RAW files - "));
        return false;
    }

    // rilascia la vecchia cache se non usata
    if (file_id_old >= 0 && file_id_old != file_id)
    {
        Retire_cache_if_unused(file_id_old);
    }

    // Verifica se il file_id sia già presente
    cache_id = Get_cache_id_from_file_id(file_id);
    if (cache_id >= 0)
    {
        // A matching cache can only be Ready, Free, or Retiring because Loading is synchronous and internal.
        Cache[cache_id].state = Ready;
        Serial.println(F("Audio .raw file already present in PSRAM chips"));
    }

    else
    {
        // Initial checks are performed only once.
        const uint32_t file_samples = Info.Raw_file_samples(file_id);

        // Wait only for a cache slot to become available.
        do
        {
            cache_id = Get_cache_free();

            if (cache_id < 0)
            {
                // Audio IRQs remain enabled and may release a Retiring cache.
                yield();
            }
        } while (cache_id < 0);

        // Reserve the selected slot immediately.
        Cache[cache_id].state = Loading;
        Cache[cache_id].file_id = file_id;
        Cache[cache_id].samples = 0;

        if (file_samples == 0)
        {
            // A missing or empty RAW file is represented by an empty Ready cache.
            Cache[cache_id].state = Ready;

            Serial.print(F("Audio .raw file not available; empty cache created for file: "));
            Serial.println(file_id);
        }

        else
        {
            rawfile.fast_open(file_id);

            // invalida cache_id
            Cache[cache_id].state = Loading;
            Cache[cache_id].file_id = file_id;
            Cache[cache_id].samples = 0;

            // variabili per la copia
            int16_t *cache_p = cache_pointer[cache_id];
            uint32_t residual_samples = file_samples;
            uint32_t samples_copied = 0;
            uint32_t cycle_counter = 0;

            AudioStartUsingSPI();
            while (residual_samples > 0)
            {
                uint32_t elapsed = audio_update_time_micros; // fissa l'istante di partenza

                if (elapsed >= COPY_TIME_MAX)
                {
                    continue;
                }

                const uint32_t read_time = COPY_TIME_MAX - elapsed;

                if (read_time < MIN_TIME_FOR_COPY_CYCLE_MICROS)
                {
                    continue;
                }

                uint16_t samples_to_read = Get_copy_samples(read_time);
                samples_to_read = samples_to_read < residual_samples ? samples_to_read : residual_samples;
                const uint32_t samples_read = rawfile.read(reinterpret_cast<byte *>(cache_p + samples_copied), 2u * samples_to_read) / 2;
                samples_copied += samples_read;
                residual_samples -= samples_read;
                ++cycle_counter;
            }

            rawfile.close();
            AudioStopUsingSPI();

            Cache[cache_id].samples = samples_copied;
            Cache[cache_id].file_id = file_id;
            Cache[cache_id].state = Ready;

            Serial.println(F("Audio .raw file copied from Flash chip to PSRAM chips"));
            Serial.print("file: ");
            Serial.println(file_id);
            Serial.print("cache_id: ");
            Serial.println(cache_id);
            Serial.print("cycles: ");
            Serial.println(cycle_counter);
        }
    }
    return true;
}

bool PatchCacheManager::Load_patch(uint8_t new_patch_id)
{
    return Load_patch(-1, new_patch_id);
}

bool PatchCacheManager::Load_patch(int16_t old_patch_id, uint8_t new_patch_id)
{
    if (old_patch_id < -1 || old_patch_id >= PATCHES_MAX)
    {
        PRINT_ERROR(F("ERROR: PatchCacheManager invalid old patch_id - "));
        return false;
    }

    if (new_patch_id >= PATCHES_MAX || !Patch[new_patch_id].used)
    {
        PRINT_ERROR(F("ERROR: PatchCacheManager invalid new patch_id - "));
        return false;
    }

    uint16_t new_file_id[INSTRUMENTS]; // Stores the distinct file IDs used by the new patch; only elements below new_files are initialized.
    uint8_t new_files = 0;

    // Build the list of distinct RAW files used by the new patch.
    for (uint8_t instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
    {
        // Skip unused instruments
        if (!Patch[new_patch_id].Instrument[instrument_id].used)
        {
            continue;
        }

        const uint16_t file_id = Sound[Get_sound_id(new_patch_id, instrument_id)].file;

        if (file_id >= FIRST_RECORDING_FILE)
        {
            PRINT_ERROR(F("ERROR: PatchCacheManager supports only RAW files - "));
            return false;
        }

        bool already_listed = false;

        // Search only the initialized portion of the array for an already listed file ID.
        for (uint8_t i = 0; i < new_files; ++i)
        {
            if (new_file_id[i] == file_id)
            {
                already_listed = true;
                break;
            }
        }

        if (!already_listed)
        {
            new_file_id[new_files++] = file_id;
        }
    }

    if (new_files == 0)
    {
        PRINT_ERROR(F("ERROR: PatchCacheManager patch has no instruments - "));
        return false;
    }

    // Leave as Ready old files used by the new patch and set a Retiring old_file_id not used in the new Patch
    if (old_patch_id >= 0)
    {
        for (uint8_t instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
        {
            // Skip not used Instruments
            if (!Patch[old_patch_id].Instrument[instrument_id].used)
            {
                continue;
            }

            // get the Instrument's file_id (old_file_id)
            const uint16_t old_file_id = Sound[Get_sound_id(old_patch_id, instrument_id)].file;

            // Scan all new files; verify if old_file_id is usefull (used_by_new_patch)
            bool used_by_new_patch = false;
            for (uint8_t i = 0; i < new_files; ++i)
            {
                if (new_file_id[i] == old_file_id)
                {
                    used_by_new_patch = true;

                    // in this case Cache[cache_id].state remains as Ready;
                    break;
                }
            }

            if (!used_by_new_patch)
            {
                const int8_t cache_id = Get_cache_id_from_file_id(old_file_id);
                if (cache_id >= 0 && Cache[cache_id].state == Ready)
                {
                    Cache[cache_id].state = Retiring;
                }
            }
        }
    }

    // Load each distinct file.
    for (uint8_t i = 0; i < new_files; ++i)
    {
        Load_audio_file(-1, new_file_id[i]);
    }

    return true;
}

void PatchCacheManager::Retire_cache_if_unused(uint16_t file_id)
{
    int8_t cache_id = Get_cache_id_from_file_id(file_id);
    if (cache_id >= 0)
    {

        uint8_t instrument_counter = 0;
        for (auto instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
        {
            // Skip unused instruments
            if (!Patch[Patch_id].Instrument[instrument_id].used)
            {
                continue;
            }

            if (Sound[Get_sound_id(Patch_id, instrument_id)].file == file_id)
            {
                ++instrument_counter;
            }
        }

        if (instrument_counter == 0)
        {
            Cache[cache_id].state = Retiring;
        }
    }
}

void PatchCacheManager::Release_unreferenced_caches(uint16_t referenced_cache_mask)
{
    for (uint8_t cache_id = 0; cache_id < PATCH_CACHE_ARRAY_COUNT; ++cache_id)
    {
        const uint16_t cache_mask = static_cast<uint16_t>(1u << cache_id); // prepara la maschera di lettura: 00000000 00000001, 00000000 00000010 ... fino a 000000001 00000000

        if (Cache[cache_id].state == Retiring && (referenced_cache_mask & cache_mask) == 0) // and bit per bit
        {
            Cache[cache_id].state = Free;
        }
    }
}