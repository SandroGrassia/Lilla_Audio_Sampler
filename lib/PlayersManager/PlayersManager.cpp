#include "PlayersManager.h"
#include <algorithm>

namespace
{
// Calibration procedure and archived measurements: docs/read-calibration.md.


// Least-squares fits to the original exact_mean_us measurements (before the table's cumulative maximum).
// PSRAM/RAM use cold source cache; Flash uses seek+read. Units: samples -> microseconds per operation.
// Validated at 600 MHz over 10..4500 samples. Mean estimates, not worst-case timing guarantees.
constexpr float Flash_read_time_us(uint32_t samples)
{
    return PlayerReadBudget::Transfer_us(PlayerReadSource::Flash, samples);
}

constexpr float Psram_read_time_us(uint32_t samples)
{
    return PlayerReadBudget::Transfer_us(PlayerReadSource::Psram, samples);
}

constexpr float Ram_read_time_us(uint32_t samples)
{
    return PlayerReadBudget::Transfer_us(PlayerReadSource::Ram, samples);
}
}

bool PlayersManager::Get_read_time_us(ReadSource source, uint32_t samples, float &time_us)
{
    time_us = INFINITY; // A failed estimate must never look like a free operation if a caller ignores the status.
    if (source != ReadSource::Flash && source != ReadSource::Psram && source != ReadSource::Ram)
    {
        return false;
    }
    if (samples == 0)
    {
        time_us = 0.0f;
        return true;
    }
    if (samples > 4500)
    {
        return false;
    }
    const uint32_t measured_samples = samples < 10 ? 10 : samples; // Preserve the conservative minimum for nonzero requests below the measured range.
    switch (source)
    {
        case ReadSource::Flash:
            time_us = Flash_read_time_us(measured_samples);
            break;
        case ReadSource::Psram:
            time_us = Psram_read_time_us(measured_samples);
            break;
        case ReadSource::Ram:
            time_us = Ram_read_time_us(measured_samples);
            break;
    }
    return true;
}

bool PlayersManager::Get_read_usage_time_us(ReadSource source, const PlayerReadUsage &usage, float &time_us)
{
    time_us = INFINITY;
    if (source != ReadSource::Flash && source != ReadSource::Psram && source != ReadSource::Ram)
    {
        return false;
    }
    if (usage.uncovered_operations != 0)
    {
        return false;
    }
    if (usage.operations == 0)
    {
        time_us = 0.0f;
        return true;
    }
    // Each individual operation was range-checked by Add(); their combined sample count may exceed 4500.
    const float extra_operations = static_cast<float>(usage.operations - 1u);
    switch (source)
    {
        case ReadSource::Flash:
            time_us = Flash_read_time_us(usage.model_samples) + extra_operations * Flash_read_time_us(0);
            break;
        case ReadSource::Psram:
            time_us = Psram_read_time_us(usage.model_samples) + extra_operations * Psram_read_time_us(0);
            break;
        case ReadSource::Ram:
            time_us = Ram_read_time_us(usage.model_samples) + extra_operations * Ram_read_time_us(0);
            break;
    }
    return true;
}

void PlayersManager::Enable_read_diagnostics(bool enabled)
{
    read_diagnostics_enabled = enabled;
    AudioPlayer::Enable_read_diagnostics(enabled);
    if (enabled)
    {
        ARM_DEMCR |= ARM_DEMCR_TRCENA;
        ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
        read_budget_diagnostics = {}; // A new observation window must not inherit an earlier allocation.
        read_diagnostics_last = {};
        read_diagnostics_peak = {};
        read_diagnostics_restart = {};
        read_diagnostics_gap = {};
    }
}

void PlayersManager::Collect_read_diagnostics(void)
{
    if (!read_diagnostics_enabled)
    {
        return;
    }
    read_diagnostics_last.cycle = audio_update_cycle;
    read_diagnostics_last.budget = read_budget_diagnostics.valid && read_budget_diagnostics.cycle == audio_update_cycle ? read_budget_diagnostics : ReadBudgetDiagnostics{}; // Never attach the latest budget to a different historical block.
    ++read_diagnostics_last.blocks;
    read_diagnostics_last.total_estimated_us = 0.0f;
    read_diagnostics_last.total_harvest_us = 0.0f;
    read_diagnostics_last.uncovered_operations = 0;
    read_diagnostics_last.restarted_players = 0;
    const float cycles_per_us = static_cast<float>(F_CPU_ACTUAL) / 1000000.0f;
    for (uint8_t player = 0; player < PLAYERS; ++player)
    {
        const PlayerReadDiagnostics &usage = Player_ptr[player].Get_read_diagnostics();
        read_diagnostics_last.players[player] = usage.cycle == audio_update_cycle ? usage : PlayerReadDiagnostics{}; // Never mix observations from different blocks.
        const PlayerReadDiagnostics &current = read_diagnostics_last.players[player];
        if ((current.flags & PlayerReadDiagnostics::RestartExecuted) != 0)
        {
            ++read_diagnostics_last.restarted_players;
        }
        float estimated = 0.0f;
        for (uint8_t source = 0; source < 3; ++source)
        {
            float source_us;
            if (!Get_read_usage_time_us(static_cast<ReadSource>(source), current.sources[source], source_us))
            {
                source_us = INFINITY;
            }
            estimated += source_us;
            read_diagnostics_last.uncovered_operations += current.sources[source].uncovered_operations;
        }
        read_diagnostics_last.estimated_us[player] = estimated;
        read_diagnostics_last.total_estimated_us += estimated;
        read_diagnostics_last.total_harvest_us += static_cast<float>(current.harvest_cycles) / cycles_per_us;
    }
    if (read_diagnostics_peak.blocks == 0 || read_diagnostics_last.total_estimated_us > read_diagnostics_peak.total_estimated_us)
    {
        read_diagnostics_peak = read_diagnostics_last;
    }
    if (read_diagnostics_last.restarted_players != 0 && (read_diagnostics_restart.blocks == 0 || read_diagnostics_last.total_estimated_us > read_diagnostics_restart.total_estimated_us))
    {
        read_diagnostics_restart = read_diagnostics_last;
    }
    const float gap_us = read_diagnostics_last.total_harvest_us - read_diagnostics_last.total_estimated_us;
    const float previous_gap_us = read_diagnostics_gap.total_harvest_us - read_diagnostics_gap.total_estimated_us;
    if (read_diagnostics_last.uncovered_operations == 0 && gap_us > 0.0f && (read_diagnostics_gap.blocks == 0 || gap_us > previous_gap_us))
    {
        read_diagnostics_gap = read_diagnostics_last;
    }
}

bool PlayersManager::Copy_read_diagnostics(ReadDiagnosticsSnapshot &last, ReadDiagnosticsSnapshot &peak, ReadDiagnosticsSnapshot &restart, ReadDiagnosticsSnapshot &gap) const
{
    last = read_diagnostics_last;
    peak = read_diagnostics_peak;
    restart = read_diagnostics_restart;
    gap = read_diagnostics_gap;
    return read_diagnostics_enabled;
}

void PlayersManager::Set_ADSR_ptr(AudioADSR *ptr)
{
    ADSR = ptr;
}

AudioTables::Pointers PlayersManager::Get_playback_tables(uint8_t instrument_id)
{
    if (instrument_id >= INSTRUMENTS || Audio_tables_ptr == nullptr || !AudioTables::Needs_tables(Preset[instrument_id]))
    {
        return {};
    }
    return Audio_tables_ptr->Get_active_pointers(instrument_id, Preset[instrument_id]);
}

void PlayersManager::MX_multicast_change_routing(int instrument_id)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if ((Player_ptr[player].Read_instrument() == instrument_id) && Player_ptr[player].isPlaying())

        {
            // configure Router_L, Router_R input/output routing_table
            if (Delay_values.instrument_route[instrument_id])
            {
                Router_L_ptr->routing_table[player] = 0;
                Router_R_ptr->routing_table[player] = 0;
            }
            else
            {
                Router_L_ptr->routing_table[player] = 1;
                Router_R_ptr->routing_table[player] = 1;
            }

            Router_L_ptr->routing_MX[player] = MX_routing_source[instrument_id];
            Router_R_ptr->routing_MX[player] = MX_routing_source[instrument_id];
        }
    }
}

int PlayersManager::Count_sample_voices(void) const
{
    int count = 0;
    for (int player = 0; player < PLAYERS; ++player)
    {
        count += Player_ptr[player].Uses_sample_voice();
    }
    return count;
}

