#include "SdSoundSource.h"
/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "AudioPlayer.h"
#include <algorithm>

int16_t AudioPlayer::samples_basket[BASKET_DIM];
float AudioPlayer::resampled_basket[AUDIO_BLOCK_SAMPLES];

PlayerReadBudget::Plan AudioPlayer::Get_read_plan(bool pending, bool edited, bool reserve_modulation) const
{
    PlayerReadBudget::Plan plan;                                                                      // One geometry snapshot; source promotion is reflected immediately.
    const bool wavetable = edited ? use_Wavetable_E : (pending ? use_Wavetable_wait : use_Wavetable); // RAM tables bypass the file reader.
    const AudioFileSource &source = pending ? source_wait : source_now;                               // Pending starts can use a different source from the outgoing voice.
    const int file = pending ? file_id_wait : file_id;                                                // File class determines Live and packet behavior.
    plan.antialias = false; // Set after resolving base pitch and any queued tuning below.
    plan.live = (file >= FIRST_LIVE_SAMPLING_FILE && !SdSoundSource::Is_sd(file));
    plan.source = wavetable ? PlayerReadSource::Ram : (plan.live || source.storage == Psram ? PlayerReadSource::Psram : PlayerReadSource::Flash);
    plan.packets = !wavetable && !plan.live && source.storage == Flash && file >= FIRST_RECORDING_FILE;
    const int mode = edited ? mode_player_E : (pending ? mode_player_wait : mode_player); // Source geometry for the chosen stage.
    plan.loop = mode >= LOOP_FWD;
    plan.pingpong = mode == LOOP_FWD_REV || mode == LOOP_REV_FWD;
    plan.span = edited ? B_Flash_sample_E - A_Flash_sample_E + 1 : (pending ? B_Flash_sample_wait - A_Flash_sample_wait + 1 : B_Flash_sample - A_Flash_sample + 1);
    plan.crossfade = plan.pingpong ? 0 : (edited ? delta_Noclick_E : (pending ? delta_Noclick_wait : delta_Noclick));
    if (plan.live && mode == LOOP_FWD)
    {
        plan.crossfade = 0;
    }
    const float tune = pending ? pitch_tune_wait : (pitch_tune_flag && !Has_pending_note() ? fmaxf(pitch_tune, pitch_tune_wait) : pitch_tune); // A staged replacement tune must not be applied to the old voice.
    const float note_pitch = pending ? pitch_note_wait : pitch_note;                                                                           // Pitch before bend and vibrato.
    const float eligibility_tune = pending ? tune : (pitch_tune_flag && !Has_pending_note() ? fminf(pitch_tune, pitch_tune_wait) : pitch_tune); // Reserve filtering on either side of a queued tuning edit.
    plan.antialias = PlayerAntialias::Enabled(pending ? tone_filter_level_wait : tone_filter_level, note_pitch * eligibility_tune);
    const float ceiling = file == SdSoundSource::PREVIEW ? 8.0f : (edited ? pitch_limit_E : (pending ? pitch_limit_wait : pitch_limit));       // Actual reader ceiling, including pending edits.
    const float modulation = reserve_modulation ? PlayerReadBudget::Maximum_modulation : fmaxf(pitch_bend, pitch_bend_wait) * fmaxf(1.01f, pitch_vibrato); // Cover queued bend and the next vibrato phase without reserving unused maximum bend.
    plan.pitch = constrain(note_pitch * tune * modulation, MIN_PITCH, ceiling);
    if (!pending && !edited)
    {
        plan.pitch = fmaxf(plan.pitch, pitch); // The outgoing crossfade runs before Update_pitch and may still use the previous block's higher pitch.
    }
    return plan;
}

float AudioPlayer::Reserved_read_us(void) const
{
    if (state == IDLE)
    {
        return 0.0f;
    }
    float reserved = PlayerReadBudget::Estimate(Get_read_plan(), AUDIO_BLOCK_SAMPLES); // Never release an outgoing reservation merely because a replacement is queued.
    if (warmup_for_play_again_flag || restart_flag)
    {
        reserved = fmaxf(reserved, PlayerReadBudget::Estimate(Get_read_plan(true), AUDIO_BLOCK_SAMPLES));
    }
    if (main_settings_editing_flag)
    {
        reserved = fmaxf(reserved, PlayerReadBudget::Estimate(Get_read_plan(false, true), AUDIO_BLOCK_SAMPLES));
    }
    return reserved;
}

float AudioPlayer::Current_read_us(uint32_t output_samples) const
{
    return state == IDLE ? 0.0f : PlayerReadBudget::Estimate(Get_read_plan(false, false, false), output_samples);
}

float AudioPlayer::Next_block_read_us(void) const
{
    if (state == IDLE)
    {
        return 0.0f;
    }
    float cost = PlayerReadBudget::Estimate(Get_read_plan(false, false, false), AUDIO_BLOCK_SAMPLES);
    if (Has_pending_note())
    {
        cost = fmaxf(cost, PlayerReadBudget::Estimate(Get_read_plan(true, false, false), AUDIO_BLOCK_SAMPLES));
    }
    if (main_settings_editing_flag)
    {
        cost = fmaxf(cost, PlayerReadBudget::Estimate(Get_read_plan(false, true, false), AUDIO_BLOCK_SAMPLES));
    }
    return cost;
}

void AudioPlayer::Set_edit_mix_samples(uint8_t value)
{
    if (main_settings_editing_flag)
    {
        mix_samples = value; // Zero disables the outgoing harvest while retaining the pending geometry update.
    }
}

void AudioPlayer::Retire_for_read_budget(void)
{
    if (state == IDLE)
    {
        return;
    }
    // Reuse already rendered audio: retirement consumes no additional Flash/PSRAM/RAM-table reads.
    if (rendered_block_valid && !budget_tail_pending)
    {
        budget_tail_sample = block[AUDIO_BLOCK_SAMPLES - 1];
        budget_tail_pan_L = pan_gain_L;
        budget_tail_pan_R = pan_gain_R;
        budget_tail_pending = true;
    }
    Close_source();
    My_LED(false);
    state = IDLE;
    idle = true;
    power_on = false;
    warmup_for_play_again_flag = false;
    restart_flag = false;
    pending_note_released = false;
    main_settings_editing_flag = false;
    patch_release_pending = false;
    rendered_block_valid = false;
}

void AudioPlayer::Set_ADSR_ptr(AudioADSR *ptr)
{
    ADSR = ptr;
}

void AudioPlayer::begin(void)
{
    idle = true;
    power_on = false;
    state = IDLE;
    myLED = false;
}

void AudioPlayer::My_LED(bool on)
{
    if (on && !myLED)
    {
        if (Lilla_state == MIDI_LOOP && track < 0)
        {
            return;
        }
        led_instrument_id = instrument_id;
        led_track = track >= 0 ? track : -1; // Track ownership survives navigation to Setup and other tool pages.
        if (led_track >= 0)
        {
            Players_statistics_ptr->Inc_total_Players_per_track_instrument(led_track, led_instrument_id);
        }
        else
        {
            Players_statistics_ptr->Inc_total_Players_per_instrument(led_instrument_id);
        }
        myLED = true;
    }
    else if (!on && myLED)
    {
        if (led_track >= 0)
        {
            Players_statistics_ptr->Dec_total_Players_per_track_instrument(led_track, led_instrument_id);
        }
        else
        {
            Players_statistics_ptr->Dec_total_Players_per_instrument(led_instrument_id);
        }
        myLED = false;
    }
}

void AudioPlayer::Set_effects(float resolution_exp, uint8_t downsampling_in) // 2.0bit <= resolution_exp <= 16.0bit  1 <= downsampling_in <= 128
{
    set_effects_flag = true;
    resolution_flag_wait = resolution_exp < 15.9;
    K_resolution_step_wait = lroundf(powf(2.0f, 16.0f - resolution_exp)); //  [2.0, 16K]
    downsampling_flag_wait = downsampling_in > 1;
    downsampling_wait = downsampling_in;
}

/*
Resolution

Original resolution is 16 bits; sample values range is from -2^15 to (2^15 - 1):
sample value --> [-32768, 32767]

When resolution changes:
- range [-32,768, 32,767] is not reduced and not exceeded
- the minimum step is reduced:

resolution_exp    K_resolution_step
-----------------------------------
  16                    1 (original)
  15                    2
  14                    4
  13                    8
  12                   16
  11                   32
  10                   64
  9                   128
  8                   256
  7                   512
  6                    1K
  5                    2K
  4                    4K
  3                    8K
  2                   16K

Also not integer values can be accepted for resolution_exp.

This is how new sample value is calculated:
new_value = round(original_value / step) * step
*/

/*
Downsampling

Original sample rate is 44100 samples/second.
Downsampling is made keeping values fixed for 2 or more samples:
sampling rate = 44100 / downsampling

sampling rate     identic samples = downsampling
100%   44.1k                      1
50%   22.05k                      2
25%      11k                      4
17.5%   5.5k                      8
10%    4.41k                     10

                                   *
                          ***     * **    *
original samples:     *  *   *   *    * **
                       **      **      *
                              *

downsampling = 4      |...|...|...|...|...|


                          ****    ****
result:               ****   .    .   ****
                             .    .
                              ****

Starting from the beginning, first group AUDIO_BLOCK_SAMPLES:
- sample 0 is repeated for downsampling times
0 --> (downsampling - 1)

- than sample downsampling is repeated for downsampling times:
  downsampling --> (2 * downsampling - 1)

- when samples reaches (AUDIO_BLOCK_SAMPLES - 1) the rest is:
  rest = AUDIO_BLOCK_SAMPLES % downsampling

  and the last value used is stored.

second group AUDIO_BLOCK_SAMPLES:
- the stored value is repeated for rest samples:
  0 --> (rest - 1)

- than sample rest is repeated for downsampling times:
  rest --> (rest + downsampling - 1)

and so on.

Code:
if (downsampling_flag)
{
    rest = (sliding + sample) % downsampling;
    if (rest == 0)
    {
        stored_sample = block[sample];
    }
    else
    {
        block[sample] = stored_sample;
    }
}

*/

void AudioPlayer::Main_settings(uint8_t mode_in, int A_value_in, int B_value_in, uint16_t delta_Noclick_in, bool use_Wavetable_in, int16_t *p_Noclick_in, int16_t *p_Wavetable_in, uint8_t tables_bank_mask_in)
{
    Noclick_wait_ptr = p_Noclick_in;
    Wavetable_wait_ptr = p_Wavetable_in;
    tables_bank_mask_wait = tables_bank_mask_in;

    A_Flash_sample_wait = A_value_in;
    B_Flash_sample_wait = B_value_in;
    delta_Noclick_wait = delta_Noclick_in;
    use_Wavetable_wait = use_Wavetable_in;

    // LIVE_SAMPLING - Read samples from  PSRAM
    if ((file_id_wait >= FIRST_LIVE_SAMPLING_FILE && !SdSoundSource::Is_sd(file_id_wait)))
    {
        mode_player_wait = mode_in;
        delta_Noclick_wait = mode_in == LOOP_FWD ? 0 : delta_Noclick_in; // Live forward loops read A through B without an endpoint blend.
        use_Wavetable_wait = false;
        pitch_limit_wait = MAX_PITCH_LIVE;
        const int live_span = LS_buffer_dim - 1;

        // il codice Main deve garantire che:
        // 0<= LS_X_sample <= (LS_buffer_dim - 1)
        // 0<= LS_X_delta <= (LS_buffer_dim - 1)

        if (LS_XY_lock)
        {
            if (mode_player_wait == ONCE_FWD) // 0
            {
                A_Flash_sample_wait = LS_X_sample;
                B_Flash_sample_wait = A_Flash_sample_wait + live_span;
                a_first_sample_wait = A_Flash_sample_wait;
            }
            else if (mode_player_wait == ONCE_REV) // 1
            {
                B_Flash_sample_wait = LS_X_sample;
                A_Flash_sample_wait = B_Flash_sample_wait - live_span;
                C_Flash_sample_wait = Mirror(B_Flash_sample_wait, A_Flash_sample_wait);
                a_first_sample_wait = B_Flash_sample_wait;
            }
            else if (mode_player_wait == LOOP_FWD || mode_player_wait == LOOP_FWD_REV) // 2 loop A-->B / 3 loop A<-->B
            {
                A_Flash_sample_wait = LS_X_sample;
                B_Flash_sample_wait = LS_X_sample + LS_XY_delta;
                C_Flash_sample_wait = Mirror(B_Flash_sample_wait, A_Flash_sample_wait);
                a_first_sample_wait = A_Flash_sample_wait;
            }

            if (false)
            {
                Serial.print(F("LS_XY_lock is TRUE - mode_player_wait: "));
                Serial.print(mode_player_wait);
                Serial.print(F("  A_Flash_sample_wait: "));
                Serial.print(A_Flash_sample_wait);
                Serial.print(F("  B_Flash_sample_wait: "));
                Serial.println(B_Flash_sample_wait);
                Serial.println();
            }
        }

        // LS_X_sample va calcolato da zero utilizzando LS_X_delta
        else
        {
            LS_Q_sample = Live_Sampler->Q_sample;
            if (mode_player_wait == ONCE_FWD)
            {
                A_Flash_sample_wait = (LS_Q_sample + LS_X_delta + 1) % LS_buffer_dim;
                B_Flash_sample_wait = A_Flash_sample_wait + live_span;
                a_first_sample_wait = A_Flash_sample_wait;
            }
            else if (mode_player_wait == ONCE_REV)
            {
                B_Flash_sample_wait = LS_Q_sample + LS_X_delta - AUDIO_BLOCK_SAMPLES;
                if (B_Flash_sample_wait > (LS_buffer_dim - 1))
                {
                    B_Flash_sample_wait -= LS_buffer_dim;
                }
                A_Flash_sample_wait = B_Flash_sample_wait - live_span;
                C_Flash_sample_wait = Mirror(B_Flash_sample_wait, A_Flash_sample_wait);
                a_first_sample_wait = B_Flash_sample_wait;
            }
            else if (mode_player_wait == LOOP_FWD || mode_player_wait == LOOP_FWD_REV) // loop A-->B / loop A<-->B
            {
                A_Flash_sample_wait = LS_Q_sample + LS_X_delta - AUDIO_BLOCK_SAMPLES;
                if (A_Flash_sample_wait > (LS_buffer_dim - 1))
                {
                    A_Flash_sample_wait -= LS_buffer_dim;
                }
                B_Flash_sample_wait = A_Flash_sample_wait + LS_XY_delta;
                C_Flash_sample_wait = Mirror(B_Flash_sample_wait, A_Flash_sample_wait);
                a_first_sample_wait = A_Flash_sample_wait;
            }

            if (false)
            {
                Serial.print("Player identity: ");
                Serial.println(identity);
                Serial.print("LS_Q_sample: ");
                Serial.println(LS_Q_sample);
                Serial.print("A_Flash_sample_wait: ");
                Serial.println(A_Flash_sample_wait);
                Serial.print("LS_Q_sample - A_Flash_sample_wait: ");
                Serial.println(LS_Q_sample - A_Flash_sample_wait);
                Serial.println();
            }
        }
    }

    // modo PERFORMANCE o MIDI_LOOP
    else
    {
        // Read samples from  RAM
        if (use_Wavetable_wait)
        {
            mode_player_wait = (mode_in == LOOP_REV_FWD ? LOOP_FWD_REV : mode_in); // mode_player 4 and 3 are identical if use_Wavetabe_wait
            pitch_limit_wait = MAX_PITCH_WAVETABLE;

            switch (mode_player_wait)
            {
            case ONCE_FWD: // A-->B

                // Flash:      A-----------------B
                // Wavetable:  0-----------------(B-A)
                Flash_first_RAM_sample_wait = A_Flash_sample_wait; // indirizzo su flash corrispondente a 0 su RAM
                a_first_sample_wait = A_Flash_sample_wait;         // indirizzo flash di startup
                Wavetable_length_wait = B_Flash_sample_wait - A_Flash_sample_wait + 1;
                break;

            case ONCE_REV: // B-->A

                // Flash:      B-----------------C
                // Wavetable:  0-----------------(B-A)

                // C_Flash_sample_wait = Mirror(B_Flash_sample_wait, A_Flash_sample_wait);
                Flash_first_RAM_sample_wait = B_Flash_sample_wait; // indirizzo su flash corrispondente a 0 su RAM
                a_first_sample_wait = B_Flash_sample_wait;         // indirizzo flash di startup
                Wavetable_length_wait = B_Flash_sample_wait - A_Flash_sample_wait + 1;
                break;

            case LOOP_FWD: // loop A-->B

                // Flash + No_click(RAM=!): (A+d)----------------(B-d)(B-d+1)!!!!!!!B
                // Wavetable:                 0----------------------------------(B-A-d)
                Flash_first_RAM_sample_wait = A_Flash_sample_wait + delta_Noclick_wait; // indirizzo su flash corrispondente a 0 su RAM
                a_first_sample_wait = Flash_first_RAM_sample_wait;                      // indirizzo flash di startup
                Wavetable_length_wait = B_Flash_sample_wait - A_Flash_sample_wait - delta_Noclick_wait + 1;
                break;

            case LOOP_FWD_REV: // loop A<-->B

                // Flash:      A-----------------B (B-1)-----------------(A+1)
                // Wavetable:  0-------------------------------------- (2B-2A-1)
                Flash_first_RAM_sample_wait = A_Flash_sample_wait; // indirizzo su flash corrispondente a 0 su RAM
                a_first_sample_wait = Flash_first_RAM_sample_wait; // indirizzo flash di startup
                Wavetable_length_wait = (B_Flash_sample_wait - A_Flash_sample_wait) << 1;
                break;

            case LOOP_REV_FWD: // loop B<-->A non previsto per Wavetable: per completezza copio lo stesso codice del caso LOOP_FWD_REV

                // Flash:      A-----------------B (B-1)-----------------(A+1)
                // Wavetable:  0-------------------------------------- (2B-2A-1)
                Flash_first_RAM_sample_wait = A_Flash_sample_wait; // indirizzo su flash corrispondente a 0 su RAM
                a_first_sample_wait = Flash_first_RAM_sample_wait; // indirizzo flash di startup
                Wavetable_length_wait = (B_Flash_sample_wait - A_Flash_sample_wait) << 1;
                break;

            case LOOP_REV: // loop B-->A

                // Flash + No_click(RAM=!):  A!!!!!!!!(A+d-1)(A+d)-----------------(B-d)
                // virtual address                                                 (B-d)----------------------------------Mirror((B-d), A)
                // Wavetable:                                                       0-------------------------------------(B-d-A)
                Flash_first_RAM_sample_wait = B_Flash_sample_wait - delta_Noclick_wait; // indirizzo su flash corrispondente a 0 su RAM
                a_first_sample_wait = Flash_first_RAM_sample_wait;                      // indirizzo flash di startup
                Wavetable_length_wait = B_Flash_sample_wait - delta_Noclick_wait - A_Flash_sample_wait + 1;
                break;

            default: // case LOOP_REV_FWD (4) non e' previsto per la wavetable
                break;
            }
        }

        // Read samples from FLASH chip
        else
        {
            pitch_limit_wait = Sample_pitch_limit(source_wait.storage == Psram);

            switch (mode_in)
            {
            case ONCE_FWD: // A-->B
                a_first_sample_wait = A_Flash_sample_wait;
                mode_player_wait = mode_in;
                break;
            case ONCE_REV: // B-->A
                C_Flash_sample_wait = Mirror(B_Flash_sample_wait, A_Flash_sample_wait);
                a_first_sample_wait = B_Flash_sample_wait;
                mode_player_wait = mode_in;
                break;
            case LOOP_FWD: // loop A-->B A-->B
                a_first_sample_wait = A_Flash_sample_wait;
                mode_player_wait = mode_in;
                break;
            case LOOP_FWD_REV: // loop A-->B B-->A
                C_Flash_sample_wait = Mirror(B_Flash_sample_wait, A_Flash_sample_wait);
                a_first_sample_wait = A_Flash_sample_wait;
                mode_player_wait = mode_in;
                break;
            case LOOP_REV_FWD: // loop B-->A A-->B
                C_Flash_sample_wait = Mirror(B_Flash_sample_wait, A_Flash_sample_wait);
                a_first_sample_wait = B_Flash_sample_wait;
                mode_player_wait = 3;
                break;
            case LOOP_REV: // loop B-->A B-->A
                B_Flash_sample_shifted_wait = B_Flash_sample_wait - delta_Noclick_wait;
                C_Flash_sample_wait = Mirror(B_Flash_sample_shifted_wait, A_Flash_sample_wait);
                a_first_sample_wait = B_Flash_sample_shifted_wait;
                mode_player_wait = mode_in;
                break;

            default:
                break;
            }
        }
    }
}

