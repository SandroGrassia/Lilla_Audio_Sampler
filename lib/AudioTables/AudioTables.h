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

    struct TableSettings
    {
        uint16_t file = 0;
        uint8_t mode = 0;
        int A = 0;
        int B = 0;
        int Noclick = 0;
        bool use_Wavetable = false;
    };

    struct Bank
    {
        int16_t Noclick[INSTRUMENTS][NOCLICK_DIM] = {};
        int16_t Wavetable[INSTRUMENTS][WavetableManager::FULL_WAVETABLE_DIM] = {};
        TableSettings settings[INSTRUMENTS] = {};
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
    AudioTables(); // Initialize both table banks and their generators.
    AudioTables(const AudioTables &) = delete; // Keep table storage at a stable address.
    AudioTables &operator=(const AudioTables &) = delete; // Prevent copying banks referenced by players.

    bool Begin_prepare(uint16_t used_instruments_mask); // Reserve a free bank with audio interrupts disabled.
    bool Prepare_instrument(uint8_t instrument_id, const Preset_struct &preset); // The caller must keep audio SPI use registered throughout Prepare_instrument() or Prepare_all().
    bool Prepare_all(const Preset_struct (&presets)[INSTRUMENTS]); // The caller must keep audio SPI use registered throughout Prepare_instrument() or Prepare_all().
    bool Activate_prepared(void); // Publish a complete bank with audio interrupts disabled.
    void Cancel_prepare(void); // Discard the unpublished bank after preparation fails.
    Pointers Get_active_pointers(uint8_t instrument_id); // Inspect an active slot from the audio IRQ or with audio interrupts disabled.
    Pointers Get_active_pointers(uint8_t instrument_id, const Preset_struct &preset); // Return pointers only when the active tables match the requested preset. Call from the audio IRQ or with audio interrupts disabled. 
    static bool Needs_tables(const Preset_struct &preset); // Identify presets that read generated tables.
    Pointers Get_replacement_pointers(const Pointers &previous); // Find equivalent active tables without changing playback parameters; call with audio interrupts disabled.
    bool Reset(uint8_t referenced_banks_mask); // Reset bank metadata with audio interrupts disabled, after all players have released their references.
    uint8_t Get_retiring_banks_mask(void) const; // Return the mask of retiring banks waiting to be released. Call from the audio IRQ or with audio interrupts disabled. 
    void Release_unreferenced_banks(uint8_t referenced_banks_mask); // Release retiring banks after all current and pending player references have been collected.
};