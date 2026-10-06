/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 */

#pragma once

void Handle_Midi_monitor(void); // Process incoming MIDI display and navigation at the original loop position.
void Golive_MIDI_MONITOR(void); // Stop players and initialize the MIDI Monitor page.
void Switch_from_MIDI_LOOP_to_MIDI_MONITOR(void); // Stop loop tracks before entering MIDI Monitor.