void AudioPlayer::Main_settings_editing(uint8_t mode_in, int A_value_in, int B_value_in, uint16_t delta_Noclick_in, bool use_Wavetable_in, int16_t *p_Noclick_in, int16_t *p_Wavetable_in, uint8_t tables_bank_mask_in)
{
    mode_player_E = (mode_in == LOOP_REV_FWD ? LOOP_FWD_REV : mode_in); // switching to mode 4 is ininfluent WHILE playing (besides, mode 4 does NOT exist in harvest functions)
    A_Flash_sample_E = A_value_in;
    B_Flash_sample_E = B_value_in;
    delta_Noclick_E = delta_Noclick_in;
    use_Wavetable_E = use_Wavetable_in;
    Noclick_E_ptr = p_Noclick_in;
    Wavetable_E_ptr = p_Wavetable_in;
    tables_bank_mask_E = tables_bank_mask_in;

    // Read samples from PSRAM chip
    if ((file_id >= FIRST_LIVE_SAMPLING_FILE && !SdSoundSource::Is_sd(file_id)))
    {
        mode_player_E = mode_in;
        delta_Noclick_E = mode_in == LOOP_FWD ? 0 : delta_Noclick_in; // Keep edited loop geometry consistent with the live reader.
        pitch_limit_E = MAX_PITCH_LIVE;
        use_Wavetable_E = false;
        const int live_span = LS_buffer_dim - 1;

        if (LS_XY_lock)
        {
            if (mode_player_E == ONCE_FWD)
            {
                A_Flash_sample_E = LS_X_sample;
                B_Flash_sample_E = A_Flash_sample_E + live_span;
            }
            else if (mode_player_E == ONCE_REV)
            {
                B_Flash_sample_E = LS_X_sample;
                A_Flash_sample_E = B_Flash_sample_E - live_span;
            }
            else if (mode_player_E == LOOP_FWD || mode_player_E == LOOP_FWD_REV)
            {
                A_Flash_sample_E = LS_X_sample;
                B_Flash_sample_E = LS_X_sample + LS_XY_delta;
            }
        }

        else
        {
            if (mode_player_E == ONCE_FWD)
            {
                A_Flash_sample_E = LS_Q_sample + LS_X_delta - AUDIO_BLOCK_SAMPLES;
                if (A_Flash_sample_E > LS_buffer_dim - 1)
                {
                    A_Flash_sample_E -= LS_buffer_dim;
                }
                B_Flash_sample_E = A_Flash_sample_E + live_span;
            }
            else if (mode_player_E == ONCE_REV)
            {
                B_Flash_sample_E = LS_Q_sample + LS_X_delta - AUDIO_BLOCK_SAMPLES;
                if (B_Flash_sample_E > LS_buffer_dim - 1)
                {
                    B_Flash_sample_E -= LS_buffer_dim;
                }
                A_Flash_sample_E = B_Flash_sample_E - live_span;
            }
            else if (mode_player_E == LOOP_FWD || mode_player_E == LOOP_FWD_REV)
            {
                A_Flash_sample_E = LS_Q_sample + LS_X_delta - AUDIO_BLOCK_SAMPLES;
                if (A_Flash_sample_E > LS_buffer_dim - 1)
                {
                    A_Flash_sample_E -= LS_buffer_dim;
                }
                B_Flash_sample_E = A_Flash_sample_E + LS_XY_delta;
            }
        }

        if (false)
        {
            Serial.print(F("mode_player_E: "));
            Serial.print(mode_player_E);
            Serial.print(F("  A_Flash_sample_E: "));
            Serial.print(A_Flash_sample_E);
            Serial.print(F("  a_first_sample: "));
            Serial.print(a_first_sample);
            Serial.print(F("  B_Flash_sample_E: "));
            Serial.println(B_Flash_sample_E);
            Serial.println();
        }
    }

    // Read samples from RAM
    if (use_Wavetable_E)
    {
        switch (mode_player_E)
        {
        case 0: // A-->B

            // Flash:      A-----------------B
            // Wavetable:  0-----------------(B-A)
            Flash_first_RAM_sample_E = A_Flash_sample_E; // address on Flash corresponding to address 0 on Wavetable
            Wavetable_length_E = B_Flash_sample_E - A_Flash_sample_E + 1;
            break;

        case 1: // B-->A

            // Flash:      B-----------------C
            // Wavetable:  0-----------------(B-A)

            Flash_first_RAM_sample_E = B_Flash_sample_E; // address on Flash corresponding to address 0 on Wavetable
            Wavetable_length_E = B_Flash_sample_E - A_Flash_sample_E + 1;
            break;

        case 2: // loop A-->B

            // Flash + No_click(RAM=!): (A+d)----------------(B-d)(B-d+1)----B
            // Wavetable:                 0----------------------------------(B-A-d)
            Flash_first_RAM_sample_E = A_Flash_sample_E + delta_Noclick_E; // address on flash corresponding to address 0 on Wavetable
            Wavetable_length_E = B_Flash_sample_E - A_Flash_sample_E - delta_Noclick_E + 1;
            break;

        case 3: // loop A-->B-->A

            // Flash:      A-----------------B (B-1)-----------------(A+1)
            // Wavetable:  0-------------------------------------- (2B-2A-1)
            Flash_first_RAM_sample_E = A_Flash_sample_E; // address on flash corresponding to address 0 on Wavetable
            Wavetable_length_E = (B_Flash_sample_E - A_Flash_sample_E) << 1;
            break;

        case 5: // loop B-->A B-->A

            // Flash + No_click(RAM=!):  A--------(A+d-1)(A+d)-----------------(B-d)
            // virtual address                                                 (B-d)----------------------------------Mirror((B-d), A)
            // Wavetable:                                                       0-------------------------------------(B-A-d)
            Flash_first_RAM_sample_E = B_Flash_sample_E - delta_Noclick_E; // address on flash corresponding to address 0 on Wavetable
            Wavetable_length_E = B_Flash_sample_E - delta_Noclick_E - A_Flash_sample_E + 1;
            break;

        default:
            break;
        }

        pitch_limit_E = MAX_PITCH_WAVETABLE;
    }

    // Read samples from FLASH chip
    else
    {
        switch (mode_player_E)
        {
        case 0: // once A-->B
            break;
        case 1: // once B-->A
            C_Flash_sample_E = Mirror(B_Flash_sample_E, A_Flash_sample_E);
            break;
        case 2: // loop A-->B A-->B
            break;
        case 3: // loop A-->B-->A loop B-->A-->B
            C_Flash_sample_E = Mirror(B_Flash_sample_E, A_Flash_sample_E);
            break;
        case 5: // loop B-->A B-->A
            B_Flash_sample_shifted_E = B_Flash_sample_E - delta_Noclick_E;
            C_Flash_sample_E = Mirror(B_Flash_sample_shifted_E, A_Flash_sample_E);
            break;
        default:
            break;
        }
        pitch_limit_E = Sample_pitch_limit(source_now.storage == Psram);
    }

    main_settings_editing_flag = true;
}

void AudioPlayer::Get_ready_to_play(float pitch_note_in, float velocity_in, int patch_in, uint8_t instrument_in, uint16_t sound_id_in, uint8_t note_in)
{
    pending_note_released = false;
    pitch_note_wait = pitch_note_in;
    velocity_gain_wait = velocity_in;
    patch_id_wait = patch_in;
    instrument_id_wait = instrument_in;
    sound_id_wait = sound_id_in;
    note_wait = note_in;
    time_stamp = millis();

    power_on = true;

    if (state == IDLE)
    {
        warmup_for_play_again_flag = false;
        restart_flag = false;
        main_settings_editing_flag = false;
        Start_playing();
    }
    else
    {
        warmup_for_play_again_flag = true;
    }
}

void AudioPlayer::Start_playing(void)
{
    char packet_filename[NAME_PACKET_SIZE];
    live_forward_last_sample = 0;
    live_start_pending = true;
    live_restart_pending = false;
    live_fade_out = false;
    live_fade_in = false;
    patch_release_pending = false; // A replacement note must not inherit the previous patch deadline.
    Close_source();
    source_now = source_wait;
    file_id = file_id_wait;
    if (source_now.storage == Flash && file_id < FIRST_LIVE_SAMPLING_FILE)
    {
        AudioStartUsingSPI();
        spi_in_use = true;
    }
    recording_flag = false;
    LS_flag = false;

    // A complete cache already contains the logical samples of either a RAW or a REC file.
    if (source_now.storage == Psram)
    {
        // No Flash handle is needed; edits continue to use this immutable source.
    }
    else if (file_id < FIRST_RECORDING_FILE)
    {
        rawfile.fast_open(file_id); // rawfile = SerialFlash.open(filename); // open file
    }

    // .rec file from DIRECT_SAMPLING
    else if (file_id < FIRST_LIVE_SAMPLING_FILE)
    {
        recording_flag = true;
        recording = (file_id - FIRST_RECORDING_FILE) / 2;
        stereo_flag = Recording[recording].stereo;

        if (!stereo_flag)
            first_packet = Recording[recording].first_packet; // solo questo file contiene dati
        else
        {
            if ((file_id - FIRST_RECORDING_FILE) % 2 == 0) // file LEFT
            {
                first_packet = Recording[recording].first_packet;
            }
            else // file RIGHT
            {
                first_packet = Recording[recording].first_packet + 1;
            }
        }

        packet_delta = 0;
        rawfile.packet_fast_open(first_packet);

        if (false)
        {
            Serial.print("First packet played is: ");
            Serial.println(first_packet);
            Serial.println(Get_packet_name(first_packet, packet_filename));
        }
    }

    // Mono.liv or Left.liv or Right.liv files from LIVE_SAMPLING
    else
    {
        LS_flag = true;
        if (file_id == FIRST_LIVE_SAMPLING_FILE)
        {
            FIFO = LS_buffer_mono_ptr;
        }
        else if (file_id == FIRST_LIVE_SAMPLING_FILE + 1)
        {
            FIFO = LS_buffer_L_ptr;
        }
        else
        {
            FIFO = LS_buffer_R_ptr;
        }
    }

    if (!rawfile)
    {
        Close_source();
    }

    // Main_settings synchronization

    // Serial.print("AudioPlayer::Start_playing(void) - identity: ");
    // Serial.println(identity);

    ADSR->Set_parametrs();

    volume_gain = volume_gain_wait;
    pan_int = pan_int_wait;
    pan_flag = false;
    pan_gain_warmup_flag = false;
    pan_gain_L = pan_gain_L_wait;
    pan_gain_R = pan_gain_R_wait;
    Noclick_ptr = Noclick_wait_ptr;
    Wavetable_ptr = Wavetable_wait_ptr;
    tables_bank_mask = tables_bank_mask_wait;

    A_Flash_sample = A_Flash_sample_wait;
    B_Flash_sample = B_Flash_sample_wait;
    delta_Noclick = delta_Noclick_wait;
    mode_player = mode_player_wait;
    use_Wavetable = use_Wavetable_wait;
    Flash_first_RAM_sample = Flash_first_RAM_sample_wait;
    a_first_sample = a_first_sample_wait;
    Wavetable_length = Wavetable_length_wait;
    pitch_limit = pitch_limit_wait;
    C_Flash_sample = C_Flash_sample_wait;
    B_Flash_sample_shifted = B_Flash_sample_shifted_wait;
    initial_index_offset = 0;
    samples_counter = 0;
    local_patch = patch_id_wait;

    // Release the previous registration before changing ownership, including zero-mix restarts.
    My_LED(false);

    instrument_id = instrument_id_wait;
    sound_id = sound_id_wait;
    note = note_wait;
    pitch_tune = pitch_tune_wait;
    pitch_tune_flag = false;
    pitch_bend = pitch_bend_wait;
    pitch_bend_flag = false;
    pitch_note = pitch_note_wait;
    tone_filter_level = tone_filter_level_wait;
    tone_filter.Reset();
    velocity_gain = velocity_gain_wait;
    volume_flag = false;
    sliding = 0;
    track = track_wait;
    Start_VCF();

    idle = false;
    state = RUNNING;
    My_LED(true);
    if (pending_note_released)
    {
        pending_note_released = false;
        ADSR->Release_note();
        power_on = false;
        state = FADING;
        My_LED(false);
    }
}

void AudioPlayer::Enforce_cycle_deadline(void)
{
    if (state == IDLE || audio_update_time_micros < AUDIO_PLAYER_DEADLINE_US)
    {
        return;
    }
    if (audio_player_emergency_stops != UINT32_MAX)
    {
        audio_player_emergency_stops = audio_player_emergency_stops + 1;
    }
    idle = true;
    power_on = false;
    state = IDLE;
    warmup_for_play_again_flag = false;
    restart_flag = false;
    pending_note_released = false;
    main_settings_editing_flag = false;
    patch_release_pending = false;
    Close_source();
    My_LED(false);
}

void AudioPlayer::Release_note(void) // release note, fires ADSR "release"
{
    if (Has_pending_note())
    {
        pending_note_released = true;
        power_on = false;
        time_stamp = millis();
        return;
    }
    if (state == IDLE)
    {
        return;
    }

    ADSR->Release_note();
    time_stamp = millis();
    power_on = false;
    state = FADING;
    My_LED(false);
}

void AudioPlayer::Release_patch(int patch_id) // Apply one non-renewable deadline to the outgoing voice and discard its queued restart.
{
    if ((warmup_for_play_again_flag || restart_flag) && patch_id_wait == patch_id)
    {
        warmup_for_play_again_flag = false;
        restart_flag = false;
    }
    if (state == IDLE || local_patch != patch_id || patch_release_pending)
    {
        return;
    }
    patch_release_started = millis();
    patch_release_pending = true;
    ADSR->Limit_release((QUICK_RELEASE_TIME - 2u * PATCH_RELEASE_BLOCK_MS) / 1000.0f); // Leave two audio blocks of margin for envelope completion and voice cleanup.
    power_on = false;
    if (state != IDLE_REQUEST)
    {
        state = FADING;
    }
    My_LED(false);
}

void AudioPlayer::Update_pitch(void)
{
    if (live_restart_pending)
    {
        return; // Keep the fade-out pitch until the booked fade-in has been rendered.
    }
    pitch = constrain(pitch_note * pitch_bend * pitch_tune * pitch_vibrato, MIN_PITCH, file_id == SdSoundSource::PREVIEW ? fminf(pitch_limit, 8.0f) : pitch_limit);

    // **** Softness Filter ****
    pitch_based_gain_correction = (pitch <= 1.0 ? 1.0 : 24.0f / (pitch - 1.0f + 24.0f));
}

