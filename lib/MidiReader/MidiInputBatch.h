#pragma once

#include <stdint.h>

struct MidiInputMessage
{
    uint8_t type = 0;
    uint8_t channel = 0; // MIDI channel 1..16; system messages use 0.
    uint8_t data1 = 0;
    uint8_t data2 = 0;
};

// One audio-cycle batch: ordered events are never overwritten; continuous controls use their final value.
class MidiInputBatch
{
public:
    static constexpr uint8_t Max_ordered_messages = 8; // Maximum ordered MIDI messages applied per audio cycle (including NoteOn/NoteOff).
    static constexpr uint16_t Max_rx_bytes = 64; // Maximum bytes parsed from the entry snapshot; excess bytes remain in Serial1.
    static constexpr uint32_t Max_parse_micros = 80; // Upper time bound, never a waiting interval; excludes Player preparation.
    MidiInputMessage ordered[Max_ordered_messages] = {};
    MidiInputMessage controls[16][3] = {};
    bool dirty[16][3] = {};
    uint8_t count = 0;

    void Clear()
    {
        count = 0;
        for (uint8_t channel = 0; channel < 16; ++channel)
        {
            for (uint8_t control = 0; control < 3; ++control)
            {
                dirty[channel][control] = false;
            }
        }
    }

    bool Full() const
    {
        return count >= Max_ordered_messages;
    }

    bool Push(const MidiInputMessage &message)
    {
        int control = -1;
        if (message.type == 0xE0)
        {
            control = 0; // Pitch Bend.
        }
        else if (message.type == 0xD0)
        {
            control = 1; // Channel Pressure.
        }
        else if (message.type == 0xB0 && message.data1 == 1)
        {
            control = 2; // Modulation only: other CC messages retain their order.
        }
        if (control >= 0 && message.channel >= 1 && message.channel <= 16)
        {
            controls[message.channel - 1][control] = message;
            dirty[message.channel - 1][control] = true;
            return true;
        }
        if (Full())
        {
            return false;
        }
        ordered[count++] = message;
        return true;
    }
};
