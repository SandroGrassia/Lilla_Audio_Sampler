/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#pragma once

#include <Arduino.h>
#include "config.h"
#include "SharedDelay.h"
#include "DisplayPrimitives.h"
#include "GlobalDisplayDelay.h"

/*

enum DELAY_element_name
{
    value_DELAY_Feedback,
    value_DELAY_Delay_time,
    value_DELAY_Delay_time_LR,
    value_DELAY_Modulation_source,
    value_DELAY_Modulation_frequency,
    value_DELAY_Modulation_depth,
    value_DELAY_Modulation_phase_LR
};

*/

class PointerDelay
{
    private:
    DELAY_pointer_struct pointer;
    DELAY_pointer_struct pointer_old;
    
    public:
    PointerDelay () {}

    void Set_pointer_to_Feedback(void);
    void Move_pointer(const int value);
    void Display_pointer(void);
    DELAY_element_name Get_pointer(void);
};