void AudioPlayer::update(void)
{
    if (budget_tail_pending)
    {
        audio_block_t *tail_L = allocate(); // Cached release uses the normal output channels, without touching the new voice.
        audio_block_t *tail_R = allocate(); // Keep the tail pending if either output allocation fails.
        if (tail_L == nullptr || tail_R == nullptr)
        {
            if (tail_L != nullptr)
            {
                release(tail_L);
            }
            if (tail_R != nullptr)
            {
                release(tail_R);
            }
            return;
        }
        for (uint32_t sample = 0; sample < AUDIO_BLOCK_SAMPLES; ++sample)
        {
            const float gain = static_cast<float>(AUDIO_BLOCK_SAMPLES - 1u - sample) / (AUDIO_BLOCK_SAMPLES - 1u); // One-block ramp from the last output sample to silence, with no replay discontinuity.
            tail_L->data[sample] = budget_tail_sample * gain * budget_tail_pan_L;
            tail_R->data[sample] = budget_tail_sample * gain * budget_tail_pan_R;
        }
        transmit(tail_L, 0);
        transmit(tail_R, 1);
        release(tail_L);
        release(tail_R);
        budget_tail_pending = false;
        return; // A replacement begins reading on the following block.
    }
    if (patch_release_pending && static_cast<uint32_t>(millis() - patch_release_started) >= QUICK_RELEASE_TIME - PATCH_RELEASE_BLOCK_MS)
    {
        patch_release_pending = false;
        if (warmup_for_play_again_flag || restart_flag)
        {
            Start_playing(); // Preserve a replacement note queued after the patch change.
            warmup_for_play_again_flag = false;
            restart_flag = false;
            main_settings_editing_flag = false;
            power_on = true;
        }
        else
        {
            idle = true;
            power_on = false;
            state = IDLE;
            main_settings_editing_flag = false;
            Close_source(); // Release Flash ownership before allocation, even when audio buffers are exhausted.
            My_LED(false);
        }
    }
    Enforce_cycle_deadline(); // Check before allocating buffers or harvesting audio; never reset the shared clock here.

    if (state == IDLE)
    {
        return; // Routers treat missing input blocks as silence; no output allocation is needed.
    }

    audio_block_t *block_L, *block_R;
    float F_basket_sample;
    float rest;
    uint8_t vibrato_array_element;
    float a_first_sample_real;
    float a_first_sample_E = 0;
    float value_float;

    bool snubber_flag = false;
    bool mix_flag = false;
    bool shoot_flag = false;

    block_L = allocate(); // allocate the audio blocks to transmit
    if (block_L == NULL)
    {
        return;
    }

    block_R = allocate(); // allocate the audio blocks to transmit
    if (block_R == NULL)
    {
        release(block_L); // Return the first allocation when the second output buffer is unavailable.
        return;
    }

    // Render the active voice.
    {
        if (warmup_for_play_again_flag)
        {
            if (mix_samples > 0)
            {
                // harvest the last set of samples from the actual file
                a_sample = a_first_sample;
                initial_index_offset = a_sample - floor(a_sample);
                b_sample = a_sample + (pitch * (mix_samples - 1));

                Harvest_samples(mix_samples);

                float volume_tmp = volume_gain * velocity_gain * ADSR_gain * pitch_based_gain_correction;

                for (uint8_t i = 0; i < mix_samples; ++i)
                {
                    F_basket_sample = (i * pitch) + initial_index_offset; // "initial_index_offset" may be modified in harvest function
                    raw_first_value_cache[i] = volume_tmp * Read_rendered_sample(i, F_basket_sample);
                }
                for (uint8_t i = mix_samples; i < AUDIO_BLOCK_SAMPLES; ++i)
                {
                    raw_first_value_cache[i] = 0;
                }

                Start_playing();
                mix_flag = true;
                shoot_flag = true;
            }

            else
            {
                restart_flag = true;
            }

            main_settings_editing_flag = false;
            warmup_for_play_again_flag = false;
        }

        if (!live_restart_pending)
        {
            if (pitch_tune_flag && !restart_flag) // A zero-mix restart must render the outgoing block with its original tuning.
            {
                pitch_tune = pitch_tune_wait;
                pitch_tune_flag = false;
            }

            if (pitch_bend_flag)
            {
                pitch_bend = pitch_bend_wait;
                pitch_bend_flag = false;
            }

            if (vibrato_flag)
            {
                vibrato_array_element_float += VIBRATO_STEP;
                vibrato_array_element = ((uint8_t)vibrato_array_element_float) % 32;
                *vibrato_array_last_element_ptr = vibrato_array_element;
                pitch_vibrato = 1.0f + (*(vibrato_array_ptr + vibrato_array_element) - 1.0f) * modulation_depth;
            }
        }

        Update_pitch();

        if (main_settings_editing_flag)
        {

            if (mode_player_E != mode_player) // calculating a_first_sample_E if mode_player has been changed
            {
                if (mode_player == ONCE_FWD && mode_player_E == ONCE_REV)
                {
                    //   A---------------------a--------B  0: once A-->B
                    //   AE||||||||||||||||||||a'|||||||BE-------aE------------------CE 1: once B-->A
                    a_first_sample_E = Mirror(B_Flash_sample_E, a_first_sample); // a_first_sample_E is virtual
                    // Serial.println("case M1");
                }
                else if (mode_player == ONCE_REV && mode_player_E == ONCE_FWD)
                {
                    //   A----------------------a'-------B-------a---------------------C 1: once B-->A
                    //   AE---------------------aE-------BE  0: once A-->B
                    a_first_sample_E = Mirror(B_Flash_sample, a_first_sample); // a_first_sample_E is real
                    // Serial.println("case M2");
                }
                else if (mode_player == ONCE_REV && mode_player_E == LOOP_FWD)
                {
                    a_first_sample_real = Mirror(B_Flash_sample, a_first_sample);

                    //    A--a'---------------------------B------------------------a----C 1: once B-->A
                    //            (AE+dE)-----------------BE    2: loop A-->B A-->B
                    if (floor(a_first_sample_real) < (A_Flash_sample_E + delta_Noclick_E))
                    {
                        a_first_sample_E = A_Flash_sample_E + delta_Noclick_E;
                        // Serial.println("case M3");
                    }

                    //    A---------------------a'--------B--------a--------------------C 1: once B-->A
                    //            (AE+dE)-------aE--------BE    2: loop A-->B A-->B
                    else
                    {
                        a_first_sample_E = a_first_sample_real;
                        // Serial.println("case M4");
                    }
                    snubber_flag = true; // the landing pool of samples is DIFFERENT because here there is Noclick table
                }
                else if (mode_player == LOOP_FWD && mode_player_E == ONCE_REV)
                {
                    //            (A+d)---a---------------B                                   2: loop A-->B A-->B
                    //    AE||||||||||||||a'||||||||||||||BE--------------aE-------------CE   1: once B-->A
                    a_first_sample_E = Mirror(B_Flash_sample_E, a_first_sample); // a_first_sample_E is virtual
                    snubber_flag = true;                                         // the landing pool of samples is DIFFERENT because here there is NOT Noclick table
                    // Serial.println("case M5");
                }
                else if (mode_player == LOOP_FWD && mode_player_E == LOOP_FWD_REV)
                {
                    //            (A+d)--------a----------B                                  2: loop A-->B A-->B
                    //    AE-------------------aE---------BE-----------------------------C   3: loop A-->B-->A
                    a_first_sample_E = a_first_sample;
                    snubber_flag = true; // the landing pool of samples is DIFFERENT because here there is NOT Noclick table
                    // Serial.println("case M6");
                }
                else if (mode_player == LOOP_FWD_REV && mode_player_E == LOOP_FWD)
                {
                    //    A--------------------------------B----------a------------------C   3: loop A-->B-->A
                    //            (AE+dE)------------------BE                                2: loop A-->B A-->B
                    if (floor(a_first_sample) > B_Flash_sample)
                    {
                        a_first_sample_real = Mirror(B_Flash_sample, a_first_sample);

                        //    A----a'--------------------------B--------------------------a--C   3: loop A-->B-->A
                        //            (AE+dE)------------------BE                                2: loop A-->B A-->B
                        if (floor(a_first_sample_real) < (A_Flash_sample_E + delta_Noclick_E))
                        {
                            a_first_sample_E = A_Flash_sample_E + delta_Noclick_E;
                            // Serial.println("case M7");
                        }

                        //    A----------------------a'--------B--------a-----------------a--C   3: loop A-->B-->A
                        //            (AE+dE)--------aE--------BE                                2: loop A-->B A-->B
                        else
                        {
                            a_first_sample_E = a_first_sample_real;
                            // Serial.println("case M8");
                        }
                        snubber_flag = true; // the landing pool of samples is DIFFERENT because here there is Noclick table
                    }

                    //   A-------------------a------------B-----------------------------C  3: loop A-->B-->A
                    //           (AE+dE)------------------BE    2: loop A-->B A-->B
                    else if (floor(a_first_sample) <= B_Flash_sample)
                    {

                        //   A--a-----------------------------B-----------------------------C  3: loop A-->B-->A
                        //           (AE+dE)------------------BE    2: loop A-->B A-->B
                        if (floor(a_first_sample) < (A_Flash_sample_E + delta_Noclick_E))
                        {
                            a_first_sample_E = A_Flash_sample_E + delta_Noclick_E;
                            // Serial.println("case M9");
                        }

                        //   A--------------------a-----------B-----------------------------C  3: loop A-->B-->A
                        //           (AE+dE)------aE----------BE    2: loop A-->B A-->B
                        else
                        {
                            a_first_sample_E = a_first_sample;
                            // Serial.println("case M10");
                        }
                        snubber_flag = true; // the landing pool of samples is DIFFERENT because here there is Noclick table
                    }
                }

                else if (mode_player == LOOP_FWD_REV && mode_player_E == LOOP_REV)
                {
                    a_first_sample_real = Mirror(B_Flash_sample, a_first_sample);

                    //    A-------a'--------------------------------B-------------------a----------------------C     3: loop A-->B B-->A
                    //    AE||||||aE'||||||||||||||||||(BE-dE)---------------------aE-----------Mirror((BE-dE), AE)  5: loop B-->A B-->A
                    if (floor(a_first_sample_real) <= (B_Flash_sample_E - delta_Noclick_E))
                    {
                        a_first_sample_E = Mirror(B_Flash_sample_E - delta_Noclick_E, a_first_sample_real);
                        // Serial.println("case M13");
                    }

                    //    A------------------------------------a'---B---a--------------------------------------C     3: loop A-->B B-->A
                    //    A||||||||||||||||||||||||||||(BE-dE)----------------------------------Mirror((BE-dE), AE)  5: loop B-->A B-->A
                    else
                    {
                        a_first_sample_E = B_Flash_sample_E - delta_Noclick_E;
                        // Serial.println("case M14");
                    }
                    snubber_flag = true; // the landing pool of samples is DIFFERENT because here there is Noclick table
                }

                else if (mode_player == LOOP_REV && mode_player_E == LOOP_FWD_REV)
                {
                    a_first_sample_real = Mirror(B_Flash_sample - delta_Noclick, a_first_sample);

                    //    A||||||||a'|||||||||||||||||||(B-d)------------------------a--Mirror((B-d), A)  5: loop B-->A B-->A
                    //    AE-------aE'-----------------------BE--------------------------aE-------CE      3: loop A-->B B-->A
                    a_first_sample_E = Mirror(B_Flash_sample_E, a_first_sample_real);
                    snubber_flag = true; // the landing pool of samples is DIFFERENT because here there is NOT Noclick table
                    // Serial.println("case M15");
                }
            }

            else // calculating a_first_sample_E if A_Flash_sample or B_Flash_sample has been changed
            {
                a_first_sample_E = a_first_sample;

                /*
                if(use_Wavetable_E && mode_player_E == 0) // once A-->B
                {
                    //   a-----AE---------------------BE
                    if(floor(a_first_sample) < A_Flash_sample_E)
                    {
                        a_first_sample_E = A_Flash_sample_E;
                        snubber_flag = true;
                    }

                    //         AE---------------------BE-----a
                    else if(floor(a_first_sample) >= B_Flash_sample_E)
                    {
                        a_first_sample_E = B_Flash_sample_E;
                    }
                }
                */

                if (mode_player_E == ONCE_FWD) // once A-->B
                {
                    //   a-----AE---------------------BE
                    if (floor(a_first_sample) < A_Flash_sample_E)
                    {
                        a_first_sample_E = A_Flash_sample_E;
                        snubber_flag = true;
                    }

                    //         AE---------------------BE-----a
                    else if (floor(a_first_sample) >= B_Flash_sample_E)
                    {
                        a_first_sample_E = B_Flash_sample_E;
                    }
                }

                else if (mode_player_E == ONCE_REV) // once B-->A
                {
                    a_first_sample_real = Mirror(B_Flash_sample, a_first_sample); // a'

                    // AE---------a'------------BE
                    if (floor(a_first_sample_real) <= B_Flash_sample_E && floor(a_first_sample_real) > A_Flash_sample_E)
                    {
                        a_first_sample_E = Mirror(B_Flash_sample_E, a_first_sample_real);
                        snubber_flag = true;
                    }

                    // out of range AE-----------------BE
                    else
                    {
                        a_first_sample_E = B_Flash_sample_E;
                    }
                }

                if (mode_player_E == LOOP_FWD) // loop A-->B A-->B
                {
                    if (use_Wavetable_E)
                    {
                        //   a********.(AE+dE)---------------------BE
                        if (floor(a_first_sample) < Flash_first_RAM_sample_E)
                        {
                            a_first_sample_E = Flash_first_RAM_sample_E;
                            snubber_flag = true;
                            // Serial.println("Wavetable! - case a loop A-->B A-->B");
                        }

                        //   .........(AE+dE)--------------------BE......a*******
                        else if (floor(a_first_sample) >= B_Flash_sample_E)
                        {
                            a_first_sample_E = Flash_first_RAM_sample_E;
                            snubber_flag = true;
                            // Serial.println("Wavetable! - case b loop A-->B A-->B");
                        }
                    }

                    else
                    {
                        //   a******...AE.....(AE+dE-1)(AE+dE)----------------(BE-dE)(BE-dE+1)----BE........
                        if (floor(a_first_sample) < A_Flash_sample_E)
                        {
                            a_first_sample_E = A_Flash_sample_E;
                            // Serial.println("NoWavetable - case a loop A-->B A-->B");
                        }

                        //  ........AE.......(AE+dE-1)(AE+dE)----------------(BE-dE)(BE-dE+1)----BE...a********
                        else if (floor(a_first_sample) >= B_Flash_sample_E)
                        {
                            a_first_sample_E = B_Flash_sample_E;
                            // Serial.println("NoWavetable - case b loop A-->B A-->B");
                        }
                        snubber_flag = true;
                    }
                }

                else if (mode_player_E == LOOP_FWD_REV) // loop A-->B-->A
                {

                    // direction is REVERSE
                    // A----------------------B-----a*********
                    if (floor(a_first_sample) >= B_Flash_sample)
                    {
                        a_first_sample_real = Mirror(B_Flash_sample, a_first_sample); // a'

                        // AE----*****a'------------BE
                        if (ceil(a_first_sample_real) <= B_Flash_sample_E && ceil(a_first_sample_real) > A_Flash_sample_E)
                        {
                            a_first_sample_E = Mirror(B_Flash_sample_E, a_first_sample_real);
                            snubber_flag = true;
                            // Serial.println("case a");
                        }

                        // a'****-AE-----------------BE
                        else if (ceil(a_first_sample_real) < A_Flash_sample_E)
                        {
                            a_first_sample_E = A_Flash_sample_E;
                            snubber_flag = true;
                            // Serial.println("case b");
                        }

                        // AE-----------------BE---a'
                        else if (ceil(a_first_sample_real) > B_Flash_sample_E)
                        {
                            a_first_sample_E = B_Flash_sample_E;
                            snubber_flag = true;
                            // Serial.println("case d");
                        }
                    }

                    // direction is FORWARD
                    // A--------------a*****--B
                    else
                    {
                        //  AE--------a*****--BE
                        if (floor(a_first_sample) >= A_Flash_sample_E && floor(a_first_sample) <= B_Flash_sample_E)
                        {
                            a_first_sample_E = a_first_sample;
                            // Serial.println("case e");
                        }
                        //  AE-------------------BE---a
                        if (floor(a_first_sample) > B_Flash_sample_E)
                        {
                            a_first_sample_E = B_Flash_sample_E;
                            snubber_flag = true;
                            // Serial.println("case e");
                        }

                        //  a*****-AE-------------------BE
                        else if (floor(a_first_sample) < A_Flash_sample_E)
                        {
                            if (use_Wavetable_E)
                            {
                                a_first_sample_E = A_Flash_sample_E;
                                snubber_flag = true;
                                // Serial.println("case f");
                            }
                        }
                    }
                }

                else if (mode_player_E == LOOP_REV) // loop B-->A B-->A
                {
                    if (use_Wavetable_E)
                    {
                        a_first_sample_real = Mirror(Flash_first_RAM_sample, a_first_sample); // a'

                        // a' MUST be between A_Flash_sample_E and (B_Flash_sample_E - delta_Noclick_E)
                        // ...........AE--<<<<<<<<a'-------------------(BE-dE)..........
                        if (ceil(a_first_sample_real) > A_Flash_sample_E && ceil(a_first_sample_real) <= Flash_first_RAM_sample_E)
                        {
                            a_first_sample_E = Mirror(Flash_first_RAM_sample_E, a_first_sample_real);
                            snubber_flag = true;
                            // Serial.println("Wavetable - case g loop B-->A B-->A");
                        }

                        // out of range
                        else
                        {
                            a_first_sample_E = Flash_first_RAM_sample_E;
                            snubber_flag = true;
                            // Serial.println("Wavetable - case h loop B-->A B-->A");
                        }
                    }

                    else
                    {
                        a_first_sample_real = Mirror(B_Flash_sample_shifted, a_first_sample); // a'

                        // a' MUST be between A_Flash_sample_E and B_Flash_sample_shifted_E
                        // ............AE--<<<<<<<a'----------------------B_5E........
                        if (ceil(a_first_sample_real) > A_Flash_sample_E && ceil(a_first_sample_real) <= B_Flash_sample_shifted_E)
                        {
                            a_first_sample_E = Mirror(B_Flash_sample_shifted_E, a_first_sample_real);
                            snubber_flag = true;
                            // Serial.println("NoWavetable - case i loop B-->A B-->A");
                        }

                        // out of range
                        else
                        {
                            a_first_sample_E = B_Flash_sample_shifted_E;
                            snubber_flag = true;
                            // Serial.println("NoWavetable - case j loop B-->A B-->A");
                        }
                    }
                }
            }

            // read HALF of samples from the old segment and old pitch and old gain
            if (snubber_flag && mix_samples > 0)
            {
                a_sample = a_first_sample; // original a_first_sample
                initial_index_offset = a_sample - floor(a_sample);
                b_sample = a_sample + pitch * (mix_samples - 1); // read half samples

                Harvest_samples(mix_samples);

                float volume_tmp = volume_gain * velocity_gain * ADSR_gain * pitch_based_gain_correction;

                // mix_samples < AUDIO_BLOCK_SAMPLES
                for (auto sample = 0; sample < mix_samples; ++sample)
                {
                    F_basket_sample = (sample * pitch) + initial_index_offset; // "initial_index_offset" may be modified in harvest function
                    raw_first_value_cache[sample] = volume_tmp * Read_rendered_sample(sample, F_basket_sample);

                    // Serial.print(sample);
                    // PRINT("raw_first",": ", raw_first_value_cache[sample]);
                }

                for (auto sample = mix_samples; sample < AUDIO_BLOCK_SAMPLES; ++sample)
                {
                    raw_first_value_cache[sample] = 0;

                    // Serial.print(sample);
                    // PRINT("raw_first",": ", raw_first_value_cache[sample]);
                }

                snubber_flag = false;
                mix_flag = true;
            }

            pitch_limit = pitch_limit_E;
            Update_pitch();

            if (mode_player != mode_player_E || A_Flash_sample != A_Flash_sample_E || B_Flash_sample != B_Flash_sample_E)
            {
                live_restart_pending = false; // A booked phase belongs to the previous loop geometry.
            }
            mode_player = mode_player_E;
            a_first_sample = a_first_sample_E;
            delta_Noclick = delta_Noclick_E;
            Wavetable_ptr = Wavetable_E_ptr;
            Noclick_ptr = Noclick_E_ptr;
            tables_bank_mask = tables_bank_mask_E;
            use_Wavetable = use_Wavetable_E;
            A_Flash_sample = A_Flash_sample_E;
            B_Flash_sample = B_Flash_sample_E;

            if (use_Wavetable)
            {
                Flash_first_RAM_sample = Flash_first_RAM_sample_E;
                Wavetable_length = Wavetable_length_E;
            }
            else
            {
                C_Flash_sample = C_Flash_sample_E;
                B_Flash_sample_shifted = B_Flash_sample_shifted_E;
            }

            main_settings_editing_flag = false;
        }

        // Harvest for samples
        live_forward_end = false;
        Prepare_live_forward_block();
        a_sample = a_first_sample;
        initial_index_offset = a_sample - floor(a_sample);
        b_sample = a_sample + (pitch * (AUDIO_BLOCK_SAMPLES - 1));

        if (false)
        {
            Serial.print(Flash_first_RAM_sample);
            Serial.print(" <= ");
            Serial.print(a_sample);
            Serial.print(" <= ");
            Serial.print(b_sample);
            Serial.print(" <= ");
            Serial.println(Flash_first_RAM_sample + Wavetable_length - 1);
        }

        Harvest_samples();
        Check_live_forward_boundary();

        if (volume_gain_warmup_flag)
        {
            volume_gain_warmup_flag = false;
            volume_flag = true;
        }

        if (pan_gain_warmup_flag)
        {
            pan_gain_warmup_flag = false;
            pan_int = pan_int_wait;                                         // Track the current ramp target, including edits during playback.
            K_pan_gain_L = (pan_gain_L_wait - pan_gain_L) / SAMPLES_VOLUME; // [Gain/sample]
            K_pan_gain_R = (pan_gain_R_wait - pan_gain_R) / SAMPLES_VOLUME; // [Gain/sample]
            JP0 = samples_counter + SAMPLES_VOLUME;                         // [sample]
            pan_flag = true;
        }

        if (set_effects_flag)
        {
            resolution_flag = resolution_flag_wait;
            K_resolution_step = K_resolution_step_wait;

            if (false)
            {
                Serial.print("Player - K_resolution_step:");
                Serial.println(K_resolution_step);
                Serial.println();
            }

            downsampling_flag = downsampling_flag_wait;
            downsampling = downsampling_wait;
            set_effects_flag = false;
        }

        for (auto sample = 0; sample < AUDIO_BLOCK_SAMPLES; ++sample)
        {
            int32_t cache;
            F_basket_sample = (sample * pitch) + initial_index_offset; // "initial_index_offset" may be modified in harvest function

            if (volume_flag)
            {
                Update_volume_gain();
            }

            if (pan_flag)
            {
                Update_pan_gain();
            }

            // Update_ADSR_gain();
            ADSR_gain = ADSR->Get_gain();

            if (!shoot_flag && mix_flag && sample < mix_samples)
            {
                value_float = mix_samples;
                cache = (sample / value_float) * volume_gain * velocity_gain * ADSR_gain * pitch_based_gain_correction * Read_rendered_sample(sample, F_basket_sample) + (mix_samples - 1 - sample) / (mix_samples - 1.0) * raw_first_value_cache[sample];
                block[sample] = Lilla_saturate16(cache);
            }
            else if (shoot_flag && mix_flag && sample < mix_samples)
            {
                value_float = mix_samples;
                cache = volume_gain * velocity_gain * ADSR_gain * pitch_based_gain_correction * Read_rendered_sample(sample, F_basket_sample) + (mix_samples - 1 - sample) / (value_float - 1.0f) * raw_first_value_cache[sample];
                block[sample] = Lilla_saturate16(cache);
            }

            else
            {
                cache = volume_gain * velocity_gain * ADSR_gain * pitch_based_gain_correction * Read_rendered_sample(sample, F_basket_sample);
                block[sample] = Lilla_saturate16(cache);
            }

            if (restart_flag)
            {
                fast_stop_gain = static_cast<float>(AUDIO_BLOCK_SAMPLES - 1 - sample) / static_cast<float>(AUDIO_BLOCK_SAMPLES - 1);
                block[sample] = block[sample] * fast_stop_gain;
            }

            if (downsampling_flag)
            {
                rest = (sliding + sample) % downsampling;
                if (rest == 0)
                {
                    stored_sample = block[sample];
                }
                else
                {
                    block[sample] = stored_sample;
                }
            }

            if (resolution_flag)
            {
                cache = block[sample];
                if (cache >= 0)
                {
                    cache = ceil(cache / K_resolution_step) * K_resolution_step;
                }
                else
                {
                    cache = -ceil(-cache / K_resolution_step) * K_resolution_step;
                }

                block[sample] = Lilla_saturate16(cache);
            }

            ++samples_counter;
        }

        if (mix_flag)
        {
            mix_flag = false;
        }

        if (shoot_flag)
        {
            shoot_flag = false;
        }

        if (downsampling_flag)
        {
            sliding = (sliding + AUDIO_BLOCK_SAMPLES) % downsampling;
        }

        tone_filter.Process(block, AUDIO_BLOCK_SAMPLES, pitch, tone_filter_level & 0x7f, AUDIO_SAMPLE_RATE_EXACT);

        if (VCF_connect)
        {
            if (VCF_modulated)
            {
                LFO_ptr->Update();
            }
            if (VCF_modulated || VCF_pivot_steps > 0)
            {
                Send_LFO_to_VCF();
            }
            else
            {
                VCF_ptr->LFO_connect = false;
            }
            VCF_ptr->Update();
        }

        if (live_forward_end)
        {
            live_restart_pending = false;
            Fade_live_forward_end();
        }
        else
        {
            Shape_live_forward_block();
        }

        for (auto sample = 0; sample < AUDIO_BLOCK_SAMPLES; sample++)
        {
            block_L->data[sample] = pan_gain_L * block[sample];
            block_R->data[sample] = pan_gain_R * block[sample];
        }

        live_forward_last_sample = block[AUDIO_BLOCK_SAMPLES - 1];
        rendered_block_valid = true; // The complete outgoing mono block is available for a source-free budget tail.
        transmit(block_L, 0);
        release(block_L);
        transmit(block_R, 1);
        release(block_R);

        if (restart_flag)
        {
            restart_flag = false;
            Start_playing();
        }

        if ((ADSR_gain < 0.0001) && (ADSR->Get_phase() > 0))
        {
            state = IDLE_REQUEST;
            My_LED(false);

            Close_source();
        }

        // Check execution time
        if (false && identity == 0)
        {
            Serial.print("Player 0 - mode: ");
            Serial.print((use_Wavetable ? "Wavetable" : "Flash"));
            Serial.print(" - pitch:");
            Serial.print(pitch);
            Serial.print(" - REAL execution_time:");
            Serial.print(audio_update_time_micros);
            Serial.println();
        }

        if (state == IDLE_REQUEST)
        {
            idle = true;
            power_on = false;
            state = IDLE;
            Close_source();
            My_LED(false);
        }
    }
}

