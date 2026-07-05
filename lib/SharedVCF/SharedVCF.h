/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#pragma once

#include <Arduino.h>

static constexpr int VCF_value_names = 8;
enum VCF_value_name : int
{
    value_VCF_Menu = 0,
    value_VCF_Gain_Volume = 1,
    value_VCF_FilterType = 2,
    value_VCF_Cutoff = 3,
    value_VCF_Resonance = 4,
    value_VCF_LfoModulationType = 5,
    value_VCF_LfoModFreqTime = 6,
    value_VCF_LfoModDepth = 7
};
