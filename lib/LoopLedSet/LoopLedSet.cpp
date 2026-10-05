/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "LoopLedSet.h"
#include <util/atomic.h>

void LoopLedSet::Request_all_LED_switch_off(void)
{
    for (auto track = 0; track < TRACKS; ++track)
    {
        Request_track_LED_switch_off(track);
    }
}

void LoopLedSet::Request_track_LED_switch_off(int track)
{
    // also NOT existing instruments
    for (auto instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
    {
        led_activity[track][instrument_id] = -2;
    }
}

void LoopLedSet::Request_LED_switch(int track, int instrument_id, bool on)
{
    if (on)
    {
        // Serial.print("Request_LED_switch(int track, int instrument_id, bool on) - true - led_activity:");
        // Serial.println(led_activity[track][instrument_id]);

        if (led_activity[track][instrument_id] < 0)
        {
            led_activity[track][instrument_id] = 2; // request switch ON
            // Serial.println("on");
        }
    }
    else
    {
        // Serial.print("Request_LED_switch(int track, int instrument_id, bool on) - false - led_activity:");
        // Serial.println(led_activity[track][instrument_id]);
        
        if (led_activity[track][instrument_id] > 0)
        {
            led_activity[track][instrument_id] = -2; // request switch OFF
            // Serial.println("off");
        }
    }
}

int LoopLedSet::Read_LED_activity(int track, int instrument_id)
{
    return led_activity[track][instrument_id];
}

int LoopLedSet::Consume_LED_activity(int track, int instrument_id)
{
    const uint32_t irq_mask = __get_primask();
    __disable_irq();
    const int activity = led_activity[track][instrument_id];
    if (activity == 2 || activity == -2)
    {
        led_activity[track][instrument_id] = activity > 0 ? 1 : -1;
    }
    if (irq_mask == 0)
    {
        __enable_irq();
    }
    return activity;
}