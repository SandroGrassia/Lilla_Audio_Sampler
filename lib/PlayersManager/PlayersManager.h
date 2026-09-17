/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#pragma once

#include <Arduino.h>
#include "SharedElements.h"
#include "SharedDelay.h"
#include "SharedMixer.h"
#include "AudioPlayer.h"
#include "LoopLedSet.h"
#include "PlayersStatistics.h"
#include "Router_16x3.h"
#include "config.h"
#include "AudioADSR.h"
#include "AudioTables.h"
#include "PatchCacheManager.h"

class PlayersManager
{
public:
    struct ReadBudgetDiagnostics // Scheduling evidence captured before rendering and retained with the matching audio block.
    {
        uint32_t cycle = 0; // Audio cycle in which the allocation was made.
        bool valid = false; // False when the collector has no matching scheduling pass.
        float reserved_us = 0.0f; // Current/pending full-block reservation used by the allocator.
        float read_headroom_us = 0.0f; // Read limit minus reserved time, before crossfade allocation.
        float deadline_headroom_us = 0.0f; // Deadline minus elapsed preparation, reservation and the existing 700 us processing allowance.
        float scheduler_elapsed_us = 0.0f; // Elapsed block time at the deadline check.
        float pre_players_us = 0.0f; // Elapsed block time after scheduling, immediately before returning to the audio clock.
        float available_us = 0.0f; // Actual initial crossfade pool after clamping and integer rounding.
        float crossfade_us = 0.0f; // Sum of the outgoing harvest and mixing allowances assigned in this pass.
        uint8_t first_player = 0; // Rotation origin; reconstruct the allocation order from this index.
        uint8_t transition[PLAYERS] = {}; // Requested transitions: 0=none, 1=restart, 2=edit, 3=both; assignment is not proof of execution.
        uint8_t mix_samples[PLAYERS] = {}; // Crossfade samples selected for this block, zero if none was affordable.
        float available_before_us[PLAYERS] = {}; // Remaining shared pool when each requested transition was examined.
        float assigned_us[PLAYERS] = {}; // Time charged for the selected outgoing segment and mixing.
        float minimum_us[PLAYERS] = {}; // Cost of the rejected 16-sample tier when a transition receives zero; otherwise zero.
    };

    struct ReadDiagnosticsSnapshot
    {
        PlayerReadDiagnostics players[PLAYERS];
        float estimated_us[PLAYERS] = {};
        float total_estimated_us = 0.0f;
        float total_harvest_us = 0.0f;
        uint32_t cycle = 0;
        uint32_t blocks = 0;
        uint32_t uncovered_operations = 0;
        uint16_t restarted_players = 0;
        ReadBudgetDiagnostics budget; // Copy of the scheduling evidence for this exact snapshot cycle.
    };

private:
    ReadBudgetDiagnostics read_budget_diagnostics; // Current scheduling pass; updated only while read diagnostics are enabled.
    ReadDiagnosticsSnapshot read_diagnostics_restart; // Highest estimated block with an actually executed restart.
    ReadDiagnosticsSnapshot read_diagnostics_gap; // Largest positive measured-harvest minus estimated-transfer difference; only covered blocks.
    bool read_diagnostics_enabled = false;
    ReadDiagnosticsSnapshot read_diagnostics_last;
    ReadDiagnosticsSnapshot read_diagnostics_peak; // Complete snapshot of the block with the largest estimated total, not a sum of independent player peaks.

    // puntatori esterni
    AudioPlayer *Player_ptr = nullptr;
    Router_16x3 *Router_L_ptr = nullptr;
    Router_16x3 *Router_R_ptr = nullptr;
    AudioTables *Audio_tables_ptr = nullptr;
    PatchCacheManager *Cache_manager_ptr = nullptr;
    int Count_sample_voices(void) const; // Count each current or reserved Flash/cache player once, including release tails.
    AudioADSR *ADSR = nullptr;
    AudioTables::Pointers Get_playback_tables(uint8_t instrument_id); // Call from the audio IRQ or with audio interrupts disabled.

    // statistiche
    int players_playing = 0;
    int players_using_Flash = 0;
    int players_using_Wavetable = 0;
    int players_using_Psram = 0;
    uint8_t players_to_restart = 0; // numero di Player che devono ripartire; la ripartenza richiede una doppia lettura di campioni da vecchio e nuovo file ed il calcolo di mix_samples fatto dalla funzione Calculate_and_set_mix_samples

