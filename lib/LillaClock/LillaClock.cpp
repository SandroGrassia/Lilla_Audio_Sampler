#include "LillaClock.h"
#include "SharedElements.h"

void LillaClock::update(void)
{
    if (identity != 0)
    {
        return;
    }
    // The audio deadline still advances while control callbacks are paused: Players keep rendering.
    audio_update_time_micros = 0;
    ++audio_update_cycle;
    if (!stop_flag)
    {
        Filter_Biquad_Manager_ptr->Update();
        Delay_Manager_ptr->Update();
        Midi_reader_ptr->Update();
    }
    if (Players_Manager_ptr != nullptr)
    {
        Players_Manager_ptr->Prepare_read_budget(); // Covers main-loop edits, source promotion and pending retries before source reads.
    }
}

void LillaClock::Start(void)
{
    stop_flag = false;
}

void LillaClock::Stop(void)
{
    stop_flag = true;
}
