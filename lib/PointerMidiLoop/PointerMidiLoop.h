/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#pragma once

#include <Arduino.h>
#include "DisplayPrimitives.h"
#include "GlobalDisplayMidiLoop.h"
#include "SharedLoop.h"

/*
// Pointer
enum LOOP_field_name
{
    field_LOOP_Menu,
    field_LOOP_TrackValues
};

enum LOOP_menu_element_name : int
{
    value_LOOP_Menu_none = -1,
    value_LOOP_New = 0,
    value_LOOP_Save = 1,
    value_LOOP_SaveAsNew = 2,
    value_LOOP_Delete = 3
};

static constexpr int LOOP_track_values = 3;
enum LOOP_track_value_name : int
{
    value_LOOP_Track_none = -1,
    value_LOOP_shift = 0,
    value_LOOP_pitch = 1,
    value_LOOP_level = 2
};

struct LOOP_field_description_struct
{
LOOP_field_name field_name;
LOOP_menu_element_name menu_element;
LOOP_track_value_name track_value_element;
};
*/

class PointerMidiLoop
{
    private:
    LOOP_field_description_struct pointer;

    public:
    PointerMidiLoop() {}

    void Set_pointer_to_first_menu_element(void);
    void Set_pointer_to_level(void);
    LOOP_field_description_struct Get_pointer(void);
    void Show_pointerTrack(const int track, const bool show);
    void Show_pointer(const bool show);
    void Move_pointer(const int value);
};