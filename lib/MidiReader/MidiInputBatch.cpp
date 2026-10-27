#include "MidiInputBatch.h"

void MidiInputBatch::Clear()
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

bool MidiInputBatch::Full() const
{
    return count >= Max_ordered_messages;
}

bool MidiInputBatch::Push(const MidiInputMessage &message)
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
