/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 */

#pragma once

#include "SharedDelay.h"

extern DELAY_element_name DELAY_local_pointer; // Current Delay menu or parameter selection.

void Handle_Delay(void); // Handle Delay controls and navigation at their original position in loop().
void Golive_DELAY_SETTINGS(void);                          // Enter and initialize the Delay settings page.
void Switch_from_LIVE_SAMPLING_to_DELAY(void);             // Open Delay settings while retaining Live Sampler as the return page.
void D_Set_value(int item, int value); // Publish one UI request through the same parameter owner used by patch changes.
bool D_Read_value(int item);           // Read a delay parameter edit into a local value and publish it through the parameter owner.
