// Volatile captures use the first eight patch caches; Flash metadata is never created.
static Patch_struct Playback_patch = {};
static int8_t Playback_partner[PLAYBACK_FILES] = {-1, -1, -1, -1, -1, -1, -1, -1};
static Delay_data_struct Playback_delay = {};
static bool Playback_delay_valid = false;
static uint8_t Playback_return_patch = 0;

FLASHMEM static void Playback_popup(const char *message)
{
    tft.fillRoundRect(8, 80, 304, 72, 4, ILI9341_RED);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(16, 92);
    tft.print(message);
}

FLASHMEM static void Playback_notice(const char *message)
{
    Playback_popup(message);
    const uint32_t started = millis();
    while (static_cast<uint32_t>(millis() - started) < 2000u)
    {
        Shifters_manager.Update();
    }
    Clear_UI_events();
}

FLASHMEM static bool Playback_confirm(const char *message)
{
    int choice = 0;
    Playback_popup(message);
    Clear_UI_events();
    do
    {
        Shifters_manager.Update();
        Read_encoder(EN_PB_Select, choice, 1, 0, 1);
        tft.setCursor(16, 120);
        tft.print(choice == 0 ? ">NO     YES " : " NO    >YES ");
    } while (!Read_pushbutton(EN_PB_Select));
    Clear_UI_events();
    return choice == 1;
}

FLASHMEM static bool Playback_keys(uint8_t (&keys)[3])
{
    const char *prompts[3] = {"FROM KEY", "ROOT KEY", "TO KEY"};
    for (int step = 0; step < 3; ++step)
    {
        Playback_popup(prompts[step]);
        tft.setCursor(16, 120);
        tft.print("PRESS KEY / SELECT TO CANCEL");
        Clear_UI_events();
        AudioNoInterrupts();
        Playback_learn_note = -1;
        Playback_learn_keys = true;
        AudioInterrupts();
        int note = -1;
        while (note < 0)
        {
            Shifters_manager.Update();
            if (Read_pushbutton(EN_PB_Select))
            {
                Playback_learn_keys = false;
                Clear_UI_events();
                return false;
            }
            AudioNoInterrupts();
            note = Playback_learn_note;
            Playback_learn_note = -1;
            AudioInterrupts();
            if (note >= 0 && step > 0 && note < keys[step - 1])
            {
                note = -1; // Enforce FROM <= ROOT <= TO without changing existing captures.
            }
        }
        Playback_learn_keys = false;
        keys[step] = note;
    }
    Clear_UI_events();
    return true;
}

FLASHMEM static bool Playback_drain_players(void)
{
    Midi_reader.Stop();
    AudioNoInterrupts();
    Players_Manager.Stop_all_players();
    AudioInterrupts();
    const uint32_t started = millis();
    bool playing;
    do
    {
        playing = false;
        AudioNoInterrupts();
        for (int i = 0; i < PLAYERS; ++i)
        {
            playing |= Player[i].isPlaying();
        }
        AudioInterrupts();
    } while (playing && static_cast<uint32_t>(millis() - started) < 100u);
    if (playing)
    {
        Midi_reader.Start();
    }
    return !playing;
}

FLASHMEM static void Playback_publish_source(int slot, int16_t *data, uint32_t samples)
{
    Playback_sources[slot] = {static_cast<int16_t>(FIRST_PLAYBACK_FILE + slot), Psram, data, samples, static_cast<int8_t>(slot)};
}

