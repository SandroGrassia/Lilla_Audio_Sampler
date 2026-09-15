// Frozen pre-refactor selection policy, used only by host regression tests.
int PlayersManager::Legacy_select(uint8_t instrument_id, uint8_t note_number, int track)
{
    // Se e' una nota appartenente ad un track: track >= -1
    // Altrimenti: track = NO_TRACK

    int8_t id_player = -1;
    bool finished = false;
    const bool needs_sample = !Preset[instrument_id].use_Wavetable && Preset[instrument_id].file < FIRST_LIVE_SAMPLING_FILE;
    const int voice_limit = OPTIMIZATION_VOICES[optimization];
    // A reduced profile takes effect after the old voices finish their fast release.
    if (needs_sample && Count_sample_voices() > voice_limit)
    {
        return -1;
    }

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

    if (!finished && needs_sample) // File voices share the profile budget regardless of their current storage.
    {
        Update_players_stistics();

        if (Count_sample_voices() < voice_limit) // a Player can be used
        {
            // 1) if there is a Player !isPlaying, it can be used
            for (auto player = 0; player < PLAYERS; ++player)
            {
                if (!Player_booked[player] && !Player_ptr[player].isPlaying())
                {
                    id_player = player;
                    finished = true;
                    // PRINT("Play from Flash - case 0", "Found available Player:", id_player);
                    break;
                }
            }

            if (!finished)
            {
                // 2) if there is a Player playing an Instrument of a different Patch, this Player can be used
                for (auto player = 0; player < PLAYERS; ++player)
                {
                    if (!Player_booked[player] && Player_ptr[player].Read_patch_wait() != Patch_id)
                    {
                        id_player = player;
                        finished = true;
                        // PRINT("Play from Flash - case 1", "Found available Player:", id_player);
                        break;
                    }
                }
            }

            if (!finished)
            {
                // 3) look for a Player !power_on and playing of the SAME INSTRUMENT: choose the OLDEST
                id_player = Legacy_Smartfind_oldest_player(instrument_id, false, true); // SMARTFIND_oldest_player(uint8_t instrument_id, bool power_on, bool playing)
                if (id_player >= 0)
                {
                    finished = true;
                    // PRINT("Play from Flash - case 2", "Found available Player:", id_player);
                }
            }

            if (!finished)
            {
                // 4) look for a Player !power_on of ANY INSTRUMENT NOT protected: choose the OLDEST
                id_player = Legacy_Simplefind_oldest_player(false); // SIMPLEFIND_oldest_player_power_on(bool power_on)
                if (id_player >= 0)
                {
                    finished = true;
                    // PRINT("Play from Flash - case 3", "Found available Player:", id_player);
                }
            }
            if (!finished)
            {
                // 5) look for a Player power_on and playing of the SAME INSTRUMENT: choose the OLDEST
                id_player = Legacy_Smartfind_oldest_player(instrument_id, true, true); // SMARTFIND_oldest_player(uint8_t instrument_id, bool power_on, bool playing)
                if (id_player >= 0)
                {
                    finished = true;
                    // PRINT("Play from Flash - case 4", "Found available Player:", id_player);
                }
            }
            if (!finished && Preset[instrument_id].precedence)
            {
                // 6) look for a Player power_on of ANY INSTRUMENT NOT protected: choose the OLDEST
                id_player = Legacy_Simplefind_oldest_player(true); // SIMPLEFIND_oldest_player(bool power_on)
                if (id_player >= 0)
                {
                    finished = true;
                    // PRINT("Play from Flash - case 5", "Found available Player:", id_player);
                }
            }
        }

        // Only an existing Flash/cache slot can be reused at the profile limit.
        else
        {
            // 5A) there is a Player flash_mode from a different Patch and playing
            for (auto player = 0; player < PLAYERS; ++player)
            {
                if (!Player_booked[player] && (Player_ptr[player].Read_patch_wait() != Patch_id) && Player_ptr[player].Uses_sample_voice() && Player_ptr[player].State() > 0)
                {
                    id_player = player;
                    finished = true;
                    // PRINT("Play from Flash - case 6", "Found available Player:", id_player);
                    break;
                }
            }

            if (!finished)
            {
                // 6) there is a Player playing the SAME INSTRUMENT: choose the OLDEST
                id_player = Legacy_Smartfind_oldest_player(instrument_id, false, true); // SMARTFIND_oldest_player(uint8_t instrument_id, bool power_on, bool playing)
                if (id_player >= 0)
                {
                    finished = true;
                    // PRINT("Play from Flash - case 7", "Found available Player:", id_player);
                }
            }

            if (!finished)
            {
                // 7)  there is a Player fading (state = 2) ANY INSTRUMENT NOT protected flash_mode: choose the OLDEST
                id_player = Legacy_Simplefind_oldest_sample_player(false, true);
                if (id_player >= 0)
                {
                    finished = true;
                    // PRINT("Play from Flash - case 9", "Found available Player:", id_player);
                }
            }

            if (!finished)
            {
                // 8)  there is a Player playing (state = 1) ANY INSTRUMENT NOT protected flash_mode: choose the OLDEST
                id_player = Legacy_Simplefind_oldest_sample_player(true, true);
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
            for (auto player = 0; player < PLAYERS; ++player)
            {
                if (!Player_booked[player] && !Player_ptr[player].isPlaying()) // isPlaying() significa !idle
                {
                    id_player = player;
                    finished = true;
                    // PRINT("Play from RAM - case 1", "Found available Player:", id_player);
                    break;
                }
            }
        }

        else // all Player are playing
        {
            // 2) look for a Player playing an Instrument of a different Patch: this Player can be taken
            for (auto player = 0; player < PLAYERS; ++player)
            {
                if (!Player_booked[player] && Player_ptr[player].Read_patch_wait() != Patch_id)
                {
                    id_player = player;
                    finished = true;
                    // PRINT("Play from RAM - case 2", "Found available Player:", id_player);
                }
            }

            if (!finished)
            {
                // 3) look for a Player !power_on (ADSR "Release" phase, or idle) and playing of the SAME INSTRUMENT: choose the OLDEST
                id_player = Legacy_Smartfind_oldest_player(instrument_id, false, true); // SMARTFIND_oldest_player(uint8_t instrument_id, bool power_on, bool playing)
                if (id_player >= 0)
                {
                    finished = true;
                    // PRINT("Play from RAM - case 3", "Found available Player:", id_player);
                }
            }

            if (!finished)
            {
                // 4) look for a Player !power_on of ANY INSTRUMENT NOT protected: choose the OLDEST
                id_player = Legacy_Simplefind_oldest_player(false); // SIMPLEFIND_oldest_player(bool power_on)
                if (id_player >= 0)
                {
                    finished = true;
                    // PRINT("Play from RAM - case 4", "Found available Player:", id_player);
                }
            }

            if (!finished)
            {
                // 5) look for a player power_on and playing of the SAME INSTRUMENT: choose the OLDEST
                id_player = Legacy_Smartfind_oldest_player(instrument_id, true, true); // SMARTFIND_oldest_player(uint8_t instrument_id, bool power_on, bool playing)
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
            id_player = Legacy_Simplefind_oldest_player(true); // SIMPLEFIND_oldest_player(bool power_on)
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
            id_player = Legacy_Simplefind_oldest_sample_player(false, true);
            if (id_player < 0)
            {
                id_player = Legacy_Simplefind_oldest_sample_player(true, true);
            }
            finished = id_player >= 0;
        }
    }
    return finished ? id_player : -1;
}

int PlayersManager::Legacy_Smartfind_oldest_player(uint8_t instrument_id, bool power_on, bool playing)
{
    unsigned long time_min = 0;
    ;
    int8_t result = -1;

    for (auto player_ext = 0; player_ext < PLAYERS; ++player_ext)
    {
        if ((Player_ptr[player_ext].Read_instrument() == instrument_id) && (Player_ptr[player_ext].isPoweredOn() == power_on) && (Player_ptr[player_ext].isPlaying() == playing) && !Player_booked[player_ext])
        {
            time_min = Player_ptr[player_ext].Read_time_stamp();
            result = player_ext;
            for (auto player = 0; player < PLAYERS; ++player) // look for the player playing/going to play for the longest time
            {
                if (((Player_ptr + player)->Read_instrument() == instrument_id) && ((Player_ptr + player)->isPoweredOn() == power_on) && ((Player_ptr + player)->isPlaying() == playing) && ((Player_ptr + player)->Read_time_stamp() < time_min) && !Player_booked[player])
                {
                    time_min = (Player_ptr + player)->Read_time_stamp();
                    result = player;
                }
            }
            break;
        }
    }
    return result;
}

int PlayersManager::Legacy_Simplefind_oldest_player(bool power_on) // "precedence" instruments are EXCLUDED
{
    unsigned long time_min = 0;
    int8_t result = -1;

    for (auto player_ext = 0; player_ext < PLAYERS; ++player_ext)
    {
        if (!Player_ptr[player_ext].Read_precedence() && (Player_ptr[player_ext].isPoweredOn() == power_on) && !Player_booked[player_ext])
        {
            time_min = Player_ptr[player_ext].Read_time_stamp();
            result = player_ext;
            for (auto player = 0; player < PLAYERS; ++player) // look for the player playing for the longest time
            {
                if (!(Player_ptr + player)->Read_precedence() && ((Player_ptr + player)->isPoweredOn() == power_on) && ((Player_ptr + player)->Read_time_stamp() < time_min) && !Player_booked[player])
                {
                    time_min = (Player_ptr + player)->Read_time_stamp();
                    result = player;
                }
            }
            break;
        }
    }
    return result;
}

int PlayersManager::Legacy_Simplefind_oldest_sample_player(bool power_on, bool playing) // "precedence" instruments are EXCLUDED
{
    unsigned long time_min = 0;
    int8_t result = -1;

    for (auto player_ext = 0; player_ext < PLAYERS; ++player_ext)
    {
        if (Player_ptr[player_ext].Uses_sample_voice() && !Player_ptr[player_ext].Read_precedence() && (Player_ptr[player_ext].isPlaying() == playing) && (Player_ptr[player_ext].isPoweredOn() == power_on) && !Player_booked[player_ext])
        {
            time_min = Player_ptr[player_ext].Read_time_stamp();
            result = player_ext;
            for (auto player = 0; player < PLAYERS; ++player) // look for the player playing for the longest time
            {
                if ((Player_ptr + player)->Uses_sample_voice() && !(Player_ptr + player)->Read_precedence() && ((Player_ptr + player)->isPlaying() == playing) && ((Player_ptr + player)->isPoweredOn() == power_on) && ((Player_ptr + player)->Read_time_stamp() < time_min) && !Player_booked[player])
                {
                    time_min = (Player_ptr + player)->Read_time_stamp();
                    result = player;
                }
            }
            break;
        }
    }
    return result;
}
