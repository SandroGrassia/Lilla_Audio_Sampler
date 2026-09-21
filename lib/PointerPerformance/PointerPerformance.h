/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#pragma once

#include <Arduino.h>
#include "config.h"
#include "SharedPerformance.h"
#include "DisplayPrimitives.h"
#include "GlobalDisplayManager.h"

/*

// menu
enum P_menu_elements_name
{
    value_P_Exit,
    value_P_Save,
    value_P_Clone,
    value_P_SaveAsNew,
    value_P_DropPatch
};

// pointer
enum P_field_name
{
    field_P_Menu,
    field_P_Patch,
    field_P_Instrument,
    field_P_Instrument_inside
};

struct P_field_description_struct
{
P_field_name field_name;
int element;
int instrument_line;
int instrument_id;
};

*/

class PointerPerformance
{
private:
    static constexpr int instrument_inside_elements = 8;
    P_field_description_struct pointer;
    void Print_pointer_and_field_desciption(void);

public:
    PointerPerformance() {}
    
    void Move_pointer(const int value, const int P_menu_max);
    P_field_description_struct Get_pointer(void);
    void Set_pointer_to_Patch(void);
    void Set_pointer_to_RootKey(const int instrument_id);
    void Move_pointer_from_inside_to_Instrument(void);
    void Move_pointer_from_Instrument_to_inside(void);
    void Set_pointer_to_last_instrument(const int instrument_id);
    void Set_pointer_to_first_menu_voice(void);
    void Display_pointer(void);
    void Delete_pointer(void);    
};
