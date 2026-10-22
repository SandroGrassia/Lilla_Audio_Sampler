/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#include "PointerControlChange.h"
#include "config.h"

FLASHMEM void PointerControlChange::Set_pointer_to_return(void)
{
    Display.Show_pointer_frame(pointer, false);
    pointer = {field_CC_Return, 0};
    Display_pointer();
}

FLASHMEM bool PointerControlChange::Move_pointer(int value)
{
    CC_field_description_struct next = pointer;
    switch (pointer.field_name)
    {
    case field_CC_Return:
        if (value < 0)
        {
            return false;
        }
        next = {field_CC_SoundGain, 0};
        break;
    case field_CC_SoundGain:
        if (value < 0 && pointer.instrument == 0)
        {
            next = {field_CC_Return, 0};
        }
        else if (value > 0 && pointer.instrument == INSTRUMENTS - 1)
        {
            next = {field_CC_LowpassFilter, 0};
        }
        else
        {
            next.instrument += value;
        }
        break;
    case field_CC_LowpassFilter:
        if (value > 0)
        {
            return false;
        }
        next = {field_CC_SoundGain, INSTRUMENTS - 1};
        break;
    }

    Display.Show_pointer_frame(pointer, false);
    pointer = next;
    Display_pointer();
    return true;
}

FLASHMEM void PointerControlChange::Display_pointer(void)
{
    Display.Show_pointer_frame(pointer, true);
}

CC_field_description_struct PointerControlChange::Get_pointer(void) const
{
    return pointer;
}
