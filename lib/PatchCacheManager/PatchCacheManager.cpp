/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 */
#include "CaptureSources.h"
#include "PatchCacheManager.h"
#include "SharedLiveSampler.h"

uint32_t PatchCacheManager::File_samples(int16_t file_id)
{
    const auto *capture = Capture_find(file_id);
    if (capture != nullptr)
    {
        return capture->audio.samples;
    }
    if (file_id < 0 || file_id >= FIRST_LIVE_SAMPLING_FILE)
    {
        return 0;
    }
    if (file_id < FIRST_RECORDING_FILE)
    {
        return FlashFileRegisterParser::length(file_id) / sizeof(int16_t);
    }
    const auto &recording = Recording[(file_id - FIRST_RECORDING_FILE) / 2];
    return recording.consistent && recording.bytes > 0 ? static_cast<uint32_t>(recording.bytes) / sizeof(int16_t) : 0;
}

void PatchCacheManager::Begin(void)
{
    for (auto &item : cache)
    {
        item = {};
    }
    for (auto &item : required)
    {
        item = {};
    }
    required_count = 0;
    retirement_counter = 0;
    for (uint8_t slot = 0; slot < CAPTURE_SOURCES; ++slot)
    {
        const auto &source = Capture_sources[slot].audio;
        if (source.psram_ptr != nullptr)
        {
            Reserve_capture(slot, source.file_id, source.samples);
        }
    }
}

int16_t *PatchCacheManager::Reserve_capture(uint8_t slot, int16_t file_id, uint32_t samples)
{
    if (slot >= CAPTURE_SOURCES || samples == 0 || samples > PATCH_CACHE_ARRAY_SAMPLES || cache_pointer[slot] == nullptr)
    {
        return nullptr;
    }
    cache[slot] = {};
    cache[slot].state = Ready;
    cache[slot].file_id = file_id;
    cache[slot].samples = samples;
    cache[slot].copied = samples;
    cache[slot].valid = true;
    return cache_pointer[slot];
}

void PatchCacheManager::Set_cache_pointer(uint8_t cache_id, int16_t *pointer)
{
        cache_pointer[cache_id] = pointer;
}

int PatchCacheManager::Find_required(int16_t file_id) const
{
    for (uint8_t i = 0; i < required_count; ++i)
    {
        if (required[i].file_id == file_id)
        {
            return i;
        }
    }
    return -1;
}

int PatchCacheManager::Find_complete(int16_t file_id, uint32_t samples) const
{
    for (uint8_t i = 0; i < PATCH_CACHE_ARRAY_COUNT; ++i)
    {
        if (cache[i].valid && cache[i].state != Loading && cache[i].file_id == file_id && cache[i].samples == samples && cache[i].copied == samples)
        {
            return i;
        }
    }
    return -1;
}

void PatchCacheManager::Retire(uint8_t cache_id)
{
    if (cache[cache_id].state == Ready)
    {
        cache[cache_id].state = Retiring;
        cache[cache_id].retired_order = ++retirement_counter;
    }
}

void PatchCacheManager::Set_required_files(const Preset_struct (&presets)[INSTRUMENTS])
{
    const auto previous = required_count;
    Request old_required[INSTRUMENTS];

    for (uint8_t i = 0; i < previous; ++i)
    {
        old_required[i] = required[i];
    }

    required_count = 0;

    for (const auto &preset : presets)
    {
        if (!preset.active || preset.file >= FIRST_LIVE_SAMPLING_FILE || Find_required(preset.file) >= 0)
        {
            continue;
        }
        Request request;
        request.file_id = preset.file;
        request.samples = File_samples(preset.file);
        request.failed = request.samples == 0 || request.samples > PATCH_CACHE_ARRAY_SAMPLES;
        for (uint8_t i = 0; i < previous; ++i)
        {
            if (old_required[i].file_id == request.file_id && old_required[i].samples == request.samples)
            {
                request.failed |= old_required[i].failed;
            }
        }
        required[required_count++] = request;
    }

    for (uint8_t i = 0; i < PATCH_CACHE_ARRAY_COUNT; ++i)
    {
        if (Capture_find(cache[i].file_id) != nullptr)
        {
            continue; // Unsaved sources must survive sampler and patch changes.
        }

        const int request = Find_required(cache[i].file_id);
        const bool keep = request >= 0 && !required[request].failed && required[request].samples == cache[i].samples;
        
        if (!keep)
        {
            if (cache[i].state == Loading)
            {
                cache[i] = {};
            }
            else
            {
                Retire(i);
            }
        }
    }

    for (uint8_t i = 0; i < required_count; ++i)
    {
        if (required[i].failed)
        {
            continue;
        }

        const int ready = Find_complete(required[i].file_id, required[i].samples);
        if (ready >= 0)
        {
            cache[ready].state = Ready;
        }
    }
}

