/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#pragma once

#include <Arduino.h>
#include "config.h"
#include "UserInterface.h"

class Switches
{
private:
    static constexpr uint8_t CONTACTS_VALUES = 16;
    // Trusted values for transition are ONLY:
    // 1110 (14)
    // 1101 (13)
    // 1011 (11)
    // 0111 (7)

    static constexpr uint8_t DEFAULT_POSITION = 0;
    static constexpr int PAUSE_SWITCH = 50; // milliseconds
    uint8_t output[SWITCHES]; // valid values: 0, 1, 2, 3
    bool changed[SWITCHES];
    uint32_t timer[SWITCHES];
    void Reset(void);

public:
    Switches()
    {
        Reset();
    }

    void Transmit_contacts(const uint8_t &switch_id, const uint8_t &contacts); // contacts range: 0b 0000 -> 0b 1111
    bool Get_change(const uint8_t &switch_id); // returns true if value has changed from last Get_change() call
    uint8_t Get_value(const uint8_t &switch_id); // returns the current stable switch position: 0, 1, 2, or 3
};
