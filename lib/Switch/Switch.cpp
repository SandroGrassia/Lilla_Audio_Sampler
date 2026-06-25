/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "Switch.h"

void Switch::Transmit_contacts(const uint8_t &switch_id, const uint8_t &contacts)
{
    if (switch_id >= SWITCHES || contacts >= CONTACTS_VALUES)
    {
        return;
    }

    if (millis() > timer[switch_id])
    {
        const auto next = matrix[output[switch_id]][contacts];

        output[switch_id] = next.output;
        if (next.restart_timer)
        {
            timer[switch_id] = millis() + PAUSE_SWITCH;
        }
    }
}

uint8_t Switch::Get_output(const uint8_t &switch_id)
{
    if (switch_id >= SWITCHES)
    {
        return DEFAULT_POSITION;
    }

    return output[switch_id];
}

void Switch::Reset(void)
{
    for (auto i = 0; i < SWITCHES; ++i)
    {
        output[i] = DEFAULT_POSITION;
        timer[i] = 0;
    }
}