int PlayersManager::Select_player_for_note(uint8_t instrument_id, uint8_t note_number, int track)
{
    // Se e' una nota appartenente ad un track: track >= -1
    // Altrimenti: track = NO_TRACK

    int8_t id_player = -1;
    bool finished = false;
    const bool needs_sample = !Preset[instrument_id].use_Wavetable && Preset[instrument_id].file < FIRST_LIVE_SAMPLING_FILE;
    const int voice_limit = PLAYERS;

    // Caso NoteOn da tastiera reale (track == NO_TRACK) if a Player is_playing with same patch_id, instrument_id and note_number, and track, this Player must be taken
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if ((Player_ptr[player].Assigned_patch() == Patch_id) && (Player_ptr[player].Assigned_note() == note_number) && (Player_ptr[player].Assigned_instrument() == instrument_id) && Player_ptr[player].isPlaying() && Player_ptr[player].Assigned_track() == track) // isPlaying() significa !idle
        {
            id_player = player;
            finished = true;
            // PRINT("Play the same note - ", "Stop the same Player:", id_player);
            break;
        }
    }

    if (!finished && needs_sample) // File voices can use all physical players regardless of their current storage.
    {
        Update_players_stistics();

        if (Count_sample_voices() < voice_limit) // a Player can be used
        {
            // 1) if there is a Player !isPlaying, it can be used
            id_player = Find_free_player();
            finished = id_player >= 0;

            if (!finished)
            {
                // 2) if there is a Player playing an Instrument of a different Patch, this Player can be used
                id_player = Find_other_patch_player(false, false);
                finished = id_player >= 0;
            }

            if (!finished)
            {
                // 3) look for a Player !power_on and playing of the SAME INSTRUMENT: choose the OLDEST
                id_player = Smartfind_oldest_player(instrument_id, false, true); // SMARTFIND_oldest_player(uint8_t instrument_id, bool power_on, bool playing)
                if (id_player >= 0)
                {
                    finished = true;
                    // PRINT("Play from Flash - case 2", "Found available Player:", id_player);
                }
            }

            if (!finished)
            {
                // 4) look for a Player !power_on of ANY INSTRUMENT NOT protected: choose the OLDEST
                id_player = Simplefind_oldest_player(false); // SIMPLEFIND_oldest_player_power_on(bool power_on)
                if (id_player >= 0)
                {
                    finished = true;
                    // PRINT("Play from Flash - case 3", "Found available Player:", id_player);
                }
            }
            if (!finished)
            {
                // 5) look for a Player power_on and playing of the SAME INSTRUMENT: choose the OLDEST
                id_player = Smartfind_oldest_player(instrument_id, true, true); // SMARTFIND_oldest_player(uint8_t instrument_id, bool power_on, bool playing)
                if (id_player >= 0)
                {
                    finished = true;
                    // PRINT("Play from Flash - case 4", "Found available Player:", id_player);
                }
            }
            if (!finished && Preset[instrument_id].precedence)
            {
                // 6) look for a Player power_on of ANY INSTRUMENT NOT protected: choose the OLDEST
                id_player = Simplefind_oldest_player(true); // SIMPLEFIND_oldest_player(bool power_on)
                if (id_player >= 0)
                {
                    finished = true;
                    // PRINT("Play from Flash - case 5", "Found available Player:", id_player);
                }
            }
        }

        // Only an existing Flash/cache slot can be reused when all physical players are file voices.
        else
        {
            // 5A) there is a Player flash_mode from a different Patch and playing
            id_player = Find_other_patch_player(true, false);
            finished = id_player >= 0;

            if (!finished)
            {
                // 6) there is a Player playing the SAME INSTRUMENT: choose the OLDEST
                id_player = Smartfind_oldest_player(instrument_id, false, true); // SMARTFIND_oldest_player(uint8_t instrument_id, bool power_on, bool playing)
                if (id_player >= 0)
                {
                    finished = true;
                    // PRINT("Play from Flash - case 7", "Found available Player:", id_player);
                }
            }

            if (!finished)
            {
                // 7)  there is a Player fading (state = 2) ANY INSTRUMENT NOT protected flash_mode: choose the OLDEST
                id_player = Simplefind_oldest_sample_player(false, true);
                if (id_player >= 0)
                {
                    finished = true;
                    // PRINT("Play from Flash - case 9", "Found available Player:", id_player);
                }
            }

            if (!finished)
            {
                // 8)  there is a Player playing (state = 1) ANY INSTRUMENT NOT protected flash_mode: choose the OLDEST
                id_player = Simplefind_oldest_sample_player(true, true);
                if (id_player >= 0)
                {
                    finished = true;
                    // PRINT("Play from Flash - case 10", "Found available Player:", id_player);
                }
            }
        }
    }

    if (!finished && !needs_sample) // AudioTables wavetables and Live Sampler keep the independent memory allocation path.
    {
        // 1) look for a Player NOT used
        Update_players_stistics();

        if (players_playing < PLAYERS) // there is at least ONE player that can be taken
        {
            id_player = Find_free_player();
            finished = id_player >= 0;
        }

        else // all Player are playing
        {
            // 2) look for a Player playing an Instrument of a different Patch: this Player can be taken
            id_player = Find_other_patch_player(false, true);
            finished = id_player >= 0;

            if (!finished)
            {
                // 3) look for a Player !power_on (ADSR "Release" phase, or idle) and playing of the SAME INSTRUMENT: choose the OLDEST
                id_player = Smartfind_oldest_player(instrument_id, false, true); // SMARTFIND_oldest_player(uint8_t instrument_id, bool power_on, bool playing)
                if (id_player >= 0)
                {
                    finished = true;
                    // PRINT("Play from RAM - case 3", "Found available Player:", id_player);
                }
            }

            if (!finished)
            {
                // 4) look for a Player !power_on of ANY INSTRUMENT NOT protected: choose the OLDEST
                id_player = Simplefind_oldest_player(false); // SIMPLEFIND_oldest_player(bool power_on)
                if (id_player >= 0)
                {
                    finished = true;
                    // PRINT("Play from RAM - case 4", "Found available Player:", id_player);
                }
            }

            if (!finished)
            {
                // 5) look for a player power_on and playing of the SAME INSTRUMENT: choose the OLDEST
                id_player = Smartfind_oldest_player(instrument_id, true, true); // SMARTFIND_oldest_player(uint8_t instrument_id, bool power_on, bool playing)
                if (id_player >= 0)
                {
                    finished = true;
                    // PRINT("Play from RAM - case 5", "Found available Player:", id_player);
                }
            }
        }

        if (!finished && Preset[instrument_id].precedence)
        {
            // 6) look for a Player power_on of ANY INSTRUMENT NOT protected: choose the OLDEST
            id_player = Simplefind_oldest_player(true); // SIMPLEFIND_oldest_player(bool power_on)
            if (id_player >= 0)
            {
                finished = true;
                // PRINT("Play from RAM - case 6", "Found available Player:", id_player);
            }
        }
    }

    // A same-note candidate may be a wavetable while all file slots are reserved.
    if (finished && needs_sample && !Player_ptr[id_player].Uses_sample_voice())
    {
        Update_players_stistics();
        if (Count_sample_voices() >= voice_limit)
        {
            id_player = Simplefind_oldest_sample_player(false, true);
            if (id_player < 0)
            {
                id_player = Simplefind_oldest_sample_player(true, true);
            }
            finished = id_player >= 0;
        }
    }
    return finished ? id_player : -1;
}

PlayerReadBudget::Plan PlayersManager::New_read_plan(uint8_t instrument, float note_pitch) const
{
    const auto &preset = Preset[instrument]; // Prepared source selected for the incoming note.
    PlayerReadBudget::Plan plan; // Match AudioPlayer's pending geometry without modifying a voice.
    plan.live = preset.file >= FIRST_LIVE_SAMPLING_FILE;
    const bool wavetable = preset.use_Wavetable && !plan.live; // Live always uses its circular PSRAM buffer.
    plan.source = wavetable ? ReadSource::Ram : (plan.live || preset.source.storage == Psram ? ReadSource::Psram : ReadSource::Flash);
    plan.packets = !wavetable && !plan.live && preset.source.storage == Flash && preset.file >= FIRST_RECORDING_FILE;
    plan.loop = preset.mode >= LOOP_FWD;
    plan.pingpong = preset.mode == LOOP_FWD_REV || preset.mode == LOOP_REV_FWD;
    plan.span = plan.live ? (plan.loop ? LS_XY_delta + 1 : LS_buffer_dim) : preset.B - preset.A + 1;
    plan.crossfade = plan.pingpong ? 0 : preset.Noclick;
    if (plan.live && preset.mode == LOOP_FWD)
    {
        plan.crossfade = PlayerReadBudget::Live_noclick_samples;
    }
    const float ceiling = Playback_pitch_limit(wavetable, preset.source.storage == Psram, plan.live); // Apply the final source ceiling, including modulation.
    plan.pitch = constrain(note_pitch * preset.pitch * PlayerReadBudget::Maximum_modulation, MIN_PITCH, ceiling);
    return plan;
}

