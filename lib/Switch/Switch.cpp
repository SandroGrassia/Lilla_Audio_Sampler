/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "Switch.h"

void Switch::Transmit_position(const uint8_t &switch_id, const uint8_t &new_position)
{
    if (switch_id >= SWITCHES || new_position >= POSITIONS)
    {
        return;
    }

    if (position[switch_id] != new_position)
    {
        position[switch_id] = new_position;
        output[switch_id] = true;
    }
}

uint8_t Switch::Get_position(const uint8_t &switch_id)
{
    if (switch_id >= SWITCHES)
    {
        return DEFAULT_POSITION;
    }

    return position[switch_id];
}

bool Switch::Get_output(const uint8_t &switch_id)
{
    if (switch_id >= SWITCHES)
    {
        return false;
    }

    auto value = output[switch_id];
    output[switch_id] = false;
    return value;
}

void Switch::Reset(void)
{
    for (auto i = 0; i < SWITCHES; ++i)
    {
        position[i] = DEFAULT_POSITION;
        output[i] = false;
    }
}
