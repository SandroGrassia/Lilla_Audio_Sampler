/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#pragma once

#include <Arduino.h>

// menu
static constexpr int S_menu_elements = 3;
enum S_menu_elements_name
{
    value_S_Return,
    value_S_Clone,
    value_S_Drop
};
static constexpr int S_row_menu = 1;
static constexpr char S_menu_char[S_menu_elements][7] = {{"RETURN"}, {"CLONE"}, {"DROP"}};
static constexpr uint8_t S_dimension_voice_menu[S_menu_elements] = {6, 5, 4};
extern uint8_t S_menu_choice;
extern bool S_Menu[S_menu_elements];
extern uint8_t S_column_menu_element[S_menu_elements];    // argument is position
extern S_menu_elements_name S_element_menu[S_menu_elements]; // argument is position
extern uint8_t S_position_menu[S_menu_elements];          // argument is element

// Pointer
enum S_field_name
{
    field_S_Menu,
    field_S_Value
};

static constexpr int S_value_names = 11;
enum S_value_name
{
    value_S_File,
    value_S_Gain,
    value_S_Pitch,
    value_S_Pan,
    value_S_Midi,
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
