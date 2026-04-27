/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#pragma once

#include <Arduino.h>
#include "DisplayPrimitives.h"
#include "GlobalDisplayMidiLoop.h"

/*
static constexpr int LOOP_menu_values = 4;
enum LOOP_menu_element_name
{
    value_LOOP_New,
    value_LOOP_Save,
    value_LOOP_SaveAsNew,
    value_LOOP_Delete
};

static constexpr int LOOP_track_values = 3;
enum LOOP_track_value_name
{
    value_LOOP_slide,
    value_LOOP_pitch,
    value_LOOP_level
};
*/

class PointerMidiLoop
{
    private:
    LOOP_menu_element_name pointerMenu;
    LOOP_track_value_name pointerTrack[TRACKS];

    public:
    PointerMidiLoop() {}

    void Set_pointerMenu_to_first_menu_element(void);
    void Show_pointerMenu(const bool show);
    void Move_pointerMenu(const int value);
    LOOP_menu_element_name Get_pointerMenu(void);

    void Move_pointerTrack(const int track, const int value);
    void Set_pointerTrack_to_level(const int track);
    void Show_pointerTrack(const int track, const bool show);
    LOOP_track_value_name Get_pointerTrack (const int track);
};