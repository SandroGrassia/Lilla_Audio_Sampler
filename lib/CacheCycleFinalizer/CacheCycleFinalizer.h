/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#pragma once

#include <Arduino.h>
#include "config.h"
#include "SharedElements.h"

class AudioPlayer;
class PatchCacheManager;

class CacheCycleFinalizer : public AudioStream
{
private:
    AudioPlayer *players = nullptr;
    PatchCacheManager *cache_manager = nullptr;

public:
    CacheCycleFinalizer() : AudioStream(0, nullptr) {}

    void Begin(AudioPlayer *players_ptr, PatchCacheManager *cache_manager_ptr);
    virtual void update();
};