AudioBytes mp3_fixture(const char *name)
{
    std::ifstream file(std::string(mp3_test_directory) + "/" + name + ".mp3", std::ios::binary);
    assert(file);
    return AudioBytes(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

void test_mp3_import()
{
    bool changed = false;
    for (const char *name : {"mono", "stereo", "vbr", "untagged", "long"})
    {
        reset();
        const auto encoded = mp3_fixture(name);
        add("7.Mp3", 0)->data = encoded;
        assert(SET_Copy_audio_files_from_SD_to_Flash(changed) && changed);
        assert(Display_Storage.completed == 1);
        verify_fallback();
        const auto &actual = SerialFlash.files.at("7.raw");
        drmp3_config config;
        drmp3_uint64 frames = 0;
        int16_t *reference = drmp3_open_memory_and_read_pcm_frames_s16(encoded.data(), encoded.size(), &config, &frames, nullptr);
        assert(reference != nullptr && config.sampleRate == 44100);
        frames = std::min<drmp3_uint64>(frames, SET_AUDIO_MAX_RAW_BYTES / 2);
        assert(actual.size() == frames * 2);
        assert(std::any_of(actual.begin(), actual.end(), [](uint8_t value) { return value != 0; }));
        for (size_t index = 0; index < frames; ++index)
        {
            const int16_t expected = config.channels == 1 ? reference[index] : static_cast<int16_t>((static_cast<int32_t>(reference[index * 2]) + reference[index * 2 + 1]) / 2);
            int16_t sample;
            memcpy(&sample, actual.data() + index * 2, 2);
            assert(sample == expected);
        }
        drmp3_free(reference, nullptr);
        if (strcmp(name, "long") == 0)
        {
            assert(actual.size() == SET_AUDIO_MAX_RAW_BYTES);
        }
    }
    for (const char *name : {"rate48000", "rate22050"})
    {
        reset();
        add("7.mp3", 0)->data = mp3_fixture(name);
        assert(SET_Copy_audio_files_from_SD_to_Flash(changed));
        assert(SerialFlash.files.count("7.raw") == 0);
        verify_fallback();
    }
    for (size_t length : {0u, 1u, 1000u})
    {
        reset();
        add("7.mp3", length);
        assert(SET_Copy_audio_files_from_SD_to_Flash(changed));
        assert(SerialFlash.files.count("7.raw") == 0);
    }
    for (int result : {0, -1})
    {
        reset();
        auto source = add("7.mp3", 0);
        source->data = mp3_fixture("long");
        source->fail_at = 20000;
        source->fail_result = result;
        assert(SET_Copy_audio_files_from_SD_to_Flash(changed));
        assert(SerialFlash.files.count("7.raw") == 0);
    }
    reset();
    add("7.mp3", 0)->data = mp3_fixture("stereo");
    SerialFlash.fail_write_at = 256;
    add("0.raw", 2);
    assert(!SET_Copy_audio_files_from_SD_to_Flash(changed));
    assert(SerialFlash.files.count("7.raw") == 0 && Display_Storage.completed == 0);
    std::puts("PASS: MP3 mono/stereo, CBR/VBR, ID3, no Xing, decoded PCM, 3 MiB cap, unsupported rates, invalid input and Flash failure");
}
