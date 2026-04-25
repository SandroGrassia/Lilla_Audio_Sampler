/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "PointerMidiLoop.h"

void PointerMidiLoop::Set_pointerMenu_to_first_menu_element(void)
{
    pointerMenu = static_cast<LOOP_menu_element_name>(element_Menu_LOOP[0]);
    Display_MidiLoop.Loop_show_pointerMenu(pointerMenu, true);
}

void PointerMidiLoop::Move_pointerMenu(const int value)
{
    bool change = false;
    LOOP_menu_element_name pointerMenu_old = pointerMenu;

    int position = position_Menu_LOOP[pointerMenu];

    if (value == 1)
    {
        if (position < LOOP_menu_max)
        {
            pointerMenu = static_cast<LOOP_menu_element_name>(element_Menu_LOOP[++position]);
            change = true;
        }
    }
    else if (value == -1)
    {
        if (position > 0)
        {
            pointerMenu = static_cast<LOOP_menu_element_name>(element_Menu_LOOP[--position]);
            change = true;
        }
    }

    if (change)
    {
        Display_MidiLoop.Loop_show_pointerMenu(pointerMenu_old, false);
        Display_MidiLoop.Loop_show_pointerMenu(pointerMenu, true);
    }
}

LOOP_menu_element_name PointerMidiLoop::Get_pointerMenu(void)
{
    return pointerMenu;
}

void PointerMidiLoop::Move_pointerTrack(const int track, const int value)
{
    LOOP_track_value_name pointerTrack_old = pointerTrack[track];
    
    if(value == 1)
    {
      if (pointerTrack[track] == value_LOOP_level)
      {
        pointerTrack[track] = value_LOOP_slide;
      }
      else
      {
        pointerTrack[track] = static_cast<LOOP_track_value_name>(pointerTrack[track] + 1);
      }
    }
    else if(value == -1)
    {
      if (pointerTrack[track] == value_LOOP_slide)
      {
        pointerTrack[track] = value_LOOP_level;
      }
      else
      {
        pointerTrack[track] = static_cast<LOOP_track_value_name>(pointerTrack[track] - 1);
      }
    }

    Display_MidiLoop.Loop_show_pointerTrack(track, pointerTrack_old, false);
    Display_MidiLoop.Loop_show_pointerTrack(track, pointerTrack[track], true);
}

LOOP_track_value_name PointerMidiLoop::Get_pointerTrack(const int track)
{
    return pointerTrack[track];
}

void PointerMidiLoop::Set_pointerTrack_to_level(const int track)
{
    pointerTrack[track] = value_LOOP_level;
    Display_MidiLoop.Loop_show_pointerTrack(track, pointerTrack[track], true);
}

void PointerMidiLoop::Show_pointerTrack(const int track, const bool show)
{
    Display_MidiLoop.Loop_show_pointerTrack(track, pointerTrack[track], show);
}

void PointerMidiLoop::Show_pointerMenu(const bool show)
{
    Display_MidiLoop.Loop_show_pointerMenu(pointerMenu, show);
}
