/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "PointerMixer.h"

void PointerMixer::Set_pointer_to_first_source(void)
{
    pointer = {field_MX_Source, 0, 0};
}

void PointerMixer::Move_pointer(const int value)
{
    pointer_old = pointer;
    bool changed = false;

    switch (pointer.field_name)
    {
    case field_MX_Source:
    {
        if (value == 1)
        {
            if (pointer.source < LINE_IN_source)
            {
                if (pointer.source < (Patch[Patch_id].instruments - 1))
                {
                    ++pointer.source;
                    changed = true;
                }
                else
                {

                    pointer.source = LINE_IN_source;
                    changed = true;
                }
            }
            else
            {
                pointer.source = 0;
                changed = true;
            }
        }
        else if (value == -1)
        {
            if (pointer.source > 0)
            {
                if (pointer.source == LINE_IN_source)
                {
                    pointer.source = Patch[Patch_id].instruments - 1;
                    changed = true;
                }
                else
                {
                    --pointer.source;
                    changed = true;
                }
            }
            else
            {
                pointer.source = LINE_IN_source;
                changed = true;
            }
        }
    }
    break;

    case field_MX_Elements:
    {
        if (value == 1)
        {
            if (pointer.element < (elements - 1))
            {
                ++pointer.element;
                changed = true;
            }
            else
            {
                pointer.element = 0;
                changed = true;
            }
        }

        else if (value == -1)
        {
            if (pointer.element > 0)
            {
                --pointer.element;
                changed = true;
            }
            else
            {
                pointer.element = (elements - 1);
                changed = true;
            }
        }
    }
    break;

    default:
        break;
    }

    if (changed)
    {
        // Display_Mixer.MX_show_pointer_frame(pointer_old, false);
        // Display_Mixer.MX_show_pointer_frame(pointer, true);
    }
}

void PointerMixer::Display_pointer(void)
{
    // Display_Mixer.MX_show_pointer_frame(pointer, true);
}
