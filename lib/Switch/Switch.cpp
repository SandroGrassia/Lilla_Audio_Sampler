/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "Switch.h"

void Switch::Transmit_contacts(const uint8_t &switch_id, const uint8_t &contacts)
{
    if (switch_id >= SWITCHES)
    {
        Serial.print(F("Switch::Transmit_contacts(): ERROR, invalid switch_id: "));
        Serial.println(switch_id);
        return;
    }

    if (millis() <= timer[switch_id])
    {
        return;
    }
    
    if (contacts >= CONTACTS_VALUES)
    {
        Serial.print(F("Switch::Transmit_contacts(): ERROR, invalid contacts: "));
        Serial.println(contacts);
        return;
    }

    uint8_t next_output = output[switch_id];

    switch (contacts)
    {
    case 7: // 0b 0111
        next_output = 3;
        break;
    case 11: // 0b 1011
        next_output = 2;
        break;
    case 13: // 0b 1101
        next_output = 1;
        break;
    case 14: // 0b 1110
        next_output = 0;
        break;
    default: // all other entries
        return;
    }

    if (next_output != output[switch_id])
    {
        output[switch_id] = next_output;
        timer[switch_id] = millis() + PAUSE_SWITCH;
    }
}

uint8_t Switch::Get_output(const uint8_t &switch_id)
{
    if (switch_id >= SWITCHES)
    {
        Serial.print(F("Switch::Get_output(): ERROR, invalid switch_id: "));
        Serial.println(switch_id);
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
