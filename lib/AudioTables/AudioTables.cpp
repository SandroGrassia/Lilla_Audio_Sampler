/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "AudioTables.h"

AudioTables::AudioTables() = default;

bool AudioTables::Begin_prepare(uint16_t used_instruments_mask)
{
    const uint16_t valid_mask = static_cast<uint16_t>((1u << INSTRUMENTS) - 1u);

    if (preparing_bank >= 0 || (used_instruments_mask & ~valid_mask) != 0)
    {
        return false;
    }

    for (uint8_t bank_id = 0; bank_id < BANK_COUNT; ++bank_id)
    {
        if (states[bank_id] != Free)
        {
            continue;
        }

        states[bank_id] = Preparing;
        banks[bank_id].required_mask = used_instruments_mask;
        banks[bank_id].prepared_mask = 0;
        preparing_bank = bank_id;
        return true;
    }

    return false;
}

bool AudioTables::Prepare_instrument(uint8_t instrument_id, const Preset_struct &preset)
{
    if (preparing_bank < 0 || instrument_id >= INSTRUMENTS)
    {
        return false;
    }

    Bank &bank = banks[preparing_bank];
    const uint16_t instrument_mask = static_cast<uint16_t>(1u << instrument_id);

    if ((bank.required_mask & instrument_mask) == 0)
    {
        return false;
    }

    // Un nuovo tentativo invalida l'eventuale preparazione precedente.
    bank.prepared_mask &= static_cast<uint16_t>(~instrument_mask);

    // Le tabelle sono destinate ai file RAW e alle registrazioni su Flash.
    // Il Live Sampler mantiene il proprio percorso di lettura.
    if (preset.file >= FIRST_LIVE_SAMPLING_FILE || preset.A < 0 || preset.B < preset.A || preset.mode > LOOP_REV)
    {
        return false;
    }

    const uint32_t span = static_cast<uint32_t>(preset.B - preset.A) + 1u;
    const bool crossfade_mode = preset.mode == LOOP_FWD || preset.mode == LOOP_REV;
    const int noclick_samples = crossfade_mode ? preset.Noclick : 0;

    // Make() di Noclick divide per delta_Noclick - 1.
    if (noclick_samples < 0 || noclick_samples == 1 || noclick_samples > NOCLICK_DIM)
    {
        return false;
    }

    if (2u * static_cast<uint32_t>(noclick_samples) > span)
    {
        return false;
    }

    // Verifica i limiti del buffer di lavoro del generatore.
    if (preset.use_Wavetable && (span < 2u || span > WavetableManager::WAVETABLE_DIM))
    {
        return false;
    }

    bool success = true;

    if (noclick_samples > 0)
    {
        success = Noclick_generator.Make(preset.file, preset.A, preset.B, noclick_samples, bank.Noclick[instrument_id]);
    }

    if (success && preset.use_Wavetable)
    {
        success = Wavetable_generator.Make(preset.file, preset.mode, preset.A, preset.B, noclick_samples, bank.Noclick[instrument_id], bank.Wavetable[instrument_id]);
    }

    if (!success)
    {
        return false;
    }

    // Record the preparation parameters before marking the instrument as ready.
    TableSettings &settings = bank.settings[instrument_id];
    settings.file = preset.file;
    settings.mode = preset.mode;
    settings.A = preset.A;
    settings.B = preset.B;
    settings.Noclick = preset.Noclick;
    settings.use_Wavetable = preset.use_Wavetable;

    bank.prepared_mask |= instrument_mask;
    return true;
}

bool AudioTables::Prepare_all(const Preset_struct (&presets)[INSTRUMENTS])
{
    if (preparing_bank < 0)
    {
        return false;
    }

    const uint16_t required_mask = banks[preparing_bank].required_mask;

    for (uint8_t instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
    {
        const uint16_t instrument_mask = static_cast<uint16_t>(1u << instrument_id);

        if ((required_mask & instrument_mask) == 0)
        {
            continue;
        }

        if (!Prepare_instrument(instrument_id, presets[instrument_id]))
        {
            Cancel_prepare();
            return false;
        }
    }

    return true;
}

bool AudioTables::Activate_prepared(void)
{
    if (preparing_bank < 0)
    {
        return false;
    }

    const uint8_t next_bank = static_cast<uint8_t>(preparing_bank);

    if (banks[next_bank].prepared_mask != banks[next_bank].required_mask)
    {
        return false;
    }

    const int8_t previous_bank = active_bank;

    if (previous_bank >= 0)
    {
        states[previous_bank] = Retiring;
    }

    states[next_bank] = Active;
    active_bank = next_bank;
    preparing_bank = -1;
    return true;
}

void AudioTables::Cancel_prepare(void)
{
    if (preparing_bank < 0)
    {
        return;
    }

    states[preparing_bank] = Free;
    preparing_bank = -1;
}

AudioTables::Pointers AudioTables::Get_active_pointers(uint8_t instrument_id)
{
    Pointers result;
    const int8_t bank_id = active_bank;

    if (bank_id < 0 || instrument_id >= INSTRUMENTS)
    {
        return result;
    }

    const uint16_t instrument_mask = static_cast<uint16_t>(1u << instrument_id);

    if ((banks[bank_id].required_mask & instrument_mask) == 0)
    {
        return result;
    }

    result.noclick = banks[bank_id].Noclick[instrument_id];
    result.wavetable = banks[bank_id].Wavetable[instrument_id];
    result.bank_mask = static_cast<uint8_t>(1u << bank_id);
    return result;
}

AudioTables::Pointers AudioTables::Get_active_pointers(uint8_t instrument_id, const Preset_struct &preset)
{
    Pointers result;
    const int8_t bank_id = active_bank;

    if (bank_id < 0 || instrument_id >= INSTRUMENTS)
    {
        return result;
    }

    Bank &bank = banks[bank_id];
    const uint16_t instrument_mask = static_cast<uint16_t>(1u << instrument_id);

    if ((bank.required_mask & instrument_mask) == 0 || (bank.prepared_mask & instrument_mask) == 0)
    {
        return result;
    }

    const TableSettings &settings = bank.settings[instrument_id];

    // Reject tables prepared for different playback parameters.
    if (settings.file != preset.file || settings.mode != preset.mode || settings.A != preset.A || settings.B != preset.B || settings.Noclick != preset.Noclick || settings.use_Wavetable != preset.use_Wavetable)
    {
        return result;
    }

    result.noclick = bank.Noclick[instrument_id];
    result.wavetable = bank.Wavetable[instrument_id];
    result.bank_mask = static_cast<uint8_t>(1u << bank_id);
    return result;
}

void AudioTables::Release_unreferenced_banks(uint8_t referenced_banks_mask)
{
    for (uint8_t bank_id = 0; bank_id < BANK_COUNT; ++bank_id)
    {
        const uint8_t bank_mask = static_cast<uint8_t>(1u << bank_id);

        if (states[bank_id] == Retiring && (referenced_banks_mask & bank_mask) == 0)
        {
            states[bank_id] = Free;
        }
    }
}
