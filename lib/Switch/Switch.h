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
    static constexpr uint8_t SWITCHES = 2;
    static constexpr uint8_t POSITIONS = 4;
    static constexpr uint8_t DEFAULT_POSITION = 0;

    uint8_t position[SWITCHES];
    bool output[SWITCHES];

public:
    Switch()
    {
        Reset();
    }

    void Transmit_position(const uint8_t &switch_id, const uint8_t &new_position);
    uint8_t Get_position(const uint8_t &switch_id);
    bool Get_output(const uint8_t &switch_id);
    void Reset(void);
};