FLASHMEM static void Playback_capture(int selected)
{
    if (LS_state == REC || LiveSampler.Is_writing())
    {
        Playback_notice("STOP RECORDING FIRST");
        LS_refresh_LS_page();
        return;
    }
    if (LS_state != PLAYONLY || LS_mode < LOOP_FWD)
    {
        Playback_notice("SELECT LOOP MODE FIRST");
        LS_refresh_LS_page();
        return;
    }
    const uint32_t samples = LS_XY_delta + 1;
    if (samples > PATCH_CACHE_ARRAY_SAMPLES)
    {
        Playback_notice("LOOP TOO LONG FOR CACHE");
        LS_refresh_LS_page();
        return;
    }
    int first = selected;
    int second = -1;
    const int partner = Playback_partner[selected];
    if (LS_stereo)
    {
        if (partner >= 0)
        {
            first = selected < partner ? selected : partner;
            second = selected < partner ? partner : selected;
        }
        else if (selected + 1 < PLAYBACK_FILES && !Playback_patch.Instrument[selected + 1].used)
        {
            second = selected + 1;
        }
    }
    if (Playback_patch.Instrument[selected].used && !Playback_confirm("REPLACE CAPTURE?"))
    {
        LS_refresh_LS_page();
        return;
    }
    uint8_t keys[3];
    if (!Playback_keys(keys))
    {
        LS_refresh_LS_page();
        return;
    }
    if (!Playback_drain_players())
    {
        Playback_notice("CAPTURE BUSY - TRY AGAIN");
        LS_refresh_LS_page();
        return;
    }
    // Recording is stopped and every cache reader has drained: copies cannot race audio.
    if (!LS_stereo && partner >= 0)
    {
        const auto &left = Playback_sources[selected];
        const auto &right = Playback_sources[partner];
        int16_t *mono = const_cast<int16_t *>(right.psram_ptr);
        for (uint32_t i = 0; i < right.samples; ++i)
        {
            mono[i] = static_cast<int16_t>((static_cast<int32_t>(left.psram_ptr[i]) + right.psram_ptr[i]) / 2);
        }
        Sound[FIRST_PLAYBACK_SOUND + partner].pan = 0;
        Playback_partner[partner] = -1;
        Playback_partner[selected] = -1;
    }
    int position = LS_X_sample % LS_buffer_dim;
    if (position < 0)
    {
        position += LS_buffer_dim;
    }
    for (int channel = 0; channel < (second < 0 ? 1 : 2); ++channel)
    {
        const int slot = channel == 0 ? first : second;
        int16_t *destination = PatchCache_Manager.Reserve_playback(slot, samples);
        for (uint32_t i = 0; i < samples; ++i)
        {
            const int index = (position + i) % LS_buffer_dim;
            destination[i] = !LS_stereo ? LS_buffer_mono_ptr[index] : (second < 0 ? static_cast<int16_t>((static_cast<int32_t>(LS_buffer_L_ptr[index]) + LS_buffer_R_ptr[index]) / 2) : (channel == 0 ? LS_buffer_L_ptr[index] : LS_buffer_R_ptr[index]));
        }
        Playback_publish_source(slot, destination, samples);
        auto &instrument = Playback_patch.Instrument[slot];
        instrument = {};
        instrument.used = true;
        instrument.sound_id = FIRST_PLAYBACK_SOUND + slot;
        instrument.from_note = keys[0];
        instrument.root_key = keys[1];
        instrument.to_note = keys[2];
        instrument.Filter = Patch[PATCHES_MAX].Instrument[0].Filter;
        auto &sound = Sound[instrument.sound_id];
        sound = Sound[SOUNDS_MAX + channel];
        sound.used = true;
        sound.file = FIRST_PLAYBACK_FILE + slot;
        sound.A = 0;
        sound.B = samples - 1;
        sound.Noclick = LS_mode == LOOP_FWD ? 128 : 0;
        sound.pan = second < 0 ? 0 : (channel == 0 ? -16 : 16);
        Playback_partner[slot] = second < 0 ? -1 : (channel == 0 ? second : first);
    }
    if (!Playback_patch.used)
    {
        Playback_return_patch = Patch_id_old;
        Playback_delay = Delay_data;
        Playback_delay_valid = true;
    }
    Playback_patch.used = true;
    Playback_patch.instruments = 0;
    for (const auto &instrument : Playback_patch.Instrument)
    {
        Playback_patch.instruments += instrument.used;
    }
    Midi_reader.Start();
    LS_refresh_LS_page();
}

