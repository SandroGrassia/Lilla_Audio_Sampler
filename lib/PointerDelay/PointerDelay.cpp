/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "PointerDelay.h"

void PointerDelay::Set_pointer_to_Feedback(void)
{
    Display_Delay.DELAY_show_pointer_frame(value_DELAY_Feedback, true);
}

void PointerDelay::Move_pointer(const int value)
{
    pointer_old = pointer;

    if (value == 1)
    {
        if (pointer == pointer_max)
        {
            pointer = value_DELAY_Feedback;
        }
        else
        {
            pointer = static_cast<DELAY_element_name>(pointer + 1);
        }
    }

    else if (value == -1)
    {
        if (pointer == value_DELAY_Feedback)
        {
            pointer = value_DELAY_Modulation_phase_LR;
        }
        else
        {
            pointer = static_cast<DELAY_element_name>(pointer - 1);
        }
    }

    Display_Delay.DELAY_show_pointer_frame(pointer_old, false);
    Display_Delay.DELAY_show_pointer_frame(pointer, true);
}

void PointerDelay::Display_pointer(void)
{
    Display_Delay.DELAY_show_pointer_frame(pointer, true);
}

DELAY_element_name PointerDelay::Get_element_name(void)
{
    return pointer;
}