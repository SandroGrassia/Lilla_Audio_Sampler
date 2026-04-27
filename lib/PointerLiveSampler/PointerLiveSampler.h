/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#pragma once

#include <Arduino.h>
#include "config.h"
#include "SharedLiveSampler.h"
#include "DisplayPrimitives.h"
#include "GlobalDisplayLiveSampler.h"

/*

enum LS_element_name
{
    value_LS_Play_mode,
    value_LS_Feedback,
};

*/

// Manages the navigation pointer for the Live Sampler page. The pointer can be in one of two fields: the menu row (field_LS_Menu)
// or the value section (field_LS_Value). Movement between fields and between elements within a field is handled by Move_pointer().
// All visual feedback is delegated to DisplayLiveSampler via LS_show_pointer_frame().

class PointerLiveSampler
{
private:
    LS_pointer_struct pointer;     // current pointer position
    LS_pointer_struct pointer_old; // previous pointer position, used to erase the old frame

public:
    PointerLiveSampler() {}

    // Moves the pointer to the first element of the menu row and renders it.
    void Set_pointer_to_first_menu_element(void);

    // Moves the pointer by +1 or -1 steps along the navigation sequence. Crossing the boundary between field_LS_Menu and field_LS_Value is
    // handled automatically. Erases the previous frame and draws the new one.
    void Move_pointer(const int value);

    // Resets the pointer to the first menu element when the current field is field_LS_Menu, without touching the display.
    void Restore_pointer(void);

    // Shows or hides the pointer frame at the current position.
    void Show_pointer(const bool show);

    // Returns a copy of the current pointer state.
    LS_pointer_struct Get_pointer(void);
};