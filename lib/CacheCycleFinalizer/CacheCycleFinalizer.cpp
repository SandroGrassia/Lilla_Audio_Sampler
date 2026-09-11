/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "CacheCycleFinalizer.h"
#include "AudioPlayer.h"
#include "PatchCacheManager.h"
#include "AudioTables.h"

void CacheCycleFinalizer::Begin(AudioPlayer *players_ptr, PatchCacheManager *cache_manager_ptr, AudioTables *audio_tables_ptr)
{
    players = players_ptr;
    cache_manager = cache_manager_ptr;
    audio_tables = audio_tables_ptr;
    active = true;
}

void CacheCycleFinalizer::update()
{
    if (players == nullptr)
    {
        return;
    }

    uint16_t referenced_cache_mask = 0;
    uint8_t referenced_banks_mask = 0;

    for (uint8_t player = 0; player < PLAYERS; ++player)
    {
        referenced_cache_mask |= players[player].Get_cache_reference_mask();
        referenced_banks_mask |= players[player].Get_tables_reference_mask();
    }

    if (cache_manager != nullptr)
    {
        cache_manager->Release_unreferenced_caches(referenced_cache_mask);
    }

    if (audio_tables != nullptr)
    {
        audio_tables->Release_unreferenced_banks(referenced_banks_mask);
    }
}