AudioFileSource PatchCacheManager::Get_source(int16_t file_id) const
{
    const auto *capture = Capture_find(file_id);
    if (capture != nullptr)
    {
        return capture->audio;
    }

    AudioFileSource result;
    result.file_id = file_id;
    result.samples = File_samples(file_id);

    for (uint8_t i = 0; i < PATCH_CACHE_ARRAY_COUNT; ++i)
    {
        if (cache[i].state == Ready && cache[i].valid && cache[i].file_id == file_id)
        {
            result.storage = Psram;
            result.psram_ptr = cache_pointer[i];
            result.samples = cache[i].samples;
            result.cache_id = i;
            break;
        }
    }
    return result;
}

bool PatchCacheManager::Prepare_copy(CopyJob &job)
{
    job = {};
    int selected = -1;
    for (uint8_t i = 0; i < PATCH_CACHE_ARRAY_COUNT; ++i)
    {
        if (cache[i].state == Loading)
        {
            selected = i;
            break;
        }
    }

    if (selected < 0)
    {
        for (uint8_t request = 0; request < required_count; ++request)
        {
            if (required[request].failed || Find_complete(required[request].file_id, required[request].samples) >= 0)
            {
                continue;
            }
            for (uint8_t i = 0; i < PATCH_CACHE_ARRAY_COUNT; ++i)
            {
                if (cache[i].state == Free && cache_pointer[i] != nullptr)
                {
                    selected = i;
                    cache[i] = {};
                    cache[i].state = Loading;
                    cache[i].file_id = required[request].file_id;
                    cache[i].samples = required[request].samples;
                    break;
                }
            }
            break;
        }
    }

    if (selected < 0)
    {
        return false;
    }

    const auto &item = cache[selected];
    job.cache_id = selected;
    job.file_id = item.file_id;
    job.first_sample = item.copied;
    const uint32_t remaining = item.samples - item.copied;
    job.samples = remaining < 512u ? remaining : 512u;
    job.destination = cache_pointer[selected] + item.copied;
    return true;
}

bool PatchCacheManager::Complete_copy(const CopyJob &job, bool success)
{
    auto &item = cache[job.cache_id];
    if (item.state != Loading || item.file_id != job.file_id || item.copied != job.first_sample)
    {
        return false;
    }

    if (!success)
    {
        const int request = Find_required(item.file_id);
        if (request >= 0)
        {
            required[request].failed = true;
        }
        item = {};
        return false;
    }
    
    item.copied += job.samples;
    if (item.copied == item.samples)
    {
        item.valid = true;
        item.state = Ready;
        return true;
    }
    return false;
}

uint16_t PatchCacheManager::Get_reclaim_mask(void) const
{
    bool missing = false;
    for (uint8_t i = 0; i < required_count; ++i)
    {
        missing |= !required[i].failed && Find_complete(required[i].file_id, required[i].samples) < 0;
    }
    if (!missing)
    {
        return 0;
    }

    int oldest = -1;
    for (uint8_t i = 0; i < PATCH_CACHE_ARRAY_COUNT; ++i)
    {
        if (cache[i].state == Free || cache[i].state == Loading)
        {
            return 0;
        }
        if (cache[i].state == Retiring && (oldest < 0 || static_cast<int32_t>(cache[i].retired_order - cache[oldest].retired_order) < 0))
        {
            oldest = i;
        }
    }

    return oldest < 0 ? 0 : static_cast<uint16_t>(1u << oldest);
}

void PatchCacheManager::Invalidate_file(int16_t file_id)
{
    for (uint8_t i = 0; i < PATCH_CACHE_ARRAY_COUNT; ++i)
    {
        if (cache[i].file_id != file_id)
        {
            continue;
        }
        cache[i].valid = false;
        if (cache[i].state == Loading || cache[i].state == Free)
        {
            cache[i] = {};
        }
        else
        {
            Retire(i);
        }
    }
    
    const int request = Find_required(file_id);
    if (request >= 0)
    {
        required[request].samples = 0;
        required[request].failed = true;
    }
}

void PatchCacheManager::Release_unreferenced_caches(uint16_t referenced_cache_mask)
{
    for (uint8_t i = 0; i < PATCH_CACHE_ARRAY_COUNT; ++i)
    {
        if (cache[i].state == Retiring && (referenced_cache_mask & static_cast<uint16_t>(1u << i)) == 0)
        {
            cache[i].state = Free;
        }
    }
}
