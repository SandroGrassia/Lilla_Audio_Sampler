/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

// AudioGain offers:
// - Output gain (output gain changes with no zip-effect!)
// - Output Mute/Unmute commands
// - Input gain is fixed 1.0
//

#include "AudioGain.h"

void AudioGain::update(void)
{
    audio_block_t *out = NULL;
    out = receiveWritable(0);

    if (out)
    {
        applyGain(out->data, multiplier);
        transmit(out);
        release(out);
    }
}

void AudioGain::Set_gain(float value) // Ramp to the exact target, including changes smaller than the previous dead band.
{
    if (gain_flag && value == gain_target)
    {
        return;
    }
    gain_target = value;
    const float difference = gain_target - gain_runtime;
    if (difference == 0.0f)
    {
        gain_flag = false;
        multiplier = gain_target * 65536.0f;
        return;
    }
    gain_samples = static_cast<int>(ceilf(fabsf(difference) / 0.001f));
    if (gain_samples < 2)
    {
        gain_samples = 2;
    }
    if (gain_samples % 2 != 0)
    {
        ++gain_samples;
    }
    gain_delta = difference / gain_samples;
    gain_step = gain_samples;
    gain_flag = true;
}

void AudioGain::Mute(void) // Remember the current gain and use the same exact-target ramp to reach silence.
{
    gain_runtime_0 = gain_runtime;
    Set_gain(0.0f); // Avoid leaving a small residual feedback signal after muting.
}

void AudioGain::Unmute(void) // Restore the saved gain through the common ramp.
{
    Set_gain(gain_runtime_0);
}

void AudioGain::applyGain(int16_t *data, int32_t mult)
{
    /*
    dati i sample di un blocco:
    [*data, *(data+1), *(data+2),.......................,*(data + AUDIO_BLOCK_SAMPLES - 1)]

    li prelevo a coppie:
    *(uint32_t *)data == [*data,*(data+1)]

    poi moltiplico *data per mult1, *(data+1) per mult2:
    int32_t val1 = signed_multiply_32x16b(mult_2, tmp32); // computes ((a[31:0] * b[15:0]) >> 16) --> ((mult_2 x *(data+1)) >> 16)
    int32_t val2 = signed_multiply_32x16t(mult_1, tmp32); // computes ((a[31:0] * b[31:16]) >> 16) --> ((mult_1 x *(data)) >> 16)

    (indispensabile) tronco a 16bit col segno:
    val1 = signed_saturate_rshift(val1, 16, 0);
    val2 = signed_saturate_rshift(val2, 16, 0);

    impacchetto:
    *p++ = pack_16b_16b(val2, val1);
    */

    uint32_t *p = (uint32_t *)data;
    const uint32_t *end = (uint32_t *)(data + AUDIO_BLOCK_SAMPLES);
    mult_1 = mult;
    mult_2 = mult;

    do
    {
        if (gain_flag)
        {
            Get_mults();
        }
        uint32_t tmp32 = *p;                                  // read 2 samples from *data
        int32_t val1 = signed_multiply_32x16b(mult_2, tmp32); // era mult_1
        int32_t val2 = signed_multiply_32x16t(mult_1, tmp32); // era mult_2
        val1 = signed_saturate_rshift(val1, 16, 0);
        val2 = signed_saturate_rshift(val2, 16, 0);
        *p++ = pack_16b_16b(val2, val1);
    } while (p < end);
}

void AudioGain::Get_mults(void) // Advance two samples and snap the final multiplier to the requested gain.
{
    gain_runtime += gain_delta;
    multiplier = gain_runtime * 65536.0f;
    mult_1 = multiplier;
    gain_runtime += gain_delta;
    multiplier = gain_runtime * 65536.0f;
    mult_2 = multiplier;
    gain_step -= 2;

    if (gain_step <= 0)
    {
        gain_runtime = gain_target;
        multiplier = gain_target * 65536.0f;
        mult_2 = multiplier;
        gain_flag = false;
    }
}