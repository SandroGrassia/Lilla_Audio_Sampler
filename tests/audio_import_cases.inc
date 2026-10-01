// These cases run against the production import in zero_raw_regression.py.
using AudioBytes = std::vector<uint8_t>;

void audio_integer(AudioBytes &bytes, uint32_t value, unsigned int width, bool big_endian)
{
    for (unsigned int index = 0; index < width; ++index)
    {
        const unsigned int shift = 8 * (big_endian ? width - index - 1 : index);
        bytes.push_back(static_cast<uint8_t>(value >> shift));
    }
}

void audio_chunk(AudioBytes &bytes, const char *id, const AudioBytes &payload, bool big_endian)
{
    bytes.insert(bytes.end(), id, id + 4);
    audio_integer(bytes, payload.size(), 4, big_endian);
    bytes.insert(bytes.end(), payload.begin(), payload.end());
    if (payload.size() % 2 != 0)
    {
        bytes.push_back(0);
    }
}

AudioBytes audio_container(const AudioBytes &chunks, bool aiff)
{
    const char *header = aiff ? "FORM" : "RIFF";
    const char *type = aiff ? "AIFF" : "WAVE";
    AudioBytes bytes(header, header + 4);
    audio_integer(bytes, chunks.size() + 4, 4, aiff);
    bytes.insert(bytes.end(), type, type + 4);
    bytes.insert(bytes.end(), chunks.begin(), chunks.end());
    return bytes;
}

AudioBytes audio_samples(uint32_t frames, uint16_t channels, bool big_endian, bool expected_mono = false)
{
    static constexpr int16_t left[] = {-32768, 32767, 32767, -32768, -30001, 30000, 1, -1, 0, 1234};
    static constexpr int16_t right[] = {-32768, 32767, -32768, 32767, -10000, 10001, 0, 0, 0, -1234};
    static constexpr int16_t mixed[] = {-32768, 32767, 0, 0, -20000, 20000, 0, 0, 0, 0};
    AudioBytes bytes;
    bytes.reserve(frames * channels * 2);
    for (uint32_t frame = 0; frame < frames; ++frame)
    {
        const unsigned int index = frame % 10;
        audio_integer(bytes, static_cast<uint16_t>(expected_mono && channels == 2 ? mixed[index] : left[index]), 2, big_endian);
        if (channels == 2 && !expected_mono)
        {
            audio_integer(bytes, static_cast<uint16_t>(right[index]), 2, big_endian);
        }
    }
    return bytes;
}

AudioBytes audio_file(bool aiff, uint32_t frames, uint16_t channels, bool decorated = false, bool reversed = false)
{
    AudioBytes format;
    AudioBytes sound;
    if (aiff)
    {
        audio_integer(format, channels, 2, true);
        audio_integer(format, frames, 4, true);
        audio_integer(format, 16, 2, true);
        const AudioBytes rate = {0x40, 0x0E, 0xAC, 0x44, 0, 0, 0, 0, 0, 0};
        format.insert(format.end(), rate.begin(), rate.end());
        audio_integer(sound, decorated ? 3 : 0, 4, true);
        audio_integer(sound, decorated ? 16 : 0, 4, true);
        if (decorated)
        {
            sound.insert(sound.end(), {0xAA, 0xBB, 0xCC});
        }
    }
    else
    {
        audio_integer(format, 1, 2, false);
        audio_integer(format, channels, 2, false);
        audio_integer(format, 44100, 4, false);
        audio_integer(format, 88200 * channels, 4, false);
        audio_integer(format, 2 * channels, 2, false);
        audio_integer(format, 16, 2, false);
    }
    const auto samples = audio_samples(frames, channels, aiff);
    sound.insert(sound.end(), samples.begin(), samples.end());
    if (aiff && decorated)
    {
        sound.insert(sound.end(), 7, 0xEE);
    }
    AudioBytes chunks;
    if (decorated)
    {
        audio_chunk(chunks, "NAME", {1, 2, 3}, aiff);
    }
    if (reversed)
    {
        audio_chunk(chunks, aiff ? "SSND" : "data", sound, aiff);
        audio_chunk(chunks, aiff ? "COMM" : "fmt ", format, aiff);
    }
    else
    {
        audio_chunk(chunks, aiff ? "COMM" : "fmt ", format, aiff);
        audio_chunk(chunks, aiff ? "SSND" : "data", sound, aiff);
    }
    if (decorated)
    {
        audio_chunk(chunks, "ANNO", {4, 5, 6, 7, 8}, aiff);
    }
    return audio_container(chunks, aiff);
}

void audio_import_rejected(const AudioBytes &bytes)
{
    reset();
    add("invalid.aiff", 0)->data = bytes;
    bool changed = false;
    assert(SET_Copy_audio_files_from_SD_to_Flash(changed) && changed);
    assert(SerialFlash.files.size() == 1 && SerialFlash.files.count("invalid.raw") == 0);
    assert(Display_Storage.invalid == 1 && Display_Storage.reported_files == 0 && Display_Storage.reported_bytes == 0);
    verify_fallback();
}

