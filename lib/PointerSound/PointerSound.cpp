/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "PointerSound.h"

FLASHMEM
void PointerSound::Set_pointer_to_file(const int S_menu_max)
{
    pointer = {field_S_Value, 0, value_S_File};
    if (Playback_active)
    {
        pointer.value_element = value_S_Gain;
    }
    Display_Sound.Show_pointer_frame(pointer, true);

    // Serial.println("PointerSound::Set_pointer_to_file(int S_menu_max) - pointer: ");
    // Print_pointer_description();
}

FLASHMEM
void PointerSound::Set_pointer_to_first_menu_element(void)
{
    pointer = {field_S_Menu, 0, value_S_File};
    Display_Sound.Show_pointer_frame(pointer, true);

    // Serial.println("PointerSound::Set_pointer_to_first_menu_element(void) - pointer: ");
    // Print_pointer_description();
}

FLASHMEM
void PointerSound::Move_pointer(const int value, const int S_menu_max)
{
    Display_Sound.Show_pointer_frame(pointer, false);

    switch (pointer.field_name)
    {
    case field_S_Menu:

        if (value == 1)
        {
            if (pointer.menu_element < S_menu_max)
            {
                ++pointer.menu_element;
            }
            else
            {
                pointer = {field_S_Value, 0, value_S_File};
            }
        }

        else if (value == -1)
        {
            if (pointer.menu_element > 0)
            {
                --pointer.menu_element;
            }
            else
            {
                pointer = {field_S_Value, 0, value_S_Noclick};
            }
        }

        break;

    case field_S_Value:

        if (value == 1)
        {
            if (pointer.value_element != value_S_Noclick)
            {
                pointer.value_element = static_cast<S_value_name>(pointer.value_element + 1);
            }
            else
            {
                pointer = {field_S_Menu, 0, value_S_File};
            }
        }
        else if (value == -1)
        {
            if (pointer.value_element != (Playback_active ? value_S_Gain : value_S_File))
            {
                pointer.value_element = static_cast<S_value_name>(pointer.value_element - 1);
            }
            else
            {
                pointer = {field_S_Menu, S_menu_max, value_S_File};
            }
        }
        break;
    }

    if (Playback_active && pointer.field_name == field_S_Value && pointer.value_element == value_S_File)
    {
        pointer.value_element = value_S_Gain;
    }
    Display_Sound.Show_pointer_frame(pointer, true);
    
    //Print_pointer_description();
}

void PointerSound::Display_pointer(void)
{
    Display_Sound.Show_pointer_frame(pointer, true);
}

S_field_description_struct PointerSound::Get_pointer(void)
{
    return pointer;
}

FLASHMEM
void PointerSound::Print_pointer_description(void)
{
    Serial.println("PointerSound::Print_pointer_description(const int pointer) - pointer: ");
    Serial.print(" field_name: ");
    Serial.print(pointer.field_name);
    Serial.print(" menu_element: ");
    Serial.print(pointer.menu_element);
    Serial.print(" value_element: ");
    Serial.println(pointer.value_element);
}
