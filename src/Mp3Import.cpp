#include "Mp3Import.h"
// scripts/mp3_flash.py keeps this translation unit in Flash; favor compact offline decoding.
#pragma GCC optimize ("Os")
#define DR_MP3_NO_STDIO
#define DR_MP3_NO_SIMD
#define DR_MP3_IMPLEMENTATION
#include <dr_mp3.h>
#include <new>

struct Mp3ImportDecoder
{
    drmp3 mp3;
    File *file = nullptr;
    bool read_failed = false;
};

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
    if (mp3->sampleRate != 44100 || (mp3->channels != 1 && mp3->channels != 2))
    {
        Serial.printf("MP3 rejected: %s (%lu Hz, %lu channels); expected 44100 Hz mono/stereo.\n", file.name(), static_cast<unsigned long>(mp3->sampleRate), static_cast<unsigned long>(mp3->channels));
        return false;
    }
    // Count actual decoded frames up to the import cap, including files without a VBR length tag.
    const uint32_t maximum_frames = maximum_bytes / sizeof(int16_t);
    uint32_t frames = 0;
    int16_t buffer[256];
    while (frames < maximum_frames)
    {
        const uint32_t remaining = maximum_frames - frames;
        const uint32_t request = remaining < 128 ? remaining : 128;
        const uint32_t count = drmp3_read_pcm_frames_s16(mp3, request, buffer);
        frames += count;
        if (count != request)
        {
            break;
        }
    }
    bytes = frames * sizeof(int16_t);
    channels = mp3->channels;
    return frames > 0 && !state->read_failed && drmp3_seek_to_pcm_frame(mp3, 0);
}

bool Mp3Import::Read(int16_t *samples, uint32_t frames)
{
    return drmp3_read_pcm_frames_s16(&decoder->mp3, frames, samples) == frames && !decoder->read_failed;
}
