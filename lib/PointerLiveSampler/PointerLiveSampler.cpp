/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "PointerLiveSampler.h"

void PointerLiveSampler::Set_pointer_to_play_mode(void)
{
    pointer = value_LS_Play_mode;
    Display_pointer();
}

void PointerLiveSampler::Move_pointer(const int value)
{
    pointer_old = pointer;

    if (value == 1)
    {
        if (pointer == pointer_max)
        {
            pointer = value_LS_Play_mode;
        }
        else
        {
            pointer = static_cast<LS_element_name>(pointer + 1);
        }
    }

    else if (value == -1)
    {
        if (pointer == value_LS_Play_mode)
        {
            pointer = static_cast<LS_element_name>(pointer_max);
        }
        else
        {
            pointer = static_cast<LS_element_name>(pointer - 1);
        }
    }

    // Display_LiveSampler.LS_show_pointer_frame(pointer_old, false);
    // Display_LiveSampler.LS_show_pointer_frame(pointer, true);
}

void PointerLiveSampler::Display_pointer(void)
{
    // Display_LiveSampler.LS_show_pointer_frame(pointer, true);
}

LS_element_name PointerLiveSampler::Get_element_name(void)
{
    return pointer;
}