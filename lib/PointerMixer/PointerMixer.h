/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#pragma once

#include <Arduino.h>
#include "config.h"
#include "SharedMixer.h"
#include "DisplayPrimitives.h"
#include "GlobalDisplayMixer.h"

/*

enum MX_field_name
{
    field_MX_Source,
    field_MX_Elements
};

enum MX_Element_name
{
    value_MX_Mute_Gain,
    value_MX_Pan,
    value_MX_Lineout,
    value_MX_Monitor
};

struct MX_pointer_struct
{
MX_field_name field_name;
int source;
int element;
};

*/

class PointerMixer
{
    private:
    MX_pointer_struct pointer;
    MX_pointer_struct pointer_old;
    

    public:
    PointerMixer () {}

    void Set_pointer_to_first_source(void);
    void Move_pointer(const int value);
    void Display_pointer(void);
    MX_pointer_struct Get_pointer(void);
};