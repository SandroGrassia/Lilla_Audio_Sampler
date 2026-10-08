/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "SharedSampler.h"

DS_Trim DS_get_trim(const Sound_struct *channels, bool stereo)
{
    DS_Trim trim = {channels[0].A, channels[0].B};
    if (stereo)
    {
        trim.first = channels[1].A < trim.first ? channels[1].A : trim.first;
        trim.last = channels[1].B > trim.last ? channels[1].B : trim.last;
    }
    return trim;
}

void DS_copy_edited_parameters(const Sound_struct &before, const Sound_struct &edited, Sound_struct &other)
{
    // Copy only edited parameters: retain each channel's source and untouched stereo pan.
    if (edited.A != before.A)
    {
        other.A = edited.A;
    }
    if (edited.B != before.B)
    {
        other.B = edited.B;
    }
    if (edited.mode != before.mode)
    {
        other.mode = edited.mode;
    }
    if (edited.pitch != before.pitch)
    {
        other.pitch = edited.pitch;
    }
    if (edited.Noclick != before.Noclick)
    {
        other.Noclick = edited.Noclick;
    }
    if (edited.pan != before.pan)
    {
        other.pan = edited.pan;
    }
    if (edited.data != before.data)
    {
        other.data = edited.data;
    }
    if (edited.attack != before.attack)
    {
        other.attack = edited.attack;
    }
    if (edited.decay != before.decay)
    {
        other.decay = edited.decay;
    }
    if (edited.sustain != before.sustain)
    {
        other.sustain = edited.sustain;
    }
    if (edited.release != before.release)
    {
        other.release = edited.release;
    }
    if (edited.gain != before.gain)
    {
        other.gain = edited.gain;
    }
}

// DIRECT SAMPLING

bool Menu_DS[DS_menu_elements];
const char Menu_DS_char[DS_menu_elements][19] = {{"CANCEL_RECORDING"}, {"PAUSE+REC"}, {"MONO_REC"}, {"STEREO_REC"}, {"STOP"}, {"MAKE_RAW"}, {"CANCEL"}, {"MAKE_MONO"}, {"MAKE_LEFT"}, {"MAKE_RIGHT"}, {"MAKE_BOTH"}, {"EXPORT_WAV_TO_SD"}};
const uint8_t dimension_voice_Menu_DS[DS_menu_elements] = {16, 9, 8, 10, 4, 8, 6, 9, 9, 10, 9, 16};
uint8_t X_position_Menu_DS[DS_menu_elements]; // argument is position
uint8_t Y_position_Menu_DS[DS_menu_elements]; // argument is position
uint8_t element_Menu_DS[DS_menu_elements];    // argument is position
uint8_t position_Menu_DS[DS_menu_elements];   // argument is element
uint8_t choice_DS_menu;
int DS_menu_max;


int recordings;
int recording = -1; // recording id online
bool DS_recording_led_visible = false;
bool DS_recording_led_redraw = false;
elapsedMillis DS_blink_timer;
bool DS_blink_ON = false;
