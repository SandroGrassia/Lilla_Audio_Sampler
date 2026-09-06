/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#pragma once

#include <Arduino.h>
#include "SharedElements.h"
#include "NoclickCrossmix.h"
#include "WavetableManager.h"

class AudioTables
{
public:
    static constexpr uint8_t BANK_COUNT = 2;

    struct Pointers
    {
        int16_t *noclick = nullptr;
        int16_t *wavetable = nullptr;
        uint8_t bank_mask = 0;
    };

private:
    static_assert(INSTRUMENTS > 0 && INSTRUMENTS <= 16);

    enum BankState : uint8_t
    {
        Free,
        Preparing,
        Active,
        Retiring
    };

    struct Bank
    {
        int16_t Noclick[INSTRUMENTS][NOCLICK_DIM] = {};
        int16_t Wavetable[INSTRUMENTS][WavetableManager::FULL_WAVETABLE_DIM] = {};
        uint16_t required_mask = 0;
        uint16_t prepared_mask = 0;
    };

    NoclickCrossmix Noclick_generator;
    WavetableManager Wavetable_generator;
    Bank banks[BANK_COUNT];

    volatile BankState states[BANK_COUNT] = {Free, Free};
    volatile int8_t active_bank = -1;
    int8_t preparing_bank = -1;

public:
    AudioTables();
    AudioTables(const AudioTables &) = delete;
    AudioTables &operator=(const AudioTables &) = delete;

    // Main, con IRQ audio disabilitati: prenota un banco Free.
    bool Begin_prepare(uint16_t used_instruments_mask);

    // Main, con IRQ audio abilitati: costruisce le tabelle.
    bool Prepare_instrument(uint8_t instrument_id, const Preset_struct &preset);

    // Main, con IRQ audio abilitati, dopo Begin_prepare() riuscito.
    // Annulla la preparazione se un Instrument segnala un errore.
    bool Prepare_all(const Preset_struct (&presets)[INSTRUMENTS]);

    // Main, con IRQ audio disabilitati: pubblica tutte le tabelle insieme.
    bool Activate_prepared(void);

    // Main: abbandona una preparazione non ancora pubblicata.
    void Cancel_prepare(void);

    // Chiamata nell'IRQ audio oppure con IRQ audio disabilitati.
    Pointers Get_active_pointers(uint8_t instrument_id);

    // Chiamata dal finalizzatore dopo aver raccolto i riferimenti dei Player.
    void Release_unreferenced_banks(uint8_t referenced_banks_mask);
};