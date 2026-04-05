/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "PointerVCF.h"

void PointerVCF::Set_pointer_to_FilterType(void)
{
    pointer = value_VCF_FilterType;
    Display_VCF.VCF_show_pointer_frame(pointer, true);

    Serial.println("PointerVCF::Set_pointer_to_FilterType(void)");
    Print_pointer_description();
}

FLASHMEM
bool PointerVCF::Move_pointer(const int value)
{
    bool changed = false;
    pointer_old = pointer;

    if (value == 1 && pointer < pointer_max)
    {
        ++pointer;
        changed = true;
    }
    else if (value == -1 && pointer > 0)
    {
        --pointer;
        changed = true;
    }

    if (changed)
    {
        Display_VCF.VCF_show_pointer_frame(pointer_old, false);
        Display_VCF.VCF_show_pointer_frame(pointer, true);
        Print_pointer_description();
    }

    return changed;
}


void PointerVCF::Display_pointer(void)
{
    Display_VCF.VCF_show_pointer_frame(pointer, true);
}


void PointerVCF::Print_pointer_description(void)
{
    Serial.print("PointerVCF::Print_pointer_description(void) - pointer: ");
    Serial.println(pointer);
}

VCF_value_name PointerVCF::Get_VCF_value_name(void)
{
    return static_cast<VCF_value_name> (pointer);
}