void AudioPlayer::Update_volume_gain(void)
{
    if (volume_gain < volume_gain_wait)
    {
        volume_gain += VOLUME_GAIN_STEP;
        if (volume_gain >= volume_gain_wait)
        {
            volume_gain = volume_gain_wait;
            volume_flag = false;
        }
    }
    else
    {
        volume_gain -= VOLUME_GAIN_STEP;
        if (volume_gain <= volume_gain_wait)
        {
            volume_gain = volume_gain_wait;
            volume_flag = false;
        }
    }
}

void AudioPlayer::Update_pan_gain(void)
{
    const uint32_t remaining = JP0 - samples_counter;
    if (remaining > 1)
    {
        pan_gain_L = pan_gain_L_table[pan_int + 16] - K_pan_gain_L * (remaining - 1);
        pan_gain_R = pan_gain_R_table[pan_int + 16] - K_pan_gain_R * (remaining - 1);
    }
    else
    {
        pan_gain_L = pan_gain_L_table[pan_int + 16];
        pan_gain_R = pan_gain_R_table[pan_int + 16];
        pan_flag = false;
    }
}


void AudioPlayer::Harvest_samples(int outputs)
{
    if (live_forward_end && LS_flag && mode_player == LOOP_FWD)
    {
        memset(samples_basket, 0, sizeof(samples_basket)); // Reject an unrecorded loop attack without reading the cache.
        memset(resampled_basket, 0, sizeof(resampled_basket)); // Preserve cached filter settings for the next note.
        interpolation_mix = 0.0f;
        initial_index_offset = 0.0f;
        return;
    }
    interpolation_mix = PlayerInterpolator::Mix(pitch);
    PlayerAntialias::Configure(antialias, PlayerAntialias::Enabled(tone_filter_level, pitch_note * pitch_tune) ? pitch : 1.0f);
    if (antialias.mix > 0.0f)
    {
        Harvest_filtered_samples(antialias.history, 1, (outputs - 1) * pitch);
        PlayerAntialias::Render(samples_basket, harvested_samples, initial_index_offset, pitch, outputs, antialias, resampled_basket);
    }
    else if (interpolation_mix > 0.0f)
    {
        Harvest_filtered_samples(PlayerInterpolator::Left, PlayerInterpolator::Taps - PlayerInterpolator::Left - 1, b_sample - a_sample);
    }
    else
    {
        (use_Wavetable ? Wavetable_harvest() : Flash_memory_harvest());
    }
}

void AudioPlayer::Harvest_filtered_samples(int left, int right, float distance)
{
    harvested_samples = static_cast<int>(floorf((a_sample - floorf(a_sample) + left) + distance)) + right + 1;
    if (use_Wavetable)
    {
        const float phase = a_sample - Flash_first_RAM_sample;
        const int first = static_cast<int>(floorf(phase)) - left;
        const int count = static_cast<int>(floorf((phase - first) + distance)) + right + 1;
        harvested_samples = count;
        initial_index_offset = phase - first;
        for (int i = 0; i < count; ++i)
        {
            const int index = first + i;
            if (mode_player >= LOOP_FWD)
            {
                samples_basket[i] = Wavetable_ptr[(index % Wavetable_length + Wavetable_length) % Wavetable_length];
            }
            else
            {
                samples_basket[i] = index >= 0 && index < Wavetable_length ? Wavetable_ptr[index] : 0;
            }
        }
        if (mode_player >= LOOP_FWD)
        {
            a_first_sample = fmodf(phase + distance + pitch, Wavetable_length) + Flash_first_RAM_sample;
        }
        else
        {
            a_first_sample = b_sample + pitch;
            if (ceilf(phase + distance) > Wavetable_length - 1 && !warmup_for_play_again_flag)
            {
                state = IDLE_REQUEST;
                Close_source();
            }
        }
        return;
    }
    if (mode_player == LOOP_FWD || mode_player == LOOP_FWD_REV || mode_player == LOOP_REV)
    {
        const int crossfade = (LS_flag && mode_player == LOOP_FWD) || mode_player == LOOP_FWD_REV ? 0 : delta_Noclick;
        const int period = Loop_period(A_Flash_sample, B_Flash_sample, crossfade, mode_player);
        const int base = mode_player == LOOP_REV ? B_Flash_sample_shifted : A_Flash_sample + crossfade;
        if (period <= 0)
        {
            memset(samples_basket, 0, sizeof(samples_basket));
            initial_index_offset = left;
            Fast_stop();
            return;
        }
        float phase = mode_player == LOOP_FWD && a_sample < base ? 0.0f : fmodf(a_sample - base, static_cast<float>(period));
        if (phase < 0.0f)
        {
            phase += period;
        }
        const int first = static_cast<int>(floorf(phase)) - left;
        const int count = static_cast<int>(floorf((phase - first) + distance)) + right + 1;
        harvested_samples = count;
        const int wrapped = (first % period + period) % period;
        initial_index_offset = phase - first;
        if (!Fill_loop_samples(samples_basket, count, wrapped, A_Flash_sample, B_Flash_sample, crossfade, mode_player, Noclick_ptr, period))
        {
            memset(samples_basket, 0, sizeof(samples_basket));
            Fast_stop();
            return;
        }
        a_first_sample = base + fmodf(phase + distance + pitch, static_cast<float>(period));
        return;
    }

    const int original_first = static_cast<int>(floorf(a_sample));
    const int first = original_first - left;
    const int count = harvested_samples;
    initial_index_offset = a_sample - first;
    if (LS_flag && mode_player == ONCE_FWD)
    {
        const int position = (original_first % LS_buffer_dim + LS_buffer_dim) % LS_buffer_dim;
        const bool first_pass = Live_Sampler->first_write_flag && Live_Sampler->Q_sample < LS_buffer_dim - 1;
        const bool writing = Live_Sampler->Is_writing();
        if (first_pass)
        {
            const int available = Live_Sampler->Q_sample >= position ? Live_Sampler->Q_sample - position + 1 : 0;
            live_forward_end = static_cast<int>(ceilf(b_sample)) - original_first + 1 > available;
            live_forward_empty = available == 0;
            const int window_first = position - left;
            const int trim_first = !writing && !live_forward_end ? A_Flash_sample - original_first + position : 0;
            const int trim_last = !writing && !live_forward_end ? B_Flash_sample - original_first + position : Live_Sampler->Q_sample;
            const int valid_first = trim_first > 0 ? trim_first : 0;
            const int valid_last = trim_last < Live_Sampler->Q_sample ? trim_last : Live_Sampler->Q_sample;
            const int begin = window_first < valid_first ? valid_first - window_first : 0;
            const int available_end = valid_last - window_first + 1;
            const int end = available_end < count ? available_end : count;
            memset(samples_basket, 0, count * sizeof(int16_t));
            if (end > begin)
            {
                Read_samples(samples_basket + begin, window_first + begin, end - begin);
                if (valid_last == Live_Sampler->Q_sample && trim_last >= Live_Sampler->Q_sample)
                {
                    for (int i = end; i < count; ++i)
                    {
                        samples_basket[i] = samples_basket[end - 1]; // Extend recorded audio into the FIR lookahead; never read beyond Q.
                    }
                }
            }
            if (live_forward_end || !writing)
            {
                a_first_sample = b_sample + pitch;
            }
            else
            {
                a_first_sample = A_Flash_sample + fmodf(b_sample + pitch - A_Flash_sample, static_cast<float>(LS_buffer_dim));
            }
            if (!live_forward_end && !writing && !warmup_for_play_again_flag && ceilf(a_first_sample) >= B_Flash_sample)
            {
                state = IDLE_REQUEST;
                Close_source();
            }
            return;
        }
        Read_samples(samples_basket, first, count);
        live_last_read_sample = first + count - 1;
        a_first_sample = A_Flash_sample + fmodf(b_sample + pitch - A_Flash_sample, static_cast<float>(LS_buffer_dim));
        return;
    }

    const bool reverse = mode_player == ONCE_REV;
    const int lower = reverse ? B_Flash_sample : A_Flash_sample;
    const int upper = reverse ? C_Flash_sample : B_Flash_sample;
    const int valid_first = first < lower ? lower : first;
    const int last = first + count - 1;
    const int valid_last = last > upper ? upper : last;
    memset(samples_basket, 0, count * sizeof(int16_t));
    if (valid_last >= valid_first)
    {
        const int length = valid_last - valid_first + 1;
        int16_t *destination = samples_basket + valid_first - first;
        Read_samples(destination, reverse ? 2 * B_Flash_sample - valid_last : valid_first, length);
        if (reverse)
        {
            for (int i = 0; i < length / 2; ++i)
            {
                const int16_t value = destination[i];
                destination[i] = destination[length - 1 - i];
                destination[length - 1 - i] = value;
            }
        }
    }
    a_first_sample = b_sample + pitch;
    const bool reaches_end = ceilf(b_sample) > upper || ceilf(a_first_sample) >= upper;
    if (reaches_end && (!warmup_for_play_again_flag || (reverse && ceilf(b_sample) <= upper)))
    {
        if (!reverse && ceilf(b_sample) <= upper)
        {
            idle = true;
            power_on = false;
        }
        state = IDLE_REQUEST;
        Close_source();
    }
}

