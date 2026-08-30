/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "PatchCacheManager.h"
#include "Functions.h"
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
    // prediligi cache_id con file invalidato
    for (auto i = 0; i < PATCH_CACHE_ARRAY_COUNT; ++i)
    {
        if (Cache[i].state == Free && Cache[i].file_id == -1)
        {
            return i;
        }
    }
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

bool PatchCacheManager::Load_audio_file(int16_t file_id_old, uint16_t file_id)
{
    bool finished = false;
    int8_t cache_id = -1;

    if (file_id >= FIRST_RECORDING_FILE)
    {
        PRINT_ERROR(F("ERROR: PatchCacheManager supports only RAW files - "));
        return false;
    }

    // Verifica se il file_id sia già presente
    cache_id = Get_cache_id_from_file_id(file_id);
    if (cache_id >= 0)
    {
        Serial.println(F("Audio .raw file already present in PSRAM chips"));
        finished = true;
    }

    else
    {
        rawfile.fast_open(file_id);
        if (!rawfile)
        {
            PRINT_ERROR(F("ERROR: PatchCacheManager unable to open RAW file - "));
            return false;
        }

        // calcola i sample da copiare
        const uint32_t file_samples = rawfile.size() / sizeof(int16_t);
        if (file_samples == 0)
        {
            rawfile.close();
            PRINT_ERROR(F("ERROR: PatchCacheManager empty RAW file - "));
            return false;
        }
        const uint32_t samples_to_copy = file_samples < PATCH_CACHE_ARRAY_SAMPLES ? file_samples : PATCH_CACHE_ARRAY_SAMPLES;

        // individua la cache_id Free
        cache_id = Get_cache_free();
        if (cache_id < 0)
        {
            PRINT_ERROR(F("ERROR: PatchCacheManager no free cache slot - "));
            return false;
        }

        // invalida cache_id
        Cache[cache_id].state = Free;
        Cache[cache_id].file_id = -1;
        Cache[cache_id].samples = 0;

        // variabili per la copia
        int16_t *cache_p = cache_pointer[cache_id];
        uint32_t residual_samples = samples_to_copy;
        uint32_t samples_copied = 0;
        uint32_t cycle_counter = 0;

        AudioStartUsingSPI();
        while (!finished)
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

            // se la lettura fallisce la cache_id viene dichiarata non valida, cache_id_old non viene modificata
            if (samples_read == 0)
            {
                rawfile.close();
                AudioStopUsingSPI();
                PRINT_ERROR(F("ERROR: PatchCacheManager zero samples read from Flash - "));
                return false;
            }

            samples_copied += samples_read;
            residual_samples -= samples_read;
            ++cycle_counter;

            if (residual_samples == 0)
            {
                finished = true;
            }
        }
        rawfile.close();
        AudioStopUsingSPI();

        Cache[cache_id].samples = samples_copied;
        Cache[cache_id].file_id = file_id;

        Serial.println(F("Audio .raw file copied from Flash chip to PSRAM chips"));
        Serial.print("file: ");
        Serial.println(file_id);
        Serial.print("cache_id: ");
        Serial.println(cache_id);
        Serial.print("cycles: ");
        Serial.println(cycle_counter);
    }

    // conferma la nuova cache
    Cache[cache_id].state = Ready;

    // Procedura per aggiornare Sound, Preset, e comunicare ai Player lo switch, protetta da IRQ audio
    //
    //
    //

    // rilascia la vecchia cache se non usata
    if (file_id_old >= 0 && file_id_old != file_id)
    {
        Free_cache_if_unused(file_id_old);
    }
    return true;
}

bool PatchCacheManager::Load_patch(int16_t old_patch_id, uint8_t new_patch_id)
{
    if (old_patch_id < -1 || old_patch_id > PATCHES_MAX)
    {
        PRINT_ERROR(F("ERROR: PatchCacheManager invalid old patch_id - "));
        return false;
    }

    if (new_patch_id >= PATCHES_MAX || !Patch[new_patch_id].used)
    {
        PRINT_ERROR(F("ERROR: PatchCacheManager invalid new patch_id - "));
        return false;
    }

    uint16_t new_file_id[INSTRUMENTS];
    uint8_t new_files = 0;

    // Costruisce l'elenco dei file RAW distinti usati dalla nuova Patch.
    for (uint8_t instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
    {
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

    // Verifica preventivamente i file non ancora presenti in cache.
    // La vecchia cache non viene modificata se un file è assente o vuoto.
    for (uint8_t i = 0; i < new_files; ++i)
    {
        // escludi i file già presenti in cache
        if (Get_cache_id_from_file_id(new_file_id[i]) >= 0)
        {
            continue;
        }

        rawfile.fast_open(new_file_id[i]);

        if (!rawfile)
        {
            PRINT_ERROR(F("ERROR: PatchCacheManager unable to open RAW file - "));
            return false;
        }

        if (rawfile.size() < sizeof(int16_t))
        {
            rawfile.close();
            PRINT_ERROR(F("ERROR: PatchCacheManager empty RAW file - "));
            return false;
        }

        rawfile.close();
    }

    // Libera soltanto i file della vecchia Patch non richiesti dalla nuova.
    // I Player devono essere già stati fermati.
    if (old_patch_id >= 0)
    {
        for (uint8_t instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
        {
            if (!Patch[old_patch_id].Instrument[instrument_id].used)
            {
                continue;
            }

            const uint16_t old_file_id = Sound[Get_sound_id(old_patch_id, instrument_id)].file;

            bool used_by_new_patch = false;

            for (uint8_t i = 0; i < new_files; ++i)
            {
                if (new_file_id[i] == old_file_id)
                {
                    used_by_new_patch = true;
                    break;
                }
            }

            if (!used_by_new_patch)
            {
                const int8_t cache_id =
                    Get_cache_id_from_file_id(old_file_id);

                if (cache_id >= 0)
                {
                    Cache[cache_id].state = Free;
                }
            }
        }
    }

    // Carica ogni file una sola volta. Un eventuale file già in cache, anche Free, viene semplicemente riportato a Ready.
    for (uint8_t i = 0; i < new_files; ++i)
    {
        if (!Load_audio_file(-1, new_file_id[i]))
        {
            return false;
        }
    }

    return true;
}

void PatchCacheManager::Free_cache_if_unused(uint16_t file_id)
{
    int8_t cache_id = Get_cache_id_from_file_id(file_id);
    if (cache_id >= 0)
    {
        uint8_t instrument_counter = 0;
        for (auto instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
        {
            if (Sound[Get_sound_id(Patch_id, instrument_id)].file == file_id)
            {
                ++instrument_counter;
            }
        }

        if (instrument_counter == 0)
        {
            Cache[cache_id].state = Free;
        }
    }
}