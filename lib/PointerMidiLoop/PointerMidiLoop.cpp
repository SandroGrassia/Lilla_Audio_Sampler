/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "PointerMidiLoop.h"

void PointerMidiLoop::Set_pointer_to_first_menu_element(void)
{
    pointer.field_name = field_LOOP_Menu;
    pointer.menu_element = LOOP_menu_max >= 0 ? static_cast<LOOP_menu_element_name>(element_Menu_LOOP[0]) : value_LOOP_Menu_none;
    pointer.track_value_element = value_LOOP_Track_none;

    Show_pointer(true);

    for (auto track = 0; track < TRACKS; ++track)
    {
        for (auto value = 0; value < LOOP_track_values; ++value)
        {
            Display_MidiLoop.Loop_show_pointerTrack(track, static_cast<LOOP_track_value_name>(value), false);
        }
    }
}

LOOP_field_description_struct PointerMidiLoop::Get_pointer(void)
{
    return pointer;
}

void PointerMidiLoop::Show_pointerTrack(const int track, const bool show)
{
    if (LOOP_events[track] == 0 && show)
    {
        return;
    }
    Display_MidiLoop.Loop_show_pointerTrack(track, pointer.track_value_element, show);
}

void PointerMidiLoop::Show_pointer(const bool show)
{
    switch (pointer.field_name)
    {
    case field_LOOP_Menu:
    {
        if (pointer.menu_element != value_LOOP_Menu_none)
        {
            Display_MidiLoop.Loop_show_pointerMenu(pointer.menu_element, show);
        }
    }
    break;

    case field_LOOP_TrackValues:
    {
        if (pointer.track_value_element != value_LOOP_Track_none)
        {
            for (auto track = 0; track < TRACKS; ++track)
            {
                Show_pointerTrack(track, show);
            }
        }
    }
    break;
    }
}

void PointerMidiLoop::Move_pointer(const int value)
{
    if (value == 0)
    {
        return;
    }

    Show_pointer(false);

    if (LOOP_menu_max < 0)
    {
        pointer.field_name = field_LOOP_Menu;
        pointer.menu_element = value_LOOP_Menu_none;
        pointer.track_value_element = value_LOOP_Track_none;
        return;
    }

    switch (pointer.field_name)
    {
    case field_LOOP_Menu:
    {
        int position = position_Menu_LOOP[pointer.menu_element];

        if (value == 1)
        {
            if (position < LOOP_menu_max)
            {
                pointer.menu_element = static_cast<LOOP_menu_element_name>(element_Menu_LOOP[++position]);
            }
            else
            {
                pointer.field_name = field_LOOP_TrackValues;
                pointer.menu_element = value_LOOP_Menu_none;
                pointer.track_value_element = value_LOOP_level;
            }
        }
        else if (value == -1)
        {
            if (position > 0)
            {
                pointer.menu_element = static_cast<LOOP_menu_element_name>(element_Menu_LOOP[--position]);
            }
            else
            {
                pointer.field_name = field_LOOP_TrackValues;
                pointer.menu_element = value_LOOP_Menu_none;
                pointer.track_value_element = value_LOOP_pitch;
            }
        }
    }
    break;

    case field_LOOP_TrackValues:
    {
        if (value == 1)
        {
            // Follow the display order without changing the field identifiers.
            if (pointer.track_value_element == value_LOOP_level)
            {
                pointer.track_value_element = value_LOOP_shift;
            }
            else if (pointer.track_value_element == value_LOOP_shift)
            {
                pointer.track_value_element = value_LOOP_pitch;
            }
            else
            {
                pointer.field_name = field_LOOP_Menu;
                pointer.menu_element = static_cast<LOOP_menu_element_name>(element_Menu_LOOP[0]);
                pointer.track_value_element = value_LOOP_Track_none;
            }
        }
        else if (value == -1)
        {
            if (pointer.track_value_element == value_LOOP_pitch)
            {
                pointer.track_value_element = value_LOOP_shift;
            }
            else if (pointer.track_value_element == value_LOOP_shift)
            {
                pointer.track_value_element = value_LOOP_level;
            }
            else
            {
                pointer.field_name = field_LOOP_Menu;
                pointer.menu_element = static_cast<LOOP_menu_element_name>(element_Menu_LOOP[LOOP_menu_max]);
                pointer.track_value_element = value_LOOP_Track_none;
            }
        }
    }
    break;
    }

    Show_pointer(true);
}