float PlayersManager::Reserved_read_us(void) const
{
    float total = 0.0f; // Includes releasing voices until they actually stop reading.
    for (int player = 0; player < PLAYERS; ++player) // At most sixteen inexpensive source forecasts.
    {
        total += Player_ptr[player].Reserved_read_us();
    }
    return total;
}

bool PlayersManager::Admit_read_budget(int target, uint8_t instrument, float incoming_us)
{
    if (!std::isfinite(incoming_us) || incoming_us > PlayerReadBudget::Limit_us)
    {
        ++budget_rejected_notes;
        return false;
    }
    bool victims[PLAYERS] = {}; // Plan first: failed admission must not stop any existing note.
    float costs[PLAYERS] = {}; // Snapshot each reservation once during selection.
    float total = incoming_us; // Replacement and outgoing voice can run in separate blocks.
    for (int player = 0; player < PLAYERS; ++player) // Preserve outgoing cost until explicitly retired.
    {
        costs[player] = Player_ptr[player].Reserved_read_us();
        total += player == target ? fmaxf(costs[player], incoming_us) - incoming_us : costs[player];
    }
    if (total > PlayerReadBudget::Limit_us && costs[target] > incoming_us)
    {
        victims[target] = true;
        total = incoming_us;
        for (int player = 0; player < PLAYERS; ++player) // Recompute instead of subtracting infinity.
        {
            if (player != target)
            {
                total += costs[player];
            }
        }
    }
    while (total > PlayerReadBudget::Limit_us)
    {
        int best = -1; // Next eligible victim, ordered like the existing voice allocator.
        int best_rank = 99; // Other patch, released same, released other, held same, held other.
        for (int player = 0; player < PLAYERS; ++player) // Never replace another event already booked in this batch.
        {
            auto &voice = Player_ptr[player]; // Current voice metadata and protection.
            if (player == target || victims[player] || Player_booked[player] || voice.Has_pending_note() || !voice.isPlaying())
            {
                continue;
            }
            const bool same = voice.Read_instrument() == instrument; // Same-instrument reuse retains the established protection exception.
            const bool other_patch = voice.Assigned_patch() != Patch_id; // Outgoing patch voices are reclaimed first.
            if (!other_patch && !same && (voice.Read_precedence() || (voice.isPoweredOn() && !Preset[instrument].precedence)))
            {
                continue;
            }
            const int rank = other_patch ? 0 : (!voice.isPoweredOn() ? (same ? 1 : 2) : (same ? 3 : 4)); // Match release and instrument priorities before age.
            if (best < 0 || rank < best_rank || (rank == best_rank && voice.Read_time_stamp() < Player_ptr[best].Read_time_stamp()))
            {
                best = player;
                best_rank = rank;
            }
        }
        if (best < 0)
        {
            ++budget_rejected_notes;
            return false;
        }
        victims[best] = true;
        total = victims[target] ? incoming_us : fmaxf(costs[target], incoming_us);
        for (int player = 0; player < PLAYERS; ++player) // Exclude all planned victims, including invalid source forecasts.
        {
            if (player != target && !victims[player])
            {
                total += costs[player];
            }
        }
    }
    for (int player = 0; player < PLAYERS; ++player) // Commit only after the complete candidate fits.
    {
        if (victims[player])
        {
            Player_ptr[player].Retire_for_read_budget();
            ++budget_retired_players;
        }
    }
    return true;
}

void PlayersManager::Prepare_read_budget(void)
{
    // Main-loop edits and cache promotion can change the forecast without a NoteOn.
    float total = Reserved_read_us(); // Fresh full-block cost, including pending edits and replacements.
    while (total > PlayerReadBudget::Limit_us)
    {
        int best = -1; // Release tails first, then unprotected held voices, protected voices only as a last resort.
        int best_rank = 99; // Protection dominates age during forced revalidation.
        for (int player = 0; player < PLAYERS; ++player) // Bounded scan; at most PLAYERS retirements.
        {
            auto &voice = Player_ptr[player]; // No mutation until the next victim is selected.
            if (!voice.isPlaying())
            {
                continue;
            }
            const int rank = (voice.Read_precedence() ? 2 : 0) + (voice.isPoweredOn() ? 1 : 0); // Retain protected notes where possible.
            if (best < 0 || rank < best_rank || (rank == best_rank && voice.Read_time_stamp() < Player_ptr[best].Read_time_stamp()))
            {
                best = player;
                best_rank = rank;
            }
        }
        if (best < 0)
        {
            break;
        }
        if (Player_ptr[best].Read_precedence())
        {
            ++budget_forced_protected;
        }
        Player_ptr[best].Retire_for_read_budget();
        ++budget_retired_players;
        total = Reserved_read_us();
    }
    budget_reserved_us = total;
    const bool previous_batch = midi_batch_active; // Include elapsed preparation time when scheduling from the audio clock.
    midi_batch_active = true;
    Calculate_and_set_mix_samples();
    midi_batch_active = previous_batch;
    if (read_diagnostics_enabled)
    {
        read_budget_diagnostics.pre_players_us = static_cast<float>(audio_update_time_micros); // Include the scheduler itself in the time spent before Player rendering.
    }
}

void PlayersManager::Play_note(uint8_t instrument_id, uint8_t note_number, float velocity_float, int track) // after receiving a NoteOn command
{
    if (instrument_id >= INSTRUMENTS || !Preset[instrument_id].active)
    {
        return;
    }
    const AudioTables::Pointers table_pointers = Get_playback_tables(instrument_id);
    // Reject an unavailable sound before allocating a voice or changing its envelope.
    if (AudioTables::Needs_tables(Preset[instrument_id]) && table_pointers.bank_mask == 0)
    {
        return;
    }

    const int id_player = Select_player_for_note(instrument_id, note_number, track);
    if (id_player < 0)
    {
        return; // All eligible voices are occupied or reserved; never overwrite a different pending note.
    }

    const float incoming_pitch = pitch_from_note[note_number + 60 - Patch[Patch_id].Instrument[instrument_id].root_key]; // Same transposition used by Get_ready_to_play.
    if (!Admit_read_budget(id_player, instrument_id, PlayerReadBudget::Estimate(New_read_plan(instrument_id, incoming_pitch), AUDIO_BLOCK_SAMPLES)))
    {
        return;
    }
    Player_booked[id_player] = true;

    if (Player_ptr[id_player].State() > 0 && !restart_Player[id_player]) // sta inviando sample, cioÃ¨ !idle
    {
        restart_Player[id_player] = true;
        ++players_to_restart;
    }

    Player_ptr[id_player].Write_precedence(Preset[instrument_id].precedence);
    Player_ptr[id_player].Write_midi_channel(Preset[instrument_id].midi_channel);
    Player_ptr[id_player].Set_modulation(modulation_value[Preset[instrument_id].midi_channel]);
    Player_ptr[id_player].Set_source(Preset[instrument_id].source);

    ADSR[id_player].Setup(Preset[instrument_id].attack, Preset[instrument_id].decay, Preset[instrument_id].sustain, Preset[instrument_id].release, Preset[instrument_id].attack_type);

    /*
    Serial.print("id_player: ");
    Serial.print(id_player);
    Serial.print(" ");
    Serial.print(Preset[instrument_id].attack);
    Serial.print(" ");
    Serial.print( Preset[instrument_id].decay);
    Serial.print(" ");
    Serial.print(Preset[instrument_id].sustain);
    Serial.print(" ");
    Serial.print(Preset[instrument_id].release);
    Serial.print(" ");
    Serial.println(Preset[instrument_id].attack_type);
    */

    Player_ptr[id_player].Write_loop_track(track);
    if (track >= 0)
    {
        Player_ptr[id_player].Set_volume(LOOP_volume[track] * Preset[instrument_id].volume);
    }
    else
    {
        Player_ptr[id_player].Set_volume(Preset[instrument_id].volume);
    }
    Player_ptr[id_player].Set_pan(Preset[instrument_id].pan);
    Player_ptr[id_player].Set_pitch(Preset[instrument_id].pitch);

    Player_ptr[id_player].Main_settings(Preset[instrument_id].mode, Preset[instrument_id].A, Preset[instrument_id].B, Preset[instrument_id].Noclick, Preset[instrument_id].use_Wavetable, table_pointers.noclick, table_pointers.wavetable, table_pointers.bank_mask);

    if (!Preset[instrument_id].lock)
    {
        Player_ptr[id_player].Set_effects(resolution_value[resolution], downsampling);
        Player_ptr[id_player].Set_pitch_bend(pitch_bend_value[Preset[instrument_id].midi_channel]);
    }
    else
    {
        Player_ptr[id_player].Set_effects(16.0, 0);
        Player_ptr[id_player].Set_pitch_bend(1.0);
    }

    // VCF and its LFO_0
    if (Preset[instrument_id].Filter.use == 1)
    {
        if (Preset[instrument_id].Filter.modulation == 4) // LFO wave "sine" modulates VCF and MIDI After touch modulates index
        {
            Player_ptr[id_player].Connect_VCF(true, Preset[instrument_id].Filter.type, Preset[instrument_id].Filter.pivot, Preset[instrument_id].Filter.resonance, true);                                                                                                              // void Connect_VCF(bool use, int type, float pivot, float resonance, bool modulated)
            Player_ptr[id_player].Connect_LFO_TO_VCF(Preset[instrument_id].Filter.modulation, Preset[instrument_id].Filter.index * after_touch_channel_value[Preset[instrument_id].midi_channel], Preset[instrument_id].Filter.periodic, Preset[instrument_id].Filter.frequency_time); // Connect_LFO_TO_VCF(uint8_t modulation, float index, uint8_t periodic, float frequency_time)
        }
        else if (Preset[instrument_id].Filter.modulation > 0) // LFO modulates VCF
        {
            Player_ptr[id_player].Connect_VCF(true, Preset[instrument_id].Filter.type, Preset[instrument_id].Filter.pivot, Preset[instrument_id].Filter.resonance, true);                                              // void Connect_VCF(bool use, int type, float pivot, float resonance, bool modulated)
            Player_ptr[id_player].Connect_LFO_TO_VCF(Preset[instrument_id].Filter.modulation, Preset[instrument_id].Filter.index, Preset[instrument_id].Filter.periodic, Preset[instrument_id].Filter.frequency_time); // Connect_LFO_TO_VCF(uint8_t modulation, float index, uint8_t periodic, float frequency_time)
        }
        else // VCF is not modulated
        {
            Player_ptr[id_player].Connect_VCF(true, Preset[instrument_id].Filter.type, Preset[instrument_id].Filter.pivot, Preset[instrument_id].Filter.resonance, false); // void Connect_VCF(bool use, int type, float pivot, float resonance, bool modulated)
        }
    }
    else
    {
        Player_ptr[id_player].Connect_VCF(false, 0, 20000, 1, false);
    }

    /*
    PLAY note; il Player setta:
    power_on = true
    idle = false
    (se interrogato Player[player].is_playing == true);
    */
    Player_ptr[id_player].Get_ready_to_play(pitch_from_note[note_number + 60 - Patch[Patch_id].Instrument[instrument_id].root_key], velocity_float, Patch_id, instrument_id, Preset[instrument_id].sound_id, note_number);

    // Gestione del Delay
    // configura su Router_L e Router_R input/output le routing_table
    if (Delay_values.instrument_route[instrument_id])
    {
        Router_L_ptr->routing_table[id_player] = 0;
        Router_R_ptr->routing_table[id_player] = 0;
    }
    else
    {
        Router_L_ptr->routing_table[id_player] = 1;
        Router_R_ptr->routing_table[id_player] = 1;
    }

    Router_L_ptr->routing_MX[id_player] = MX_routing_source[instrument_id];
    Router_R_ptr->routing_MX[id_player] = MX_routing_source[instrument_id];

    // display Players activity
    if (false)
    {
        for (auto player = 0; player < PLAYERS; ++player)
        {
            if (Player_ptr[player].isPoweredOn())
            {
                Serial.print(Player_ptr[player].Read_note());
                Serial.print("/");
                Serial.print(Player_ptr[player].Read_instrument());
            }
            else
            {
                Serial.print("NN");
            }
            Serial.print(" ");
        }
        Serial.println();
    }
}

