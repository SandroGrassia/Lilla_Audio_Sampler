/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "Switches.h"

void Switches::Transmit_contacts(const uint8_t &switch_id, const uint8_t &contacts)
{
    if (switch_id >= SWITCHES)
    {
        Serial.print(F("Switches::Transmit_contacts(): ERROR, invalid switch_id: "));
        Serial.println(switch_id);
        return;
    }
    if (millis() <= timer[switch_id])
    {
        return;
    }
    if (contacts >= CONTACTS_VALUES)
    {
        Serial.print(F("Switches::Transmit_contacts(): ERROR, invalid contacts: "));
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
        changed[switch_id] = true;
        timer[switch_id] = millis() + PAUSE_SWITCH;
    }
}

uint8_t Switches::Get_value(const uint8_t &switch_id)
{
    if (switch_id >= SWITCHES)
    {
        Serial.print(F("Switches::Get_value(): ERROR, invalid switch_id: "));
        Serial.println(switch_id);
        return DEFAULT_POSITION;
    }
    return output[switch_id];
}

bool Switches::Get_change(const uint8_t &switch_id)
{
    if (switch_id >= SWITCHES)
    {
        Serial.print(F("Switches::Get_change(): ERROR, invalid switch_id: "));
        Serial.println(switch_id);
        return false;
    }

    bool value = changed[switch_id];
    changed[switch_id] = false;
    return value;
}

void Switches::Reset(void)
{
    for (auto i = 0; i < SWITCHES; ++i)
    {
        output[i] = DEFAULT_POSITION;
        changed[i] = false;
        timer[i] = 0;
    }
}
