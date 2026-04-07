/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#pragma once

#include <Arduino.h>
#include "SharedElements.h"
#include "config.h"


extern uint8_t MX_source;
extern uint8_t MX_routing_source[];
extern bool MX_mute[];

// pointer
enum MX_field_name
{
    field_MX_Source,
    field_MX_Elements
};

enum MX_Element_name
{
    value_MX_Mute,
    value_MX_Gain,
    value_MX_Pan,
    value_MX_Lineout,
    value_MX_Monitor
};

struct MX_field_description_struct
{
MX_field_name field_name;
int source;
int element;
};