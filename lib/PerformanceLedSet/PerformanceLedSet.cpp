/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "PerformanceLedSet.h"
#include <util/atomic.h>

void PerformanceLedSet::Request_all_LED_switch_off(void)
{
    for (auto instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
    {
        led_activity[instrument_id] = -2; // request switch OFF
    }
}

void PerformanceLedSet::Request_LED_switch(int instrument_id, bool on)
{
    if (on)
    {
        if (led_activity[instrument_id] < 0)
        {
            led_activity[instrument_id] = 2; // request switch ON
        }
    }
    else
    {
        if (led_activity[instrument_id] > 0)
        {
            led_activity[instrument_id] = -2; // request switch OFF
        }
    }
}

int PerformanceLedSet::Read_LED_activity(int instrument_id)
{
    return led_activity[instrument_id];
}

int PerformanceLedSet::Consume_LED_activity(int instrument_id)
{
    const uint32_t irq_mask = __get_primask();
    __disable_irq();
    const int activity = led_activity[instrument_id];
    if (activity == 2 || activity == -2)
    {
        led_activity[instrument_id] = activity > 0 ? 1 : -1;
    }
    if (irq_mask == 0)
    {
        __enable_irq();
    }
    return activity;
}

void PerformanceLedSet::Restore_all_LED(void)
{
    const uint32_t irq_mask = __get_primask();
    __disable_irq();
    for (auto instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
    {
        if (abs(led_activity[instrument_id]) == 1)
        {
            led_activity[instrument_id] = 2 * led_activity[instrument_id]; // write LED switch ON (+2) or OFF (-2)
        }
    }
    if (irq_mask == 0)
    {
        __enable_irq();
    }
}