void PlayersManager::Release_Player_noteOff(uint8_t player, int track) // after receiving a NoteOff command
{
    Player_ptr[player].Release_note();
    Player_ptr[player].Write_time_stamp(millis());

    // display Players activity
    if (false)
    {
        for (auto player = 0; player < PLAYERS; ++player)
        {
            if (Player_ptr[player].isPoweredOn())
            {
                Serial.print(Player_ptr[player].Read_note());
                Serial.print("/");
                Serial.print(Player_ptr[player].Read_instrument());
            }
            else
                Serial.print("NN");
            Serial.print(" ");
        }
        Serial.println();
    }
}

void PlayersManager::Begin_midi_batch(void)
{
    midi_batch_active = true;
    Reset_booked_and_restart_player();
    Reset_players_to_restart();
    for (int player = 0; player < PLAYERS; ++player)
    {
        Player_booked[player] = Player_ptr[player].Has_pending_note();
    }
}

void PlayersManager::End_midi_batch(void)
{
    // Rebuild from actual pending starts, including requests retained after audio allocation failure.
    Reset_players_to_restart();
    for (int player = 0; player < PLAYERS; ++player)
    {
        restart_Player[player] = Player_ptr[player].Needs_restart_mix();
        if (restart_Player[player])
        {
            ++players_to_restart;
        }
    }
    // The audio clock schedules all restarts and edits together after the complete control batch.
    midi_batch_active = false;
}

void PlayersManager::Set_modulation(uint8_t midi_channel, uint8_t value)
{
    if (midi_channel >= 16)
    {
        return;
    }
    modulation_value[midi_channel] = value;
    for (int player = 0; player < PLAYERS; ++player)
    {
        if (Player_ptr[player].Read_midi_channel() == midi_channel)
        {
            Player_ptr[player].Set_modulation(value);
        }
    }
}

void PlayersManager::Reset_booked_and_restart_player(void)
{
    // Player_booked serve ad evitare che gli Instrument che sono attivati da NoteOn non competano sullo stesso Player
    // players_to_restart, se risultera' >0, richiede il calcolo dei vari valori di samples (mix_samples_for_Player[player])
    // che ciascun Player da riavviare (restart_Player[player] == true) dovra' utilizzare
    for (auto player = 0; player < PLAYERS; ++player)
    {
        Player_booked[player] = false;
        restart_Player[player] = false;
    }
}

bool PlayersManager::Get_restart_player(int player)
{
    return restart_Player[player];
}

void PlayersManager::Cancel_restart_player(int player)
{
    restart_Player[player] = false;
}

bool PlayersManager::Can_select_player(int player, const Player_filter &filter) const
{
    if (Player_booked[player] || Player_ptr[player].Has_pending_note())
    {
        return false; // Only the explicit same-note retrigger path may replace a pending request.
    }
    if (filter.instrument >= 0 && Player_ptr[player].Read_instrument() != filter.instrument)
    {
        return false;
    }
    if (filter.powered >= 0 && Player_ptr[player].isPoweredOn() != static_cast<bool>(filter.powered))
    {
        return false;
    }
    if (filter.playing >= 0 && Player_ptr[player].isPlaying() != static_cast<bool>(filter.playing))
    {
        return false;
    }
    if (filter.unprotected && Player_ptr[player].Read_precedence())
    {
        return false;
    }
    if (filter.sample_only && !Player_ptr[player].Uses_sample_voice())
    {
        return false;
    }
    if (filter.other_patch && Player_ptr[player].Read_patch_wait() == Patch_id)
    {
        return false;
    }
    return true;
}

int PlayersManager::Find_player(const Player_filter &filter, Selection_order order)
{
    int result = -1;
    unsigned long oldest = 0;
    for (int player = 0; player < PLAYERS; ++player)
    {
        if (!Can_select_player(player, filter))
        {
            continue;
        }
        if (order == Selection_order::First)
        {
            return player;
        }
        const unsigned long timestamp = Player_ptr[player].Read_time_stamp();
        if (order == Selection_order::Last || result < 0 || timestamp < oldest)
        {
            result = player;
            oldest = timestamp; // Strict comparison preserves the lowest index when timestamps tie.
        }
    }
    return result;
}

int PlayersManager::Find_free_player(void)
{
    Player_filter filter;
    filter.playing = 0;
    return Find_player(filter, Selection_order::First);
}

int PlayersManager::Find_other_patch_player(bool sample_only, bool prefer_last)
{
    Player_filter filter;
    filter.other_patch = true;
    filter.sample_only = sample_only;
    filter.playing = sample_only ? 1 : -1;
    return Find_player(filter, prefer_last ? Selection_order::Last : Selection_order::First);
}

int PlayersManager::Smartfind_oldest_player(uint8_t instrument_id, bool power_on, bool playing)
{
    Player_filter filter;
    filter.instrument = instrument_id;
    filter.powered = power_on;
    filter.playing = playing;
    return Find_player(filter, Selection_order::Oldest);
}

int PlayersManager::Simplefind_oldest_player(bool power_on) // "precedence" instruments are EXCLUDED
{
    Player_filter filter;
    filter.powered = power_on;
    filter.unprotected = true;
    return Find_player(filter, Selection_order::Oldest);
}

int PlayersManager::Simplefind_oldest_sample_player(bool power_on, bool playing) // "precedence" instruments are EXCLUDED
{
    Player_filter filter;
    filter.powered = power_on;
    filter.playing = playing;
    filter.unprotected = true;
    filter.sample_only = true;
    return Find_player(filter, Selection_order::Oldest);
}

