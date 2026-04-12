/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#pragma once

#include <Arduino.h>
#include "config.h"
#include "SharedLS.h"
#include "DisplayPrimitives.h"
#include "GlobalDisplayLiveSampler.h"

/*

enum LS_element_name
{
    value_LS_Play_mode,
    value_LS_Feedback,
};

*/

class PointerLiveSampler
{
private:
    LS_element_name pointer;
    LS_element_name pointer_old;
    static constexpr int pointer_max = LS_element_names - 1;

public:
    PointerLiveSampler() {}

    void Set_pointer_to_play_mode(void);
    void Move_pointer(const int value);
    void Display_pointer(void);
    LS_element_name Get_element_name(void);
};