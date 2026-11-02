/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "SharedLoop.h"

// MIDI LOOP
DMAMEM LOOP_struct LOOP_element[TRACKS][LOOP_LAYERS][LOOP_EVENTS]; // Access only entries below LOOP_layer_events; RAM2 is not zero-initialized.
uint32_t LOOP_events[TRACKS];
uint32_t LOOP_layer_events[TRACKS][LOOP_LAYERS];
DMAMEM uint32_t LOOP_time_order[TRACKS][LOOP_LAYERS][LOOP_EVENTS];
int LOOP_slide[TRACKS];
float LOOP_stretch = 1.0;

// Menu
int LOOP_menu_max;
const char Menu_LOOP_char[LOOP_menu_values][12] = {{"NEW"}, {"SAVE"}, {"SAVE_AS_NEW"}, {"DELETE"}};
const uint8_t dimension_voice_Menu_LOOP[LOOP_menu_values] = {3, 4, 11, 6};
bool Menu_LOOP[LOOP_menu_values];
uint8_t X_position_Menu_LOOP[LOOP_menu_values]; // argument is position
uint8_t element_Menu_LOOP[LOOP_menu_values];  // argument is position
uint8_t position_Menu_LOOP[LOOP_menu_values]; // argument is element

// LOOP play/stop
bool LOOP_track_run[TRACKS] = {false};
uint32_t LOOP_play_time[TRACKS][LOOP_LAYERS] = {0};
uint32_t LOOP_play_event[TRACKS][LOOP_LAYERS] = {0};
float LOOP_volume[TRACKS] = {0};
uint32_t LOOP_time;
int LOOP_pitch_int[TRACKS] = {0};

// LOOP learn
int LOOP_learning_track;
int LOOP_learning_layer;
bool LOOP_learn_flag;
uint32_t LOOP_elements;
elapsedMillis LOOP_learn_clock;
uint32_t LOOP_learn_end_time;
int LOOP_clock_memo;
uint32_t LOOP_last_event;

// LOOP_metronomo
bool LOOP_metronomo_run = false; // il metronomo e' running

// LOOP metronomo, richieste da MidiReader a Main
bool LOOP_metronomo_flag_IN[2] = {false, false}; // accendi led_0, switch led del metronomo

// LOOP metronomo, richieste da Main a MidiReader
// bool LOOP_metronomo_flag_OUT = {false}; // run metronomo
// unsigned long LOOP_metronomo_update_time = 0;  // prossimo istante di aggiornamento

// LOOP salvataggio su SD
int LOOP_id;
int LOOP_suggested_id = NEW_LOOP;

int LOOP_First_free_layer(int track)
{
    for (int layer = 0; layer < LOOP_LAYERS; ++layer)
    {
        if (LOOP_layer_events[track][layer] == 0)
        {
            return layer;
        }
    }
    return -1;
}

bool LOOP_Is_master_recording(void)
{
    return LOOP_learning_track == MASTER_TRACK && LOOP_learning_layer == 0;
}

void LOOP_Begin_recording(int track, int layer)
{
    LOOP_learning_track = track;
    LOOP_learning_layer = layer;
    LOOP_elements = 0;
    LOOP_last_event = 0;
    LOOP_learn_clock = 0;
    LOOP_learn_flag = true;
}

bool LOOP_Finish_recording(bool cancel)
{
    LOOP_learn_flag = false;
    if (cancel)
    {
        LOOP_elements = 0;
    }
    LOOP_layer_events[LOOP_learning_track][LOOP_learning_layer] = LOOP_elements;
    LOOP_events[LOOP_learning_track] += LOOP_elements;
    return LOOP_elements > 0;
}

void LOOP_set_time_order(int track)
{
    for (int layer = 0; layer < LOOP_LAYERS; ++layer)
    {
        const uint32_t count = LOOP_layer_events[track][layer];
        if (count == 0)
        {
            continue;
        }
        uint32_t first = 0;
        for (uint32_t event = 1; event < count; ++event)
        {
            if (LOOP_element[track][layer][event].time < LOOP_element[track][layer][first].time)
            {
                first = event;
            }
        }
        for (uint32_t event = 0; event < count; ++event)
        {
            LOOP_time_order[track][layer][event] = (first + event) % count;
        }
    }
}

void LOOP_restart_layer(int track, int layer)
{
    const uint32_t now = LOOP_Clock();
    const uint32_t phase = now % LOOP_time;
    const uint32_t cycle_start = now - phase;
    const uint32_t count = LOOP_layer_events[track][layer];
    for (uint32_t position = 0; position < count; ++position)
    {
        const uint32_t event = LOOP_time_order[track][layer][position];
        const uint32_t time = LOOP_element[track][layer][event].time;
        if (time >= phase)
        {
            LOOP_play_event[track][layer] = event;
            LOOP_play_time[track][layer] = cycle_start + time;
            return;
        }
    }
    const uint32_t first = LOOP_time_order[track][layer][0];
    LOOP_play_event[track][layer] = first;
    LOOP_play_time[track][layer] = cycle_start + LOOP_time + LOOP_element[track][layer][first].time;
}

void LOOP_restart_procedure(int track)
{
    for (int layer = 0; layer < LOOP_LAYERS; ++layer)
    {
        if (LOOP_layer_events[track][layer] > 0)
        {
            LOOP_restart_layer(track, layer);
        }
    }
    LOOP_track_run[track] = LOOP_events[track] > 0;
}

void LOOP_Shift_track(int track, int jump)
{
    for (int layer = 0; layer < LOOP_LAYERS; ++layer)
    {
        for (uint32_t event = 0; event < LOOP_layer_events[track][layer]; ++event)
        {
            LOOP_element[track][layer][event].time = (LOOP_element[track][layer][event].time + jump) % LOOP_time;
        }
    }
    LOOP_set_time_order(track);
}
