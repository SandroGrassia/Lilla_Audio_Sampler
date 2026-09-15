/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#pragma once

#include <Arduino.h>
#include <AudioStream.h>
#include "MidiReader.h"
#include "FilterBiquadManager.h"
#include "DelayManager.h"
#include "config.h"

class LillaClock : public AudioStream
{
private:
    bool stop_flag = true;

public:
    LillaClock(void) : AudioStream(0, nullptr)
    {
        active = true; // Keep control callbacks scheduled without audio connections.
    }

    uint8_t identity;
    MidiReader *Midi_reader_ptr = nullptr;
    FilterBiquadManager *Filter_Biquad_Manager_ptr = nullptr;
    DelayManager *Delay_Manager_ptr = nullptr;

    bool Is_running(void) const { return !stop_flag; } // Read the callback state with audio interrupts disabled.
    void Start(void); // Enable control callbacks without changing MIDI keyboard state.
    void Stop(void); // Pause control callbacks while players and the finalizer continue processing audio.
    virtual void update(void); // Trigger 0 runs the enabled control batch before Player rendering; Trigger 1 is reserved.
};
