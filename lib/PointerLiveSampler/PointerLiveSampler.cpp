/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "PointerLiveSampler.h"

void PointerLiveSampler::Set_pointer_to_first_menu_element(void)
{
    pointer.field_name = field_LS_Menu;
    pointer.menu_element = static_cast<LS_menu_element_name>(element_Menu_LS[0]);
    Display_LiveSampler.LS_show_pointer_frame(pointer, true);
}

FLASHMEM
void PointerLiveSampler::Move_pointer(const int value)
{
    pointer_old = pointer;

    switch (pointer.field_name)
    {
    case field_LS_Menu:
    {
        int menu_position = position_Menu_LS[pointer.menu_element];

        if (value == 1)
        {
            if (menu_position == LS_menu_max)
            {
                pointer.field_name = field_LS_Value;
                pointer.value_element = value_LS_Gain;
            }
            else
            {
                pointer.menu_element = static_cast<LS_menu_element_name>(element_Menu_LS[++menu_position]);
            }
        }
        else if (value == -1)
        {
            if (menu_position > 0)
            {
                pointer.menu_element = static_cast<LS_menu_element_name>(element_Menu_LS[--menu_position]);
            }
            else
            {
                pointer.field_name = field_LS_Value;
                pointer.value_element = static_cast<LS_value_name>(LS_value_names - 1);
            }
        }
    }
    break;

    case field_LS_Value:
    {
        if (value == 1)
        {
            if (pointer.value_element < LS_value_names - 1)
            {
                pointer.value_element = static_cast<LS_value_name>(pointer.value_element + 1);
            }
            else
            {
                pointer.field_name = field_LS_Menu;
                pointer.menu_element = static_cast<LS_menu_element_name>(element_Menu_LS[0]);
            }
        }
        else if (value == -1)
        {
            if (pointer.value_element == value_LS_Gain)
            {
                pointer.field_name = field_LS_Menu;
                pointer.menu_element = static_cast<LS_menu_element_name>(element_Menu_LS[LS_menu_max]);
            }
            else
            {
                pointer.value_element = static_cast<LS_value_name>(pointer.value_element - 1);
            }
        }
    }
    break;
    }

    Display_LiveSampler.LS_show_pointer_frame(pointer_old, false);
    Display_LiveSampler.LS_show_pointer_frame(pointer, true);
}

void PointerLiveSampler::Show_pointer(const bool show)
{
    Display_LiveSampler.LS_show_pointer_frame(pointer, show);
}

LS_pointer_struct PointerLiveSampler::Get_pointer(void)
{
    return pointer;
}
