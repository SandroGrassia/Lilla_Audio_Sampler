/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#pragma once

#include <Arduino.h>
#include "SharedElements.h"

// menu
static constexpr int P_menu_elements = 5;
enum P_menu_elements_name
{
    value_P_Exit,
    value_P_Save,
    value_P_Clone,
    value_P_SaveAsNew,
    value_P_DropPatch
};
static constexpr char P_menu_char[P_menu_elements][12] = {{"EXIT"}, {"SAVE"}, {"CLONE"}, {"SAVE_AS_NEW"}, {"DROP"}};
static constexpr uint8_t P_dimension_voice_menu[P_menu_elements] = {4, 4, 5, 11, 4};
extern int P_column_menu_element[P_menu_elements]; // argument is position
extern int P_row_menu_element[P_menu_elements];    // argument is position
extern P_menu_elements_name P_element_menu[P_menu_elements];    // argument is position
extern uint8_t P_position_Menu[P_menu_elements];   // argument is element

// pointer
enum P_field_name
{
    field_P_Menu,
    field_P_Patch,
    field_P_Instrument,
    field_P_Instrument_inside
};

enum P_instrument_inside_name
{
    value_P_Lock,
    value_P_Precedence,
    value_P_Midi,
    value_P_RootKey,
    value_P_FromKey,
    value_P_ToKey,
    value_P_Pan,
    value_P_Gain
};

struct P_field_description_struct
{
    P_field_name field_name;
    int element;
    int instrument_line;
    int instrument_id;
};

extern int8_t instrument_on_position[INSTRUMENTS];
void P_Update_line_of_all_instruments(void);