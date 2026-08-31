/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#pragma once

#include <Arduino.h>
#include "config.h"
#include "SharedElements.h"

#include "LillaSerialFlash.h"

class PatchCacheManager
{
    private:
    static constexpr uint32_t SAMPLES_MAX = 3000; // limitato dal max trasferimento possibile SPI/QSPI entro 2.9ms
    static constexpr uint8_t MIN_TIME_FOR_COPY_CYCLE_MICROS = 50;
    static constexpr uint16_t COPY_TIME_MAX = 2800;
    

    LillaSerialFlashFile rawfile;

    enum CacheState : uint8_t
    {
        Free,
        Ready,
    };

    struct CacheStruct
    {
        CacheState state;
        uint32_t samples;
        int16_t file_id;
    };
    CacheStruct Cache[PATCH_CACHE_ARRAY_COUNT];
    int16_t* cache_pointer[PATCH_CACHE_ARRAY_COUNT];
    
    uint16_t Get_copy_samples(uint16_t read_time_micros);
    int8_t Get_cache_free(void);
    int8_t Get_cache_id_from_file_id(uint16_t file_id);
    void Free_cache_if_unused(uint16_t file_id);

public:
    PatchCacheManager()
    {
        Begin();
    };
    
    void Set_cache_pointer(uint8_t cache_id, int16_t* pointer);
    void Begin(void); // reset inner arrays
    bool Load_patch(int16_t old_patch_id, uint8_t new_patch_id);
    bool Load_patch(uint8_t new_patch_id);
    bool Load_audio_file(int16_t file_id_old, uint16_t file_id);
};