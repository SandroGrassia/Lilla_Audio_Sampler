/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#pragma once

#include <Arduino.h>
#include "SharedElements.h" // per INSTRUMENT_MAX
#include "config.h"
#include "Functions.h"

class PerformanceLedSet
{
private:

    int8_t led_activity[INSTRUMENTS]; // -2: request switch-OFF   -1: led OFF    +1: led ON   +2: request switch-ON

public:
    PerformanceLedSet()
    {
        // Zero is not a valid LED state: accept the first NoteOn even for previously unused instruments.
        for (int instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
        {
            led_activity[instrument_id] = -1;
        }
    }

    void Request_all_LED_switch_off(void);
    void Request_LED_switch(int instrument_id, bool on); // called by PlayersStatistics - do NOT access to Display
    int Read_LED_activity(int instrument_id); // called by main
    void Write_LED_activity(int instrument_id, bool on); // called by main
    void Restore_all_LED(void);
};
