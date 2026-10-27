#pragma once

#include <Arduino.h>
#include <AudioStream.h>

class AutoTuneInput : public AudioStream
{
private:
    const int16_t *samples = nullptr;  // Source window supplied to the auto-tune FFT input.
    volatile uint16_t position = 1024; // Next sample to transmit; 1024 marks a completed FFT window.

public:
    AutoTuneInput();                  // Create an audio source with no input queues.
    bool Idle() const;                // Return whether the complete FFT window has been transmitted.
    void Begin(const int16_t *input); // Called with audio interrupts disabled and the previous window fully sent.
    void update() override;           // Send the next source block to the FFT analyzer.
};