void test_audio_import()
{
    bool changed = false;
    for (const char *extension : {".aiff", ".AIFF", ".AiF", ".aif", ".wav", ".WAV"})
    {
        const bool aiff = extension[1] != 'w' && extension[1] != 'W';
        for (uint16_t channels : {1, 2})
        {
            for (uint32_t frames : {1u, 63u, 64u, 65u, 127u, 128u, 129u, 513u})
            {
                for (bool decorated : {false, true})
                {
                    for (bool reversed : {false, true})
                    {
                        reset();
                        const std::string name = std::string("Sample.name") + extension;
                        add(name.c_str(), 0)->data = audio_file(aiff, frames, channels, decorated, reversed);
                        assert(SET_Copy_audio_files_from_SD_to_Flash(changed) && changed);
                        assert(SerialFlash.files.at("Sample.name.raw") == audio_samples(frames, channels, false, true));
                        assert(Display_Storage.reported_bytes == frames * 2 && Display_Storage.reported_files == 1 && Display_Storage.invalid == 0);
                    }
                }
            }
        }
    }

    // Verify the output limit before, at and after 3 MiB for all import formats.
    constexpr uint32_t max_bytes = 3 * 1024 * 1024;
    for (int kind : {0, 1, 2})
    {
        for (uint16_t channels : {1, 2})
        {
            if (kind == 0 && channels == 2)
            {
                continue;
            }
            for (uint32_t frames : {max_bytes / 2 - 1, max_bytes / 2, max_bytes / 2 + 129})
            {
                reset();
                const char *name = kind == 0 ? "limit.raw" : kind == 1 ? "limit.wav" : "limit.aiff";
                auto input = add(name, 0);
                input->data = kind == 0 ? audio_samples(frames, channels, false) : audio_file(kind == 2, frames, channels);
                assert(SET_Copy_audio_files_from_SD_to_Flash(changed) && changed);
                const uint32_t saved_frames = std::min(frames, max_bytes / 2);
                assert(SerialFlash.files.at("limit.raw") == audio_samples(saved_frames, channels, false, true));
                assert(Display_Storage.reported_bytes == saved_frames * 2);
                const uint32_t header = kind == 0 ? 0 : kind == 1 ? 44 : 54;
                assert(input->max_read_end == header + saved_frames * channels * 2);
            }
        }
    }

    // Invalid encoding, sample rate, channel count, frame count and chunk bounds.
    const auto valid = audio_file(true, 129, 2);
    for (const auto &change : std::vector<std::pair<size_t, uint8_t>>{{0, 'X'}, {11, 'C'}, {21, 0}, {21, 3}, {27, 24}, {28, 0xC0}, {31, 0x45}, {37, 1}, {22, 0xFF}, {7, 0xFF}, {16, 0xFF}, {19, 17}, {42, 0xFF}, {46, 0xFF}})
    {
        auto invalid = valid;
        invalid[change.first] = change.second;
        audio_import_rejected(invalid);
    }
    audio_import_rejected(audio_file(true, 0, 2));
    for (size_t bytes : {0u, 11u, 20u, 37u, 53u, 100u})
    {
        audio_import_rejected(AudioBytes(valid.begin(), valid.begin() + bytes));
    }
    const AudioBytes comm(valid.begin() + 12, valid.begin() + 38);
    const AudioBytes ssnd(valid.begin() + 38, valid.end());
    audio_import_rejected(audio_container(comm, true));
    audio_import_rejected(audio_container(ssnd, true));
    for (const auto &extra : {comm, ssnd, AudioBytes{0x01}, AudioBytes{'N', 'A', 'M', 'E', 0, 0, 0, 1, 0x42}})
    {
        AudioBytes chunks(valid.begin() + 12, valid.end());
        chunks.insert(chunks.end(), extra.begin(), extra.end());
        audio_import_rejected(audio_container(chunks, true));
    }
    auto partial_frame = valid;
    partial_frame.pop_back();
    partial_frame[45] -= 1;
    partial_frame.push_back(0); // Keep the FORM length and chunk padding valid.
    audio_import_rejected(partial_frame);

    reset();
    add("0.AIFF", 0)->data = valid;
    add("0.wav", 0)->data = audio_file(false, 64, 1);
    add("0.raw", 256);
    add("ignored.aiff.bak", 20);
    add(".aiff", 20);
    add("folder.aiff", 0, true);
    assert(SET_Copy_audio_files_from_SD_to_Flash(changed) && changed);
    assert(SerialFlash.files.size() == 1 && SerialFlash.files.at("0.raw") == audio_samples(129, 2, false, true));
    assert(Display_Storage.duplicates == 2);

    // A payload read error or short Flash write must remove the partial file.
    for (bool write_failure : {false, true})
    {
        reset();
        auto input = add("0.aiff", 0);
        input->data = valid;
        if (write_failure)
        {
            SerialFlash.fail_write_at = 128;
        }
        else
        {
            input->fail_at = 54 + 256;
        }
        assert(!SET_Copy_audio_files_from_SD_to_Flash(changed) && changed);
        assert(SerialFlash.files.count("0.raw") == 0 && Display_Storage.completed == 0);
    }
    std::puts("PASS: AIFF/WAV mono/stereo PCM, byte order, chunk order/padding/offset, malformed input, 3 MiB cap, summary, duplicates and I/O failures");
}