void PlayersManager::Change_from_key(int patch_id, int instrument_id, int from_key_new)
{
    if (from_key_new > Patch[patch_id].Instrument[instrument_id].from_note)
    {
        for (auto player = 0; player < PLAYERS; ++player)
        {
            if (Player_ptr[player].Read_note() < from_key_new && Player_ptr[player].isPoweredOn() && Player_ptr[player].Read_instrument() == instrument_id)
            {
                Release_player(player);
            }
        }
    }
    Patch[patch_id].Instrument[instrument_id].from_note = from_key_new;
    Update_map_Instrument_for_notes(Patch[patch_id].Instrument[instrument_id].from_note, Patch[patch_id].Instrument[instrument_id].to_note, instrument_id);
}

void PlayersManager::Change_to_key(int patch_id, int instrument_id, int to_key_new)
{
    if (to_key_new < Patch[patch_id].Instrument[instrument_id].to_note)
    {
        for (auto player = 0; player < PLAYERS; ++player)
        {
            if (Player_ptr[player].Read_note() > to_key_new && Player_ptr[player].isPoweredOn() && Player_ptr[player].Read_instrument() == instrument_id)
            {
                Release_player(player);
            }
        }
    }
    Patch[patch_id].Instrument[instrument_id].to_note = to_key_new;
    Update_map_Instrument_for_notes(Patch[patch_id].Instrument[instrument_id].from_note, Patch[patch_id].Instrument[instrument_id].to_note, instrument_id);
}

void PlayersManager::Multicast_change_players_notes(int patch_id, int instrument_id) // usato quando si cambia la root_key
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if (Player_ptr[player].isPoweredOn() && Player_ptr[player].Read_instrument() == instrument_id)
        {
            Player_ptr[player].Set_note(pitch_from_note[Player_ptr[player].Read_note() + 60 - Patch[patch_id].Instrument[instrument_id].root_key]);
        }
    }
}

void PlayersManager::Multicast_release_players(int sound_id)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if (Player_ptr[player].Read_sound_id() == sound_id)
        {
            Release_player(player);
        }
    }
}

void PlayersManager::Broadcast_volume(void)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if (Player_ptr[player].isPlaying())
        {
            Player_ptr[player].Update_volume(Preset[Player_ptr[player].Read_instrument()].volume);
        }
    }
}

void PlayersManager::Multicast_volume_for_instrument_edit(int instrument_id)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if ((Player_ptr[player].Read_instrument() == instrument_id) && Player_ptr[player].isPlaying())
        {
            Player_ptr[player].Update_volume(Preset[instrument_id].volume);
        }
    }
}

void PlayersManager::Multicast_pitch_for_sound_edit(int instrument_id)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if ((Player_ptr[player].Read_instrument() == instrument_id) && Player_ptr[player].isPlaying())
        {
            Player_ptr[player].Set_pitch(Preset[instrument_id].pitch);
        }
    }
}

void PlayersManager::Multicast_pan(int instrument_id)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if ((Player_ptr[player].Read_instrument() == instrument_id) && Player_ptr[player].isPlaying())
        {
            Player_ptr[player].Update_pan(Preset[instrument_id].pan);
        }
    }
}

void PlayersManager::Multicast_effects(float resolution, uint8_t downsampling)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if (Player_ptr[player].isPlaying() && !Preset[Player_ptr[player].Read_instrument()].lock)
        {
            Player_ptr[player].Set_effects(resolution, downsampling);
        }
    }
}

void PlayersManager::Broadcast_reset_effect(float resolution, uint8_t downsampling, int effect)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if (effect == 0) // reset resolution
        {
            Player_ptr[player].Set_effects(16.0, downsampling);
        }

        else if (effect == 1) // reset downsampling
        {
            Player_ptr[player].Set_effects(resolution, 1);
        }

        else
        {
            return;
        }
    }
}