FLASHMEM static bool Playback_enter(void)
{
    if (!Playback_patch.used)
    {
        return false;
    }
    if (LiveSampler.Is_writing())
    {
        LiveSampler.Stop();
    }
    const uint32_t started = millis();
    while (LiveSampler.Is_writing() && static_cast<uint32_t>(millis() - started) < 100u)
    {
        Shifters_manager.Update();
    }
    if (LiveSampler.Is_writing() || !Playback_drain_players())
    {
        Playback_notice("PLAYBACK BUSY - TRY AGAIN");
        LS_refresh_LS_page();
        return true;
    }
    LS_state = PLAYONLY;
    const Patch_struct previous = Patch[PATCHES_MAX];
    Patch[PATCHES_MAX] = Playback_patch;
    Patch_id = PATCHES_MAX;
    if (!S_Fill_all_tables())
    {
        Patch[PATCHES_MAX] = previous;
        Midi_reader.Start();
        LS_refresh_LS_page();
        return true;
    }
    Playback_active = true;
    if (Playback_delay_valid)
    {
        Delay_manager.New_values(&Playback_delay);
    }
    Patch_id_old = Playback_return_patch;
    P_Update_all_maps_Instrument_for_notes();
    Midi_reader.Start();
    Golive_with_PERFORMANCE(Patch_id);
    return true;
}

FLASHMEM static void Playback_remember(void)
{
    if (Playback_active)
    {
        Playback_patch = Patch[PATCHES_MAX];
        Playback_delay = Delay_data;
        Playback_delay_valid = true;
        Playback_active = false;
        Patch_id_old = Playback_return_patch;
    }
}

FLASHMEM static bool Playback_clone(int instrument_id, int &target)
{
    for (target = 0; target < PLAYBACK_FILES; ++target)
    {
        if (!Patch[Patch_id].Instrument[target].used)
        {
            break;
        }
    }
    if (target == PLAYBACK_FILES || !Playback_drain_players())
    {
        return false;
    }
    const auto original = Playback_sources[instrument_id];
    int16_t *destination = PatchCache_Manager.Reserve_playback(target, original.samples);
    if (destination == nullptr)
    {
        Midi_reader.Start();
        return false;
    }
    memcpy(destination, original.psram_ptr, original.samples * sizeof(int16_t));
    Playback_publish_source(target, destination, original.samples);
    auto &patch = Patch[Patch_id];
    patch.Instrument[target] = patch.Instrument[instrument_id];
    patch.Instrument[target].sound_id = FIRST_PLAYBACK_SOUND + target;
    Sound[FIRST_PLAYBACK_SOUND + target] = Sound[FIRST_PLAYBACK_SOUND + instrument_id];
    Sound[FIRST_PLAYBACK_SOUND + target].file = FIRST_PLAYBACK_FILE + target;
    Playback_partner[target] = -1;
    ++patch.instruments;
    const bool ready = S_Fill_all_tables();
    if (!ready)
    {
        patch.Instrument[target] = {};
        --patch.instruments;
        Playback_sources[target] = {};
        Sound[FIRST_PLAYBACK_SOUND + target] = {};
        PatchCache_Manager.Invalidate_file(FIRST_PLAYBACK_FILE + target);
    }
    Playback_patch = patch;
    P_Update_all_maps_Instrument_for_notes();
    Midi_reader.Start();
    return ready;
}

FLASHMEM static bool Playback_abandon(bool ask = true)
{
    if (!Playback_patch.used)
    {
        return true;
    }
    if ((ask && !Playback_confirm("LOSE PLAYBACK AND ALL SOUNDS?")) || !Playback_drain_players())
    {
        return false;
    }
    AudioNoInterrupts();
    for (int slot = 0; slot < PLAYBACK_FILES; ++slot)
    {
        PatchCache_Manager.Invalidate_file(FIRST_PLAYBACK_FILE + slot);
        Playback_sources[slot] = {};
        Playback_partner[slot] = -1;
        Sound[FIRST_PLAYBACK_SOUND + slot] = {};
    }
    PatchCache_Manager.Release_unreferenced_caches(0);
    Playback_patch = {};
    Playback_active = false;
    Playback_delay_valid = false;
    AudioInterrupts();
    Midi_reader.Start();
    return true;
}
