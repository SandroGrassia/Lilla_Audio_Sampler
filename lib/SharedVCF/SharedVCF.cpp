/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 */

#include "SharedVCF.h"
#include "SharedElements.h"
#include <cmath>

float Calc_vcf_cutoff(uint8_t value)
{
    if (value >= VCF_CUTOFF_TABLE_BASE && value <= VCF_CUTOFF_TABLE_BASE + LPF_MAX)
    {
        return lowpass_value[value - VCF_CUTOFF_TABLE_BASE];
    }
    return 20.0f * powf(2.0f, value / 10.0f);
}

int VCF_cutoff_index(uint8_t value)
{
    if (value >= VCF_CUTOFF_TABLE_BASE && value <= VCF_CUTOFF_TABLE_BASE + LPF_MAX)
    {
        return value - VCF_CUTOFF_TABLE_BASE;
    }
    const float frequency = Calc_vcf_cutoff(value);
    int nearest = 0;
    for (int index = 1; index <= LPF_MAX; ++index)
    {
        if (fabsf(lowpass_value[index] - frequency) < fabsf(lowpass_value[nearest] - frequency))
        {
            nearest = index;
        }
    }
    return nearest;
}
