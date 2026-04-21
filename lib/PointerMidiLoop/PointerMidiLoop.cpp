/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "PointerMidiLoop.h"

void PointerMidiLoop::Set_pointerMenu_to_first_menu_element(void)
{
    pointerMenu = static_cast<LOOP_menu_element_name>(element_Menu_Loop[0]);
    Display_MidiLoop.Loop_show_pointerMenu(pointerMenu, true);
}

void PointerMidiLoop::Move_pointerMenu(const int value)
{
    bool change = false;
    pointerMenu_old = pointerMenu;

    int position = position_Menu_Loop[pointerMenu];
    
    if (value == 1)
    {
        if(position < Loop_menu_max)
        {
            pointerMenu = static_cast<LOOP_menu_element_name>(element_Menu_Loop[++position]);
			change = true;
        }
    }
    else if (value == -1)
    {
        if (position > 0)
        {
            pointerMenu = static_cast<LOOP_menu_element_name>(element_Menu_Loop[--position]);
			change = true;
        }
    }

    if(change)
    {
        Display_MidiLoop.Loop_show_pointerMenu(pointerMenu_old, false);
        Display_MidiLoop.Loop_show_pointerMenu(pointerMenu, false);
    }
}

LOOP_menu_element_name PointerMidiLoop::Get_pointerMenu(void)
{
    return pointerMenu;
}



void PointerMidiLoop::Set_pointerMain_to_loop(void)
{
    pointerMain = value_LOOP_Loop;
    Display_MidiLoop.Loop_show_pointerMain(pointerMain, true);
}

void PointerMidiLoop::Switch_pointerMain(void)
{
}

LOOP_main_value_name PointerMidiLoop::Get_pointerMain(void)
{
    return pointerMain;
}

void PointerMidiLoop::Move_pointerTrack(const int track, const int value)
{
}

LOOP_track_value_name PointerMidiLoop::Get_pointerTrack(const int track)
{
    return pointerTrack[track];
}

void PointerMidiLoop::Set_pointerTrack_to_level(const int track)
{
}

void PointerMidiLoop::Show_pointerTrack(const int track, const bool show)
{
}