bool AudioPlayer::Harvest_live_forward_end(void)
{
    if (!LS_flag || mode_player != ONCE_FWD || !Live_Sampler->first_write_flag || Live_Sampler->Q_sample >= LS_buffer_dim - 1)
    {
        return false;
    }

    const int first = static_cast<int>(floorf(a_sample));
    const int count = static_cast<int>(ceilf(b_sample)) - first + 1;
    const int position = ((first % LS_buffer_dim) + LS_buffer_dim) % LS_buffer_dim;
    const int available = Live_Sampler->Q_sample >= position ? Live_Sampler->Q_sample - position + 1 : 0;
    if (count <= available)
    {
        return false;
    }

    if (available > 0)
    {
        Read_samples(samples_basket, position, available);
    }

    live_forward_empty = available == 0;
    // Never interpolate against unwritten memory; extend the last valid value into the fade.
    const int16_t tail = available > 0 ? samples_basket[available - 1] : 0;
    for (int sample = available; sample < count; ++sample)
    {
        samples_basket[sample] = tail;
    }
    a_first_sample = b_sample + pitch;
    return true;
}

bool AudioPlayer::Live_loop_samples_recorded(int phase, int count, int write_position) const
{
    const int span = B_Flash_sample - A_Flash_sample + 1;
    int remaining = std::min(count, span); // Repeated turns touch the same source samples.
    while (remaining > 0)
    {
        const int position = (A_Flash_sample + phase) % LS_buffer_dim;
        const int chunk = std::min(remaining, std::min(span - phase, LS_buffer_dim - position));
        if (position + chunk - 1 > write_position)
        {
            return false;
        }
        remaining -= chunk;
        phase = (phase + chunk) % span;
    }
    return true;
}

void AudioPlayer::Prepare_live_forward_block(void)
{
    live_fade_out = false;
    live_fade_in = false;
    const bool forward_loop = mode_player == LOOP_FWD && pitch >= 1.0f;
    if (!LS_flag || (mode_player != ONCE_FWD && !forward_loop))
    {
        live_start_pending = false;
        live_restart_pending = false;
        return;
    }

    if (live_start_pending)
    {
        live_start_pending = false;
        const bool cache_full = !Live_Sampler->first_write_flag || Live_Sampler->Q_sample == LS_buffer_dim - 1;
        if (forward_loop)
        {
            const int span = B_Flash_sample - A_Flash_sample + 1;
            A_Flash_sample = (A_Flash_sample % LS_buffer_dim + LS_buffer_dim) % LS_buffer_dim;
            B_Flash_sample = A_Flash_sample + span - 1;
            a_first_sample = A_Flash_sample;
            if (!cache_full)
            {
                PlayerAntialias::Configure(antialias, PlayerAntialias::Enabled(tone_filter_level, pitch_note * pitch_tune) ? pitch : 1.0f);
                const int history = antialias.mix > 0.0f ? antialias.history : 0;
                const int count = history > 0 ? history + static_cast<int>(floorf((AUDIO_BLOCK_SAMPLES - 1) * pitch)) + 2 : static_cast<int>(ceilf((AUDIO_BLOCK_SAMPLES - 1) * pitch)) + 1;
                if (!Live_loop_samples_recorded((span - history % span) % span, count, Live_Sampler->Q_sample))
                {
                    live_forward_end = true;
                    live_forward_empty = true;
                    live_forward_last_sample = 0;
                    live_restart_pending = false;
                    return;
                }
            }
        }
        else if (cache_full && LS_state == REC && Live_Sampler->Is_writing())
        {
            const int write_position = Live_Sampler->Q_sample;
            if (pitch > 1.0f)
            {
                // FIXED keeps its prepared attack unless it falls inside the latest written block.
                if (!LS_XY_lock || (write_position - A_Flash_sample + LS_buffer_dim) % LS_buffer_dim < AUDIO_BLOCK_SAMPLES)
                {
                    const int start_offset = (LS_XY_lock || LS_X_delta == 0) ? 1 : LS_X_delta;
                    A_Flash_sample = (write_position + start_offset) % LS_buffer_dim;
                    B_Flash_sample = A_Flash_sample + LS_buffer_dim - 1;
                    a_first_sample = A_Flash_sample;
                }
            }
            else
            {
                const int start_position = (A_Flash_sample % LS_buffer_dim + LS_buffer_dim) % LS_buffer_dim;
                const int distance_from_write = (write_position - start_position + LS_buffer_dim) % LS_buffer_dim;
                const int forbidden_samples = pitch == 1.0f ? AUDIO_BLOCK_SAMPLES - 1 : AUDIO_BLOCK_SAMPLES;
                if (distance_from_write < forbidden_samples)
                {
                    const int start_offset = pitch == 1.0f ? 1 : -AUDIO_BLOCK_SAMPLES;
                    A_Flash_sample = (write_position + start_offset + LS_buffer_dim) % LS_buffer_dim;
                    B_Flash_sample = A_Flash_sample + LS_buffer_dim - 1;
                    a_first_sample = A_Flash_sample;
                }
            }
        }
    }

    if (live_restart_pending)
    {
        live_restart_pending = false;
        const int period = forward_loop ? B_Flash_sample - A_Flash_sample + 1 : LS_buffer_dim;
        float relative_start = fmodf(live_restart_sample - A_Flash_sample, static_cast<float>(period));
        if (relative_start < 0.0f)
        {
            relative_start += period;
        }
        if (forward_loop && pitch > 1.0f && Live_Sampler->first_write_flag && Live_Sampler->Q_sample < LS_buffer_dim - 1)
        {
            const int history = antialias.mix > 0.0f ? antialias.history : 0;
            const int count = history > 0 ? history + static_cast<int>(floorf((AUDIO_BLOCK_SAMPLES - 1) * pitch)) + 2 : static_cast<int>(ceilf((AUDIO_BLOCK_SAMPLES - 1) * pitch)) + 1;
            const int phase = (static_cast<int>(relative_start) - history % period + period) % period;
            if (!Live_loop_samples_recorded(phase, count, Live_Sampler->Q_sample))
            {
                live_forward_end = true;
                live_forward_empty = true;
                live_forward_last_sample = 0;
                return;
            }
        }
        a_first_sample = A_Flash_sample + relative_start;
        live_fade_in = true;
    }
}


void AudioPlayer::Check_live_forward_boundary(void)
{
    if (!LS_flag || LS_state != REC || live_fade_in || live_forward_end || !Live_Sampler->Is_writing())
    {
        return;
    }
    if (mode_player == LOOP_FWD && pitch >= 1.0f)
    {
        const int span = B_Flash_sample - A_Flash_sample + 1;
        const int start_phase = (static_cast<int>(floorf(a_sample - A_Flash_sample)) % span + span) % span;
        const int first_position = (A_Flash_sample % LS_buffer_dim + LS_buffer_dim) % LS_buffer_dim;
        const int write_phase = (Live_Sampler->Q_sample - first_position + LS_buffer_dim) % LS_buffer_dim;
        const int history = antialias.mix > 0.0f ? antialias.history : 0;
        const int count = history > 0 ? harvested_samples - history : static_cast<int>(ceilf(b_sample)) - static_cast<int>(floorf(a_sample)) + 1;
        if (write_phase < span && (write_phase - start_phase + span) % span < count - 1)
        {
            live_fade_out = true;
            live_restart_sample = a_sample; // Repeat this phase, including a possible B-to-A wrap.
            if (pitch > 1.0f)
            {
                const int next_write = (Live_Sampler->Q_sample + AUDIO_BLOCK_SAMPLES) % LS_buffer_dim;
                const int next_write_phase = (next_write - first_position + LS_buffer_dim) % LS_buffer_dim;
                const int next_count = history > 0 ? history + static_cast<int>(floorf((AUDIO_BLOCK_SAMPLES - 1) * pitch)) + 2 : static_cast<int>(ceilf((AUDIO_BLOCK_SAMPLES - 1) * pitch)) + 1;
                const bool next_cache_full = !Live_Sampler->first_write_flag || Live_Sampler->Q_sample + AUDIO_BLOCK_SAMPLES >= LS_buffer_dim - 1;
                int restart_phase = static_cast<int>(floorf(a_first_sample - A_Flash_sample));
                if (next_write_phase < span)
                {
                    // At full capacity start the entire prefetch after Q; first fill must end at Q instead.
                    restart_phase = next_cache_full ? (next_write_phase + 1 + history) % span : (next_write_phase - (next_count - 1) % span + history + span) % span;
                }
                if (!next_cache_full && !Live_loop_samples_recorded((restart_phase - history % span + span) % span, next_count, next_write))
                {
                    restart_phase = history % span;
                    if (!Live_loop_samples_recorded(0, next_count, next_write))
                    {
                        live_forward_end = true; // No recorded window can supply this block; use the existing notice and stop ramp.
                        live_forward_empty = false;
                        return;
                    }
                }
                // If the prefetch spans more than a whole loop, no phase excludes Q: keep the ramp and retry after fade-in.
                live_restart_sample = A_Flash_sample + restart_phase;
            }
            live_restart_pending = true;
        }
        return;
    }
    if (mode_player != ONCE_FWD || pitch == 1.0f)
    {
        return;
    }
    if (Live_Sampler->first_write_flag && Live_Sampler->Q_sample < LS_buffer_dim - 1)
    {
        return;
    }

    const int write_position = Live_Sampler->Q_sample;
    // Exclude past IIR warmup from forward crossings; retain interpolation lookahead.
    const int boundary_samples = harvested_samples - (pitch > 1.0f && antialias.mix > 0.0f ? antialias.history : 0);
    const int first_read_sample = live_last_read_sample - boundary_samples + 1;
    const int first_position = (first_read_sample % LS_buffer_dim + LS_buffer_dim) % LS_buffer_dim;
    const int distance_to_write = (write_position - first_position + LS_buffer_dim) % LS_buffer_dim;
    const bool crosses_write_boundary = distance_to_write < boundary_samples - 1;
    if (pitch > 1.0f)
    {
        const int last_position = (live_last_read_sample % LS_buffer_dim + LS_buffer_dim) % LS_buffer_dim;
        const int distance_from_write = (write_position - last_position + LS_buffer_dim) % LS_buffer_dim;
        if (distance_from_write < AUDIO_BLOCK_SAMPLES || crosses_write_boundary)
        {
            live_fade_out = true;
            live_restart_sample = (write_position + 1 + AUDIO_BLOCK_SAMPLES) % LS_buffer_dim;
            live_restart_pending = true;
        }
    }
    else
    {
        if (crosses_write_boundary)
        {
            live_fade_out = true;
            live_restart_sample = a_sample - AUDIO_BLOCK_SAMPLES;
            live_restart_pending = true;
        }
    }
}

void AudioPlayer::Shape_live_forward_block(void)
{
    if (live_fade_out || live_fade_in)
    {
        for (int sample = 0; sample < AUDIO_BLOCK_SAMPLES; ++sample)
        {
            const int ramp_sample = live_fade_in ? sample : AUDIO_BLOCK_SAMPLES - 1 - sample;
            block[sample] = static_cast<int16_t>(static_cast<int32_t>(block[sample]) * ramp_sample / (AUDIO_BLOCK_SAMPLES - 1));
        }
    }
}

bool AudioPlayer::Harvest_live_continuous(void)
{
    if (!LS_flag || mode_player != ONCE_FWD)
    {
        return false;
    }
    if (!Live_Sampler->Is_writing() && Live_Sampler->first_write_flag && Live_Sampler->Q_sample < LS_buffer_dim - 1)
    {
        return false;
    }

    const int first = static_cast<int>(floorf(a_sample));
    const int count = static_cast<int>(ceilf(b_sample)) - first + 1;
    const int position = ((first % LS_buffer_dim) + LS_buffer_dim) % LS_buffer_dim;
    Read_samples(samples_basket, position, count);
    harvested_samples = count;
    live_last_read_sample = first + count - 1;
    // Keep the fractional phase and original start reference, also for FWD after recording stops.
    a_first_sample = A_Flash_sample + fmodf(b_sample + pitch - A_Flash_sample, static_cast<float>(LS_buffer_dim));
    return true;
}

bool AudioPlayer::Take_live_unrecorded_notice(void)
{
    const bool pending = live_unrecorded_notice;
    live_unrecorded_notice = false;
    return pending;
}

void AudioPlayer::Fade_live_forward_end(void)
{
    live_unrecorded_notice = true;
    // Fade after resolution/downsampling so the final transmitted sample is exactly zero.
    for (int sample = 0; sample < AUDIO_BLOCK_SAMPLES; ++sample)
    {
        const float gain = static_cast<float>(AUDIO_BLOCK_SAMPLES - 1 - sample) / static_cast<float>(AUDIO_BLOCK_SAMPLES - 1);
        const int16_t value = live_forward_empty ? live_forward_last_sample : block[sample];
        block[sample] = static_cast<int16_t>(value * gain);
    }
    state = IDLE_REQUEST;
}

