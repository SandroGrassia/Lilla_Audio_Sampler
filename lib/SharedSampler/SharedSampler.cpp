/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "SharedSampler.h"

// DIRECT SAMPLING

bool Menu_DS[DS_menu_elements];
const char Menu_DS_char[DS_menu_elements][19] = {{"CANCEL_RECORDING"}, {"PAUSE+REC"}, {"MONO_REC"}, {"STEREO_REC"}, {"STOP"}, {"MAKE_RAW"}, {"CANCEL"}, {"MAKE_MONO"}, {"MAKE_LEFT"}, {"MAKE_RIGHT"}, {"MAKE_BOTH"}, {"EXPORT_RAW_TO_SD"}};
const uint8_t dimension_voice_Menu_DS[DS_menu_elements] = {16, 9, 8, 10, 4, 8, 6, 8, 9, 10, 9, 16};
uint8_t X_position_Menu_DS[DS_menu_elements]; // argument is position
uint8_t Y_position_Menu_DS[DS_menu_elements]; // argument is position
uint8_t element_Menu_DS[DS_menu_elements];    // argument is position
uint8_t position_Menu_DS[DS_menu_elements];   // argument is element
uint8_t choice_DS_menu;
int DS_menu_max;


int recordings;
int recording = -1; // recording id online
int DS_gain;
elapsedMillis DS_blink_timer;
bool DS_blink_ON = false;