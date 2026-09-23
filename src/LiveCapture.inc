// Live capture creates ordinary patch/Sound metadata; RAW storage is deferred until Save.
static int Capture_target = -1;
static uint8_t Capture_return_patch = 0;
static int8_t Capture_pair[INSTRUMENTS] = {-1, -1, -1, -1, -1, -1, -1, -1};

FLASHMEM static void Capture_popup(const char *message)
{
    tft.fillRoundRect(8, 80, 304, 72, 4, ILI9341_RED);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(16, 92);
    tft.print(message);
}

FLASHMEM static void Capture_notice(const char *message)
{
    Capture_popup(message);
    const uint32_t started = millis();
    while (static_cast<uint32_t>(millis() - started) < 2000u)
    {
        Shifters_manager.Update();
    }
    Clear_UI_events();
}

FLASHMEM static bool Capture_confirm(void)
{
    int choice = 0;
    Capture_popup("REPLACE CAPTURE?");
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

FLASHMEM static bool Capture_root(uint8_t &root)
{
    Capture_popup("ROOT KEY");
    tft.setCursor(16, 120);
    tft.print("PRESS KEY / SELECT TO CANCEL");
    Clear_UI_events();
    AudioNoInterrupts();
    Capture_learn_note = -1;
    Capture_learn_key = true;
    AudioInterrupts();
    int note = -1;
    while (note < 0)
    {
        Shifters_manager.Update();
        if (Read_pushbutton(EN_PB_Select))
        {
            Capture_learn_key = false;
            Clear_UI_events();
            return false;
        }
        AudioNoInterrupts();
        note = Capture_learn_note;
        Capture_learn_note = -1;
        AudioInterrupts();
    }
    Capture_learn_key = false;
    root = note;
    Clear_UI_events();
    return true;
}

FLASHMEM static bool Capture_drain(void)
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

FLASHMEM static bool Capture_referenced(int file, int except_sound = -1)
{
    for (int id = 0; id < SOUNDS_MAX; ++id)
    {
        if ((id != except_sound && Sound[id].used && Sound[id].file == file) || (S_Sound_cache_P[id].used && S_Sound_cache_P[id].file == file))
        {
            return true;
        }
    }
    return false;
}

FLASHMEM static void Capture_collect(void)
{
    // Snapshots protect Discard; retiring players retain their cache until their release ends.
    AudioNoInterrupts();
    for (auto &source : Capture_sources)
    {
        if (source.audio.psram_ptr != nullptr && !Capture_referenced(source.audio.file_id))
        {
            const int file = source.audio.file_id;
            source = {};
            PatchCache_Manager.Invalidate_file(file);
        }
    }
    PatchCache_Manager.Release_unreferenced_caches(Players_Manager.Get_cache_reference_mask());
    AudioInterrupts();
}

FLASHMEM static void Capture_sound(int selected)
{
    if (LS_state == REC || LiveSampler.Is_writing() || LS_state != PLAYONLY || LS_mode < LOOP_FWD)
    {
        Capture_notice("STOP REC AND SELECT LOOP MODE");
        LS_refresh_LS_page();
        return;
    }
    const uint32_t samples = LS_XY_delta + 1;
    if (samples == 0 || samples > PATCH_CACHE_ARRAY_SAMPLES || LS_buffer_dim <= 0)
    {
        Capture_notice("LOOP TOO LONG FOR CACHE");
        LS_refresh_LS_page();
        return;
    }
    const bool creating = Capture_target < 0 || Capture_target != Patch_id_old || !Patch[Capture_target].used;
    const int target = creating ? S_Get_Patch_id_free() : Capture_target;
    if (target < 0 || (creating && !P_Verify_is_Patch_original(Patch_id_old)))
    {
        Capture_notice(target < 0 ? "NO FREE PATCH" : "SAVE CURRENT PATCH FIRST");
        LS_refresh_LS_page();
        return;
    }
    int first = selected;
    int second = -1;
    const int partner = creating ? -1 : Capture_pair[selected];
    if (LS_stereo)
    {
        if (partner >= 0 && Patch[target].Instrument[partner].used && Patch[target].Instrument[selected].used)
        {
            first = selected < partner ? selected : partner;
            second = selected < partner ? partner : selected;
        }
        else if (selected + 1 < INSTRUMENTS && (creating || !Patch[target].Instrument[selected + 1].used))
        {
            second = selected + 1;
        }
    }
    if (!creating && Patch[target].Instrument[selected].used && !Capture_confirm())
    {
        LS_refresh_LS_page();
        return;
    }
    uint8_t root = 60;
    if (!Capture_root(root) || !Capture_drain())
    {
        LS_refresh_LS_page();
        return;
    }
    Capture_collect();
    const int channels = second < 0 ? 1 : 2;
    int sounds[2] = {-1, -1};
    int slots[2] = {-1, -1};
    int files[2] = {-1, -1};
    int16_t *destinations[2] = {};
    for (int channel = 0; channel < channels; ++channel)
    {
        const int instrument = channel == 0 ? first : second;
        if (!creating && Patch[target].Instrument[instrument].used)
        {
            sounds[channel] = Patch[target].Instrument[instrument].sound_id;
        }
        else
        {
            for (int id = 0; id < SOUNDS_MAX; ++id)
            {
                if (!Sound[id].used && id != sounds[0])
                {
                    sounds[channel] = id;
                    break;
                }
            }
        }
        for (int slot = 0; slot < CAPTURE_SOURCES; ++slot)
        {
            const auto &source = Capture_sources[slot];
            const bool reusable = !source.written && !Capture_referenced(source.audio.file_id, sounds[channel]);
            if (slot != slots[0] && (source.audio.psram_ptr == nullptr || reusable))
            {
                slots[channel] = slot;
                break;
            }
        }
        if (slots[channel] >= 0 && Capture_sources[slots[channel]].audio.psram_ptr != nullptr)
        {
            files[channel] = Capture_sources[slots[channel]].audio.file_id;
        }
        else
        {
            for (int file = 1; file < FIRST_RECORDING_FILE; ++file)
            {
                if (file != files[0] && Capture_find(file) == nullptr && !SerialFlash.exists(name_file[file]))
                {
                    files[channel] = file;
                    break;
                }
            }
        }
        if (sounds[channel] < 0 || slots[channel] < 0 || files[channel] < 0)
        {
            Midi_reader.Start();
            Capture_notice("NO FREE SOUND / CACHE / FILE");
            LS_refresh_LS_page();
            return;
        }
    }
    for (int channel = 0; channel < channels; ++channel)
    {
        destinations[channel] = PatchCache_Manager.Reserve_capture(slots[channel], files[channel], samples);
        if (destinations[channel] == nullptr)
        {
            Midi_reader.Start();
            Capture_notice("CAPTURE CACHE UNAVAILABLE");
            LS_refresh_LS_page();
            return;
        }
    }
    if (creating)
    {
        Capture_return_patch = Patch_id_old;
        Capture_target = target;
        Capture_new_patch = target;
        Capture_patch_delay = Delay_data;
        Patch_cache_P = Patch[target];
        S_Copy_all_Sound_to_Sound_cache_P();
        Patch[target] = {};
        for (auto &pair : Capture_pair)
        {
            pair = -1;
        }
    }
    int position = LS_X_sample % LS_buffer_dim;
    if (position < 0)
    {
        position += LS_buffer_dim;
    }
    for (int channel = 0; channel < channels; ++channel)
    {
        const int instrument_id = channel == 0 ? first : second;
        int16_t *destination = destinations[channel];
        for (uint32_t i = 0; i < samples; ++i)
        {
            const int index = (position + i) % LS_buffer_dim;
            destination[i] = !LS_stereo ? LS_buffer_mono_ptr[index] : (second < 0 ? static_cast<int16_t>((static_cast<int32_t>(LS_buffer_L_ptr[index]) + LS_buffer_R_ptr[index]) / 2) : (channel == 0 ? LS_buffer_L_ptr[index] : LS_buffer_R_ptr[index]));
        }
        Capture_sources[slots[channel]] = {{static_cast<int16_t>(files[channel]), Psram, destination, samples, static_cast<int8_t>(slots[channel])}, false};
        auto &instrument = Patch[target].Instrument[instrument_id];
        instrument = {};
        instrument.used = true;
        instrument.sound_id = sounds[channel];
        instrument.from_note = instrument.root_key = instrument.to_note = root;
        instrument.Filter = Patch[PATCHES_MAX].Instrument[0].Filter;
        auto &sound = Sound[sounds[channel]];
        sound = Sound[SOUNDS_MAX + channel];
        sound.used = true;
        sound.file = files[channel];
        sound.A = 0;
        sound.B = samples - 1;
        sound.Noclick = LS_mode == LOOP_FWD ? 128 : 0;
        sound.pan = second < 0 ? 0 : (channel == 0 ? -16 : 16);
        const int previous_partner = Capture_pair[instrument_id];
        if (previous_partner >= 0)
        {
            Capture_pair[previous_partner] = -1;
        }
        Capture_pair[instrument_id] = -1;
    }
    if (second >= 0)
    {
        Capture_pair[first] = second;
        Capture_pair[second] = first;
    }
    Patch[target].used = true;
    Patch[target].instruments = 0;
    for (const auto &instrument : Patch[target].Instrument)
    {
        Patch[target].instruments += instrument.used;
    }
    Patch_id_old = target;
    P_Update_Patches_number();
    Midi_reader.Start();
    LS_refresh_LS_page();
}

FLASHMEM static bool Capture_write(CaptureSource &source)
{
    if (source.written)
    {
        return true; // A retry after another channel failed must not allocate again.
    }
    const char *name = name_file[source.audio.file_id];
    const uint32_t bytes = source.audio.samples * sizeof(int16_t);
    if (SerialFlash.exists(name) || !SerialFlash.create(name, bytes))
    {
        return false;
    }
    SerialFlashFile file = SerialFlash.open(name);
    bool complete = static_cast<bool>(file);
    const auto *data = reinterpret_cast<const uint8_t *>(source.audio.psram_ptr);
    for (uint32_t offset = 0; complete && offset < bytes; offset += 256u)
    {
        const uint32_t count = bytes - offset < 256u ? bytes - offset : 256u;
        complete = file.write(data + offset, count) == count;
        Shifters_manager.Update();
    }
    SerialFlash.wait();
    file.seek(0);
    uint8_t verify[256];
    for (uint32_t offset = 0; complete && offset < bytes; offset += sizeof(verify))
    {
        const uint32_t count = bytes - offset < sizeof(verify) ? bytes - offset : sizeof(verify);
        complete = file.read(verify, count) == count && memcmp(verify, data + offset, count) == 0;
    }
    file.close();
    if (!complete)
    {
        SerialFlash.remove(name); // Hide incomplete data; SerialFlash cannot reclaim its allocation.
        return false;
    }
    source.written = true;
    return true;
}

FLASHMEM static bool Capture_materialize(void)
{
    bool needed = false;
    for (int id = 0; id < SOUNDS_MAX; ++id)
    {
        needed |= Sound[id].used && Capture_pending(Sound[id].file);
    }
    if (!needed)
    {
        return true;
    }
    if (!Capture_drain())
    {
        Capture_notice("SAVE BUSY - TRY AGAIN");
        return false;
    }
    Capture_popup("SAVING RAW FILES...");
    bool complete = true;
    for (int id = 0; complete && id < SOUNDS_MAX; ++id)
    {
        auto *source = Sound[id].used ? Capture_find(Sound[id].file) : nullptr;
        if (source != nullptr)
        {
            complete = Capture_write(*source);
        }
    }
    File_scanner.Read_all_file_data();
    Midi_reader.Start();
    Clear_UI_events();
    if (!complete)
    {
        Capture_notice("RAW SAVE FAILED - RETRY");
    }
    return complete;
}

FLASHMEM static void Capture_finish_save(void)
{
    AudioNoInterrupts();
    for (auto &source : Capture_sources)
    {
        if (source.written)
        {
            source = {}; // Its ordinary cache remains valid and may now be reclaimed normally.
        }
    }
    PatchCache_Manager.Set_required_files(Preset);
    AudioInterrupts();
}
