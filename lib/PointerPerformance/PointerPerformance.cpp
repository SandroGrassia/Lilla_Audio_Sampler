/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "PointerPerformance.h"

void PointerPerformance::Set_pointer_to_last_instrument(const int instrument_id)
{
    pointer = {field_P_Instrument, 0, P_line_of_instrument[instrument_id], instrument_id};
    Display_Manager.P_show_pointer_frame(pointer, true); // show the new frame
}

void PointerPerformance::Set_pointer_to_RootKey(const int instrument_id)
{
    Delete_pointer();
    pointer = {field_P_Instrument_inside, value_P_RootKey, P_line_of_instrument[instrument_id], instrument_id};
    Display_pointer();
}

void PointerPerformance::Set_pointer_to_Patch(void)
{
    pointer = {field_P_Patch, 0, 0, 0};
    Display_Manager.P_show_pointer_frame(pointer, true); // show the new frame
}

void PointerPerformance::Set_pointer_to_first_menu_voice(void)
{
    pointer = {field_P_Menu, 0, 0, 0};
    Display_Manager.P_show_pointer_frame(pointer, true); // show the new frame
}

void PointerPerformance::Move_pointer_from_inside_to_Instrument(void)
{
    P_field_description_struct full_pointer_old = pointer;
    pointer.field_name = field_P_Instrument;

    Display_Manager.P_show_pointer_frame(full_pointer_old, false); // delete the old frame
    Display_Manager.P_show_pointer_frame(pointer, true);           // show the new frame
}

void PointerPerformance::Move_pointer_from_Instrument_to_inside(void)
{
    P_field_description_struct full_pointer_old = pointer;
    pointer.field_name = field_P_Instrument_inside;
    pointer.element = 0;

    Display_Manager.P_show_pointer_frame(full_pointer_old, false); // delete the old frame
    Display_Manager.P_show_pointer_frame(pointer, true);           // show the new frame
}

void PointerPerformance::Display_pointer(void)
{
    Display_Manager.P_show_pointer_frame(pointer, true);
}

void PointerPerformance::Delete_pointer(void)
{
    Display_Manager.P_show_pointer_frame(pointer, false);
}

P_field_description_struct PointerPerformance::Get_pointer(void)
{
    return pointer;
}

FLASHMEM
void PointerPerformance::Move_pointer(const int value, const int P_menu_max)
{
    bool changed = false;
    P_field_description_struct full_pointer_old = pointer;

    switch (pointer.field_name)
    {
    case field_P_Menu:
        if (value == +1)
        {
            changed = true;

            if (pointer.element < P_menu_max)
            {
                ++pointer.element;
            }
            else
            {
                pointer = {field_P_Patch, 0, 0, 0};
            }
        }
        else if (value == -1)
        {
            if (pointer.element > 0)
            {
                changed = true;
                --pointer.element;
            }
            else if (Patch[Patch_id].instruments > 0) // redundant, but...
            {
                changed = true;
                pointer = {field_P_Instrument, 0, Patch[Patch_id].instruments - 1, instrument_on_position[Patch[Patch_id].instruments - 1]};
            }
        }
        break;

    case field_P_Patch:
        if (value == +1)
        {
            if (Patch[Patch_id].instruments > 0)
            {
                changed = true;
                pointer = {field_P_Instrument, 0, 0, instrument_on_position[0]};
            }
        }
        else if (value == -1)
        {
            changed = true;
            pointer = P_menu_max < 0 ? P_field_description_struct{field_P_Instrument, 0, Patch[Patch_id].instruments - 1, instrument_on_position[Patch[Patch_id].instruments - 1]} : P_field_description_struct{field_P_Menu, P_menu_max, 0, 0};
        }
        break;

    case field_P_Instrument:
        if (value == +1)
        {
            changed = true;

            if (pointer.instrument_line == (Patch[Patch_id].instruments - 1))
            {
                pointer = {P_menu_max < 0 ? field_P_Patch : field_P_Menu, 0, 0, 0};
            }
            else
            {
                ++pointer.instrument_line;
                pointer.instrument_id = instrument_on_position[pointer.instrument_line];
            }
        }
        else if (value == -1)
        {
            changed = true;

            if (pointer.instrument_line == 0)
            {
                pointer = {field_P_Patch, 0, 0, 0};
            }
            else
            {
                --pointer.instrument_line;
                pointer.instrument_id = instrument_on_position[pointer.instrument_line];
            }
        }
        break;

    case field_P_Instrument_inside:
        if (value == +1)
        {
            changed = true;

            if (pointer.element < (instrument_inside_elements - 1))
            {
                ++pointer.element;
            }
            else
            {
                pointer.element = 0;
            }
        }
        else if (value == -1)
        {
            changed = true;

            if (pointer.element > 0)
            {
                --pointer.element;
            }
            else
            {
                pointer.element = instrument_inside_elements - 1;
            }
        }
        break;
    }

    if (changed)
    {
        Display_Manager.P_show_pointer_frame(full_pointer_old, false);
        Display_Manager.P_show_pointer_frame(pointer, true);
    }
}

FLASHMEM
void PointerPerformance::Print_pointer_and_field_desciption(void)
{
    Serial.print("PointerPerformance:: field_description[P_pointer].field_name: ");
    Serial.print(pointer.field_name);
    Serial.print(" pointer.element: ");
    Serial.print(pointer.element);
    Serial.print(" pointer.instrument_line: ");
    Serial.print(pointer.instrument_line);
    Serial.print(" pointer.instrument_id: ");
    Serial.println(pointer.instrument_id);
}
