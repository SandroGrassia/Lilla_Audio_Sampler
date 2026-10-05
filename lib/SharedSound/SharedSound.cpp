/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "SharedSound.h"

// menu
uint8_t S_menu_choice;
uint8_t S_column_menu_element[S_menu_elements];    // argument is position
S_menu_elements_name S_element_menu[S_menu_elements]; // argument is position
uint8_t S_position_menu[S_menu_elements];          // argument is element
bool S_Menu[S_menu_elements];