    uint32_t budget_rejected_notes = 0; // Notes rejected without modifying the selected voices.
    uint32_t budget_retired_players = 0; // Source-free retirements required by admission or later edits.
    uint32_t budget_forced_protected = 0; // Last-resort retirements after an edit exceeds the budget with only protected voices left.
    float budget_reserved_us = 0.0f; // Full-block reservation at the last scheduling pass.
    float budget_crossfade_us = 0.0f; // Additional outgoing reads assigned for that block.
    bool Admit_read_budget(int target, uint8_t instrument, float incoming_us); // Commit all necessary victims only after a complete feasible selection.
    PlayerReadBudget::Plan New_read_plan(uint8_t instrument, float note_pitch) const; // Forecast the same source and geometry used by Main_settings.

    // Play notes
    bool Player_booked[PLAYERS] = {false};
    uint8_t modulation_value[16] = {0}; // Independent CC1 state for active, pending and future notes on each channel.
    bool midi_batch_active = false;
    uint8_t restart_mix_first_player = 0; // Rotate only the individual restart-crossfade fallback; selection priorities remain unchanged.

    enum class Selection_order { First, Last, Oldest };
    struct Player_filter
    {
        int instrument = -1; // -1 means any instrument/state.
        int powered = -1;
        int playing = -1;
        bool unprotected = false;
        bool sample_only = false;
        bool other_patch = false;
    };
    bool Can_select_player(int player, const Player_filter &filter) const;
    int Find_player(const Player_filter &filter, Selection_order order);
    int Find_free_player(void);
    int Find_other_patch_player(bool sample_only, bool prefer_last);
    int Select_player_for_note(uint8_t instrument_id, uint8_t note_number, int track);
    bool restart_Player[PLAYERS] = {false}; // questo array serve per contare, ad ogni ciclo, il numero di Player che devono ripartire; la ripartenza richiede una doppia lettura di campioni da vecchio e nuovo file ed il calcolo di mix_samples fatto dalla funzione Calculate_and_set_mix_samples

    static inline float Calc_pitch(float value)
    {
        return pow(2.0f, value / 192.0f); // 0: no shift
    }

    static inline float Calc_attack(float value)
    {
        return (value / 100.0f);
    }

    static inline float Calc_decay(float value)
    {
        return (value / 25.0f);
    }

    static inline float Calc_sustain(float value)
    {
        return (value / 50.0f);
    }

    static inline float Calc_release(float value)
    {
        return (value / 2.0f);
    }

public:
    struct ReadBudgetStatus // Copy with audio interrupts disabled; counters run independently of READ_DIAG.
    {
        float reserved_us; // Last scheduled full-block reservation at maximum modulation.
        float crossfade_us; // Additional outgoing harvest allowance.
        uint32_t rejected_notes; // Cumulative admission failures.
        uint32_t retired_players; // Cumulative budget-driven retirements.
        uint32_t forced_protected; // Cumulative forced retirements after runtime changes.
    };
    ReadBudgetStatus Get_read_budget_status(void) const { return {budget_reserved_us, budget_crossfade_us, budget_rejected_notes, budget_retired_players, budget_forced_protected}; } // Snapshot only; no IRQ serial output.
    float Reserved_read_us(void) const; // Sum current and pending reservations without stale idle costs.
    void Prepare_read_budget(void); // Revalidate edits and source changes before any Player renders.

    using ReadSource = PlayerReadSource; // Select seek+read, zero+copy, or RAM2 copy into RAM1 respectively.
    [[nodiscard]] static bool Get_read_time_us(ReadSource source, uint32_t samples, float &time_us); // Linear cold-cache estimate in us; zero samples cost zero, 1..9 use 10. Invalid source or count >4500 returns false and infinity. Sum separate calls for multiple reads; this excludes other Player processing.

    [[nodiscard]] static bool Get_read_usage_time_us(ReadSource source, const PlayerReadUsage &usage, float &time_us); // Sum the affine model per operation, including its fixed cost and per-read minimum.
    void Enable_read_diagnostics(bool enabled); // Call with audio IRQ disabled; enabling resets the observation window.
    void Collect_read_diagnostics(void); // Called after ALL Players by CacheCycleFinalizer; never changes voice selection.
    bool Copy_read_diagnostics(ReadDiagnosticsSnapshot &last, ReadDiagnosticsSnapshot &peak, ReadDiagnosticsSnapshot &restart, ReadDiagnosticsSnapshot &gap) const; // Call with audio IRQ disabled, then print outside the critical section.

