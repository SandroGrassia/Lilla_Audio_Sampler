/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#pragma once

#include <Arduino.h>
#include <AudioStream.h>
#include "config.h"
#include "SharedElements.h"

class AudioPlayer;
class PatchCacheManager;
class AudioTables;
class PlayersManager;

class CacheCycleFinalizer : public AudioStream
{
private:
    AudioPlayer *players = nullptr;
    PatchCacheManager *cache_manager = nullptr;
    AudioTables *audio_tables = nullptr;
    PlayersManager *players_manager = nullptr;

public:
    CacheCycleFinalizer() : AudioStream(0, nullptr) {}

    void Begin(AudioPlayer *players_ptr, PatchCacheManager *cache_manager_ptr, AudioTables *audio_tables_ptr = nullptr, PlayersManager *players_manager_ptr = nullptr);
    virtual void update();
};