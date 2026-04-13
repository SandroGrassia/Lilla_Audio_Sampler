/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "PointerLiveSampler.h"

void PointerLiveSampler::Set_pointer_to_Recording(void)
{
    pointer.field_name = field_LS_Menu;
    pointer.menu_element = value_LS_Recording;
    Display_pointer();
}

FLASHMEM
void PointerLiveSampler::Move_pointer(const int value)
{
    pointer_old = pointer;

    switch (pointer.field_name)
    {
    case field_LS_Menu:
    {
        int menu_position =  position_Menu_LS[pointer.menu_element]; 
        
        if (value == 1)
        {
            if (menu_position == LS_menu_max)
            {
                pointer.field_name = field_LS_Value;
                pointer.value_element = value_LS_Play_mode;
            }
            else
            {
                 pointer.menu_element = static_cast<LS_menu_element_name>(element_Menu_LS[++menu_position]);
            }
        }
        else if(value == -1)
        {
            if (menu_position > 0)
            {
                pointer.menu_element = static_cast<LS_menu_element_name>(element_Menu_LS[--menu_position]);       
            }
        }
    }
    break;

    case field_LS_Value:
    {
        if (value == 1 && pointer.value_element == value_LS_Play_mode)
        {
            pointer.value_element = value_LS_Feedback;
        }
        else if(value == -1)
        {
            if (pointer.value_element == value_LS_Feedback)
            {
                pointer.value_element = value_LS_Play_mode;
            }
            else
            {
                pointer.field_name = field_LS_Menu;
                pointer.menu_element = static_cast<LS_menu_element_name>(element_Menu_LS[LS_menu_max]);
            }
        }    
    }
    break;

    default:
        break;
    }

    if (pointer.field_name == field_LS_Menu)
    {
    }

    // Display_LiveSampler.LS_show_pointer_frame(pointer_old, false);
    // Display_LiveSampler.LS_show_pointer_frame(pointer, true);
}

void PointerLiveSampler::Display_pointer(void)
{
    // Display_LiveSampler.LS_show_pointer_frame(pointer, true);
}

LS_pointer_struct PointerLiveSampler::Get_pointer(void)
{
    return pointer;
}