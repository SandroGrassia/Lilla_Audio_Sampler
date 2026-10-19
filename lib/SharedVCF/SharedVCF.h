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

// Keep legacy patch frequencies intact; new edits use exact shared LPF values.
static constexpr uint8_t VCF_CUTOFF_TABLE_BASE = 128;
float Calc_vcf_cutoff(uint8_t value);
int VCF_cutoff_index(uint8_t value);