void PlayersManager::Update_all_Preset(int patch_id, float volume_patch)
{
    for (uint8_t instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
    {
        Preset[instrument_id] = {};
        if (Patch[patch_id].Instrument[instrument_id].used)
        {
            Update_Preset(patch_id, instrument_id, volume_patch);
        }
    }
    Cache_manager_ptr->Set_required_files(Preset);
    Refresh_cache_sources();
}

void PlayersManager::Update_all_Preset_volume(int patch_id, float volume_patch)
{
    for (auto instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
    {
        if (Patch[patch_id].Instrument[instrument_id].used)
        {
            Update_Preset_volume(patch_id, instrument_id, volume_patch);
        }
    }
}

Preset_struct PlayersManager::Build_Preset(int patch_id, int instrument_id, float volume_patch)
{
    Preset_struct result = {};
    result.active = true;
    const uint16_t sound_id = Get_sound_id(patch_id, instrument_id);

    result.volume = MX_mute[instrument_id] ? 0.0f : volume_patch * Volume_float[Sound[sound_id].gain];
    result.pan = Sound[sound_id].pan;
    result.sound_id = sound_id;
    result.file = Sound[sound_id].file;
    result.source = Cache_manager_ptr->Get_source(result.file);
    result.midi_channel = Get_midi_channel(patch_id, instrument_id);
    result.pitch = Calc_pitch(Sound[sound_id].pitch);
    result.mode = Sound[sound_id].mode;
    result.A = Sound[sound_id].A;
    result.B = Sound[sound_id].B;
    result.use_Wavetable = (result.B - result.A + 1) <= BLOCK_MIN;
    result.Noclick = Sound[sound_id].Noclick;
    result.attack_type = bitRead(Sound[sound_id].data, 0);
    result.attack = Calc_attack(Sound[sound_id].attack);
    result.decay = Calc_decay(Sound[sound_id].decay);
    result.sustain = Calc_sustain(Sound[sound_id].sustain);
    result.release = Calc_release(Sound[sound_id].release);
    result.precedence = Patch[patch_id].Instrument[instrument_id].precedence;
    result.lock = Patch[patch_id].Instrument[instrument_id].lock;

    result.Filter.use = Patch[patch_id].Instrument[instrument_id].Filter.use;
    result.Filter.type = Patch[patch_id].Instrument[instrument_id].Filter.type;

    const float value = Patch[patch_id].Instrument[instrument_id].Filter.pivot / 10.0f;
    result.Filter.pivot = 20.0f * pow(2.0f, value);
    result.Filter.resonance = (5.0f + Patch[patch_id].Instrument[instrument_id].Filter.resonance) / 5.0f;
    result.Filter.index = Patch[patch_id].Instrument[instrument_id].Filter.index / 20.0f;
    result.Filter.modulation = Patch[patch_id].Instrument[instrument_id].Filter.modulation;

    if (result.Filter.modulation == 3 || result.Filter.modulation == 4)
    {
        result.Filter.periodic = 1;
        result.Filter.frequency_time = Patch[patch_id].Instrument[instrument_id].Filter.frequency_time * Patch[patch_id].Instrument[instrument_id].Filter.frequency_time / 40.0f;
    }
    else
    {
        result.Filter.periodic = 0;
        result.Filter.frequency_time = Patch[patch_id].Instrument[instrument_id].Filter.frequency_time / 8.0f;
    }

    return result;
}

bool PlayersManager::Build_presets_snapshot(int patch_id, float volume_patch, Preset_struct (&presets)[INSTRUMENTS], uint16_t &tables_mask)
{
    tables_mask = 0;

    if (patch_id < 0 || patch_id > PATCHES_MAX)
    {
        return false;
    }

    for (uint8_t instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
    {
        presets[instrument_id] = {};

        if (!Patch[patch_id].Instrument[instrument_id].used)
        {
            continue;
        }

        presets[instrument_id] = Build_Preset(patch_id, instrument_id, volume_patch);

        // Live Sampler instruments do not require AudioTables storage.
        if (presets[instrument_id].file < FIRST_LIVE_SAMPLING_FILE)
        {
            tables_mask |= static_cast<uint16_t>(1u << instrument_id);
        }
    }

    return true;
}

bool PlayersManager::Activate_prepared_presets(const Preset_struct (&presets)[INSTRUMENTS])
{
    if (Audio_tables_ptr == nullptr)
    {
        return false;
    }

    if (!Audio_tables_ptr->Activate_prepared())
    {
        return false;
    }

    // Publish the matching presets before audio interrupts are restored.
    for (uint8_t instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
    {
        Preset[instrument_id] = presets[instrument_id];
    }
    Cache_manager_ptr->Set_required_files(Preset);
    Refresh_cache_sources();
    Refresh_audio_table_references();
    return true;
}

void PlayersManager::Update_Preset(int patch_id, int instrument_id, float volume_patch)
{
    Preset[instrument_id] = Build_Preset(patch_id, instrument_id, volume_patch);
}

void PlayersManager::Update_Preset_volume(int patch_id, int instrument_id, float volume_patch)
{
    if (MX_mute[instrument_id])
    {
        Preset[instrument_id].volume = 0.0;
    }
    else
        Preset[instrument_id].volume = volume_patch * Volume_float[Sound[Get_sound_id(patch_id, instrument_id)].gain];
}

void PlayersManager::Update_Preset_pan(int patch_id, int instrument_id)
{
    Preset[instrument_id].pan = Sound[Get_sound_id(patch_id, instrument_id)].pan;
}

void PlayersManager::Update_Preset_sound_id(int patch_id, int instrument_id)
{
    Preset[instrument_id].sound_id = Get_sound_id(patch_id, instrument_id);
}

void PlayersManager::Update_Preset_file(int patch_id, int instrument_id)
{
    Preset[instrument_id].file = Sound[Get_sound_id(patch_id, instrument_id)].file;
    Preset[instrument_id].source = Cache_manager_ptr->Get_source(Preset[instrument_id].file);
}

void PlayersManager::Update_Preset_midi_channel(int patch_id, int instrument_id)
{
    Preset[instrument_id].midi_channel = Get_midi_channel(patch_id, instrument_id);
}

void PlayersManager::Update_Preset_pitch(int patch_id, int instrument_id)
{
    Preset[instrument_id].pitch = Calc_pitch(Sound[Get_sound_id(patch_id, instrument_id)].pitch);
}

void PlayersManager::Update_Preset_mode(int patch_id, int instrument_id)
{
    Preset[instrument_id].mode = Sound[Get_sound_id(patch_id, instrument_id)].mode;
}

void PlayersManager::Update_Preset_A_B_Wavetable(int patch_id, int instrument_id)
{
    Preset[instrument_id].A = Sound[Get_sound_id(patch_id, instrument_id)].A;
    Preset[instrument_id].B = Sound[Get_sound_id(patch_id, instrument_id)].B;
    Preset[instrument_id].use_Wavetable = (Preset[instrument_id].B - Preset[instrument_id].A + 1) <= BLOCK_MIN;
}

void PlayersManager::Update_Preset_Noclick(int patch_id, int instrument_id)
{
    Preset[instrument_id].Noclick = Sound[Get_sound_id(patch_id, instrument_id)].Noclick;
}

void PlayersManager::Update_Preset_attack_type(int patch_id, int instrument_id)
{
    Preset[instrument_id].attack_type = bitRead(Sound[Get_sound_id(patch_id, instrument_id)].data, 0);
}

void PlayersManager::Update_Preset_attack(int patch_id, int instrument_id)
{
    Preset[instrument_id].attack = Calc_attack(Sound[Get_sound_id(patch_id, instrument_id)].attack);
}

void PlayersManager::Update_Preset_decay(int patch_id, int instrument_id)
{
    Preset[instrument_id].decay = Calc_decay(Sound[Get_sound_id(patch_id, instrument_id)].decay);
}

void PlayersManager::Update_Preset_sustain(int patch_id, int instrument_id)
{
    Preset[instrument_id].sustain = Calc_sustain(Sound[Get_sound_id(patch_id, instrument_id)].sustain);
}

void PlayersManager::Update_Preset_release(int patch_id, int instrument_id)
{
    Preset[instrument_id].release = Calc_release(Sound[Get_sound_id(patch_id, instrument_id)].release);
}

void PlayersManager::Update_Preset_precedence(int patch_id, int instrument_id)
{
    Preset[instrument_id].precedence = Patch[patch_id].Instrument[instrument_id].precedence;
}

void PlayersManager::Update_Preset_lock(int patch_id, int instrument_id)
{
    Preset[instrument_id].lock = Patch[patch_id].Instrument[instrument_id].lock;
}

void PlayersManager::Multicast_IF_update_filter_type(int instrument_id)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if ((Player_ptr[player].Read_instrument() == instrument_id) && Player_ptr[player].isPlaying())
        {
            if (Preset[instrument_id].Filter.use == 1)
            {
                if (Preset[instrument_id].Filter.modulation == 4) // LFO wave "sine" modulates VCF and MIDI After touch modulates index
                {
                    Player_ptr[player].Connect_VCF(true, Preset[instrument_id].Filter.type, Preset[instrument_id].Filter.pivot, Preset[instrument_id].Filter.resonance, true);                                                                                                              // void Connect_VCF(bool use, int type, float pivot, float resonance, bool modulated)
                    Player_ptr[player].Connect_LFO_TO_VCF(Preset[instrument_id].Filter.modulation, Preset[instrument_id].Filter.index * after_touch_channel_value[Preset[instrument_id].midi_channel], Preset[instrument_id].Filter.periodic, Preset[instrument_id].Filter.frequency_time); // Connect_LFO_TO_VCF(uint8_t modulation, float index, uint8_t periodic, float frequency_time)
                }
                else if (Preset[instrument_id].Filter.modulation > 0) // LFO modulates VCF
                {
                    Player_ptr[player].Connect_VCF(true, Preset[instrument_id].Filter.type, Preset[instrument_id].Filter.pivot, Preset[instrument_id].Filter.resonance, true);                                              // void Connect_VCF(bool use, int type, float pivot, float resonance, bool modulated)
                    Player_ptr[player].Connect_LFO_TO_VCF(Preset[instrument_id].Filter.modulation, Preset[instrument_id].Filter.index, Preset[instrument_id].Filter.periodic, Preset[instrument_id].Filter.frequency_time); // Connect_LFO_TO_VCF(uint8_t modulation, float index, uint8_t periodic, float frequency_time)
                }
                else // VCF is not modulated
                {
                    Player_ptr[player].Connect_VCF(true, Preset[instrument_id].Filter.type, Preset[instrument_id].Filter.pivot, Preset[instrument_id].Filter.resonance, false); // void Connect_VCF(bool use, int type, float pivot, float resonance, bool modulated)
                }
            }
            else
                Player_ptr[player].Connect_VCF(false, 0, 20000, 1, false);

            Player_ptr[player].Start_VCF();
        }
    }
}

void PlayersManager::Update_IF_resonance(int patch_id, int instrument_id)
{
    Update_Preset_IF_resonance(patch_id, instrument_id);
    if (Preset[instrument_id].Filter.use == 1)
    {
        Multicast_IF_resonance(instrument_id);
    }
}

void PlayersManager::Multicast_IF_pivot(int instrument_id)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if (((Player_ptr + player)->Read_instrument() == instrument_id) && (Player_ptr + player)->isPlaying())
        {
            (Player_ptr + player)->Update_VCF_pivot(Preset[instrument_id].Filter.pivot);
        }
    }
}

void PlayersManager::Multicast_IF_frequency_filter(int instrument_id)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if (((Player_ptr + player)->Read_instrument() == instrument_id) && (Player_ptr + player)->isPlaying())
        {
            if ((Preset[instrument_id].Filter.modulation != 0) && (Preset[instrument_id].Filter.modulation != 4)) // Connect_LFO_TO_VCF(uint8_t modulation, float index, uint8_t periodic, float frequency_time)
            {
                (Player_ptr + player)->Connect_LFO_TO_VCF(Preset[instrument_id].Filter.modulation, Preset[instrument_id].Filter.index, Preset[instrument_id].Filter.periodic, Preset[instrument_id].Filter.frequency_time);
            }

            else // Connect_LFO_TO_VCF(uint8_t modulation, float index, uint8_t periodic, float frequency_time)
            {
                (Player_ptr + player)->Connect_LFO_TO_VCF(Preset[instrument_id].Filter.modulation, Preset[instrument_id].Filter.index * after_touch_channel_value[Preset[instrument_id].midi_channel], Preset[instrument_id].Filter.periodic, Preset[instrument_id].Filter.frequency_time);
            }
        }
    }
}

void PlayersManager::Multicast_IF_resonance(int instrument_id)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if (((Player_ptr + player)->Read_instrument() == instrument_id) && (Player_ptr + player)->isPlaying())
        {
            (Player_ptr + player)->Update_VCF_resonance(Preset[instrument_id].Filter.resonance);
        }
    }
}

void PlayersManager::Update_Preset_IF(int patch_id, int instrument_id)
{
    Preset[instrument_id].Filter.use = Patch[patch_id].Instrument[instrument_id].Filter.use;
    Preset[instrument_id].Filter.type = Patch[patch_id].Instrument[instrument_id].Filter.type; // 0 -> 3

    float value = Patch[patch_id].Instrument[instrument_id].Filter.pivot / 10.0f;                                        // 0 --> 100  0 --> 10
    Preset[instrument_id].Filter.pivot = 20.0f * pow(2.0f, value);                                                       //  20 --> 20048
    Preset[instrument_id].Filter.resonance = (5.0f + Patch[patch_id].Instrument[instrument_id].Filter.resonance) / 5.0f; // 0 --> 40
    Preset[instrument_id].Filter.index = Patch[patch_id].Instrument[instrument_id].Filter.index / 20.0f;                 // 0 --> 20 : 0 --> 1.0
    Update_Preset_IF_modulation(patch_id, instrument_id);
}

