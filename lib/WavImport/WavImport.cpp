#include "WavImport.h"
#include <cmath>
#include <cstring>
#include <new>

struct WavImport::State
{
    static constexpr uint32_t TAPS = 128, PHASES = 128, RATE = 44100;
    Info info;
    File *file = nullptr;
    float *filter = nullptr;
    float history[256] = {};
    uint8_t buffer[1024];
    uint32_t input_position = 0, output_position = 0, buffered = 0, cursor = 0;
};

FLASHMEM uint32_t WavImport::Little(const uint8_t *bytes, uint32_t count)
{
    uint32_t value = 0;
    for (uint32_t i = 0; i < count; ++i)
    {
        value |= static_cast<uint32_t>(bytes[i]) << (8 * i);
    }
    return value;
}

FLASHMEM bool WavImport::Read_exact(File &file, void *destination, uint32_t bytes)
{
    auto *output = static_cast<uint8_t *>(destination);
    uint32_t total = 0;
    while (total < bytes)
    {
        const int count = file.read(output + total, bytes - total);
        if (count <= 0)
        {
            return false;
        }
        total += count;
    }
    return true;
}

FLASHMEM bool WavImport::Inspect(File &file, Info &info)
{
    info = {};
    uint8_t header[40];
    const uint64_t size = file.size();
    if (size > UINT32_MAX || !file.seek(0) || !Read_exact(file, header, 12) || memcmp(header, "RIFF", 4) != 0 || memcmp(header + 8, "WAVE", 4) != 0)
    {
        return false;
    }
    const uint32_t length = Little(header + 4, 4);
    if (length < 4 || length > UINT32_MAX - 8)
    {
        return false;
    }
    uint32_t end = length + 8;
    const bool corrected = static_cast<uint64_t>(end) == size + 4;
    if (end > size)
    {
        if (!corrected)
        {
            return false;
        }
        end = static_cast<uint32_t>(size);
    }
    bool format_found = false, data_found = false;
    uint32_t position = 12, data_bytes = 0;
    while (position <= end && end - position >= 8)
    {
        if (!file.seek(position) || !Read_exact(file, header, 8))
        {
            return false;
        }
        const uint32_t chunk = Little(header + 4, 4);
        const uint32_t start = position + 8;
        if (chunk > end - start || (chunk & 1u) > end - start - chunk)
        {
            return false;
        }
        if (memcmp(header, "fmt ", 4) == 0)
        {
            if (format_found || chunk < 16 || !Read_exact(file, header, chunk < sizeof(header) ? chunk : sizeof(header)))
            {
                return false;
            }
            info.encoding = Little(header, 2);
            info.channels = Little(header + 2, 2);
            info.rate = Little(header + 4, 4);
            info.alignment = Little(header + 12, 2);
            info.bits = Little(header + 14, 2);
            if (info.encoding == 0xFFFE)
            {
                static constexpr uint8_t suffix[14] = {0, 0, 0, 0, 0x10, 0, 0x80, 0, 0, 0xAA, 0, 0x38, 0x9B, 0x71};
                if (chunk < 40 || Little(header + 16, 2) < 22 || Little(header + 16, 2) > chunk - 18 || memcmp(header + 26, suffix, sizeof(suffix)) != 0)
                {
                    return false;
                }
                info.encoding = Little(header + 24, 2);
                const uint16_t valid_bits = Little(header + 18, 2);
                if (valid_bits == 0 || valid_bits > info.bits || (info.encoding == 3 && valid_bits != info.bits))
                {
                    return false;
                }
            }
            const bool pcm = info.encoding == 1 && (info.bits == 8 || info.bits == 16 || info.bits == 24 || info.bits == 32);
            const bool floating = info.encoding == 3 && (info.bits == 32 || info.bits == 64);
            if ((!pcm && !floating) || (info.channels != 1 && info.channels != 2) || info.rate < 8000 || info.rate > 192000 || info.alignment != info.channels * (info.bits / 8) || Little(header + 8, 4) != info.rate * info.alignment)
            {
                return false;
            }
            format_found = true;
        }
        else if (memcmp(header, "data", 4) == 0)
        {
            if (data_found || chunk == 0)
            {
                return false;
            }
            info.offset = start;
            data_bytes = chunk;
            data_found = true;
        }
        position = start + chunk + (chunk & 1u);
    }
    if (!format_found || !data_found || position != end || data_bytes % info.alignment != 0)
    {
        return false;
    }
    info.frames = data_bytes / info.alignment;
    const uint64_t output_frames = (static_cast<uint64_t>(info.frames) * State::RATE + info.rate - 1) / info.rate;
    if (output_frames > UINT32_MAX / sizeof(int16_t))
    {
        return false;
    }
    info.bytes = output_frames * sizeof(int16_t);
    if (corrected)
    {
        Serial.println("  WAV: RIFF size corrected by -4 bytes for reading; original unchanged");
    }
    return true;
}

FLASHMEM WavImport::~WavImport()
{
    if (state != nullptr)
    {
        delete[] state->filter;
        delete state;
    }
}

