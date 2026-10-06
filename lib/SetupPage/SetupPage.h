/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 */

#pragma once

void Handle_Setup(void); // Process Setup controls and navigation at the original loop position.
void Golive_SETUP(void); // Enter and initialize the Setup page.
void Switch_from_MIDI_LOOP_to_SETUP(void); // Keep the loop running while editing setup.
