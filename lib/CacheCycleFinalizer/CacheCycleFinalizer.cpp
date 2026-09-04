/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "CacheCycleFinalizer.h"
#include "AudioPlayer.h"
#include "PatchCacheManager.h"

void CacheCycleFinalizer::Begin(AudioPlayer *players_ptr, PatchCacheManager *cache_manager_ptr)
{
    players = players_ptr;
    cache_manager = cache_manager_ptr;
    active = true;
}

void CacheCycleFinalizer::update()
{
    uint16_t referenced_cache_mask = 0; // referenced_cache_mask is the mask where each bit=1 corresponds to a cache that a Player is reading

    for (uint8_t player = 0; player < PLAYERS; ++player)
    {
        referenced_cache_mask |= players[player].Get_cache_reference_mask(); // Get_cache_reference_mask responds with a mask where each bit=1 corresponds to a cache that a Player is reading
    }

    cache_manager->Release_unreferenced_caches(referenced_cache_mask);
}