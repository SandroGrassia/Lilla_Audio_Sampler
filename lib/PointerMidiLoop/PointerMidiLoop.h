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
static constexpr int LOOP_track_values = 3;
enum LOOP_track_value_name
{
    value_LOOP_slide,
    value_LOOP_trasnport,
    value_LOOP_level
};
*/


class PointerMidiLoop
{
    private:
    int pointer_menu;
    int pointer_menu_old;
    int pointer_loop_patch;
    int pointer_loop_patch_old;
    LOOP_track_value_name pointer_track[TRACKS];
    LOOP_track_value_name pointer_track_old[TRACKS];


    public:
    PointerMidiLoop() {}

    void Set_pointer_menu_to_first_menu_element(const int pointer);
    int Get_pointer_menu(const int pointer);
    void Move_pointer_menu(const int value);

    void Set_pointer_loop_patch_to_loop(void);
    void Switch_pointer_loop_patch(void);
    int Get_pointer_loop_patch(void);

    void Move_pointer_track(const int pointer, const int value);
    LOOP_track_value_name Get_pointer_track (const int pointer);
    void Set_pointer_track_to_level(const int pointer);
    void Show_pointer_track(const int pointer, const bool show);
};
