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
    static constexpr uint16_t NON_AUDIO_TIME_MAX = 2800;
    uint16_t Get_copy_samples(int16_t available_time);

    LillaSerialFlashFile rawfile;

    enum CacheState : uint8_t
    {
        Free,
        Writing,
        Ready,
        Error
    };

    struct CacheStruct
    {
        int16_t* pointer = nullptr;
        CacheState state = CacheState::Free;
        uint32_t samples;
        uint8_t patch_id;
        uint8_t instrument_id;
        uint16_t file_id;
    };
    CacheStruct Cache[PATCH_CACHE_ARRAY_COUNT];
    int16_t* cache_pointer[PATCH_CACHE_ARRAY_COUNT];

    int8_t Get_cache_free(void);
    int8_t Get_cache_from_patch_and_instrument(uint8_t patch_id, uint8_t instrument_id);

public:
    PatchCacheManager()
    {
        Begin();
    };

    void Begin(); // reset inner arrays
    bool Load_patch(uint8_t patch_id);
    bool Load_audio_file(uint8_t patch_id, uint8_t instrument_id, uint16_t new_file_id, uint32_t samples);
    int16_t* Get_patch_pointer(int file_id);
};