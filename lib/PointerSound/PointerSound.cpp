/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "PointerSound.h"

FLASHMEM
void PointerSound::Update_field_description(const int S_menu_max)
{
    Serial.println("Update_field_description(void)");

    for (auto i = 0; i <= S_menu_max; ++i)
    {
        field_description[i].field_name = field_S_Menu;
        field_description[i].menu_element = i;

        Serial.print(i);
        Serial.print("  field_S_Menu: ");
        Serial.println(field_description[i].menu_element);
    }

    auto place = S_menu_max + 1;

    for (auto i = 0; i < S_value_names; ++i)
    {
        field_description[place + i].field_name = field_S_Value;
        field_description[place + i].value_element = static_cast<S_value_name>(i);

        Serial.print(place + i);
        Serial.print("  field_S_value - value_element: ");
        Serial.println(field_description[place + i].value_element);
    }

    pointer_max = S_menu_max + 1 + S_value_names;
}

FLASHMEM
void PointerSound::Print_pointer_description(void)
{
    Serial.print("PointerSound::Print_pointer_description(const int pointer) - pointer: ");
    Serial.print(pointer);
    Serial.print(" field_name: ");
    Serial.print(field_description[pointer].field_name);
    Serial.print(" menu_element: ");
    Serial.print(field_description[pointer].menu_element);
    Serial.print(" value_element: ");
    Serial.println(field_description[pointer].value_element);
}

FLASHMEM
void PointerSound::Restore_pointer_value(const S_field_description_struct field_description_in, const int S_menu_max)
{
    bool done = false;

    if (field_description_in.field_name == field_S_Menu)
    {
        pointer = 0;
        done = true;
    }

    else
    {
        for (auto i = S_menu_max + 1; i < field_description_max_elements; ++i)
        {
            if ((field_description[i].field_name == field_description_in.field_name) &&
                (field_description[i].value_element == field_description_in.value_element))

            {
                pointer = i;
                done = true;
            }
        }
    }

    if (done)
    {
        Serial.println("void Restore_pointer_value(const S_field_description_struct field_description_in, const int S_menu_max");
        Print_pointer_description();
    }
    else
    {
        Serial.print("void Restore_pointer_value(const S_field_description_struct field_description_in, const int S_menu_max) - ************************ BIG MISTAKE!!! POINTER ERROR************************** ");
    }
}

FLASHMEM
void PointerSound::Set_pointer_to_file(const int S_menu_max)
{
    pointer = S_menu_max + 1;
    Display_Sound.S_show_pointer_frame(field_description[pointer], true);

    Serial.println("PointerSound::Set_pointer_to_file(int S_menu_max) - pointer: ");
    Print_pointer_description();
}

void PointerSound::Set_pointer_to_first_element(void)
{
    pointer = 0;
    Display_Sound.S_show_pointer_frame(field_description[pointer], true);

    Serial.println("PointerSound::Set_pointer_to_first_element(void) - pointer: ");
    Print_pointer_description();
}

FLASHMEM
bool PointerSound::Move_pointer(const int value, const int S_menu_max)
{
    bool changed = false;
    pointer_old = pointer;

    switch (field_description[pointer].field_name)
    {
    case field_S_Menu:

        if (value == 1)
        {
            changed = true;
            ++pointer;
        }

        else if (value == -1)
        {
            if (pointer > 0)
            {
                changed = true;
                --pointer;
            }
            else
            {
                changed = true;
                pointer = pointer_max;
            }
        }

        break;

    case field_S_Value:

        if (value == 1)
        {
            if (field_description[pointer].value_element != value_S_Noclick)
            {
                changed = true;
                ++pointer;
            }
            else
            {
                changed = true;
                pointer = 0;
            }
        }
        else if (value == -1)
        {
            changed = true;
            --pointer;
        }
        break;
    }

    if (changed)
    {
        Display_Sound.S_show_pointer_frame(field_description[pointer_old], false);
        Display_Sound.S_show_pointer_frame(field_description[pointer], true);

        Print_pointer_description();
    }
    return changed;
}

FLASHMEM
void PointerSound::Display_pointer(void)
{
    Display_Sound.S_show_pointer_frame(field_description[pointer], true);
}

FLASHMEM
S_field_description_struct PointerSound::Get_field_description(void)
{
    return field_description[pointer];
}