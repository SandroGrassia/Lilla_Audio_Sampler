/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "WavetableManager.h"

int16_t WavetableManager::cache[WavetableManager::WAVETABLE_DIM] = {0};

bool WavetableManager::Make(int file_id, int8_t mode, int A_Flash_sample, int B_Flash_sample, uint16_t delta_Noclick, int16_t *p_Noclick, int16_t *destination)
{
    if (destination == nullptr)
    {
        return false;
    }

    const uint32_t span = static_cast<uint32_t>(B_Flash_sample - A_Flash_sample) + 1u;

    if (span < 2u || span > static_cast<uint32_t>(WAVETABLE_DIM))
    {
        return false;
    }

    const bool crossfade_mode = mode == LOOP_FWD || mode == LOOP_REV;

    if (!crossfade_mode)
    {
        delta_Noclick = 0;
    }

    if (delta_Noclick == 1 || delta_Noclick > NOCLICK_DIM || 2u * static_cast<uint32_t>(delta_Noclick) > span || (delta_Noclick > 0 && p_Noclick == nullptr))
    {
        return false;
    }

    // A------(A+d-1)(A+d)-----------------(B-d)(B-d+1)------(B)
    // ********************************************************
    int length_max = B_Flash_sample - A_Flash_sample + 1;

    // A------(A+d-1)(A+d)-----------------(B-d)(B-d+1)------(B)
    //                 ***********************
    int length_min = B_Flash_sample - A_Flash_sample - (2 * delta_Noclick) + 1;

    // A------(A+d-1)(A+d)-----------------(B-d)(B-d+1)------(B)
    //                 >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    //                         oppure:
    // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    int length_mix = B_Flash_sample - A_Flash_sample - delta_Noclick + 1;

    if (mode == ONCE_FWD || mode == LOOP_REV_FWD)
    {
        mode = LOOP_FWD_REV;
    }

    switch (mode)
    {
    case ONCE_REV: // mode 1: B-->A than STOP


        // A>>>>>>>>>>>>>length_max>>>>>>>>>>>>>B
        if (!LillaSerialFlashFile::Read_audio_samples(file_id, cache, A_Flash_sample, length_max))
        {
            return false;
        }

        // B>>>>>>>>>>>>length_max>>>>>>>>>>>>>>A
        for (auto sample = 0; sample < (length_max); ++sample)
        {
            destination[sample] = cache[length_max - 1 - sample];
        }

        break;

    case LOOP_FWD: // mode 2: loop A-->B A-->B.

        // forward play from (A_Flash_sample + delta_Noclick) to B_Flash_sample

        // (A+d)******length_min*********(B-d)
        if (!LillaSerialFlashFile::Read_audio_samples(file_id, destination, A_Flash_sample + delta_Noclick, length_min))
        {
            return false;
        }

        // (B-d+1)***delta_Noclick***B
        for (auto sample = 0; sample < delta_Noclick; ++sample)
        {
            destination[length_min + sample] = *(p_Noclick + sample);
        }

        // REWORK_Wavetable(1, 0.5);
        break;

    case LOOP_FWD_REV: // mode 3: loop A-->B-->A

        // play forward from A_Flash_sample to B_Flash_sample and reverse

        // A>>>>>>>>>>>>>length_max>>>>>>>>>>>>>B
        if (!LillaSerialFlashFile::Read_audio_samples(file_id, destination, A_Flash_sample, length_max))
        {
            return false;
        }

        // (A+1)<<<<<<<(length_max - 2)<<<<<<<(B-1)
        for (auto sample = 0; sample < (length_max - 2); ++sample)
        {
            destination[length_max + sample] = destination[(length_max - 2) - sample];
        }

        // REWORK_Wavetable(3);
        break;

    case LOOP_REV: // mode 5: B-->A B-->A

        // reverse play from (A_Flash_sample) to (B_Flash_sample - delta_Noclick)

        // (A)>>>>delta>>>>(A+d-1)
        for (auto sample = 0; sample < delta_Noclick; ++sample)
        {
            cache[sample] = *(p_Noclick + sample);
        }

        // (A)>>>>>delta>>>>(A+d-1)(A+d)>>>>>>>>>>>>>>>>length_min>>>>>>>>>>>>(B-d)
        if (!LillaSerialFlashFile::Read_audio_samples(file_id, cache + delta_Noclick, A_Flash_sample + delta_Noclick, length_min))
        {
            return false;
        }

        for (auto sample = 0; sample < length_mix; ++sample)
        {
            destination[sample] = cache[(length_mix - 1) - sample];
        }
        break;

    default:
        return false;
    }
    return true;
}