#include "Mp3Import.h"
// scripts/mp3_flash.py keeps this translation unit in Flash; favor compact offline decoding.
#pragma GCC optimize ("Os")
#define DR_MP3_NO_STDIO
#define DR_MP3_NO_SIMD
#define DR_MP3_IMPLEMENTATION
#include <dr_mp3.h>
#include <new>
#include <cmath>

static constexpr uint32_t MP3_OUTPUT_RATE = 44100;
static constexpr uint32_t MP3_FILTER_TAPS = 64;

struct Mp3ImportDecoder
{
    drmp3 mp3;
    File *file = nullptr;
    bool read_failed = false;
    uint32_t input_frames = 0;
    uint32_t input_position = 0;
    uint32_t output_position = 0;
    uint32_t phase_divisor = 1;
    float *filter = nullptr;
    float history[128] = {};
    int16_t input[256];
    uint32_t buffered_frames = 0;
    uint32_t buffered_position = 0;
};

static bool Mp3_prepare_filter(Mp3ImportDecoder &state)
{
    uint32_t divisor = state.mp3.sampleRate;
    uint32_t remainder = MP3_OUTPUT_RATE;
    while (remainder != 0)
    {
        const uint32_t next = divisor % remainder;
        divisor = remainder;
        remainder = next;
    }
    state.phase_divisor = divisor;
    const uint32_t phases = MP3_OUTPUT_RATE / divisor;
    state.filter = new (std::nothrow) float[phases * MP3_FILTER_TAPS];
    if (state.filter == nullptr)
    {
        return false;
    }
    // Blackman-windowed sinc: leave a transition band below the lower Nyquist frequency.
    const float cutoff = 0.94f * (state.mp3.sampleRate > MP3_OUTPUT_RATE ? static_cast<float>(MP3_OUTPUT_RATE) / state.mp3.sampleRate : 1.0f);
    constexpr float pi = 3.14159265358979323846f;
    for (uint32_t phase = 0; phase < phases; ++phase)
    {
        float sum = 0;
        for (uint32_t tap = 0; tap < MP3_FILTER_TAPS; ++tap)
        {
            const float distance = static_cast<float>(tap) - 31.0f - static_cast<float>(phase) / phases;
            const float angle = pi * cutoff * distance;
            const float sinc = std::fabs(angle) < 0.000001f ? cutoff : cutoff * std::sin(angle) / angle;
            const float window = 0.42f + 0.5f * std::cos(pi * distance / 32.0f) + 0.08f * std::cos(2.0f * pi * distance / 32.0f);
            const float coefficient = sinc * window;
            state.filter[phase * MP3_FILTER_TAPS + tap] = coefficient;
            sum += coefficient;
        }
        for (uint32_t tap = 0; tap < MP3_FILTER_TAPS; ++tap)
        {
            state.filter[phase * MP3_FILTER_TAPS + tap] /= sum;
        }
    }
    return true;
}

static bool Mp3_fill_history(Mp3ImportDecoder &state, uint32_t last_frame)
{
    while (state.input_position <= last_frame && state.input_position < state.input_frames)
    {
        if (state.buffered_position == state.buffered_frames)
        {
            const uint32_t remaining = state.input_frames - state.input_position;
            state.buffered_frames = remaining < 128 ? remaining : 128;
            state.buffered_position = 0;
            if (drmp3_read_pcm_frames_s16(&state.mp3, state.buffered_frames, state.input) != state.buffered_frames || state.read_failed)
            {
                return false;
            }
        }
        const uint32_t index = state.buffered_position++ * state.mp3.channels;
        const float mono = state.mp3.channels == 1 ? state.input[index] : (static_cast<float>(state.input[index]) + state.input[index + 1]) * 0.5f;
        state.history[state.input_position++ % 128] = mono;
    }
    return true;
}

static size_t Mp3_read(void *context, void *buffer, size_t bytes)
{
    Mp3ImportDecoder &state = *static_cast<Mp3ImportDecoder *>(context);
    size_t total = 0;
    while (total < bytes)
    {
        const int count = state.file->read(static_cast<uint8_t *>(buffer) + total, bytes - total);
        if (count <= 0)
        {
            state.read_failed = state.read_failed || count < 0 || state.file->position() < state.file->size();
            break;
        }
        total += static_cast<size_t>(count);
    }
    return total;
}