void PlayersManager::Update_Preset_IF_resonance(int patch_id, int instrument_id)
{
    Preset[instrument_id].Filter.resonance = (5.0f + Patch[patch_id].Instrument[instrument_id].Filter.resonance) / 5.0f; // 0 --> 40
}

void PlayersManager::Update_Preset_IF_filter_type(int patch_id, int instrument_id)
{
    Preset[instrument_id].Filter.type = Patch[patch_id].Instrument[instrument_id].Filter.type; // 0 -> 3
}

void PlayersManager::Update_Preset_IF_modulation(int patch_id, int instrument_id)
{
    Preset[instrument_id].Filter.modulation = Patch[patch_id].Instrument[instrument_id].Filter.modulation; // 0 -> 4
    if (Preset[instrument_id].Filter.modulation == 3 || Preset[instrument_id].Filter.modulation == 4)
    {
        Preset[instrument_id].Filter.periodic = 1;
        Preset[instrument_id].Filter.frequency_time = Patch[patch_id].Instrument[instrument_id].Filter.frequency_time * Patch[patch_id].Instrument[instrument_id].Filter.frequency_time / 40.0f; // 0 --> 40
    }
    else
    {
        Preset[instrument_id].Filter.periodic = 0;
        Preset[instrument_id].Filter.frequency_time = Patch[patch_id].Instrument[instrument_id].Filter.frequency_time / 8.0f; // 0 --> 40
    }
}

void PlayersManager::Update_Preset_IF_index(int patch_id, int instrument_id)
{
    Update_Preset_IF(patch_id, instrument_id);
    if (Preset[instrument_id].Filter.use == 1)
    {
        if (Preset[instrument_id].Filter.modulation == 4)
        {
            Multicast_IF_index(instrument_id, Preset[instrument_id].Filter.index * after_touch_channel_value[Preset[instrument_id].midi_channel]);
        }
        else if (Preset[instrument_id].Filter.modulation != 0)
        {
            Multicast_IF_index(instrument_id, Preset[instrument_id].Filter.index);
        }
    }
}

void PlayersManager::Multicast_IF_index(int instrument_id, float value)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if ((Player_ptr[player].Assigned_instrument() == instrument_id) && Player_ptr[player].isPlaying())
        {
            Player_ptr[player].Update_LFO_index(value);
        }
    }
}

void PlayersManager::Multicast_main_settings_editing(int patch_id, int instrument_id)
{
    const AudioTables::Pointers table_pointers = Get_playback_tables(instrument_id); // Resolve the edited table before modifying any Player.
    if (AudioTables::Needs_tables(Preset[instrument_id]) && table_pointers.bank_mask == 0)
    {
        return;
    }
    for (int player = 0; player < PLAYERS; ++player) // Geometry is committed by AudioPlayer after the next budget pass.
    {
        Player_ptr[player].Apply_preset_edit(patch_id, instrument_id, Preset[instrument_id], table_pointers);
    }
}

bool PlayersManager::Get_use_Wavetable(int sound_id)
{
    return ((Sound[sound_id].B - Sound[sound_id].A + 1) <= BLOCK_MIN); // BLOCK_MIN is defined in Lilla_player.h
}

int8_t PlayersManager::Find_oldest_player(int instrument_id, bool power_on, bool playing)
{
    unsigned long time_min = 0;
    int8_t result = 0;
    for (auto player_ext = 0; player_ext < PLAYERS; ++player_ext)
    {
        if ((Player_ptr[player_ext].Read_instrument() == instrument_id) && (Player_ptr[player_ext].isPoweredOn() == power_on) && (Player_ptr[player_ext].isPlaying() == playing))
        {
            time_min = Player_ptr[player_ext].Read_time_stamp();
            result = player_ext;
            for (auto player = 0; player < PLAYERS; ++player) // look for the player playing for the longest time
            {
                if (((Player_ptr + player)->Read_instrument() == instrument_id) && ((Player_ptr + player)->isPoweredOn() == power_on) && ((Player_ptr + player)->isPlaying() == playing) && ((Player_ptr + player)->Read_time_stamp() < time_min))
                {
                    time_min = (Player_ptr + player)->Read_time_stamp();
                    result = player;
                }
            }
            return result;
        }
    }
    return -1;
}

void PlayersManager::Release_player(int player, int track) // after receiving a NoteOff command
{
    (Player_ptr + player)->Release_note();
    (Player_ptr + player)->Write_time_stamp(millis());
}

void PlayersManager::Release_player(int player) // after receiving a NoteOff command
{
    (Player_ptr + player)->Release_note();
    (Player_ptr + player)->Write_time_stamp(millis());
}

void PlayersManager::Release_all_players_for_instrument(int instrument_id) // after receiving a NoteOff command
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if ((Player_ptr + player)->isPoweredOn() && (Player_ptr + player)->Read_instrument() == instrument_id)
        {
            Release_player(player);
        }
    }
}

void PlayersManager::Release_all_players_for_instrument_solo(int instrument_id)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if ((Player_ptr + player)->isPlaying() && ((Player_ptr + player)->Read_instrument() != instrument_id))
        {

            if ((Player_ptr + player)->isPoweredOn())
            {
                (Player_ptr + player)->Fast_stop();
            }
        }
        // S_Map_one_Instrument_for_all_notes(instrument_id); // E' PALESEMENTE UN ERRORE!!!
    }
}

void PlayersManager::Release_all_players(void)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        Release_player(player);
    }
}

void PlayersManager::Release_softly_all_players(int patch_id) // Bound every outgoing voice and queued restart by QUICK_RELEASE_TIME.
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        Player_ptr[player].Release_patch(patch_id); // Include existing release tails and cancel pending notes belonging to the outgoing patch.
    }
}

void PlayersManager::Stop_all_players(void) // meglio Fast_stop...
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        (Player_ptr + player)->Fast_stop(); // Attiva il Release (ADSR) con tempo di caduta pari a 10 samples
        (Player_ptr + player)->Write_time_stamp(millis());
    }
}

uint16_t PlayersManager::Fast_stop_players_using_tables(uint8_t banks_mask)
{
    static_assert(PLAYERS <= 16);

    uint16_t stopped_players_mask = 0;

    for (uint8_t player_id = 0; player_id < PLAYERS; ++player_id)
    {
        if (Player_ptr[player_id].Fast_stop_using_tables(banks_mask))
        {
            Player_ptr[player_id].Write_time_stamp(millis());
            stopped_players_mask |= static_cast<uint16_t>(1u << player_id);
        }
    }

    return stopped_players_mask;
}

void PlayersManager::Release_all_players_loop(void)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if ((Player_ptr + player)->isPoweredOn() && ((Player_ptr + player)->Read_loop_track() >= 0))
        {
            Release_player(player);
        }
    }
}

void PlayersManager::Release_all_players_loop(int track)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if ((Player_ptr + player)->isPoweredOn() && ((Player_ptr + player)->Read_loop_track() == track))
        {
            Release_player(player);
        }
    }
}

float PlayersManager::Get_cross_mix_time(int player, int mix_samples)
{
    if (mix_samples <= 0)
    {
        return 0.0f;
    }
    return Player_ptr[player].Current_read_us(mix_samples) + 0.2f * mix_samples; // Source-aware outgoing harvest plus the existing mixing allowance.
}

int PlayersManager::Get_span_for_all_cross_mix(void)
{
    const float reserved = Reserved_read_us(); // Current/pending maxima already cover zero-mix sequential restarts.
    const float read_headroom = PlayerReadBudget::Limit_us - reserved; // Spare read allowance before the first transition.
    const float elapsed = midi_batch_active ? static_cast<float>(audio_update_time_micros) : 0.0f; // Sample once so diagnostics describe the exact deadline decision.
    const float deadline_headroom = midi_batch_active ? AUDIO_PLAYER_DEADLINE_US - elapsed - reserved - 700.0f : INFINITY; // Keep the existing processing allowance and main-loop behavior.
    const float available = fminf(read_headroom, deadline_headroom); // The tighter constraint determines the crossfade pool.
    if (read_diagnostics_enabled)
    {
        read_budget_diagnostics.reserved_us = reserved;
        read_budget_diagnostics.read_headroom_us = read_headroom;
        read_budget_diagnostics.deadline_headroom_us = deadline_headroom;
        read_budget_diagnostics.scheduler_elapsed_us = elapsed;
    }
    return available > 0.0f ? static_cast<int>(available) : 0;
}

