/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#pragma once

// Selectable fields on the CONTROL CHANGE ASSIGNMENT page.
enum CC_field_name
{
    field_CC_Return,
    field_CC_SoundGain,
    field_CC_LowpassFilter
};

struct CC_field_description_struct
{
    CC_field_name field_name;
    int instrument; // Used only for field_CC_SoundGain.
};
