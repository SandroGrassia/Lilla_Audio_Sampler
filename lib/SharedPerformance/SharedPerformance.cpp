/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "SharedPerformance.h"

int8_t instrument_on_position[INSTRUMENTS_MAX];

void P_Update_line_of_all_instruments(void)
{
    uint8_t line = 0;
    for (auto instrument_id = 0; instrument_id < INSTRUMENTS_MAX; ++instrument_id)
    {
        P_line_of_instrument[instrument_id] = -1;
        instrument_on_position[instrument_id] = -1;
    }
    for (auto instrument_id = 0; instrument_id < INSTRUMENTS_MAX; ++instrument_id)
    {
        if (Patch[Patch_id].Instrument[instrument_id].used)
        {
            P_line_of_instrument[instrument_id] = line;
            instrument_on_position[line] = instrument_id;
            ++line;
        }
    }
}