/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#pragma once

#include <Arduino.h>
#include "SharedElements.h"
#include "config.h"


static constexpr float depth_array[40] = {
        0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 1.0,
        2, 3, 4, 5, 6, 8, 10, 12, 14, 16,
        18, 20, 24, 28, 32, 36, 40, 44, 48, 52,
        56, 60, 65, 70, 75, 80, 85, 90, 95, 100};

enum Delay_parameters
    {
        SAMPLES,
        SAMPLES_LR,
        MODULATION_DEPTH,
        MODULATION_FREQUENCY,
        MODULATION_PHASE_LR,
        LOOP_GAIN,
        INSTRUMENT_ROUTE,
        MODULATION_SOURCE
    };

static constexpr int DELAY_ITEMS = 8; // all Delay parameters
static constexpr int DELAY_LPF_ITEMS = 6; // parameters from SAMPLES to LOOP_GAIN are filtered with LPFs 

struct Delay_data_struct // DELAY_DATA_DIM byte
{ 
    uint16_t samples;
    int16_t samples_LR;
    uint8_t instrument_route;
    uint8_t modulation_source;
    uint8_t modulation_depth;
    uint8_t modulation_frequency;
    uint16_t modulation_phase_LR;
    uint16_t loop_gain;
};
static constexpr int DELAY_DATA_DIM = sizeof(Delay_data_struct);

static constexpr int Delay_data_limits[DELAY_LPF_ITEMS][2] = {
    {0, 99}, // SAMPLES
    {-10, 10}, // SAMPLES_LR
    {0, 39}, // MODULATION_DEPTH
    {0, 90}, // MODULATION_FREQUENCY
    {0, 359}, // MODULATION_PHASE_LR
    {0, 9} // LOOP_GAIN
};

struct Delay_values_struct
{
    float loop_gain;
    float samples;
    float samples_LR;
    bool instrument_route[INSTRUMENTS];
    uint8_t modulation_source; // 0: none 1:wave 2:signal
    float modulation_depth;        // 0.0 --> 1.0 modulation index
    float modulation_frequency;    // only for waveform
    uint16_t modulation_phase_LR;
};

extern Delay_values_struct Delay_values;
extern Delay_data_struct Delay_data;

float Delay_feedback(int8_t value);
void Calc_Delay_values(const Delay_data_struct data);
void Turn_ON_Delay(bool ON);
void Calc_delay_routing(uint8_t value);
int Calc_delay_samples(int value);
int Calc_delay_samples_LR(int value);
float Calc_delay_depth (int value);
float Calc_delay_frequency(int value);
void Print_Delay_data(const Delay_data_struct &data);
void Print_Delay_values(Delay_values_struct Delay_values);

// Pointer
static constexpr int DELAY_element_names = 7;
enum DELAY_element_name
{
    value_DELAY_Feedback,
    value_DELAY_Delay_time,
    value_DELAY_Delay_time_LR,
    value_DELAY_Modulation_source,
    value_DELAY_Modulation_frequency,
    value_DELAY_Modulation_depth,
    value_DELAY_Modulation_phase_LR
};