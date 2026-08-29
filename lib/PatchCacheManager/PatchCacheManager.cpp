/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "PatchCacheManager.h"
#include "Functions.h"
#include <spi_interrupt.h>

uint16_t PatchCacheManager::Get_copy_samples(int16_t available_time)
{
    // non fornire samples_per_cycle > SAMPLES_MAX
}

int8_t PatchCacheManager::Get_cache_from_patch_and_instrument(uint8_t patch_id, uint8_t instrument_id)
{
    for (auto i = 0; i < PATCH_CACHE_ARRAY_COUNT; ++i)
    {
        if (Cache[i].patch_id == patch_id && Cache[i].instrument_id == instrument_id && Cache[i].state == Ready)
        {
            return i;
        }
    }
    return -1;
}

bool PatchCacheManager::Load_audio_file(uint8_t patch_id, uint8_t instrument_id, uint16_t file_id, uint32_t samples)
{
    if (file_id >= FIRST_RECORDING_FILE)
    {
        PRINT_ERROR(F("ERROR: PatchCacheManager supports only RAW files - "));
        return false;
    }

    rawfile.fast_open(file_id);
    if (!rawfile)
    {
        PRINT_ERROR(F("ERROR: PatchCacheManager unable to open RAW file - "));
        return false;
    }

    // Define cache_id
    int8_t cache_id;
    int16_t *cache_p;

    // cache da liberare al termine della copia
    const int8_t old_cache_id = Get_cache_from_patch_and_instrument(patch_id, instrument_id);

    cache_id = Get_cache_free();
    if (cache_id < 0)
    {
        PRINT_ERROR(F("ERROR: PatchCacheManager no free cache slot - "));
        return false;
    }

    cache_p = cache_pointer[cache_id];

    uint32_t residual_samples = samples <= PATCH_CACHE_ARRAY_SAMPLES ? samples : PATCH_CACHE_ARRAY_SAMPLES;
    residual_samples = (residual_samples <= rawfile.size() >> 1 ? residual_samples : rawfile.size() >> 1);

    uint32_t samples_copied = 0;

    Cache[cache_id].state = Writing;
    Cache[cache_id].file_id = file_id;
    Cache[cache_id].samples = residual_samples;
    Cache[cache_id].patch_id = patch_id;
    Cache[cache_id].instrument_id = instrument_id;

    bool finished = 0;
    
    AudioStartUsingSPI();
    while (!finished)
    {
        
        uint32_t elapsed = audio_update_time_micros; // fissa l'istante di partenza
        if (elapsed >= NON_AUDIO_TIME_MAX)
        {
            continue;
        }
        
        const uint32_t read_time = NON_AUDIO_TIME_MAX - elapsed;
        if (read_time < MIN_TIME_FOR_COPY_CYCLE_MICROS)
        {
            continue;
        }

        uint16_t samples_to_read = Get_copy_samples(read_time);
        samples_to_read = samples_to_read < residual_samples ? samples_to_read : residual_samples;
        const uint32_t samples_read = rawfile.read(reinterpret_cast<byte *>(cache_p + samples_copied), 2u * samples_to_read) / 2;
        if (samples_read == 0)
        {
            Cache[cache_id].state = Free;
            rawfile.close();
            AudioStopUsingSPI();
            PRINT_ERROR(F("ERROR: PatchCacheManager zero samples read from Flash - "));
            return false;
        }

        samples_copied += samples_read;
        residual_samples -= samples_read;
        if (residual_samples == 0)
        {
            Cache[cache_id].state = Ready;
            finished = true;
        }
    }
    rawfile.close();
    AudioStopUsingSPI();

    // Procedura per comunicare ai Player lo switch
    //
    //
    //

    if (old_cache_id >= 0)
    {
        Cache[old_cache_id].state = Free;
    }

    return true;
}
