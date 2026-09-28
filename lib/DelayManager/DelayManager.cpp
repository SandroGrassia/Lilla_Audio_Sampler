/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 */

#include "DelayManager.h"

int DelayManager::Get_value(int item) const // Keep requested values separate from the effective DSP values.
{
    switch (item)
    {
    case SAMPLES:
        return Delay_data.samples;
    case SAMPLES_LR:
        return Delay_data.samples_LR;
    case MODULATION_DEPTH:
        return Delay_data.modulation_depth;
    case MODULATION_FREQUENCY:
        return Delay_data.modulation_frequency;
    case MODULATION_PHASE_LR:
        return Delay_data.modulation_phase_LR;
    case LOOP_GAIN:
        return Delay_data.loop_gain;
    case INSTRUMENT_ROUTE:
        return Delay_data.instrument_route;
    case MODULATION_SOURCE:
        return Delay_data.modulation_source;
    default:
        return 0;
    }
}

bool DelayManager::Set_value(int item, int value) // The latest request wins for this parameter while unrelated ramps continue.
{
    if (item < 0 || item >= DELAY_ITEMS)
    {
        return false;
    }
    if (item < DELAY_LPF_ITEMS)
    {
        value = constrain(value, Delay_data_limits[item][0], Delay_data_limits[item][1]);
    }
    else
    {
        value = constrain(value, 0, item == INSTRUMENT_ROUTE ? 255 : 2);
    }
    if (value == Get_value(item))
    {
        return flag[item];
    }
    switch (item)
    {
    case SAMPLES:
        Delay_data.samples = value;
        break;
    case SAMPLES_LR:
        Delay_data.samples_LR = value;
        break;
    case MODULATION_DEPTH:
        Delay_data.modulation_depth = value;
        Start_LPF(item, Delay_values.modulation_depth, Calc_delay_depth(value)); // Smooth the effective modulation depth, not its nonlinear UI index.
        break;
    case MODULATION_FREQUENCY:
        Delay_data.modulation_frequency = value;
        Start_LPF(item, Delay_values.modulation_frequency, Calc_delay_frequency(value));
        break;
    case MODULATION_PHASE_LR:
        Delay_data.modulation_phase_LR = value;
        Start_LPF(item, Delay_values.modulation_phase_LR, value);
        break;
    case LOOP_GAIN:
        Delay_data.loop_gain = value;
        Start_LPF(item, Delay_values.loop_gain, Delay_feedback(value)); // Retarget feedback from the last value delivered to AudioGain.
        break;
    case INSTRUMENT_ROUTE:
        Delay_data.instrument_route = value;
        break;
    case MODULATION_SOURCE:
        Delay_data.modulation_source = value;
        break;
    }
    flag[item] = true;
    return true;
}

bool DelayManager::New_values(const Delay_data_struct *data) // Publish a stable patch snapshot through the same setters used by the UI.
{
    const Delay_data_struct target = *data;
    bool pending = false;
    pending |= Set_value(SAMPLES, target.samples);
    pending |= Set_value(SAMPLES_LR, target.samples_LR);
    pending |= Set_value(MODULATION_DEPTH, target.modulation_depth);
    pending |= Set_value(MODULATION_FREQUENCY, target.modulation_frequency);
    pending |= Set_value(MODULATION_PHASE_LR, target.modulation_phase_LR);
    pending |= Set_value(LOOP_GAIN, target.loop_gain);
    pending |= Set_value(INSTRUMENT_ROUTE, target.instrument_route);
    pending |= Set_value(MODULATION_SOURCE, target.modulation_source);
    return pending;
}

