/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#pragma once

#include <Arduino.h>
#include "SharedSampler.h"
#include "DisplayPrimitives.h"
#include "GlobalDisplaySampler.h"

class PointerSampler
{
private:
    DS_pointer_struct pointer;
    DS_pointer_struct pointer_old;

public:
    PointerSampler() {}

    void Set_pointer_to_first_menu_element(void);
    void Move_pointer(const int value);
    void Move_pointer_within_menu(const int value);
    void Restore_pointer(void);
    void Show_pointer(const bool show);
    void Print_pointer(void);
    DS_pointer_struct Get_pointer(void);
};