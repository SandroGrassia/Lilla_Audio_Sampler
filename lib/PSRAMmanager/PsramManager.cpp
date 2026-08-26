/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "PsramManager.h"

int16_t *PsramManager::New_samples_array(uint32_t dimension_bytes) // restituisce il puntatore int16_t* al primo elemento dell'array
{
    int16_t *_array = nullptr;

    if (dimension_bytes >= 2)
    {
        // Allocate the array on PSRAM
        _array = (int16_t *)extmem_malloc(dimension_bytes);

        if(_array == nullptr)
        {
            return nullptr;
        }

        // Reset array elements
        for (uint32_t i = 0; i < (dimension_bytes >> 1); ++i)
        {
            *(_array + i) = 0;
        }
    }
    return _array;
}

bool PsramManager::Remove_samples_array(int16_t *_array)
{
    if (_array == nullptr)
    {
        return false;
    }

    extmem_free(_array);
    return true;
}