void AudioPlayer::Flash_memory_harvest(void)
{
    live_forward_end = Harvest_live_forward_end();
    if (live_forward_end)
    {
        return;
    }

    if (Harvest_live_continuous())
    {
        return;
    }

    if (mode_player == LOOP_FWD || mode_player == LOOP_FWD_REV || mode_player == LOOP_REV)
    {
        Loop_memory_harvest(); // Assemble repeated loop segments before audio interpolation.
        return;
    }
    int samples_to_read, samples_to_read_1, samples_to_read_2, samples_to_read_3; // int16_t
    int16_t samples_basket_local[BASKET_DIM];
    int16_t samples_basket_local_2[BASKET_DIM];
    float a_Flash_sample;
    float b_Flash_sample, b_Flash_sample_new;
    float b_sample_tmp;
    float a_b_distance;
    int32_t INT_a_Flash_sample;
    int32_t INT_b_Flash_sample;
    uint16_t first_index;
    bool forward;

    switch (mode_player)
    {
    case 0: // A-->B
        a_Flash_sample = a_sample;
        b_Flash_sample = b_sample;
        INT_a_Flash_sample = floor(a_Flash_sample);
        INT_b_Flash_sample = ceil(b_Flash_sample);
        samples_to_read = INT_b_Flash_sample - INT_a_Flash_sample + 1;

        // --A-------a*******b-----B------
        if (INT_b_Flash_sample <= B_Flash_sample)
        {
            Read_samples(samples_basket, INT_a_Flash_sample, samples_to_read); // Read_samples (int16_t *destination, int seek_in, int samples_in)

            a_first_sample = b_sample + pitch;
            if (ceil(a_first_sample) >= B_Flash_sample && !warmup_for_play_again_flag)
            {
                idle = true;
                power_on = false;
                state = IDLE_REQUEST;

                Close_source();
            }
        }

        // ---A------------------a*B*******b-----
        else if (INT_a_Flash_sample <= B_Flash_sample)
        {
            samples_to_read_1 = B_Flash_sample - INT_a_Flash_sample + 1;

            Read_samples(samples_basket, INT_a_Flash_sample, samples_to_read_1); // Read_samples (int16_t *destination, int seek_in, int samples_in)

            for (auto sample = samples_to_read_1; sample < samples_to_read; ++sample)
            {
                samples_basket[sample] = 0;
            }

            if (!warmup_for_play_again_flag)
            {
                state = IDLE_REQUEST;

                Close_source();
            }
        }

        // -------------A----------------------B---a*******b-----
        else
        {
            for (auto sample = 0; sample < samples_to_read; ++sample)
            {
                samples_basket[sample] = 0;
            }

            if (!warmup_for_play_again_flag)
            {
                state = IDLE_REQUEST;

                Close_source();
            }
        }
        break;

    case 1: // B-->A
        a_b_distance = b_sample - a_sample;
        a_Flash_sample = Mirror(B_Flash_sample, a_sample);
        b_Flash_sample = a_Flash_sample - a_b_distance;

        INT_a_Flash_sample = ceil(a_Flash_sample);  // highest sample
        INT_b_Flash_sample = floor(b_Flash_sample); // lowest sample
        samples_to_read = INT_a_Flash_sample - INT_b_Flash_sample + 1;

        //  ----------A----b'*********a'----B--------------------C
        if (INT_b_Flash_sample >= A_Flash_sample)
        {
            Read_samples(samples_basket_local, INT_b_Flash_sample, samples_to_read); // Read_samples (int16_t *destination, int seek_in, int samples_in)

            for (auto sample = 0; sample < samples_to_read; ++sample)
            {
                samples_basket[sample] = samples_basket_local[samples_to_read - 1 - sample];
            }

            b_sample_tmp = Mirror(B_Flash_sample, b_Flash_sample);

            a_first_sample = b_sample_tmp + pitch;
            if (ceil(a_first_sample) >= C_Flash_sample)
            {
                state = IDLE_REQUEST;

                Close_source();
            }
        }

        //  -----b'****A*****a'-------------B--------------------C
        else
        {
            samples_to_read_1 = INT_a_Flash_sample - A_Flash_sample + 1;

            Read_samples(samples_basket_local, A_Flash_sample, samples_to_read_1); // Read_samples (int16_t *destination, int seek_in, int samples_in)

            for (auto sample = 0; sample < samples_to_read_1; ++sample)
            {
                samples_basket[sample] = samples_basket_local[samples_to_read_1 - 1 - sample];
            }

            for (auto sample = samples_to_read_1; sample < samples_to_read; ++sample)
            {
                samples_basket[sample] = 0;
            }

            if (!warmup_for_play_again_flag)
            {
                state = IDLE_REQUEST;

                Close_source();
            }
        }
        break;

    case 2: // loop A-->B

        // .........(A+d)-----------------(B-d)(B-d+1)----B......a*******b......
        // ....a****(A+d)**b--------------(B-d)(B-d+1)----B.....................
        if ((floor(a_sample) > B_Flash_sample) || (floor(a_sample) < (A_Flash_sample + delta_Noclick)))
        {
            a_b_distance = b_sample - a_sample;
            a_sample = A_Flash_sample + delta_Noclick;
            initial_index_offset = 0.0;
            b_sample = a_sample + a_b_distance; // .........(A+d)a*****b---------(B-d)(B-d+1)----B..........
        }
        INT_b_Flash_sample = ceil(b_sample);
        INT_a_Flash_sample = floor(a_sample);
        samples_to_read = INT_b_Flash_sample - INT_a_Flash_sample + 1;

        // .........(A+d)------a*****b----(B-d)(B-d+1)----B.......... --> single read
        if (ceil(b_sample) <= (B_Flash_sample - delta_Noclick))
        {

            Read_samples(samples_basket, INT_a_Flash_sample, samples_to_read); // Read_samples (int16_t *destination, int seek_in, int samples_in)

            a_first_sample = b_sample + pitch;

            // ......(A+d)-----------------(B-d)(B-d+1)----B....(B+1)..(a_f_s)
            if (floor(a_first_sample) > B_Flash_sample)
            {
                a_first_sample = (A_Flash_sample + delta_Noclick) + (a_first_sample - B_Flash_sample);
            }
        }

        // ..........(A+d)-----------a***(B-d)(B-d+1)**b----B......  --> double read
        else if ((INT_a_Flash_sample <= (B_Flash_sample - delta_Noclick)) && ((INT_b_Flash_sample >= (B_Flash_sample - delta_Noclick + 1)) && (INT_b_Flash_sample <= (B_Flash_sample))))
        {
            // read from flash
            samples_to_read_1 = (B_Flash_sample - delta_Noclick) - INT_a_Flash_sample + 1;

            Read_samples(samples_basket, INT_a_Flash_sample, samples_to_read_1); // Read_samples (int16_t *destination, int seek_in, int samples_in)

            // read from RAM and merge
            samples_to_read_2 = INT_b_Flash_sample - (B_Flash_sample - delta_Noclick + 1) + 1;
            for (auto sample = 0; sample < samples_to_read_2; ++sample)
            {
                samples_basket[samples_to_read_1 + sample] = *(Noclick_ptr + sample);
            }

            a_first_sample = b_sample + pitch;
            if (floor(a_first_sample) > B_Flash_sample)
            {
                a_first_sample = (A_Flash_sample + delta_Noclick) + (a_first_sample - B_Flash_sample);
            }
        }

        // ...........(A+d)----------------(B-d)(B-d+1)---a***b---B....  --> single read
        else if ((INT_a_Flash_sample >= (B_Flash_sample - delta_Noclick + 1)) && (INT_b_Flash_sample <= B_Flash_sample))
        {
            // read from RAM
            first_index = floor(a_sample) - (B_Flash_sample - delta_Noclick + 1);
            for (auto sample = 0; sample < samples_to_read; ++sample)
            {
                samples_basket[sample] = *(Noclick_ptr + first_index + sample);
            }

            a_first_sample = b_sample + pitch;
            if (floor(a_first_sample) > B_Flash_sample)
            {
                a_first_sample = (A_Flash_sample + delta_Noclick) + (a_first_sample - B_Flash_sample);
            }
        }

        // ...........(A+d)***b'----------(B-d)(B-d+1)------a***B****b..
        // ...........(A+d)***b'----------(B-d)(B-d+1)------a***B.......  --> double read
        else if ((INT_a_Flash_sample >= (B_Flash_sample - delta_Noclick + 1)) && (INT_b_Flash_sample > B_Flash_sample))
        {
            // read from RAM
            samples_to_read_1 = B_Flash_sample - INT_a_Flash_sample + 1;
            first_index = floor(a_sample) - (B_Flash_sample - delta_Noclick + 1);
            for (auto sample = 0; sample < samples_to_read_1; ++sample)
            {
                samples_basket[sample] = *(Noclick_ptr + first_index + sample);
            }

            // read again from flash and merge
            samples_to_read_2 = samples_to_read - samples_to_read_1;

            Read_samples(samples_basket_local, (A_Flash_sample + delta_Noclick), samples_to_read_2); // Read_samples (int16_t *destination, int seek_in, int samples_in)

            Append(samples_basket, samples_to_read_1, samples_basket_local, samples_to_read_2);

            a_first_sample = (A_Flash_sample + delta_Noclick) + (b_sample - B_Flash_sample - 1) + pitch;
        }

        // .............(A+d)***b'--------a**(B-d)(B-d+1)*******B***b.......
        // .............(A+d)***b'--------a**(B-d)(B-d+1)*******B........... --> triple read
        else if ((INT_a_Flash_sample <= (B_Flash_sample - delta_Noclick)) && (INT_b_Flash_sample > B_Flash_sample))
        {
            // read from flash
            samples_to_read_1 = (B_Flash_sample - delta_Noclick) - INT_a_Flash_sample + 1;

            Read_samples(samples_basket, INT_a_Flash_sample, samples_to_read_1); // Read_samples (int16_t *destination, int seek_in, int samples_in)

            // read from RAM and merge
            samples_to_read_2 = delta_Noclick;
            for (auto sample = 0; sample < samples_to_read_2; ++sample)
            {
                samples_basket[samples_to_read_1 + sample] = *(Noclick_ptr + sample);
            }

            // read again from flash and merge
            samples_to_read_3 = samples_to_read - samples_to_read_1 - samples_to_read_2;

            Read_samples(samples_basket_local, (A_Flash_sample + delta_Noclick), samples_to_read_3); // Read_samples (int16_t *destination, int seek_in, int samples_in)

            Append(samples_basket, samples_to_read_1 + samples_to_read_2, samples_basket_local, samples_to_read_3);

            a_first_sample = (A_Flash_sample + delta_Noclick) + (b_sample - B_Flash_sample - 1) + pitch;
        }
        break;

    case 3: // loop A-->B-->A
        // check direction
        forward = false;
        if (floor(a_sample) < B_Flash_sample) // initial direction: forward (a_sample is REAL address)
        {
            forward = true;
        }

        if (forward) //  ----A--------a>>>>--B-----  (a_sample is REAL address)
        {
            INT_a_Flash_sample = floor(a_sample);
            INT_b_Flash_sample = ceil(b_sample);
            samples_to_read = INT_b_Flash_sample - INT_a_Flash_sample + 1;

            if (b_sample <= B_Flash_sample) //  ----A--------a>>>>>b-B-----  (b_sample is REAL address)
            {
                // read from INT_a_Flash_sample to INT_b_Flash_sample
                if (false)
                {
                    Serial.print("  samples_to_read: ");
                    Serial.print(samples_to_read);
                    Serial.print("  INT_a_Flash_sample: ");
                    Serial.println(INT_a_Flash_sample);
                    Serial.println();
                }

                Read_samples(samples_basket, INT_a_Flash_sample, samples_to_read); // Read_samples (int16_t *destination, int seek_in, int samples_in)

                a_first_sample = b_sample + pitch;
            }

            else //  ----A---------a>>>B>>b-------------C   (b_sample is NOT_REAL address)
            {
                // read from INT_a_Flash_sample to B_Flash_sample (included)
                samples_to_read_1 = B_Flash_sample - INT_a_Flash_sample + 1;

                Read_samples(samples_basket, INT_a_Flash_sample, samples_to_read_1); // Read_samples (int16_t *destination, int seek_in, int samples_in)

                // read from INT_b_Flash_sample to (B_Flash_sample - 1)
                b_Flash_sample = Mirror(B_Flash_sample, b_sample);
                INT_b_Flash_sample = floor(b_Flash_sample);
                samples_to_read_2 = samples_to_read - samples_to_read_1; // DO NOT read restart sample B_Flash_sample

                if (false)
                {
                    Serial.print("  samples_to_read_2: ");
                    Serial.print(samples_to_read_2);
                    Serial.print("  INT_b_Flash_sample: ");
                    Serial.println(INT_b_Flash_sample);
                    Serial.println();
                }

                Read_samples(samples_basket_local, INT_b_Flash_sample, samples_to_read_2); // Read_samples (int16_t *destination, int seek_in, int samples_in)

                // merge arrays transposing sample_basket_local
                Append_reversed(samples_basket, samples_to_read_1, samples_basket_local, samples_to_read_2);

                a_first_sample = b_sample + pitch;
            }
        }

        else // reverse  ----A---------------B----a>>>>>-----C (a_sample is NOT_REAL address)
        {
            a_Flash_sample = Mirror(B_Flash_sample, a_sample);
            b_Flash_sample = Mirror(B_Flash_sample, b_sample);
            INT_a_Flash_sample = ceil(a_Flash_sample);
            INT_b_Flash_sample = floor(b_Flash_sample);

            if (false)
            {
                Serial.print("a_Flash_sample: ");
                Serial.print(a_Flash_sample);
                Serial.print("  b_Flash_sample: ");
                Serial.println(b_Flash_sample);
                Serial.println();
            }

            if (INT_b_Flash_sample >= A_Flash_sample) // -----A---bF<<<<<aF-------B---- (all REAL addresses)
            {
                samples_to_read = INT_a_Flash_sample - INT_b_Flash_sample + 1;

                Read_samples(samples_basket_local, INT_b_Flash_sample, samples_to_read); // Read_samples (int16_t *destination, int seek_in, int samples_in)

                // transpose samples
                Append_reversed(samples_basket, 0, samples_basket_local, samples_to_read);

                b_sample_tmp = Mirror(B_Flash_sample, b_Flash_sample);

                a_first_sample = b_sample + pitch;
                if (a_first_sample > C_Flash_sample)
                {
                    a_first_sample = A_Flash_sample + (a_first_sample - C_Flash_sample);
                }
            }

            else // ----bF<<<A<<aF-----------B---- (all REAL addresses, but b_Flash_sample has to be "mirrored" again)
            {
                // read from A_Flash_sample to INT_a_Flash_sample (both included)
                samples_to_read = INT_a_Flash_sample - A_Flash_sample + 1;

                // Serial.print("samples_to_read: ");
                // Serial.print(samples_to_read);

                Read_samples(samples_basket_local, A_Flash_sample, samples_to_read); // Read_samples (int16_t *destination, int seek_in, int samples_in)

                // swap samples
                Append_reversed(samples_basket, 0, samples_basket_local, samples_to_read);

                // read from (A_Flash_sample + 1) to INT_b_Flash_sample
                b_Flash_sample = Mirror(A_Flash_sample, b_Flash_sample); // ---A----bF*****aF-----------B---- (all REAL addresses)
                INT_b_Flash_sample = ceil(b_Flash_sample);

                samples_to_read_1 = INT_b_Flash_sample - A_Flash_sample; // // DO NOT read restart sample A_Flash_sample

                // Serial.print(" samples_to_read_1: ");
                // Serial.println(samples_to_read_1);

                Read_samples(samples_basket_local, (A_Flash_sample + 1), samples_to_read_1); // Read_samples (int16_t *destination, int seek_in, int samples_in)

                // merge arrays
                Append(samples_basket, samples_to_read, samples_basket_local, samples_to_read_1);

                a_first_sample = b_Flash_sample + pitch;
            }
        }
        break;

    case 5: // loop B-->A

        // a_sample and b_sample are virtual sample
        // B_Flash_sample_shifted is (B_value_in - delta_Noclick)
        a_b_distance = b_sample - a_sample;
        a_Flash_sample = Mirror(B_Flash_sample_shifted, a_sample);
        b_Flash_sample = Mirror(B_Flash_sample_shifted, b_sample); // a_Flash_sample - a_b_distance;
        INT_a_Flash_sample = ceil(a_Flash_sample);
        INT_b_Flash_sample = floor(b_Flash_sample);
        samples_to_read = INT_a_Flash_sample - INT_b_Flash_sample + 1;

        // .........A------------(A+d-1)(A+d)-------bF<<<<<<<aF--------(B_5)..... --> single read
        if ((INT_b_Flash_sample >= (A_Flash_sample + delta_Noclick)))
        {
            // read from flash and transpose
            Read_samples(samples_basket_local, INT_b_Flash_sample, samples_to_read); // Read_samples (int16_t *destination, int seek_in, int samples_in)

            for (auto sample = 0; sample < samples_to_read; ++sample)
            {
                samples_basket[sample] = samples_basket_local[samples_to_read - 1 - sample];
            }

            // Append_reversed(samples_basket, 0, samples_basket_local, samples_to_read);
            b_sample_tmp = Mirror(B_Flash_sample_shifted, b_Flash_sample);
            a_first_sample = b_sample_tmp + pitch;
            if (floor(a_first_sample) > C_Flash_sample)
            {
                a_first_sample = B_Flash_sample_shifted + (a_first_sample - C_Flash_sample);
            }
        }

        // .........A------bF<<(A+d-1)(A+d)<<<<<aF----------------------(B_5)..... --> double read
        else if ((INT_a_Flash_sample >= (A_Flash_sample + delta_Noclick)) && (INT_b_Flash_sample >= A_Flash_sample) && (INT_b_Flash_sample <= (A_Flash_sample + delta_Noclick - 1)))
        {
            // from RAM
            samples_to_read_1 = A_Flash_sample + delta_Noclick - INT_b_Flash_sample;
            first_index = INT_b_Flash_sample - A_Flash_sample;
            for (auto sample = 0; sample < samples_to_read_1; ++sample)
            {
                samples_basket_local[sample] = *(Noclick_ptr + first_index + sample);
            }

            // from flash
            samples_to_read_2 = INT_a_Flash_sample - (A_Flash_sample + delta_Noclick) + 1;

            Read_samples(samples_basket_local_2, (A_Flash_sample + delta_Noclick), samples_to_read_2); // Read_samples (int16_t *destination, int seek_in, int samples_in)

            // Append
            for (auto sample = 0; sample < samples_to_read_2; ++sample)
            {
                samples_basket_local[samples_to_read_1 + sample] = samples_basket_local_2[sample];
            }

            // Copy and transpose
            for (auto sample = 0; sample < samples_to_read; ++sample)
            {
                samples_basket[sample] = samples_basket_local[samples_to_read - 1 - sample];
            }

            b_sample_tmp = Mirror(B_Flash_sample_shifted, b_Flash_sample);
            a_first_sample = b_sample_tmp + pitch;
            if (floor(a_first_sample) > C_Flash_sample)
            {
                a_first_sample = B_Flash_sample_shifted + (a_first_sample - C_Flash_sample);
            }
        }

        // ....bF<<<A<<<<<<<<<<<<(A+d-1)(A+d)<<<<aF-----------------(B_5).......
        // .........A<<<<<<<<<<<<(A+d-1)(A+d)<<<<aF-----------b'F<<<(B_5)....... --> triple read
        else if ((INT_a_Flash_sample >= (A_Flash_sample + delta_Noclick)) && (INT_b_Flash_sample < A_Flash_sample))
        {
            b_Flash_sample_new = B_Flash_sample_shifted - (A_Flash_sample - 1 - b_Flash_sample);
            INT_b_Flash_sample = floor(b_Flash_sample_new);

            // from flash - last
            samples_to_read_1 = B_Flash_sample_shifted - INT_b_Flash_sample + 1;

            Read_samples(samples_basket_local, INT_b_Flash_sample, samples_to_read_1); // Read_samples (int16_t *destination, int seek_in, int samples_in)

            // from RAM
            samples_to_read_2 = delta_Noclick;
            first_index = 0;

            // Append directly
            for (auto sample = 0; sample < samples_to_read_2; ++sample)
            {
                samples_basket_local[sample + samples_to_read_1] = *(Noclick_ptr + first_index + sample);
            }

            // from flash - first
            samples_to_read_3 = INT_a_Flash_sample - (A_Flash_sample + delta_Noclick) + 1;

            Read_samples(samples_basket_local_2, (A_Flash_sample + delta_Noclick), samples_to_read_3); // Read_samples (int16_t *destination, int seek_in, int samples_in)

            // Append
            for (auto sample = 0; sample < samples_to_read_3; ++sample)
            {
                samples_basket_local[samples_to_read_1 + samples_to_read_2 + sample] = samples_basket_local_2[sample];
            }

            // Copy and transpose
            for (auto sample = 0; sample < samples_to_read; ++sample)
            {
                samples_basket[sample] = samples_basket_local[samples_to_read - 1 - sample];
            }

            b_sample_tmp = Mirror(B_Flash_sample_shifted, b_Flash_sample_new);
            a_first_sample = b_sample_tmp + pitch;
        }

        // ...........A--bF<<<<<aF---(A+d-1)(A+d)--------------------(B_5).....  --> single read
        else if ((INT_a_Flash_sample <= (A_Flash_sample + delta_Noclick - 1)) && (INT_b_Flash_sample >= A_Flash_sample))
        {
            // from RAM
            first_index = INT_b_Flash_sample - A_Flash_sample;
            for (auto sample = 0; sample < samples_to_read; ++sample)
            {
                samples_basket_local[sample] = *(Noclick_ptr + first_index + sample);
            }

            // Transpose
            for (auto sample = 0; sample < samples_to_read; ++sample)
            {
                samples_basket[sample] = samples_basket_local[samples_to_read - 1 - sample];
            }

            b_sample_tmp = Mirror(B_Flash_sample_shifted, b_Flash_sample);
            a_first_sample = b_sample_tmp + pitch;
            if (floor(a_first_sample) > C_Flash_sample)
            {
                a_first_sample = B_Flash_sample_shifted + (a_first_sample - C_Flash_sample);
            }
        }

        // ....bF<<<A<<<<<<<<aF-----(A+d-1)(A+d)--------------------(B_5).......
        // ...........A<<<<<<aF-----(A+d-1)(A+d)-------------b'F<<<<(B_5)....... --> double read
        else if ((INT_a_Flash_sample <= (A_Flash_sample + delta_Noclick - 1)) && (INT_b_Flash_sample < A_Flash_sample))
        {
            b_Flash_sample_new = B_Flash_sample_shifted - (A_Flash_sample - 1 - b_Flash_sample);
            INT_b_Flash_sample = floor(b_Flash_sample_new);

            // from flash
            samples_to_read_1 = B_Flash_sample_shifted - INT_b_Flash_sample + 1;
            Read_samples(samples_basket_local, INT_b_Flash_sample, samples_to_read_1); // Read_samples (int16_t *destination, int seek_in, int samples_in)

            // from RAM
            samples_to_read_2 = INT_a_Flash_sample - A_Flash_sample + 1;
            first_index = 0;

            // Append directly
            for (auto sample = 0; sample < samples_to_read_2; ++sample)
            {
                samples_basket_local[sample + samples_to_read_1] = *(Noclick_ptr + first_index + sample);
            }

            // Transpose
            for (auto sample = 0; sample < samples_to_read; ++sample)
            {
                samples_basket[sample] = samples_basket_local[samples_to_read - 1 - sample];
            }

            b_sample_tmp = Mirror(B_Flash_sample_shifted, b_Flash_sample_new);
            a_first_sample = b_sample_tmp + pitch;
        }
        break;
    }
}

