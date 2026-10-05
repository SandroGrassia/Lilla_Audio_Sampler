/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "SharedPerformance.h"

int P_column_menu_element[P_menu_elements];           // argument is position
int P_row_menu_element[P_menu_elements];              // argument is position
P_menu_elements_name P_element_menu[P_menu_elements]; // argument is position
uint8_t P_position_Menu[P_menu_elements];             // argument is element
int8_t instrument_on_position[INSTRUMENTS];

void P_Update_line_of_all_instruments(void)
{
    uint8_t line = 0;
    for (auto instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
    {
        P_line_of_instrument[instrument_id] = -1;
        instrument_on_position[instrument_id] = -1;
    }
    for (auto instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
    {
        if (Patch[Patch_id].Instrument[instrument_id].used)
        {
            P_line_of_instrument[instrument_id] = line;
            instrument_on_position[line] = instrument_id;
            ++line;
        }
    }
}