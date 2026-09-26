/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#pragma once

#include "DisplayPrimitives.h"

// MIDI monitor and physical control test screens.
class DisplayDiagnostics
{
private:
    int L_POPUP = 0;
    int H_POPUP = 0;
    int X_POPUP = 0;
    int Y_POPUP = 0;

public:
    DisplayDiagnostics() {}

    void Encoder_pushbutton_test_board(void);
    void Encoder_pushbutton_test_result(const int device, const int element, const int value);
    void Midi_monitor_page(void);
    void Midi_monitor_frame(void);
    void Midi_monitor_data(uint8_t incoming_midi_channel, uint8_t incoming_midi_message, int8_t incoming_note_number, int8_t incoming_velocity, int32_t incoming_midi_value, int8_t incoming_number);
};
