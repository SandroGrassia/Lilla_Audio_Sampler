/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#pragma once

#include <Arduino.h>
#include "config.h"
#include "SharedSound.h"
#include "DisplayPrimitives.h"
#include "GlobalDisplaySound.h"

/*
enum S_field_name
{
    field_S_Menu,
    field_S_Value
};
static constexpr int S_menu_elements = 3;
static constexpr int S_value_names = 11;
enum S_value_name
{
    value_S_File,
    value_S_Midi,
    value_S_Pitch,
    value_S_Gain,
    value_S_Pan,
    value_S_Attack,
    value_S_Decay,
    value_S_Sustain,
    value_S_Release,
    value_S_PlayMode,
    value_S_Noclick
};

struct S_field_description_struct
{
    S_field_name field_name;
    int menu_element;           // in case of field_S_Menu
    S_value_name value_element; // in case of field_S_Value
};
*/


class PointerSound
{
private:
    static constexpr int field_description_max_elements = S_value_names + S_menu_elements; // field = where the pointer stands
    int pointer;
    int pointer_old;
    int pointer_max;
    S_field_description_struct field_description[field_description_max_elements];
    void Print_pointer_description(void);

public:
    PointerSound() {}

    void Update_field_description(const int S_menu_max);
    void Restore_pointer_value(const S_field_description_struct field_description_in, const int S_menu_max); // must be called when S_menu_elements changes
    void Set_pointer_to_file(const int S_menu_max);
    bool Move_pointer(const int value, const int S_menu_max);
    void Display_pointer(void);
    S_field_description_struct Get_field_description(void);
};