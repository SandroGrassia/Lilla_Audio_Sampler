/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 */
#include "PatchCacheManager.h"
#include "SharedLiveSampler.h"
#include "PlayersManager.h"
#include <spi_interrupt.h>

uint32_t PatchCacheManager::File_samples(int16_t file_id)
{
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
}

void PatchCacheManager::Set_cache_pointer(uint8_t cache_id, int16_t *pointer)
{
    if (cache_id < PATCH_CACHE_ARRAY_COUNT)
    {
        cache_pointer[cache_id] = pointer;
    }
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
    if (job.cache_id < 0 || job.cache_id >= PATCH_CACHE_ARRAY_COUNT)
    {
        return false;
    }
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

void PatchCacheManager::Process_pending_loads(PlayersManager &players, bool copying_allowed) // Run one bounded cache-loading step from main, never from the audio interrupt.
{
    // The caller owns UI and recording policy; this class only checks whether background copying is allowed.
    if (!copying_allowed || NVIC_IS_ENABLED(IRQ_SOFTWARE) == 0)
    {
        return;
    }

    // Entry requires IRQ_SOFTWARE to be enabled, so each critical-section exit restores that state.
    CopyJob job;

    NVIC_DISABLE_IRQ(IRQ_SOFTWARE); // Take a coherent snapshot while the audio callback cannot change player references.
    const uint32_t cycle = audio_update_cycle;

    // loop() can run many times per audio block: allow at most one attempt per block, and skip late attempts.
    // The CYCLE_TIME_LIMIT threshold leaves a conservative margin for up to 512 samples before the next audio deadline.
    if (cycle == last_cycle || audio_update_time_micros > CYCLE_TIME_LIMIT)
    {
        NVIC_ENABLE_IRQ(IRQ_SOFTWARE);
        return;
    }

    last_cycle = cycle;
    Release_unreferenced_caches(players.Get_cache_reference_mask()); // Reuse retired buffers only after every player has released them.
    const bool copying = Prepare_copy(job);                          // Reserve the next chunk; incomplete files remain unavailable to players.
    const uint16_t reclaim_mask = copying ? 0 : Get_reclaim_mask();  // Request space only when pending work cannot obtain a free cache.

    if (reclaim_mask != blocked_mask)
    {
        blocked_mask = reclaim_mask;
        blocked_since_ms = millis(); // Start a new grace period whenever the blocking cache selection changes.
    }
    if (reclaim_mask != 0 && static_cast<uint32_t>(millis() - blocked_since_ms) >= 20u)
    {
        players.Fast_stop_players_using_cache(reclaim_mask); // After 20 ms, fade eligible old readers; their buffers are not freed until references disappear.
    }
    if (copying)
    {
        AudioStartUsingSPI(); // Register Flash bus use before allowing audio callbacks to run again.
    }
    NVIC_ENABLE_IRQ(IRQ_SOFTWARE); // Perform the Flash transfer outside the audio critical section.
    if (!copying)
    {
        return;
    }
    const bool success = LillaSerialFlashFile::Read_audio_samples(job.file_id, job.destination, job.first_sample, job.samples); // Copy only this reserved chunk into PSRAM, with audio interrupts enabled.

    NVIC_DISABLE_IRQ(IRQ_SOFTWARE);
    AudioStopUsingSPI();                            // Balance the SPI reservation even when the Flash read fails.
    const bool ready = Complete_copy(job, success); // Publish only a fully copied file; failed reads leave playback on Flash.
    if (ready)
    {
        players.Refresh_cache_sources(); // Promote current and queued matching voices atomically to the completed PSRAM source.
    }
    NVIC_ENABLE_IRQ(IRQ_SOFTWARE); // Restore audio processing
}
