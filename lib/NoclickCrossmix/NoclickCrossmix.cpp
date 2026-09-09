/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

// NoclickCrossmix.h crea il vettore di cross-mix per loop Forward o loop Reverse

#include "NoclickCrossmix.h"

int16_t *NoclickCrossmix::get_pointer()
{
    return Noclick; // &NoClick[0];
}

bool NoclickCrossmix::Make(int file_id, int32_t A_Flash_sample, int32_t B_Flash_sample, uint16_t delta_Noclick)
{
    return Make(file_id, A_Flash_sample, B_Flash_sample, delta_Noclick, Noclick);
}

bool NoclickCrossmix::Make(int file_id, int32_t A_Flash_sample, int32_t B_Flash_sample, uint16_t delta_Noclick, int16_t *destination)
{
    if (delta_Noclick == 0)
    {
        return true;
    }

    if (destination == nullptr || A_Flash_sample < 0 || B_Flash_sample < A_Flash_sample || delta_Noclick < 2 || delta_Noclick > NOCLICK_DIM)
    {
        return false;
    }

    const uint32_t span = static_cast<uint32_t>(B_Flash_sample - A_Flash_sample) + 1u;

    if (2u * static_cast<uint32_t>(delta_Noclick) > span)
    {
        return false;
    }

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