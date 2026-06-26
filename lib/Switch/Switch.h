/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#pragma once

#include <Arduino.h>

class Switch
{
private:
    static constexpr uint8_t CONTACTS_VALUES = 16;
    // Trusted values for transition are:
    // 1110 (14)
    // 1101 (13)
    // 1011 (11)
    // 0111 (7)

    static constexpr uint8_t DEFAULT_POSITION = 0;
    static constexpr int PAUSE_SWITCH = 50; // milliseconds
    static constexpr uint8_t SWITCHES = 2;
    uint8_t output[SWITCHES]; // valid values: 0, 1, 2, 3
    uint32_t timer[SWITCHES];

public:
    Switch()
    {
        Reset();
    }

    void Transmit_contacts(const uint8_t &switch_id, const uint8_t &contacts); // contacts: 0 -> 15
    uint8_t Get_output(const uint8_t &switch_id);
    void Reset(void);
};
