#include "AutoTuneInput.h"
#include <cstring>

AutoTuneInput::AutoTuneInput() : AudioStream(0, nullptr)
{
}

bool AutoTuneInput::Idle() const
{
    return position == 1024;
}

void AutoTuneInput::Begin(const int16_t *input) // Called with audio interrupts disabled and the previous window fully sent.
{
    samples = input;
    position = 0;
}

void AutoTuneInput::update()
{
    if (Idle())
    {
        return;
    }
    audio_block_t *block = allocate();
    if (block == nullptr)
    {
        return;
    }
    memcpy(block->data, samples + position, AUDIO_BLOCK_SAMPLES * sizeof(int16_t));
    transmit(block);
    release(block);
    position += AUDIO_BLOCK_SAMPLES;
}
