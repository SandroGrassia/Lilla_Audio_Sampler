/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#pragma once

#include <Arduino.h>
#include "config.h"
#include "SharedVCF.h"
#include "DisplayPrimitives.h"
#include "GlobalDisplayVCF.h"

class PointerVCF
{
private:
    int pointer;
    int pointer_old;
    static constexpr int pointer_max = VCF_value_names - 1;
    void Print_pointer_description(void);

public:
    PointerVCF() {}

    void Set_pointer_to_FilterType(void);
    bool Move_pointer(const int value);
    void Display_pointer(void);
    VCF_value_name Get_VCF_value_name(void);
};