FLASHMEM bool WavImport::Prepare_filter()
{
    state->filter = new (std::nothrow) float[(State::PHASES + 1) * State::TAPS];
    if (state->filter == nullptr)
    {
        return false;
    }
    // Interpolate between fixed phases to bound memory even for unusual input sample rates.
    const float cutoff = 0.94f * (state->info.rate > State::RATE ? static_cast<float>(State::RATE) / state->info.rate : 1.0f);
    constexpr float pi = 3.14159265358979323846f;
    for (uint32_t phase = 0; phase <= State::PHASES; ++phase)
    {
        float sum = 0;
        for (uint32_t tap = 0; tap < State::TAPS; ++tap)
        {
            const float distance = static_cast<float>(tap) - 63.0f - static_cast<float>(phase) / State::PHASES;
            const float angle = pi * cutoff * distance;
            const float sinc = std::fabs(angle) < 0.000001f ? cutoff : cutoff * std::sin(angle) / angle;
            const float window = 0.42f + 0.5f * std::cos(pi * distance / 64.0f) + 0.08f * std::cos(2.0f * pi * distance / 64.0f);
            const float value = sinc * window;
            state->filter[phase * State::TAPS + tap] = value;
            sum += value;
        }
        for (uint32_t tap = 0; tap < State::TAPS; ++tap)
        {
            state->filter[phase * State::TAPS + tap] /= sum;
        }
    }
    return true;
}

FLASHMEM bool WavImport::Open(File &file, uint32_t &bytes, uint16_t &channels)
{
    if (state != nullptr)
    {
        delete[] state->filter;
        delete state;
    }
    state = new (std::nothrow) State;
    if (state == nullptr || !Inspect(file, state->info) || !file.seek(state->info.offset))
    {
        return false;
    }
    state->file = &file;
    bytes = state->info.bytes;
    channels = 1;
    return state->info.rate == State::RATE || Prepare_filter();
}

FLASHMEM bool WavImport::Fill_history(uint32_t last)
{
    const Info &info = state->info;
    while (state->input_position <= last && state->input_position < info.frames)
    {
        if (state->cursor == state->buffered)
        {
            const uint32_t remaining = info.frames - state->input_position;
            state->buffered = remaining < 64 ? remaining : 64;
            state->cursor = 0;
            if (!Read_exact(*state->file, state->buffer, state->buffered * info.alignment))
            {
                return false;
            }
        }
        const uint8_t *input = state->buffer + state->cursor++ * info.alignment;
        double mono = 0;
        for (uint16_t channel = 0; channel < info.channels; ++channel)
        {
            const uint8_t *sample = input + channel * (info.bits / 8);
            double value;
            if (info.encoding == 3)
            {
                if (info.bits == 32)
                {
                    const uint32_t encoded = Little(sample, 4);
                    float decoded;
                    memcpy(&decoded, &encoded, sizeof(decoded));
                    value = decoded;
                }
                else
                {
                    const uint64_t encoded = static_cast<uint64_t>(Little(sample, 4)) | (static_cast<uint64_t>(Little(sample + 4, 4)) << 32);
                    memcpy(&value, &encoded, sizeof(value));
                }
                if (!std::isfinite(value))
                {
                    return false;
                }
                value = value < -1.0 ? -32768.0 : value > 1.0 ? 32768.0 : value * 32768.0;
            }
            else if (info.bits == 8)
            {
                value = (static_cast<int>(sample[0]) - 128) * 256;
            }
            else
            {
                const uint32_t encoded = Little(sample, info.bits / 8);
                const int64_t signed_value = (encoded & (1u << (info.bits - 1))) != 0 ? static_cast<int64_t>(encoded) - (int64_t{1} << info.bits) : encoded;
                value = static_cast<double>(signed_value) / (uint32_t{1} << (info.bits - 16));
            }
            mono += value / info.channels;
        }
        state->history[state->input_position++ % 256] = static_cast<float>(mono);
    }
    return true;
}

FLASHMEM bool WavImport::Read(int16_t *samples, uint32_t frames)
{
    for (uint32_t frame = 0; frame < frames; ++frame)
    {
        const uint64_t position = static_cast<uint64_t>(state->output_position++) * state->info.rate;
        const int64_t center = position / State::RATE;
        const uint64_t requested = center + (state->filter == nullptr ? 0 : State::TAPS / 2);
        const uint32_t last = requested < state->info.frames ? static_cast<uint32_t>(requested) : state->info.frames - 1;
        if (!Fill_history(last))
        {
            return false;
        }
        float value = 0;
        if (state->filter == nullptr)
        {
            value = state->history[center % 256];
        }
        else
        {
            const uint32_t fraction = (position % State::RATE) * State::PHASES;
            const uint32_t phase = fraction / State::RATE;
            const float mix = static_cast<float>(fraction % State::RATE) / State::RATE;
            for (uint32_t tap = 0; tap < State::TAPS; ++tap)
            {
                const int64_t source = center + tap - 63;
                if (source >= 0 && static_cast<uint64_t>(source) < state->info.frames)
                {
                    const float first = state->filter[phase * State::TAPS + tap];
                    const float next = state->filter[(phase + 1) * State::TAPS + tap];
                    value += state->history[source % 256] * (first + mix * (next - first));
                }
            }
        }
        value = value < -32768.0f ? -32768.0f : value > 32767.0f ? 32767.0f : value;
        const bool legacy_pcm = state->filter == nullptr && state->info.encoding == 1 && state->info.bits == 16;
        samples[frame] = legacy_pcm ? static_cast<int16_t>(value) : static_cast<int16_t>(std::lround(value));
    }
    return true;
}
