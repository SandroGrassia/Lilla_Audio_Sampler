/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "PointerMidiLoop.h"

void PointerMidiLoop::Set_pointerMenu_to_first_menu_element(void)
{
    if (!Menu_Loop[0] && !Menu_Loop[1] && !Menu_Loop[2])
    {
        // Loop_menu_position_0 = -1;
        return;
    }

    Frame_by_col_row(X_position_Menu_Loop[0], 1, dimension_voice_Menu_Loop[element_Menu_Loop[0]], true);
    choice_loop_menu = element_Menu_Loop[0];
}

LOOP_menu_element_name PointerMidiLoop::Get_pointerMenu(void)
{

}

void PointerMidiLoop::Move_pointerMenu(const int value)
{
    bool change = false;
    pointerMenu_old = pointerMenu;

    int menu_position = position_Menu_Loop[pointerMenu];
    
    if (value == 1)
    {
        if(menu_position < Loop_menu_max)
        {
            pointerMenu = static_cast<LOOP_menu_element_name>(element_Menu_Loop[++menu_position]);
			change = true;
        }
    }
    else if (value == -1)
    {
        if (menu_position > 0)
        {
            pointerMenu = static_cast<LOOP_menu_element_name>(element_Menu_Loop[--menu_position]);
			change = true;
        }
    }

    if(change)
    {
        Display_MidiLoop.Loop_show_pointerMenu(pointerMenu_old, false);
        Display_MidiLoop.Loop_show_pointerMenu(pointerMenu, false);
    }
}

void PointerMidiLoop::Set_pointerMain_to_loop(void)
{
}

void PointerMidiLoop::Switch_pointerMain(void)
{
}

LOOP_main_element_name PointerMidiLoop::Get_pointerMain(void)
{
}

void PointerMidiLoop::Move_pointerTrack(const int track, const int value)
{
}

LOOP_track_value_name PointerMidiLoop::Get_pointerTrack(const int pointer)
{
}

void PointerMidiLoop::Set_pointerTrack_to_level(const int track)
{
}

void PointerMidiLoop::Show_pointerTrack(const int track, const bool show)
{
}
