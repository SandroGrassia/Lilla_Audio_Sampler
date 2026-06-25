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

    static constexpr uint8_t CONTACTS_VALUES = 16;
    static constexpr uint8_t OUTPUTS = 4;
    static constexpr uint8_t DEFAULT_POSITION = 0;
    static constexpr int PAUSE_SWITCH = 50; // milliseconds

    uint8_t output[SWITCHES]; // valid values: 0, 1, 2, 3
    uint32_t timer[SWITCHES];

    struct NextOutputAndPause
    {
        uint8_t output;   // 0, 1, 2, 3
        bool restart_timer;
    };

// clang-format off
static constexpr NextOutputAndPause matrix[OUTPUTS][CONTACTS_VALUES] =
{
    //0           1           2           3           4           5           6           7 ->0       8           9           10          11 ->1      12          13 ->2      14 ->3      15
    {{0, false}, {0, false}, {0, false}, {0, false}, {0, false}, {0, false}, {0, false}, {0, false}, {0, false}, {0, false}, {0, false}, {1, true},  {0, false}, {2, true},  {3, true},  {0, false}}, // 0
    {{1, false}, {1, false}, {1, false}, {1, false}, {1, false}, {1, false}, {1, false}, {0, true},  {1, false}, {1, false}, {1, false}, {1, false}, {1, false}, {2, true},  {3, true},  {1, false}}, // 1
    {{2, false}, {2, false}, {2, false}, {2, false}, {2, false}, {2, false}, {2, false}, {0, true},  {2, false}, {2, false}, {2, false}, {1, true},  {2, false}, {2, false}, {3, true},  {2, false}}, // 2
    {{3, false}, {3, false}, {3, false}, {3, false}, {3, false}, {3, false}, {3, false}, {0, true},  {3, false}, {3, false}, {3, false}, {1, true},  {3, false}, {2, true},  {3, false}, {3, false}}  // 3
};
// clang-format on

public:
    Switch()
    {
        Reset();
    }

    void Transmit_contacts(const uint8_t &switch_id, const uint8_t &contacts); // contacts: 0 -> 15
    uint8_t Get_output(const uint8_t &switch_id);
    void Reset(void);
};
