/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "PointerPerformance.h"

FLASHMEM
void PointerPerformance::Update_pointer_fields_description(const int P_menu_max)
{
    Serial.println("Update_pointer_fields_description(void)");

    for (auto i = 0; i <= P_menu_max; ++i)
    {
        field_description[i].field_name = field_P_Menu;
        field_description[i].element = i;

        Serial.print(i);
        Serial.print("  field_P_Menu: ");
        Serial.println(field_description[i].element);
    }
    field_description[P_menu_max + 1].field_name = field_P_Patch;
    field_description[P_menu_max + 1].element = 0;

    Serial.print(P_menu_max + 1);
    Serial.print("  field_P_Patch: ");
    Serial.println(field_description[P_menu_max + 1].element);

    auto place = P_menu_max + 2;

    for (auto i = 0; i < (9 * (Patch[Patch_id].instruments)); ++i)
    {
        if ((i % 9) == 0)
        {
            field_description[place + i].field_name = field_P_Instrument;
            field_description[place + i].instrument_line = i / 9;
            field_description[place + i].instrument_id = instrument_on_position[i / 9];

            Serial.print(place + i);
            Serial.print("  field_P_Instrument - instrument_line: ");
            Serial.print(field_description[place + i].instrument_line);
            Serial.print("  instrument_id: ");
            Serial.println(field_description[place + i].instrument_id);
        }
        else
        {
            field_description[place + i].field_name = field_P_Instrument_inside;
            field_description[place + i].element = ((i % 9) - 1);
            field_description[place + i].instrument_line = i / 9;
            field_description[place + i].instrument_id = (instrument_on_position[i / 9]);

            Serial.print(place + i);
            Serial.print("  field_P_Instrument_inside - instrument_line: ");
            Serial.print(field_description[place + i].instrument_line);
            Serial.print("  instrument_id: ");
            Serial.print(field_description[place + i].instrument_id);
            Serial.print("  element: ");
            Serial.println(field_description[place + i].element);
        }
    }
}

FLASHMEM
void PointerPerformance::Restore_pointer_value(const P_field_description_struct field_description_in, const int P_menu_max)
{
    bool done = false;

    if (field_description_in.field_name == field_P_Menu)
    {
        pointer = 0;
        done = true;
    }
    else
    {
        for (auto i = P_menu_max + 1; i < field_description_max_elements; ++i)
        {
            if ((field_description[i].field_name == field_description_in.field_name) &&
                (field_description[i].element == field_description_in.element) &&
                (field_description[i].instrument_id == field_description_in.instrument_id))
            {
                pointer = i;
                done = true;
            }
        }
    }

    if (done)
    {
        Serial.print("void Restore_pointer_value(const P_field_description_struct field_description_in, const int P_menu_max) - pointer: ");
        Serial.println(pointer);
    }
    else
    {
        Serial.print("void Restore_pointer_value(const P_field_description_struct field_description, const int P_menu_max) - ************************ BIG MISTAKE!!! POINTER ERROR************************** ");
    }
}

void PointerPerformance::Set_pointer_to_last_instrument(const int instrument_id, const int P_menu_max)
{
    pointer = P_menu_max + 2 + P_line_of_instrument[instrument_id] * (instrument_inside_elements + 1);
    Display_Manager.P_show_pointer_frame(field_description[pointer], true); // show the new frame

    Print_pointer_and_field_desciption(pointer);
}

void PointerPerformance::Set_pointer_to_Patch(int P_menu_max)
{
    pointer = P_menu_max + 1;
    Display_Manager.P_show_pointer_frame(field_description[pointer], true); // show the new frame

    Print_pointer_and_field_desciption(pointer);
}

void PointerPerformance::Set_pointer_to_first_menu_voice(void)
{
    pointer = 0;
    Display_Manager.P_show_pointer_frame(field_description[pointer], true); // show the new frame

    Print_pointer_and_field_desciption(pointer);
}

void PointerPerformance::Move_pointer_from_inside_to_Instrument(void)
{
    pointer_old = pointer;
    pointer -= field_description[pointer].element + 1;

    Display_Manager.P_show_pointer_frame(field_description[pointer_old], false); // delete the old frame
    Display_Manager.P_show_pointer_frame(field_description[pointer], true);      // show the new frame

    Print_pointer_and_field_desciption(pointer);
}

void PointerPerformance::Move_pointer_from_Instrument_to_inside(void)
{
    pointer_old = pointer;
    ++pointer;

    Display_Manager.P_show_pointer_frame(field_description[pointer_old], false); // delete the old frame
    Display_Manager.P_show_pointer_frame(field_description[pointer], true);      // show the new frame

    Print_pointer_and_field_desciption(pointer);
}

void PointerPerformance::Display_pointer(void)
{
    Display_Manager.P_show_pointer_frame(field_description[pointer], true);
}

void PointerPerformance::Delete_pointer(void)
{
    Display_Manager.P_show_pointer_frame(field_description[pointer], false);
}

P_field_description_struct PointerPerformance::Get_field_description(void)
{
    return field_description[pointer];
}

FLASHMEM
void PointerPerformance::Move_pointer(const int value, const int P_menu_max)
{
    bool changed = false;
    pointer_old = pointer;

    switch (field_description[pointer].field_name)
    {
    case field_P_Menu:
        if (value == +1)
        {
            changed = true;
            ++pointer;
        }
        else if ((value == -1) && (pointer > 0))
        {
            changed = true;
            --pointer;
        }
        break;

    case field_P_Patch:
        if (value == +1)
        {
            changed = true;
            ++pointer;
        }
        else if (value == -1)
        {
            changed = true;
            --pointer;
        }
        break;

    case field_P_Instrument:
        if (value == +1)
        {
            if (field_description[pointer].instrument_line < (Patch[Patch_id].instruments - 1))
            {
                changed = true;
                pointer += instrument_inside_elements + 1;
            }
        }
        else if (value == -1)
        {
            if (field_description[pointer].instrument_line == 0)
            {
                changed = true;
                --pointer;
            }
            else
            {
                changed = true;
                pointer -= instrument_inside_elements + 1;
            }
        }
        break;

    case field_P_Instrument_inside:

        if (value == +1)
        {
            if (field_description[pointer].element < (instrument_inside_elements - 1))
            {
                changed = true;
                ++pointer;
            }
            else
            {
                changed = true;
                pointer -= instrument_inside_elements - 1;
            }
        }
        else if (value == -1)
        {
            if (field_description[pointer].element > 0)
            {
                changed = true;
                --pointer;
            }
            else
            {
                changed = true;
                pointer += instrument_inside_elements - 1;
            }
        }
        break;
    }

    if (changed)
    {
        Display_Manager.P_show_pointer_frame(field_description[pointer_old], false); // delete the old frame
        Display_Manager.P_show_pointer_frame(field_description[pointer], true);      // show the new frame

        Print_pointer_and_field_desciption(pointer);
    }
}

FLASHMEM
void PointerPerformance::Print_pointer_and_field_desciption(const int pointer)
{
    Serial.print("PointerPerformance::Print_pointer_and_field_desciption ---> pointer: ");
    Serial.println(pointer);
    Serial.print("field_description[P_pointer].field_name: ");
    Serial.print(field_description[pointer].field_name);
    Serial.print(" field_description[P_pointer].element: ");
    Serial.print(field_description[pointer].element);
    Serial.print(" field_description[P_pointer].instrument_line: ");
    Serial.print(field_description[pointer].instrument_line);
    Serial.print(" field_description[P_pointer].instrument_id: ");
    Serial.println(field_description[pointer].instrument_id);
}