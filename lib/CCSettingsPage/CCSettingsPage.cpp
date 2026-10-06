/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 */

#include "CCSettingsPage.h"
#include "main.h"
#include "UserInterface.h"
#include "SharedElements.h"
#include "SharedMM.h"
#include "GlobalDisplaySetup.h"
#include "ArchivingManager.h"
#include "PlayersManager.h"
#include "MidiReader.h"

uint8_t CC_Sound_gain_cache[INSTRUMENTS]; // Cached per-instrument Sound gains used by the Control Change page.
static uint8_t CC_lowpass_filter_cache;          // Cached low-pass filter control value.
static int8_t CC_menu;                           // Current Control Change menu selection.
static int CC_number;                            // Selected MIDI Control Change number.

void Handle_CC_settings(void)
{
    if (Lilla_state == CC_SETTINGS)
    {

        if (Read_encoder(EN_PB_Select, CC_menu, 9, 0, 1))
        {
            Display_Setup.CC_show_frame_menu(CC_menu);

            Clear_UI_events();

            if (CC_menu > 0 && CC_menu < 9)
            {
                CC_number = CC_Sound_gain[CC_menu - 1];
            }
            else if (CC_menu == 9)
            {
                CC_number = CC_lowpass_filter_value;
            }
        }

        if (Read_encoder(EN_PB_Value, CC_number, 127, 0, 1))
        {
            if (CC_menu > 0 && CC_menu < 9)
            {
                CC_Sound_gain[CC_menu - 1] = CC_number;
                Display_Setup.CC_show_sound_gain(CC_menu - 1);
            }
            else if (CC_menu == 9)
            {
                CC_lowpass_filter_value = CC_number;
                Display_Setup.CC_show_lowpass_filter_value();
            }
        }

        // scegli l'item
        if (Read_pushbutton(EN_PB_Value))
        {
            if (CC_menu > 0 && CC_menu < 9)
            {
                CC_Sound_gain[CC_menu - 1] = 0;
                Display_Setup.CC_show_sound_gain(CC_menu - 1);
            }
            else if (CC_menu == 9)
            {
                CC_lowpass_filter_value = 0;
                Display_Setup.CC_show_lowpass_filter_value();
            }
        }

        // Autolearning
        if (display_wait)
        {
            if (CC_menu > 0 && CC_menu < 9)
            {
                CC_Sound_gain[CC_menu - 1] = CC_midi_controller;
                Display_Setup.CC_show_sound_gain(CC_menu - 1);
                CC_number = CC_Sound_gain[CC_menu - 1];
            }
            else if (CC_menu == 9)
            {
                CC_lowpass_filter_value = CC_midi_controller;
                Display_Setup.CC_show_lowpass_filter_value();
                CC_number = CC_lowpass_filter_value;
            }
            display_wait = false;
        }

        // Return to SETUP
        if (Read_pushbutton(EN_PB_Select) && CC_menu == 0)
        {
            CC_Save_settings();
            Golive_SETUP();
        }
    }
}

void Golive_CC_SETTINGS(void)
{
    Lilla_state = CC_SETTINGS;

    display_wait = false;

    for (auto local_instrument_id = 0; local_instrument_id < INSTRUMENTS; ++local_instrument_id)
    {
        CC_Sound_gain_cache[local_instrument_id] = CC_Sound_gain[local_instrument_id];
    }

    CC_lowpass_filter_cache = CC_lowpass_filter_value;
    Display_Setup.CC_show_ControlChange_page();

    Display_Setup.CC_show_all_sound_gains();
    Display_Setup.CC_show_lowpass_filter_value();

    CC_menu = 0;
    Display_Setup.CC_show_frame_menu(CC_menu);

    Clear_UI_events();
}

byte CC_Save_settings(void)
{
    Midi_reader.Stop();
    const byte result = Archive.Save_CC_settings(CC_Sound_gain, CC_lowpass_filter_value);

    Players_Manager.Stop_all_players();
    Midi_reader.Start();
    return result;
}

byte CC_Read_all_Sound_gain()
{
    return Archive.Read_CC_settings(CC_Sound_gain, CC_lowpass_filter_value);
}