    PlayersManager(AudioPlayer *P, Router_16x3 *RL, Router_16x3 *RR, AudioTables *AT, PatchCacheManager *PC) : Player_ptr(P), Router_L_ptr(RL), Router_R_ptr(RR), Audio_tables_ptr(AT), Cache_manager_ptr(PC) {} // Connect players, routers, tables and the file cache owner.
    void Set_ADSR_ptr(AudioADSR* ptr); // requires &ADSR[0] from main.cpp

    // chiamate da MidiReader
    void Reset_booked_and_restart_player(void);
    void Begin_midi_batch(void);
    void End_midi_batch(void);
    void Set_modulation(uint8_t midi_channel, uint8_t value);
    void Reset_players_to_restart(void);

    /*
    Play_note, chiamata da:
    MidiReader a seguito di
    - NoteOn da MIDI
    - evento NoteOn da Midi Loop

    Compiti:
    - individua il Player (id_player) da utilizzare
    - se Player(id_player).isPlaying() incrementa players_to_restart, utilizzata da void Calculate_and_set_mix_samples()
    - chiama le funzioni di AudioPlayer che inizializzano Player(id_player) utilizzando i valori di Preset
    - chiama le funzioni di AudioPlayer che loro volta inizializzano immediatamente il VCF (anche se il Player(id_player) va in restart)
    - chiama la funzione AudioPlayer::Get_ready_to_play
    - aggiorna Router_L_ptr/R per instradamento verso Delay si/no
    - aggiorna PlayersStatistics per l'aggiornamento dei led degli Instrument
    - richiede a main() di aggiornare i led
    */

    void MX_multicast_change_routing(int instrument_id);
    void Play_note(uint8_t instrument_id, uint8_t note_number, float velocity_float, int track);

    int Get_players_to_restart(void);
    void Calculate_and_set_mix_samples(void);
    void Multicast_stop_players_for_NoteOff(int midi_channel, int note_number, int track);
    void Broadcast_pitch_bend(int midi_channel, float value);
    void Multicast_all_notes_off(int midi_channel);
    void Multicast_update_vibrato(int midi_channel, bool vibrato_active);
    void Multicast_stop_players_for_loop_track(int track);

    void Update_players_stistics(void); // Count Wavetable, Flash and PSRAM readers separately.
    int Get_players_playing(void);
    int Get_players_using_Flash(void);
    int Get_players_using_Wavetable(void); // Return the number of voices reading wavetables.
    int Get_players_using_Psram(void); // Return the number of voices reading cached or live PSRAM samples.
    void Refresh_cache_sources(void); // Publish ready sources and promote identical current data; call with audio interrupts disabled.
    uint16_t Get_cache_reference_mask(void); // Combine current and queued player references; call with audio interrupts disabled.
    uint16_t Fast_stop_players_using_cache(uint16_t mask); // Fade only readers of the cache selected for reclamation.
    void Release_Player_noteOff(uint8_t player, int track = NO_TRACK);

    bool Get_restart_player(int player);
    void Cancel_restart_player(int player);

    float Get_cross_mix_time(int player, int mix_samples);
    int Get_span_for_all_cross_mix(void);

    int Smartfind_oldest_player(uint8_t instrument_id, bool power_on, bool playing);
    int Simplefind_oldest_player(bool power_on);                     // "precedence" instruments are EXCLUDED
    int Simplefind_oldest_sample_player(bool power_on, bool playing); // Reuse a shared Flash/cache slot; protected and booked players are excluded.

    void Change_from_key(int patch_id, int instrument_id, int from_key_new);
    void Change_to_key(int patch_id, int instrument_id, int to_key_new);
    void Multicast_change_players_notes(int patch_id, int instrument_id);
    bool Get_use_Wavetable(int sound_id);
    int8_t Find_oldest_player(int instrument_id, bool power_on, bool playing);

