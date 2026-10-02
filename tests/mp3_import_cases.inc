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
    for (uint32_t rate : {8000u, 11025u, 12000u, 16000u, 22050u, 24000u, 32000u, 48000u})
    {
        for (uint32_t channels : {1u, 2u})
        {
            reset();
            const std::string name = "resample_" + std::to_string(rate) + "_" + std::to_string(channels);
            const auto encoded = mp3_fixture(name.c_str());
            add("7.mp3", 0)->data = encoded;
            assert(SET_Copy_audio_files_from_SD_to_Flash(changed));
            const auto &actual = SerialFlash.files.at("7.raw");
            drmp3_config config;
            drmp3_uint64 frames = 0;
            int16_t *decoded = drmp3_open_memory_and_read_pcm_frames_s16(encoded.data(), encoded.size(), &config, &frames, nullptr);
            assert(decoded != nullptr && config.sampleRate == rate);
            assert(actual.size() == ((frames * 44100 + rate - 1) / rate) * 2);
            drmp3_free(decoded, nullptr);
            std::ifstream reference_file(std::string(mp3_test_directory) + "/" + name + ".raw", std::ios::binary);
            const AudioBytes reference{std::istreambuf_iterator<char>(reference_file), std::istreambuf_iterator<char>()};
            assert(reference.size() > 2000);
            double error = 0;
            double energy = 0;
            const size_t end = std::min(actual.size(), reference.size()) / 2 - 512;
            for (size_t index = 512; index < end; ++index)
            {
                int16_t value, expected;
                memcpy(&value, actual.data() + index * 2, 2);
                memcpy(&expected, reference.data() + index * 2, 2);
                error += static_cast<double>(value - expected) * (value - expected);
                energy += static_cast<double>(expected) * expected;
            }
            assert(energy > 0 && error / energy < 0.0001);
            // A different caller block size must produce exactly the same output.
            File source(entries.front());
            Mp3Import importer;
            uint32_t bytes = 0;
            uint16_t output_channels = 0;
            assert(importer.Open(source, bytes, output_channels, SET_AUDIO_MAX_RAW_BYTES));
            assert(output_channels == 1 && bytes == actual.size());
            for (uint32_t offset = 0; offset < bytes;)
            {
                int16_t block[37];
                const uint32_t count = std::min<uint32_t>(37, (bytes - offset) / 2);
                assert(importer.Read(block, count));
                assert(memcmp(block, actual.data() + offset, count * 2) == 0);
                offset += count * 2;
            }
        }
    }
    reset();
    add("7.mp3", 0)->data = mp3_fixture("long48000");
    assert(SET_Copy_audio_files_from_SD_to_Flash(changed));
    assert(SerialFlash.files.at("7.raw").size() == SET_AUDIO_MAX_RAW_BYTES);
    // Measure the filter response independently of the MP3 encoder's own low-pass filter.
    Mp3ImportDecoder filter_state{};
    filter_state.mp3.sampleRate = 48000;
    assert(Mp3_prepare_filter(filter_state));
    for (uint32_t phase = 0; phase < 44100 / filter_state.phase_divisor; ++phase)
    {
        for (double frequency : {1000.0, 23000.0})
        {
            double real = 0;
            double imaginary = 0;
            for (uint32_t tap = 0; tap < MP3_FILTER_TAPS; ++tap)
            {
                const double angle = 2 * 3.14159265358979323846 * frequency * tap / 48000;
                real += filter_state.filter[phase * MP3_FILTER_TAPS + tap] * std::cos(angle);
                imaginary += filter_state.filter[phase * MP3_FILTER_TAPS + tap] * std::sin(angle);
            }
            const double power = real * real + imaginary * imaginary;
            assert(frequency == 1000 ? power > 0.99 && power < 1.01 : power < 0.0001);
        }
    }
    delete[] filter_state.filter;
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
    std::puts("PASS: MP3 mono/stereo, CBR/VBR, ID3, all standard sample rates, FFmpeg reference, block continuity, anti-alias filter, 3 MiB cap and I/O failures");
}
