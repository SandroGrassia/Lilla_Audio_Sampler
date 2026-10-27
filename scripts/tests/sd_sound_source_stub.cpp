#include "sd_sound_source_stub.h"

bool SdSoundSource::Is_sd(int file)
{
    return file >= FIRST && file <= PREVIEW;
}

uint32_t SdSoundSource::Samples(int)
{
    return available ? 4 : 0;
}

bool SdSoundSource::Prepare(int)
{
    return available;
}

bool SdSoundSource::Load(int, int16_t *destination)
{
    if (!available)
    {
        return false;
    }
    for (int i = 0; i < 4; ++i)
    {
        destination[i] = 100 + i;
    }
    return true;
}

bool SdSoundSource::Read(int, int16_t *, int, int)
{
    return false;
}

void SdSoundSource::Bind(int, const int16_t *data)
{
    bound = data;
}

void SdSoundSource::Unbind_all()
{
    bound = nullptr;
}

const char *SdSoundSource::Name(int)
{
    return "SD AUDIO UNAVAILABLE";
}