void PlayersManager::Multicast_reset_pitch_bend_effects(int instrument_id)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if (Player_ptr[player].Read_instrument() == instrument_id)
        {
            Player_ptr[player].Set_pitch_bend(1.0);
            Player_ptr[player].Set_effects(16.0, 0);
        }
    }
}

void PlayersManager::Broadcast_pitch_bend(int midi_channel, float value)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if (Player_ptr[player].isPlaying() && !Preset[Player_ptr[player].Assigned_instrument()].lock && Preset[Player_ptr[player].Assigned_instrument()].midi_channel == midi_channel) // if(!bitRead(Patch[patch_id].Instrument[Player_ptr[player].instrument_id].info, 1) && (Get_midi_channel(patch_id, Player_ptr[player].instrument_id) == midi_channel))
        {
            Player_ptr[player].Set_pitch_bend(value);
        }
    }
}

void PlayersManager::Broadcast_restore_pitch_bend_and_effects(int midi_channel, float value)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if (!Preset[Player_ptr[player].Read_instrument()].lock && Preset[Player_ptr[player].Read_instrument()].midi_channel == midi_channel) // if(!bitRead(Patch[patch_id].Instrument[Player_ptr[player].instrument_id].info, 1))
        {
            Player_ptr[player].Set_pitch_bend(value);
            Player_ptr[player].Set_effects(resolution_value[resolution], downsampling);
        }
    }
}

void PlayersManager::Multicast_volume_for_MIDI_LOOP_running(int track, float value)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if ((Player_ptr[player].Read_loop_track() == track) && Player_ptr[player].isPlaying())
        {
            Player_ptr[player].Update_volume(value * Preset[Player_ptr[player].Read_instrument()].volume);
        }
    }
}

void PlayersManager::Update_players_stistics(void)
{
    players_using_Wavetable = 0;
    players_using_Flash = 0;
    players_using_Psram = 0;

    for (auto player = 0; player < PLAYERS; ++player)
    {
        if (Player_ptr[player].isPlaying())
        {
            if (Player_ptr[player].Uses_flash())
            {
                ++players_using_Flash;
            }
            else if (Player_ptr[player].Read_use_Wavetable())
            {
                ++players_using_Wavetable;
            }
            else
            {
                ++players_using_Psram;
            }
        }
    }
    players_playing = players_using_Wavetable + players_using_Flash + players_using_Psram;
}

int PlayersManager::Get_players_playing(void)
{
    return players_playing;
}

int PlayersManager::Get_players_using_Flash(void)
{
    return players_using_Flash;
}

int PlayersManager::Get_players_using_Wavetable(void)
{
    return players_using_Wavetable;
}

void PlayersManager::Calculate_and_set_mix_samples(void)
{
    if (read_diagnostics_enabled)
    {
        read_budget_diagnostics = {}; // Clear decisions from the previous block, including idle Player entries.
        read_budget_diagnostics.cycle = audio_update_cycle;
        read_budget_diagnostics.valid = true;
        read_budget_diagnostics.first_player = restart_mix_first_player;
    }
    float available = Get_span_for_all_cross_mix(); // Shared pool for note restarts and geometry edits.
    budget_crossfade_us = 0.0f;
    if (read_diagnostics_enabled)
    {
        read_budget_diagnostics.available_us = available; // Preserve the rounded pool actually consumed below.
    }
    const uint8_t first = restart_mix_first_player; // Rotate scarce crossfade time without changing note selection.
    bool assigned = false; // Advance fairness only when a transition is present.
    for (int offset = 0; offset < PLAYERS; ++offset) // Account each additional outgoing harvest exactly once.
    {
        const int player = (first + offset) % PLAYERS; // Fair scheduling order.
        auto &voice = Player_ptr[player]; // Current and pending state share one transition.
        if (!voice.Needs_restart_mix() && !voice.Has_pending_edit())
        {
            continue;
        }
        assigned = true;
        if (read_diagnostics_enabled)
        {
            read_budget_diagnostics.transition[player] = (voice.Needs_restart_mix() ? 1u : 0u) | (voice.Has_pending_edit() ? 2u : 0u);
            read_budget_diagnostics.available_before_us[player] = available;
        }
        uint8_t chosen = 0; // A zero-mix restart uses separate blocks and never needs two source harvests.
        for (const uint8_t samples : {64, 48, 32, 24, 16}) // Longest affordable outgoing fade.
        {
            const float cost = Get_cross_mix_time(player, samples); // Actual outgoing source at maximum modulation.
            if (cost <= available)
            {
                chosen = samples;
                available -= cost;
                budget_crossfade_us += cost;
                if (read_diagnostics_enabled)
                {
                    read_budget_diagnostics.assigned_us[player] = cost;
                }
                break;
            }
            if (read_diagnostics_enabled && samples == 16)
            {
                read_budget_diagnostics.minimum_us[player] = cost; // No extra forecast call: record the minimum tier only when it was rejected.
            }
        }
        if (read_diagnostics_enabled)
        {
            read_budget_diagnostics.mix_samples[player] = chosen;
        }
        voice.Set_mix_samples(chosen);
        voice.Set_edit_mix_samples(chosen);
        restart_Player[player] = false;
    }
    if (read_diagnostics_enabled)
    {
        read_budget_diagnostics.crossfade_us = budget_crossfade_us;
    }
    players_to_restart = 0;
    if (assigned)
    {
        restart_mix_first_player = (first + 1) % PLAYERS;
    }
}

void PlayersManager::Reset_players_to_restart(void)
{
    players_to_restart = 0;
}

int PlayersManager::Get_players_to_restart(void)
{
    return players_to_restart;
}

void PlayersManager::Multicast_update_vibrato(int midi_channel, bool vibrato_active)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if (Player_ptr[player].Read_midi_channel() == midi_channel)
        {
            Player_ptr[player].Set_vibrato_flag(vibrato_active);
        }
    }
}

void PlayersManager::Multicast_all_notes_off(int midi_channel)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if (Player_ptr[player].isPoweredOn() && Player_ptr[player].Read_midi_channel() == midi_channel)
        {
            Release_Player_noteOff(player);
        }
    }
}

void PlayersManager::Multicast_stop_players_for_loop_track(int track)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if (Player_ptr[player].isPoweredOn() && Player_ptr[player].Assigned_track() == track)
        {
            Release_Player_noteOff(player, track);
        }
    }
}

void PlayersManager::Multicast_stop_players_for_NoteOff(int midi_channel, int note_number, int track)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if (Player_ptr[player].isPoweredOn() && Player_ptr[player].Assigned_note() == note_number && Player_ptr[player].Read_midi_channel() == midi_channel && Player_ptr[player].Assigned_track() == track)
        {
            Release_Player_noteOff(player, track);
        }
    }
}

void PlayersManager::Broadcast_FIFO_stereo(int16_t *LS_buffer_L_ptr, int16_t *LS_buffer_R_ptr)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        Player_ptr[player].LS_buffer_L_ptr = LS_buffer_L_ptr;
        Player_ptr[player].LS_buffer_R_ptr = LS_buffer_R_ptr;
    }
}

void PlayersManager::Broadcast_FIFO_mono(int16_t *LS_buffer_mono_ptr)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        Player_ptr[player].LS_buffer_mono_ptr = LS_buffer_mono_ptr;
    }
}

uint8_t PlayersManager::Refresh_audio_table_references(void)
{
    uint8_t referenced_banks = 0;
    for (uint8_t player_id = 0; player_id < PLAYERS; ++player_id)
    {
        if (Audio_tables_ptr != nullptr)
        {
            Player_ptr[player_id].Refresh_audio_table_references(*Audio_tables_ptr);
        }
        referenced_banks |= Player_ptr[player_id].Get_tables_reference_mask();
    }
    return referenced_banks;
}

int PlayersManager::Get_players_using_Psram(void)
{
    return players_using_Psram;
}

void PlayersManager::Refresh_cache_sources(void)
{
    for (auto &preset : Preset)
    {
        if (!preset.active)
        {
            continue;
        }
        preset.source = Cache_manager_ptr->Get_source(preset.file);
        if (preset.source.storage != Psram)
        {
            continue;
        }
        for (uint8_t player = 0; player < PLAYERS; ++player)
        {
            Player_ptr[player].Refresh_cached_source(preset.source);
        }
    }
}

uint16_t PlayersManager::Get_cache_reference_mask(void)
{
    uint16_t mask = 0;
    for (uint8_t player = 0; player < PLAYERS; ++player)
    {
        mask |= Player_ptr[player].Get_cache_reference_mask();
    }
    return mask;
}

uint16_t PlayersManager::Fast_stop_players_using_cache(uint16_t mask)
{
    uint16_t stopped = 0;
    for (uint8_t player = 0; player < PLAYERS; ++player)
    {
        if (Player_ptr[player].Fast_stop_using_cache(mask))
        {
            stopped |= static_cast<uint16_t>(1u << player);
        }
    }
    return stopped;
}
