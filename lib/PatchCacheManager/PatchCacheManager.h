/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 */
#pragma once

#include <Arduino.h>
#include "SharedElements.h"
#include "LillaSerialFlash.h"

class PatchCacheManager
{
public:
    struct CopyJob
    {
        int8_t cache_id = -1;
        int16_t file_id = -1;
        uint32_t first_sample = 0;
        uint16_t samples = 0;
        int16_t *destination = nullptr;
    };

private:
    enum CacheState : uint8_t { Free, Loading, Ready, Retiring };
    struct CacheStruct
    {
        CacheState state = Free;
        int16_t file_id = -1;
        uint32_t samples = 0;
        uint32_t copied = 0;
        uint32_t retired_order = 0;
        bool valid = false;
    };
    struct Request
    {
        int16_t file_id = -1;
        uint32_t samples = 0;
        bool failed = false;
    };
    CacheStruct cache[PATCH_CACHE_ARRAY_COUNT];
    int16_t *cache_pointer[PATCH_CACHE_ARRAY_COUNT] = {};
    Request required[INSTRUMENTS];
    uint8_t required_count = 0;
    uint32_t retirement_counter = 0;
    static uint32_t File_samples(int16_t file_id); // Read the complete logical file length from metadata without copying audio.
    int Find_required(int16_t file_id) const; // Find an active file request, including requests that fall back to Flash.
    int Find_complete(int16_t file_id, uint32_t samples) const; // Find reusable, fully copied data that has not been invalidated.
    void Retire(uint8_t cache_id); // Keep old audio readable until all player references disappear.

public:
    PatchCacheManager() = default; // Initialize metadata without accessing external RAM during static construction.
    void Set_cache_pointer(uint8_t cache_id, int16_t *pointer); // Attach an allocated PSRAM buffer during setup.
    void Begin(void); // Invalidate all caches after players have been drained; call with audio interrupts disabled.
    void Set_required_files(const Preset_struct (&presets)[INSTRUMENTS]); // Pin the published patch files and queue missing data; call with audio interrupts disabled.
    AudioFileSource Get_source(int16_t file_id) const; // Return only complete, pinned data, otherwise select Flash.
    bool Prepare_copy(CopyJob &job); // Reserve one bounded copy operation; call from main with audio interrupts disabled.
    bool Complete_copy(const CopyJob &job, bool success); // Publish a completed file or discard a failed copy; call with audio interrupts disabled.
    uint16_t Get_reclaim_mask(void) const; // Select the oldest retiring cache only when a pending file has no free slot.
    void Invalidate_file(int16_t file_id); // Prevent reuse after file replacement while preserving existing readers.
    void Release_unreferenced_caches(uint16_t referenced_cache_mask); // Recycle retired buffers after the final audio reader releases them.
};
