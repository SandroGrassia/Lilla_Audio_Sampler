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

class AudioTables // Own generated audio tables and coordinate preparation, activation and retirement of two banks.
{
public:
    static constexpr uint8_t BANK_COUNT = 2; // Keep two banks so new tables can be prepared while existing voices retain the previous bank.

    struct Pointers // Group playback table addresses with the bank references needed to keep them valid.
    {
        int16_t *noclick = nullptr; // Address of the instrument crossfade samples, or null when no table is available.
        int16_t *wavetable = nullptr; // Address of the instrument wavetable samples, or null when no table is available.
        uint8_t bank_mask = 0; // Bit mask identifying the banks that must remain allocated while these pointers are used.
    };

private:
    static_assert(INSTRUMENTS > 0 && INSTRUMENTS <= 16); // Ensure every instrument fits in the 16-bit preparation masks.

    enum BankState : uint8_t // Describe the lifetime of each table bank.
    {
        Free, // Available for a new preparation.
        Preparing, // Reserved for table generation and not yet published to players.
        Active, // Published as the source of tables for new notes.
        Retiring // No longer active, but retained until existing player references disappear.
    };

    struct TableSettings // Record the preset parameters used to generate an instrument table slot.
    {
        uint16_t file = 0;
        uint8_t mode = 0; // Playback mode used to build the tables.
        int A = 0;
        int B = 0;
        int Noclick = 0; // Requested crossfade length in samples.
        bool use_Wavetable = false; // Whether preparation includes a wavetable for this instrument.
    };

    struct Bank // Store all instrument tables and preparation metadata for one bank.
    {
        int16_t Noclick[INSTRUMENTS][NOCLICK_DIM] = {}; // Per-instrument crossfade sample storage.
        int16_t Wavetable[INSTRUMENTS][WavetableManager::FULL_WAVETABLE_DIM] = {}; // Per-instrument wavetable sample storage.
        TableSettings settings[INSTRUMENTS] = {}; // Generation parameters used to validate each instrument slot against a preset.
        uint16_t required_mask = 0; // Instrument slots required by the current preparation request.
        uint16_t prepared_mask = 0; // Instrument slots whose tables have been generated successfully.
    };

    NoclickCrossmix Noclick_generator; // Generate crossfade samples into the selected preparation bank.
    WavetableManager Wavetable_generator; // Generate wavetable samples into the selected preparation bank.
    Bank banks[BANK_COUNT]; // Own both banks at stable addresses for the lifetime of this object.

    volatile BankState states[BANK_COUNT] = {Free, Free}; // Track bank ownership shared with audio callbacks; access requires the documented IRQ discipline.
    volatile int8_t active_bank = -1; // Index of the published bank, or -1 when no bank is active.
    int8_t preparing_bank = -1; // Index reserved by Begin_prepare(); returns to -1 after successful activation or cancellation.

public:
    AudioTables(); // Initialize both table banks and their generators.
    AudioTables(const AudioTables &) = delete; // Keep table storage at a stable address.
    AudioTables &operator=(const AudioTables &) = delete; // Prevent copying banks referenced by players.

    bool Begin_prepare(uint16_t used_instruments_mask); // Reserve a free bank with audio interrupts disabled.
    bool Prepare_instrument(uint8_t instrument_id, const Preset_struct &preset); // Generate one required slot in the bank reserved by Begin_prepare(); keep audio SPI use registered throughout the call.
    bool Prepare_all(const Preset_struct (&presets)[INSTRUMENTS]); // Generate every required slot in the reserved bank, cancelling on failure; keep audio SPI use registered throughout the call.
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