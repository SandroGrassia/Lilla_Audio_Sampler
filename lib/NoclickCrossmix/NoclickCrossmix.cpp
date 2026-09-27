/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

// NoclickCrossmix.h crea il vettore di cross-mix per loop Forward o loop Reverse

#include "NoclickCrossmix.h"

bool NoclickCrossmix::Make(int file_id, int32_t A_Flash_sample, int32_t B_Flash_sample, uint16_t delta_Noclick, int16_t *destination)
{
    int16_t cache[NOCLICK_DIM];

    if (!LillaSerialFlashFile::Read_audio_samples(file_id, cache, A_Flash_sample, delta_Noclick))
    {
        return false;
    }

    if (!LillaSerialFlashFile::Read_audio_samples(file_id, destination, B_Flash_sample - delta_Noclick + 1, delta_Noclick))
    {
        return false;
    }

    float h;

    for (auto sample = 0; sample < delta_Noclick; ++sample)
    {
        h = static_cast<float>(sample) / (static_cast<float>(delta_Noclick) - 1.0f);
        cache[sample] = static_cast<float>(cache[sample]) * h;
        destination[sample] = static_cast<float>(destination[sample]) * (1.0f - h) + cache[sample];
    }

    return true;
}