void DelayManager::Apply_delay_times(void) // Restore the unshifted channel whenever the signed stereo offset changes.
{
    Delay_values.samples = Calc_delay_samples(Delay_data.samples);
    Delay_values.samples_LR = Calc_delay_samples_LR(Delay_data.samples_LR);
    const int left = Delay_values.samples + (Delay_values.samples_LR > 0 ? Delay_values.samples_LR : 0);
    const int right = Delay_values.samples - (Delay_values.samples_LR < 0 ? Delay_values.samples_LR : 0);
    Delay_L_ptr->Set_delay_central_value(left);  // Let the audio reader ramp toward the final target without an upstream time filter.
    Delay_R_ptr->Set_delay_central_value(right); // Always restore the other channel as well, including at zero offset.
}

void DelayManager::Update(void) // Apply independent transitions once per audio block and clear completed flags.
{
    if (flag[INSTRUMENT_ROUTE])
    {
        Calc_delay_routing(Delay_data.instrument_route);
        for (int instrument = 0; instrument < INSTRUMENTS; ++instrument)
        {
            Players_Manager_ptr->MX_multicast_change_routing(instrument); // Pass the instrument index; the routing function reads its enable flag itself.
        }
        flag[INSTRUMENT_ROUTE] = false;
    }
    if (flag[MODULATION_SOURCE])
    {
        Delay_values.modulation_source = Delay_data.modulation_source;
        Delay_L_ptr->Set_delay_modulation_source(Delay_values.modulation_source);
        Delay_R_ptr->Set_delay_modulation_source(Delay_values.modulation_source);
        flag[MODULATION_SOURCE] = false;
    }
    if (flag[SAMPLES] || flag[SAMPLES_LR])
    {
        Apply_delay_times(); // Publish the combined base time and stereo offset only once.
        flag[SAMPLES] = false;
        flag[SAMPLES_LR] = false;
    }
    if (flag[MODULATION_DEPTH])
    {
        Delay_values.modulation_depth = New_value(MODULATION_DEPTH);
        Delay_L_ptr->Set_delay_modulation_gain(Delay_values.modulation_depth);
        Delay_R_ptr->Set_delay_modulation_gain(Delay_values.modulation_depth);
    }
    if (flag[MODULATION_FREQUENCY])
    {
        Delay_values.modulation_frequency = New_value(MODULATION_FREQUENCY);
        LFO_D_ptr[0]->Set_frequency(Delay_values.modulation_frequency);
        LFO_D_ptr[1]->Set_frequency(Delay_values.modulation_frequency);
    }
    if (flag[MODULATION_PHASE_LR])
    {
        Delay_values.modulation_phase_LR = lround(New_value(MODULATION_PHASE_LR));
        LFO_D_ptr[0]->Set_phase(Delay_values.modulation_phase_LR);
    }
    if (flag[LOOP_GAIN])
    {
        Delay_values.loop_gain = New_value(LOOP_GAIN);
        D_gain_L_feedback_ptr->Set_gain(Delay_values.loop_gain);
        D_gain_R_feedback_ptr->Set_gain(Delay_values.loop_gain);
    }
}

void DelayManager::Stop(void) // Complete pending manager targets without bypassing the audio objects' own ramps.
{
    for (int item = 0; item < DELAY_LPF_ITEMS; ++item)
    {
        remaining[item] = 0;
    }
    Update(); // Apply exact targets and clear all pending flags.
}

void DelayManager::Start_LPF(int item, double current, double target) // Initialize a bounded 30-block transition from the current applied value.
{
    y_1[item] = current;
    y_2[item] = current;
    x_1[item] = current;
    x[item] = target;
    lower[item] = current < target ? current : target;
    upper[item] = current > target ? current : target;
    remaining[item] = steps;
}

double DelayManager::New_value(int item) // Prevent overshoot and explicitly settle the last step at the requested target.
{
    if (remaining[item] <= 1)
    {
        remaining[item] = 0;
        flag[item] = false;
        return x[item];
    }
    --remaining[item];
    const double result = constrain(d1 * y_1[item] + d2 * y_2[item] + n0 * x[item] + n1 * x_1[item], lower[item], upper[item]);
    x_1[item] = x[item];
    y_2[item] = y_1[item];
    y_1[item] = result;
    return result;
}
