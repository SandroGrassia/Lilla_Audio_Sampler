/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#pragma once

#include <Arduino.h>
#include "SharedElements.h"  // only for INSTRUMENTS
#include "SharedLoop.h"
#include "GraphicElements.h"
#include <SPI.h>
#include "config.h"

/*
LoopLedSet

PlayersStatistics requests ON/OFF transitions for each track/instrument without accessing the display.
The main loop consumes pending transitions and draws them outside the audio IRQ.
Restore_all_LED() requests a full redraw after the page is cleared, preserving active notes and pending transitions.
Only a playback reset should clear the player counters and request all LEDs OFF.
*/

class LoopLedSet
{
private:
    // riferimenti esterni
    volatile int8_t led_activity[TRACKS][INSTRUMENTS]; // -2: pending OFF, -1: acknowledged OFF, +1: acknowledged ON, +2: pending ON

public:
    LoopLedSet()
    {
        Request_all_LED_switch_off();    
    }
      
    void Restore_all_LED(void); // Request a redraw of all 32 states without changing playback.
    void Request_all_LED_switch_off(void); // called by main
    void Request_track_LED_switch_off(int track); // 
    void Request_LED_switch(int track, int instrument_id, bool on); // called by PlayersStatistics - do NOT access to Display
    int Read_LED_activity(int track, int instrument_id); // called by main
    int Consume_LED_activity(int track, int instrument_id); // Atomically snapshot and acknowledge; draw after returning.
};