int AudioPlayer::Loop_period(int first, int last, int crossfade, uint8_t mode) const // Return the sample period for forward, reverse or ping-pong loops.
{
    const int span = last - first + 1;
    if (first < 0 || span < 2 || crossfade < 0 || crossfade > span / 2)
    {
        return 0;
    }
    if (mode == LOOP_FWD_REV)
    {
        return 2 * (span - 1);
    }
    return mode == LOOP_FWD || mode == LOOP_REV ? span - crossfade : 0;
}

bool AudioPlayer::Fill_loop_samples(int16_t *destination, int count, int phase, int first, int last, int crossfade, uint8_t mode, const int16_t *noclick, int period) // Fill repeated loop segments without crossing source or NoClick boundaries.
{
    if (mode != LOOP_FWD_REV && crossfade > 0 && noclick == nullptr)
    {
        return false;
    }
    phase %= period;
    const int span = last - first + 1;
    const int raw_count = span - 2 * crossfade;
    while (count > 0)
    {
        int available;
        int source_first = 0;
        bool reverse = false;
        bool from_noclick = false;
        if (mode == LOOP_FWD_REV)
        {
            reverse = phase >= span;
            available = (reverse ? period : span) - phase;
            source_first = reverse ? first + period - phase : first + phase;
        }
        else if (phase < raw_count)
        {
            reverse = mode == LOOP_REV;
            available = raw_count - phase;
            source_first = reverse ? last - crossfade - phase : first + crossfade + phase;
        }
        else
        {
            from_noclick = true;
            available = period - phase;
        }
        const int chunk = count < available ? count : available;
        if (from_noclick)
        {
            for (int i = 0; i < chunk; ++i)
            {
                destination[i] = noclick[mode == LOOP_REV ? crossfade - 1 - (phase - raw_count) - i : phase - raw_count + i];
            }
        }
        else
        {
            Read_samples(destination, reverse ? source_first - chunk + 1 : source_first, chunk); // Read the bounded segment from the active Flash or PSRAM source.
            if (reverse)
            {
                for (int i = 0; i < chunk / 2; ++i)
                {
                    const int16_t value = destination[i];
                    destination[i] = destination[chunk - 1 - i];
                    destination[chunk - 1 - i] = value;
                }
            }
        }
        destination += chunk;
        count -= chunk;
        phase = (phase + chunk) % period;
    }
    return true;
}

void AudioPlayer::Loop_memory_harvest(void) // Assemble loop samples and preserve the fractional playback position.
{
    const int crossfade = (LS_flag && mode_player == LOOP_FWD) || mode_player == LOOP_FWD_REV ? 0 : delta_Noclick;
    const int period = Loop_period(A_Flash_sample, B_Flash_sample, crossfade, mode_player);
    const int base = mode_player == LOOP_REV ? B_Flash_sample_shifted : A_Flash_sample + crossfade;
    const float distance = b_sample - a_sample;
    if (period <= 0)
    {
        memset(samples_basket, 0, sizeof(samples_basket)); // Clear the sample buffer to silence before stopping playback for an invalid loop period.
        Fast_stop();
        return;
    }
    // Forward crossfade loops start after the part already merged into NoClick.
    float phase = mode_player == LOOP_FWD && a_sample < base ? 0.0f : fmodf(a_sample - base, static_cast<float>(period));
    if (phase < 0.0f)
    {
        phase += period;
    }
    const int first = static_cast<int>(floorf(phase));
    const int count = static_cast<int>(ceilf(phase + distance)) - first + 1;
    initial_index_offset = phase - first;
    if (count <= 0 || count > BASKET_DIM || !Fill_loop_samples(samples_basket, count, first, A_Flash_sample, B_Flash_sample, crossfade, mode_player, Noclick_ptr, period))
    {
        memset(samples_basket, 0, sizeof(samples_basket)); // Clear the sample buffer to silence when the requested count is invalid or loop assembly fails.
        Fast_stop();
        return;
    }
    a_first_sample = base + fmodf(phase + distance + pitch, static_cast<float>(period));
}

void AudioPlayer::Wavetable_harvest()
{
    uint16_t samples_to_read = 0;
    uint16_t samples_to_read_W;
    float Wavetable_LOW_sample;    // indirizzo float su RAM
    float Wavetable_HIGH_sample;   // indirizzo float su RAM
    int Wavetable_LOW_sample_int;  // indirizzo su RAM
    int Wavetable_HIGH_sample_int; // indirizzo su RAM
    int16_t sample = 0;

    Wavetable_LOW_sample = a_sample - Flash_first_RAM_sample;  // address on RAM corresponding to a_sample on Flash
    Wavetable_HIGH_sample = b_sample - Flash_first_RAM_sample; // address on RAM corresponding to b_sample on Flash
    Wavetable_LOW_sample_int = floor(Wavetable_LOW_sample);
    Wavetable_HIGH_sample_int = ceil(Wavetable_HIGH_sample);

    if (mode_player < 2) // A-->B  B-->A
    {
        initial_index_offset = Wavetable_LOW_sample - Wavetable_LOW_sample_int;
        samples_to_read = Wavetable_HIGH_sample_int - Wavetable_LOW_sample_int + 1;

        // 0-----a*******b----(WTL-1)
        if (Wavetable_HIGH_sample_int <= (Wavetable_length - 1))
        {
            while (sample < samples_to_read)
            {
                samples_basket[sample] = *(Wavetable_ptr + Wavetable_LOW_sample_int + sample);
                ++sample;
            }
        }

        // 0-----a*******(WTL-1)**b
        else
        {
            samples_to_read_W = Wavetable_length - Wavetable_LOW_sample_int;
            while (sample < samples_to_read_W)
            {
                samples_basket[sample] = *(Wavetable_ptr + Wavetable_LOW_sample_int + sample);
                ++sample;
            }
            while (sample < samples_to_read)
            {
                samples_basket[sample] = 0;
                ++sample;
            }

            if (!warmup_for_play_again_flag)
            {
                state = IDLE_REQUEST;

                Close_source();
            }
        }

        a_first_sample = b_sample + pitch;
        return;
    }

    else // mode_player >= 2 (loops)
    {
        initial_index_offset = Wavetable_LOW_sample - Wavetable_LOW_sample_int;
        samples_to_read = Wavetable_HIGH_sample_int - Wavetable_LOW_sample_int + 1;

        // read from Wavetable_LOW_sample_int to Wavetable_HIGH_sample_int
        while (sample < samples_to_read)
        {
            samples_basket[sample] = *(Wavetable_ptr + ((Wavetable_LOW_sample_int + sample) % Wavetable_length));
            ++sample;
        }

        a_first_sample = fmod(Wavetable_HIGH_sample + pitch, Wavetable_length) + Flash_first_RAM_sample; // The function fmod() returns the floating-point remainder of x/y.
    }
}

void AudioPlayer::Set_identity(uint8_t value)
{
    identity = value;
}
bool AudioPlayer::isPlaying(void)
{
    return !idle;
}

bool AudioPlayer::isPoweredOn(void)
{
    return power_on;
}

void AudioPlayer::Set_source(const AudioFileSource &source)
{
    source_wait = source;
    file_id_wait = source.file_id;
}

void AudioPlayer::Close_source(void)
{
    rawfile.close();
    if (spi_in_use)
    {
        spi_in_use = false;
        AudioStopUsingSPI();
    }
}

void AudioPlayer::Refresh_cached_source(const AudioFileSource &source)
{
    if (state != IDLE && source_now.storage == Flash && file_id == source.file_id && A_Flash_sample >= static_cast<int>(source.first_sample) && B_Flash_sample < static_cast<int>(source.first_sample + source.samples) && (!main_settings_editing_flag || (A_Flash_sample_E >= static_cast<int>(source.first_sample) && B_Flash_sample_E < static_cast<int>(source.first_sample + source.samples))))
    {
        // Keep the playhead and edit geometry; only the reader and its pitch ceiling change.
        Close_source();
        source_now = source;
        pitch_limit = Playback_pitch_limit(use_Wavetable, true, (file_id >= FIRST_LIVE_SAMPLING_FILE && !SdSoundSource::Is_sd(file_id)));
        if (main_settings_editing_flag)
        {
            pitch_limit_E = Playback_pitch_limit(use_Wavetable_E, true, (file_id >= FIRST_LIVE_SAMPLING_FILE && !SdSoundSource::Is_sd(file_id)));
        }
        Update_pitch();
    }
    if ((warmup_for_play_again_flag || restart_flag) && source_wait.storage == Flash && file_id_wait == source.file_id && A_Flash_sample_wait >= static_cast<int>(source.first_sample) && B_Flash_sample_wait < static_cast<int>(source.first_sample + source.samples))
    {
        source_wait = source;
        pitch_limit_wait = Playback_pitch_limit(use_Wavetable_wait, true, (file_id_wait >= FIRST_LIVE_SAMPLING_FILE && !SdSoundSource::Is_sd(file_id_wait)));
    }
}

bool AudioPlayer::Uses_sample_voice(void) const
{
    if (state == IDLE)
    {
        return false;
    }
    const bool current = !use_Wavetable && file_id >= 0 && (file_id < FIRST_LIVE_SAMPLING_FILE || SdSoundSource::Is_sd(file_id));
    const bool starting = (warmup_for_play_again_flag || restart_flag) && !use_Wavetable_wait && file_id_wait >= 0 && (file_id_wait < FIRST_LIVE_SAMPLING_FILE || SdSoundSource::Is_sd(file_id_wait));
    const bool editing = main_settings_editing_flag && !use_Wavetable_E && file_id >= 0 && (file_id < FIRST_LIVE_SAMPLING_FILE || SdSoundSource::Is_sd(file_id));
    return current || starting || editing;
}

bool AudioPlayer::Uses_flash(void) const
{
    if (state == IDLE)
    {
        return false;
    }
    const bool current = !use_Wavetable && source_now.storage == Flash && (file_id < FIRST_LIVE_SAMPLING_FILE || SdSoundSource::Is_sd(file_id));
    const bool starting = (warmup_for_play_again_flag || restart_flag) && !use_Wavetable_wait && source_wait.storage == Flash && (file_id_wait < FIRST_LIVE_SAMPLING_FILE || SdSoundSource::Is_sd(file_id_wait));
    const bool editing = main_settings_editing_flag && !use_Wavetable_E && source_now.storage == Flash && (file_id < FIRST_LIVE_SAMPLING_FILE || SdSoundSource::Is_sd(file_id));
    return current || starting || editing;
}

bool AudioPlayer::Fast_stop_using_cache(uint16_t cache_mask)
{
    if (state == IDLE || state == IDLE_REQUEST || source_now.cache_id < 0 || warmup_for_play_again_flag || restart_flag || main_settings_editing_flag)
    {
        return false;
    }
    if ((cache_mask & static_cast<uint16_t>(1u << source_now.cache_id)) == 0)
    {
        return false;
    }
    Fast_stop();
    return true;
}

void AudioPlayer::Set_volume(float volume_gain_value)
{
    volume_gain_wait = volume_gain_value;
}

void AudioPlayer::Update_volume(float volume_gain_value)
{
    volume_gain_wait = volume_gain_value;
    if (!idle && (volume_flag || volume_gain_value != volume_gain))
    {
        volume_gain_warmup_flag = true;
    }
}

void AudioPlayer::Set_pan(int pan_int_value)
{
    pan_int_wait = pan_int_value;
    pan_gain_L_wait = pan_gain_L_table[pan_int_wait + 16];
    pan_gain_R_wait = pan_gain_R_table[pan_int_wait + 16];
}

void AudioPlayer::Set_pitch(float pitch_tune_in) // 0.5= half speed, 1.0= no change 2.0=double speed
{
    pitch_tune_wait = pitch_tune_in;
    pitch_tune_flag = true;
}

void AudioPlayer::Set_note(float pitch_note_in)
{
    pitch_note = pitch_note_in;
}

void AudioPlayer::Set_pitch_bend(float pitch_bend_in) // 0.5= half speed, 1.0= no change 2.0=double speed
{
    pitch_bend_wait = pitch_bend_in;
    pitch_bend_flag = true;
}

void AudioPlayer::Write_loop_track(int track)
{
    track_wait = track;
}

int AudioPlayer::Read_loop_track(void)
{
    return track;
}

void AudioPlayer::Fast_stop(void)
{
    if (state == IDLE)
    {
        return;
    }

    ADSR->Fast_stop();
    state = IDLE_REQUEST;
}

void AudioPlayer::Panic_stop(void)
{
    warmup_for_play_again_flag = false;
    restart_flag = false;
    pending_note_released = false;
    Fast_stop();
}

bool AudioPlayer::Fast_stop_using_tables(uint8_t banks_mask)
{
    if (state == IDLE || state == IDLE_REQUEST || (tables_bank_mask & banks_mask) == 0)
    {
        return false;
    }

    // Let pending playback changes complete before selecting the voice to stop.
    if (warmup_for_play_again_flag || restart_flag || main_settings_editing_flag)
    {
        return false;
    }

    Fast_stop();
    return true;
}

void AudioPlayer::Update_pan(float pan_int_value)
{
    pan_int_wait = pan_int_value;
    pan_gain_L_wait = pan_gain_L_table[pan_int_wait + 16];
    pan_gain_R_wait = pan_gain_R_table[pan_int_wait + 16];

    if (!idle && (pan_int_wait != pan_int))
    {
        pan_gain_warmup_flag = true;
    }
}

void AudioPlayer::Set_vibrato_pointers(float *p_vibrato_array_in, uint8_t *p_vibrato_array_last_element_in)
{
    vibrato_array_ptr = p_vibrato_array_in;
    vibrato_array_last_element_ptr = p_vibrato_array_last_element_in;
}

void AudioPlayer::Set_vibrato_flag(bool value)
{
    if (vibrato_flag)
    {
        if (value)
        {
            return;
        }
        else
        {
            vibrato_flag = false;
            pitch_vibrato = 1.0;
        }
    }
    else
    {
        if (value)
        {
            vibrato_flag = true;
            vibrato_array_element_float = *vibrato_array_last_element_ptr;
            return;
        }
        else
        {
            return;
        }
    }
}

void AudioPlayer::Set_modulation(uint8_t value)
{
    modulation_depth = value / 127.0f;
    Set_vibrato_flag(value != 0);
}

void AudioPlayer::Set_mix_samples(uint8_t value)
{
    mix_samples = (value == 1 ? 2 : value);
}

float AudioPlayer::Read_pitch(void)
{
    return constrain(pitch_note * pitch_bend * pitch_tune * pitch_vibrato, MIN_PITCH, pitch_limit);
}

