/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#pragma once

#include <Arduino.h>

static constexpr int VCF_value_names = 8;
enum VCF_value_name
{
    value_VCF_Menu,
    value_VCF_Gain_Volume,
    value_VCF_FilterType,
    value_VCF_Cutoff,
    value_VCF_Resonance,
    value_VCF_LfoModulationType,
    value_VCF_LfoModFreqTime,
    value_VCF_LfoModDepth
};
