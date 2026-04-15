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
    LS_pointer_struct pointer;
    LS_pointer_struct pointer_old;

public:
    PointerLiveSampler() {}

    void Set_pointer_to_first_menu_element(void);
    void Move_pointer(const int value);
    void Restore_pointer(void);
    void Show_pointer(const bool show);
    LS_pointer_struct Get_pointer(void);
};