static drmp3_bool32 Mp3_seek(void *context, int offset, drmp3_seek_origin origin)
{
    File &file = *static_cast<Mp3ImportDecoder *>(context)->file;
    const int64_t base = origin == DRMP3_SEEK_SET ? 0 : origin == DRMP3_SEEK_CUR ? file.position() : file.size();
    const int64_t target = base + offset;
    return target >= 0 && static_cast<uint64_t>(target) <= file.size() && file.seek(static_cast<uint32_t>(target));
}

static drmp3_bool32 Mp3_tell(void *context, drmp3_int64 *position)
{
    *position = static_cast<Mp3ImportDecoder *>(context)->file->position();
    return DRMP3_TRUE;
}

Mp3Import::~Mp3Import()
{
    if (decoder != nullptr)
    {
        drmp3_uninit(&decoder->mp3);
        delete[] decoder->filter;
        delete decoder;
    }
}

bool Mp3Import::Open(File &file, uint32_t &bytes, uint16_t &channels, uint32_t maximum_bytes)
{
    Mp3ImportDecoder *state = new (std::nothrow) Mp3ImportDecoder;
    if (state == nullptr)
    {
        return false;
    }
    state->file = &file;
    drmp3 *mp3 = &state->mp3;
    if (!file.seek(0) || !drmp3_init(mp3, Mp3_read, Mp3_seek, Mp3_tell, nullptr, state, nullptr))
    {
        delete state;
        return false;
    }
    decoder = state;
    if (mp3->sampleRate == 0 || (mp3->channels != 1 && mp3->channels != 2))
    {
        return false;
    }
    // Count actual decoded frames up to the import cap, including files without a VBR length tag.
    const uint32_t maximum_frames = maximum_bytes / sizeof(int16_t);
    const bool resample = mp3->sampleRate != MP3_OUTPUT_RATE;
    const uint32_t input_limit = resample ? static_cast<uint32_t>((static_cast<uint64_t>(maximum_frames) * mp3->sampleRate + MP3_OUTPUT_RATE - 1) / MP3_OUTPUT_RATE) + MP3_FILTER_TAPS / 2 : maximum_frames;
    uint32_t frames = 0;
    int16_t buffer[256];
    while (frames < input_limit)
    {
        const uint32_t remaining = input_limit - frames;
        const uint32_t request = remaining < 128 ? remaining : 128;
        const uint32_t count = drmp3_read_pcm_frames_s16(mp3, request, buffer);
        frames += count;
        if (count != request)
        {
            break;
        }
    }
    state->input_frames = frames;
    const uint64_t output_frames = (static_cast<uint64_t>(frames) * MP3_OUTPUT_RATE + mp3->sampleRate - 1) / mp3->sampleRate;
    bytes = static_cast<uint32_t>(output_frames < maximum_frames ? output_frames : maximum_frames) * sizeof(int16_t);
    channels = resample ? 1 : mp3->channels;
    if (frames == 0 || state->read_failed || !drmp3_seek_to_pcm_frame(mp3, 0))
    {
        return false;
    }
    return !resample || Mp3_prepare_filter(*state);
}

bool Mp3Import::Read(int16_t *samples, uint32_t frames)
{
    if (decoder->filter == nullptr)
    {
        return drmp3_read_pcm_frames_s16(&decoder->mp3, frames, samples) == frames && !decoder->read_failed;
    }
    for (uint32_t frame = 0; frame < frames; ++frame)
    {
        // Integer phase accumulation preserves pitch and duration across every read block.
        const uint64_t position = static_cast<uint64_t>(decoder->output_position++) * decoder->mp3.sampleRate;
        const int32_t center = position / MP3_OUTPUT_RATE;
        const uint32_t phase = (position % MP3_OUTPUT_RATE) / decoder->phase_divisor;
        if (!Mp3_fill_history(*decoder, center + MP3_FILTER_TAPS / 2))
        {
            return false;
        }
        float value = 0;
        for (uint32_t tap = 0; tap < MP3_FILTER_TAPS; ++tap)
        {
            const int32_t source = center + static_cast<int32_t>(tap) - 31;
            if (source >= 0 && static_cast<uint32_t>(source) < decoder->input_frames)
            {
                value += decoder->history[source % 128] * decoder->filter[phase * MP3_FILTER_TAPS + tap];
            }
        }
        value = value < -32768.0f ? -32768.0f : value > 32767.0f ? 32767.0f : value;
        samples[frame] = static_cast<int16_t>(std::lround(value));
    }
    return true;
}
