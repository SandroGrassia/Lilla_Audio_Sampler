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

    void Clear(); // Reset the ordered message count and pending control flags.

    bool Full() const; // Return whether the ordered message capacity has been reached.

    bool Push(const MidiInputMessage &message); // Coalesce continuous controls or append an ordered MIDI event.
};
