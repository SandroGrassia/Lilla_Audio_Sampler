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
        const uint32_t samples_to_copy = file_samples < PATCH_CACHE_ARRAY_SAMPLES? file_samples : PATCH_CACHE_ARRAY_SAMPLES;

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

bool PatchCacheManager::Load_patch(uint8_t patch_id)
{
    return false;
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