    void Release_player(int player);
    void Release_all_players_for_instrument(int instrument_id);
    void Release_all_players_for_instrument_solo(int instrument_id);
    void Release_all_players(void);
    void Release_softly_all_players(int patch_id); // Stop outgoing patch voices within QUICK_RELEASE_TIME, including existing release tails.
    void Stop_all_players(void); // BROADCAST_stop_all_Players()
    uint16_t Fast_stop_players_using_tables(uint8_t banks_mask); // Return a player bitmask identifying newly requested stops. Call from the audio IRQ or with audio interrupts disabled.

    void Release_player(int player, int track);
    void Release_all_players_loop(void);
    void Release_all_players_loop(int track);
    void Multicast_volume_for_MIDI_LOOP_running(int track, float value);

    void Multicast_release_players(int sound_id);
    void Broadcast_volume(void);
    void Multicast_volume_for_instrument_edit(int instrument_id); // chiamata da main e MidiReader
    void Multicast_pitch_for_sound_edit(int instrument_id);
    void Multicast_pan(int instrument_id);

    void Multicast_effects(float resolution, uint8_t downsampling);
    void Broadcast_reset_effect(float resolution, uint8_t downsampling, int effect);

    void Multicast_main_settings_editing(int patch_id, int instrument_id);
    void Multicast_reset_pitch_bend_effects(int instrument_id);
    void Broadcast_restore_pitch_bend_and_effects(int midi_channel, float value);

    void Update_all_Preset(int patch_id, float volume_patch); // Publish the active instruments and refresh their pinned cache sources.
    void Update_all_Preset_volume(int patch_id, float volume_patch);
    
    Preset_struct Build_Preset(int patch_id, int instrument_id, float volume_patch); // Build a preset without global Preset array modifications.
    bool Build_presets_snapshot(int patch_id, float volume_patch, Preset_struct (&presets)[INSTRUMENTS], uint16_t &tables_mask); // Call from main.cpp with AudioNoInterrupts(). The destination must be a separate snapshot array.
    uint8_t Refresh_audio_table_references(void); // Move equivalent current and pending references, then return all referenced banks; call with audio interrupts disabled.
    bool Activate_prepared_presets(const Preset_struct (&presets)[INSTRUMENTS]); // Call with AudioNoInterrupts(), using the same snapshot passed to AudioTables::Prepare_all().
    void Update_Preset(int patch_id, int instrument_id, float volume_patch);
    void Update_Preset_volume(int patch_id, int instrument_id, float volume_patch); // chiamata da main e MidiReader
    void Update_Preset_pan(int patch_id, int instrument_id);
    void Update_Preset_sound_id(int patch_id, int instrument_id);
    void Update_Preset_file(int patch_id, int instrument_id);
    void Update_Preset_midi_channel(int patch_id, int instrument_id);
    void Update_Preset_pitch(int patch_id, int instrument_id);
    void Update_Preset_mode(int patch_id, int instrument_id);
    void Update_Preset_A_B_Wavetable(int patch_id, int instrument_id);
    void Update_Preset_Noclick(int patch_id, int instrument_id);
    void Update_Preset_attack_type(int patch_id, int instrument_id);
    void Update_Preset_attack(int patch_id, int instrument_id);
    void Update_Preset_decay(int patch_id, int instrument_id);
    void Update_Preset_sustain(int patch_id, int instrument_id);
    void Update_Preset_release(int patch_id, int instrument_id);
    void Update_Preset_precedence(int patch_id, int instrument_id);
    void Update_Preset_lock(int patch_id, int instrument_id);

    void Multicast_IF_update_filter_type(int instrument_id);
    void Update_IF_resonance(int patch_id, int instrument_id);
    void Multicast_IF_pivot(int instrument_id);
    void Multicast_IF_frequency_filter(int instrument_id);
    void Multicast_IF_resonance(int instrument_id);
    void Multicast_IF_index(int instrument_id, float value); // chiamata da main e MidiReader
    void Update_Preset_IF(int patch_id, int instrument_id);
    void Update_Preset_IF_resonance(int patch_id, int instrument_id);
    void Update_Preset_IF_filter_type(int patch_id, int instrument_id);
    void Update_Preset_IF_modulation(int patch_id, int instrument_id);
    void Update_Preset_IF_index(int patch_id, int instrument_id);

    void Broadcast_FIFO_stereo(int16_t *_LS_buffer_L, int16_t *_LS_buffer_R); // chiamata da main LIVE_SAMPLER
    void Broadcast_FIFO_mono(int16_t *_LS_buffer_mono);                       // chiamata da main LIVE_SAMPLER
};