void AudioPlayer::Connect_VCF(bool use, int type, float pivot, float resonance, bool modulated)
{
    VCF_connect = use;
    VCF_modulated = modulated;
    VCF_frequency_pivot = pivot;
    VCF_pivot_target = pivot;
    VCF_pivot_steps = 0;

    if (false)
    {
        Serial.print("VCF_frequency_pivot:");
        Serial.println(VCF_frequency_pivot);
        Serial.print("VCF_modulated:");
        Serial.println(VCF_modulated);
        Serial.println();
    }

    if (VCF_connect && VCF_modulated)
    {
        VCF_ptr->LFO_connect = true;
        VCF_ptr->filter_type = type;
        VCF_ptr->q_value = resonance; // Butterworth: 0.7071

        VCF_ptr->_block = block;                   // int16_t*
        VCF_ptr->_frequency = VCF_frequency_array; // float*
    }

    else if (VCF_connect && !VCF_modulated)
    {
        VCF_ptr->LFO_connect = false;
        VCF_ptr->filter_type = type;
        VCF_ptr->q_value = resonance;                // Butterworth: 0.7071
        VCF_ptr->Set_filter(0, VCF_frequency_pivot); // Set_filter(uint32_t stage, float frequency, float q = 0.7071)

        VCF_ptr->_block = block;                   // int16_t*
        VCF_ptr->_frequency = VCF_frequency_array; // float*
    }
}

void AudioPlayer::Connect_LFO_TO_VCF(uint8_t modulation, float index, uint8_t periodic, float frequency_time) // used by Midi_Reader
{
    LFO_modulation = modulation;
    LFO_index = index;
    LFO_index_steps = 0;
    LFO_periodic = periodic;

    if (LFO_periodic == 1)
    {
        LFO_ptr->Set_frequency(frequency_time);
    }
    else
    {
        LFO_seconds = frequency_time;
    }
}

void AudioPlayer::Start_VCF(void)
{
    if (VCF_connect)
    {
        // VFC setup
        Update_pitch();
        VCF_central_frequency = VCF_frequency_pivot * pitch;

        // Serial.print("Player.h - VCF_connect - VCF_central_frequency:");
        // Serial.println(VCF_central_frequency);

        if (VCF_modulated)
        {
            if (LFO_periodic == 1)
            {
                // Serial.println("LFO Setup: PERIODIC");
                LFO_ptr->Set_amplitude(3000); // amplitude <= 32767
            }
            else
            {
                // Serial.println("LFO Setup: APERIODIC");
                LFO_ptr->Setup_aperiodic_wave(LFO_seconds, LFO_index * 3000, LFO_modulation); // LFO_ptr->SETUP_wave(LFO_seconds, LFO_index * 1000.0 * pitch, LFO_modulation); // SETUP_wave(float seconds, float max_value, int waveform) // aperiodic wave - period: seconds  n: waveform
            }
        }
    }
}

void AudioPlayer::Update_VCF_pivot(float new_pivot)
{
    VCF_pivot_target = new_pivot;
    VCF_pivot_steps = 40;
    VCF_pivot_grain = (new_pivot - VCF_frequency_pivot) / 40.0f;
}

void AudioPlayer::Update_LFO_index(float new_index)
{
    float delta_index = new_index - LFO_index;

    if (abs(delta_index) > 0.01)
    {
        LFO_index_steps = 1000;
        LFO_index_grain = (delta_index) / 1000.0f; // == /LFO_index_steps
    }
}

void AudioPlayer::Update_VCF_resonance(float resonance)
{
    VCF_ptr->q_value = resonance; // Butterworth: 0.7071
}

void AudioPlayer::Send_LFO_to_VCF(void)
{

    /*
     * Calculates the four VCF cutoff frequencies used while processing the next
     * audio block.
     *
     * The LFO output is sampled at the beginning and at one-quarter intervals of
     * the 128-sample block. Each LFO value is converted from a logarithmic pitch
     * offset into a cutoff-frequency multiplier using 2^x. The resulting frequency
     * is constrained to the supported VCF range of 50 Hz to 15 kHz.
     *
     * The first frequency is applied immediately by calling Set_filter(). The
     * remaining three frequencies are stored in VCF_frequency_array[] and applied
     * by AudioVCF::Update() at samples 32, 64, and 96. Distributing the coefficient
     * changes across the block produces smoother filter modulation and helps avoid
     * clicks and zipper noise.
     *
     * When the LFO modulation index or the VCF pivot frequency is being changed,
     * their values are advanced gradually at the same four positions within the
     * block. This prevents sudden parameter changes from reaching the filter.
     */

    VCF_ptr->LFO_connect = true;
    for (int section = 0; section < 4; ++section)
    {
        if (VCF_pivot_steps > 0)
        {
            VCF_frequency_pivot += VCF_pivot_grain;
            --VCF_pivot_steps;
            if (VCF_pivot_steps == 0)
            {
                VCF_frequency_pivot = VCF_pivot_target;
            }
            VCF_central_frequency = VCF_frequency_pivot * pitch; // Preserve the established pitch reference outside manual cutoff ramps.
        }
        if (VCF_modulated && LFO_index_steps > 0)
        {
            LFO_index += LFO_index_grain;
            --LFO_index_steps;
        }
        if (VCF_modulated)
        {
            VCF_frequency_array[section] = constrain(VCF_central_frequency * exp2f((LFO_ptr->block[section * AUDIO_BLOCK_SAMPLES / 4] / 1000.0f) * LFO_index), 50, 15000);
        }
        else
        {
            VCF_frequency_array[section] = VCF_frequency_pivot;
        }
    }
    VCF_ptr->Set_filter(0, VCF_frequency_array[0]);
}

float AudioPlayer::Mirror(float pivot, float value)
{
    return (2.0 * pivot) - value;
}

void AudioPlayer::Append_reversed(int16_t *target_ptr, uint16_t first_index, int16_t *source_ptr, uint16_t N)
{
    target_ptr += first_index;
    source_ptr += N - 1;

    for (auto i = 0; i < N; ++i)
    {
        *target_ptr = *source_ptr;
        ++target_ptr;
        --source_ptr;
    }
}

void AudioPlayer::Append(int16_t *target_ptr, uint16_t first_index, int16_t *source_ptr, uint16_t N)
{
    target_ptr += first_index;

    for (auto i = 0; i < N; ++i)
    {
        *target_ptr = *source_ptr;
        ++target_ptr;
        ++source_ptr;
    }
}

void AudioPlayer::Read_samples(int16_t *destination, int first_sample, int total_samples)
{
    if (total_samples > 4500)
    {
        Read_samples(destination, first_sample, 4500); // Keep each physical transfer within the calibrated range, including IIR history at 35x.
        Read_samples(destination + 4500, first_sample + 4500, total_samples - 4500);
        return;
    }
    char packet_filename[NAME_PACKET_SIZE];
    if (total_samples <= 0)
    {
        return;
    }
    if (source_now.storage == Psram)
    {
        first_sample -= static_cast<int>(source_now.first_sample);
        // The common reader preserves the existing forward, reverse and crossmix algorithms.
        const int first_valid = first_sample < 0 ? -first_sample : 0;
        const int available = static_cast<int>(source_now.samples) - first_sample;
        const int end_valid = available < total_samples ? available : total_samples;
        memset(destination, 0, static_cast<size_t>(total_samples) * sizeof(int16_t)); // Initialize the requested range to silence so unavailable PSRAM samples remain zero after copying valid data.
        if (source_now.psram_ptr != nullptr && end_valid > first_valid)
        {
            memcpy(destination + first_valid, source_now.psram_ptr + first_sample + first_valid, static_cast<size_t>(end_valid - first_valid) * sizeof(int16_t));
        }
        return;
    }
    int first_byte = (first_sample) * 2; // PD - 2
    int total_bytes = total_samples * 2; // 8
    byte *destination_byte = (byte *)destination;
    int first_part_samples;
    int second_part_bytes;

    /*
    Nella modalità SAMPLER, si devono leggere i file Packet di dimensione 64KB - PD = PACKET_DIMENSION = 64K

    Packet       |     first Packet 13         |          Packet 14          |          Packet 15          |
    local Byte   | 0 1 2  ............. (PD-1) | 0 1 2 3 4 .. 11 .... (PD-1) | 0 ..... 5........... (PD-1) |
    first/last                        fB  *                                    * * * * lB
    local_fB = PD - 2
    local_lB = 5
    */

    if (recording_flag)
    {
        int needed_packet_delta = (stereo_flag ? 2 : 1) * (first_byte >> 16); // 2*(PD - 2)/PD = 0

        // Serial.print("needed_packet_delta is: ");
        // Serial.println(needed_packet_delta);

        if (needed_packet_delta != packet_delta)
        {
            rawfile.close();
            packet_delta = needed_packet_delta; // 0
            rawfile.packet_fast_open(first_packet + packet_delta); // 13

            // Serial.print(F("1 - Packet played is: "));
        }

        int local_first_byte = first_byte % PACKET_DIM;           // (PD - 2)%PD = (PD - 2)
        int local_last_byte = local_first_byte + total_bytes - 1; // (PD - 2) + 8 - 1 = PD + 5

        if (local_last_byte < PACKET_DIM) // 1 Block is needed
        {
            rawfile.seek(local_first_byte);
            rawfile.read(destination_byte, total_bytes);
        }

        else // 2 Blocks are needed - with T41@600MHz adds 40microseconds
        {
            if (false)
            {
                Serial.print("2 Blocks are needed");
                Serial.print("first_sample: ");
                Serial.print(first_sample);
                Serial.print(" total_samples: ");
                Serial.println(total_samples);
                Serial.println();
            }

            // local_timer = 0;

            int first_part = PACKET_DIM - local_first_byte; // PD - (PD - 2) = 2
            int second_part = total_bytes - first_part;     // 8 - 2 = 6

            rawfile.seek(local_first_byte);             // (PD - 2)
            rawfile.read(destination_byte, first_part); // read 2 bytes
            rawfile.close();

            packet_delta = packet_delta + (stereo_flag ? 2 : 1); // 0 + 2 = 2
            rawfile.packet_fast_open(first_packet + packet_delta); // 15
            rawfile.seek(0);
            rawfile.read(destination_byte + first_part, second_part); // read 6 bytes

            if (false) // it true: uncomment local_timer = 0;
            {
                Serial.println("*** Player.h  ****  Flash reading_time is: ");
                Serial.println(local_timer);
                Serial.print(F("2 - Packet played is: "));
                Serial.println(Get_packet_name(first_packet + packet_delta, packet_filename));
                Serial.println();
            }
        }
    }

    else if (LS_flag)
    {
        while (first_sample > LS_buffer_dim - 1)
        {
            first_sample -= LS_buffer_dim;
        }

        while (first_sample < 0)
        {
            first_sample += LS_buffer_dim;
        }

        int last_sample = first_sample + total_samples - 1;

        if (last_sample <= LS_buffer_dim - 1)
        {
            memcpy(destination, (FIFO + first_sample), total_bytes);
        }

        else
        {
            first_part_samples = LS_buffer_dim - first_sample; // lenght in samples
            second_part_bytes = total_bytes - 2 * first_part_samples;
            memcpy(destination, (FIFO + first_sample), 2 * first_part_samples);
            memcpy(destination + first_part_samples, FIFO, second_part_bytes);
        }
    }

    else
    {
        rawfile.seek(first_byte);
        rawfile.read(destination_byte, total_bytes);
    }
}

int AudioPlayer::Read_patch_wait(void)
{
    return patch_id_wait;
}

bool AudioPlayer::Read_precedence(void)
{
    return precedence;
}

int AudioPlayer::Read_midi_channel(void)
{
    return midi_channel;
}

int AudioPlayer::Read_instrument(void)
{
    return instrument_id;
}

int AudioPlayer::Read_sound_id(void)
{
    return sound_id;
}

int AudioPlayer::Read_note(void)
{
    return note;
}

unsigned long AudioPlayer::Read_time_stamp(void)
{
    return time_stamp;
}

int AudioPlayer::Read_use_Wavetable(void)
{
    return use_Wavetable;
}

int AudioPlayer::State(void)
{
    return state;
}

int AudioPlayer::Read_update_time(void)
{
    return update_time;
}

void AudioPlayer::Write_midi_channel(int value)
{
    midi_channel = value;
}

void AudioPlayer::Write_precedence(bool value)
{
    precedence = value;
}

void AudioPlayer::Write_time_stamp(unsigned long value)
{
    time_stamp = value;
}

uint16_t AudioPlayer::Get_cache_reference_mask(void)
{
    if (state == IDLE)
    {
        return 0;
    }
    // An edit reads the current file until its crossmix completes; a restart can also read a different cache.
    uint16_t mask = source_now.cache_id >= 0 ? static_cast<uint16_t>(1u << source_now.cache_id) : 0;
    if ((warmup_for_play_again_flag || restart_flag) && source_wait.cache_id >= 0)
    {
        mask |= static_cast<uint16_t>(1u << source_wait.cache_id);
    }
    return mask;
}

uint8_t AudioPlayer::Get_tables_reference_mask(void)
{
    if (state == IDLE)
    {
        return 0;
    }

    uint8_t referenced_banks_mask = tables_bank_mask;

    if (warmup_for_play_again_flag || restart_flag)
    {
        referenced_banks_mask |= tables_bank_mask_wait;
    }

    if (main_settings_editing_flag)
    {
        referenced_banks_mask |= tables_bank_mask_E;
    }

    return referenced_banks_mask;
}

void AudioPlayer::Refresh_audio_table_references(AudioTables &tables)
{
    if (state != IDLE && tables_bank_mask != 0)
    {
        const AudioTables::Pointers replacement = tables.Get_replacement_pointers({Noclick_ptr, Wavetable_ptr, tables_bank_mask});
        Noclick_ptr = replacement.noclick;
        Wavetable_ptr = replacement.wavetable;
        tables_bank_mask = replacement.bank_mask;
    }
    if ((warmup_for_play_again_flag || restart_flag) && tables_bank_mask_wait != 0)
    {
        const AudioTables::Pointers replacement = tables.Get_replacement_pointers({Noclick_wait_ptr, Wavetable_wait_ptr, tables_bank_mask_wait});
        Noclick_wait_ptr = replacement.noclick;
        Wavetable_wait_ptr = replacement.wavetable;
        tables_bank_mask_wait = replacement.bank_mask;
    }
    if (main_settings_editing_flag && tables_bank_mask_E != 0)
    {
        const AudioTables::Pointers replacement = tables.Get_replacement_pointers({Noclick_E_ptr, Wavetable_E_ptr, tables_bank_mask_E});
        Noclick_E_ptr = replacement.noclick;
        Wavetable_E_ptr = replacement.wavetable;
        tables_bank_mask_E = replacement.bank_mask;
    }
}

bool AudioPlayer::Apply_preset_edit(int patch, int instrument, const Preset_struct &preset, const AudioTables::Pointers &tables)
{
    // A restart consumes the wait parameters and can discard an edit queued for the old note.
    if ((warmup_for_play_again_flag || restart_flag) && patch_id_wait == patch && instrument_id_wait == instrument && file_id_wait == preset.file)
    {
        Set_source(preset.source);
        Main_settings(preset.mode, preset.A, preset.B, preset.Noclick, preset.use_Wavetable, tables.noclick, tables.wavetable, tables.bank_mask);
    }
    if (state == IDLE || local_patch != patch || instrument_id != instrument || file_id != preset.file)
    {
        return false;
    }
    if (source_now.storage == Psram && file_id < FIRST_RECORDING_FILE && (static_cast<uint32_t>(preset.A) < source_now.first_sample || static_cast<uint32_t>(preset.B) >= source_now.first_sample + source_now.samples))
    {
        source_now = {};
        source_now.file_id = file_id;
        rawfile.fast_open(file_id);
        AudioStartUsingSPI();
        spi_in_use = true;
        pitch_limit = Playback_pitch_limit(use_Wavetable, false, false);
        Update_pitch();
    }
    Main_settings_editing(preset.mode, preset.A, preset.B, preset.Noclick, preset.use_Wavetable, tables.noclick, tables.wavetable, tables.bank_mask);
    return true;
}

void AudioPlayer::Update_tone_filter(int patch, int instrument, uint8_t level)
{
    if (local_patch == patch && instrument_id == instrument)
    {
        tone_filter_level = level;
    }
    if (Has_pending_note() && patch_id_wait == patch && instrument_id_wait == instrument)
    {
        tone_filter_level_wait = level;
    }
}

float AudioPlayer::Read_rendered_sample(int sample, float position) const
{
    return antialias.mix > 0.0f ? resampled_basket[sample] : PlayerInterpolator::Read(samples_basket, position, interpolation_mix);
}

bool AudioPlayer::Has_pending_edit(void) const
{
    return main_settings_editing_flag;
}

void AudioPlayer::Set_tone_filter(uint8_t level)
{
    tone_filter_level_wait = level;
}

bool AudioPlayer::Has_pending_note(void) const
{
    return state != IDLE && (warmup_for_play_again_flag || restart_flag);
}

bool AudioPlayer::Needs_restart_mix(void) const
{
    return state != IDLE && warmup_for_play_again_flag;
}

int AudioPlayer::Assigned_note(void) const
{
    return Has_pending_note() ? note_wait : note;
}

int AudioPlayer::Assigned_track(void) const
{
    return Has_pending_note() ? track_wait : track;
}

int AudioPlayer::Assigned_instrument(void) const
{
    return Has_pending_note() ? instrument_id_wait : instrument_id;
}

int AudioPlayer::Assigned_patch(void) const
{
    return Has_pending_note() ? patch_id_wait : local_patch;
}
