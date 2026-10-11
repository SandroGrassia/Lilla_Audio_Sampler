"""Check production timbre filter response, bypass, transitions and voice reset."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = r"""
#include "PlayerToneFilter.h"
#include <cassert>
#include <cmath>
#include <cstdint>
double measure(PlayerToneFilter &filter, float pitch, uint8_t level, float frequency)
{
    int16_t block[128];
    double input_power = 0.0, output_power = 0.0;
    for (int b = 0; b < 160; ++b)
    {
        for (int i = 0; i < 128; ++i)
        {
            block[i] = static_cast<int16_t>(10000.0 * std::sin(6.28318530718 * frequency * (b * 128 + i) / 44100.0));
            if (b > 30)
            {
                input_power += double(block[i]) * block[i];
            }
        }
        filter.Process(block, 128, pitch, level, 44100.0f);
        if (b > 30)
        {
            for (auto sample : block)
            {
                output_power += double(sample) * sample;
            }
        }
    }
    return std::sqrt(output_power / input_power);
}

int main()
{
    const float pitch = std::exp2(11.0f / 12.0f);
    const float endpoints[] = {20000, 5000, 4500, 4200, 4000, 3800};
    PlayerToneFilter filter;
    for (uint8_t level = 0; level <= 5; ++level)
    {
        assert(std::fabs(PlayerToneFilter::Cutoff(1.0f, level) - 20000.0f) < 1.0f);
        assert(std::fabs(PlayerToneFilter::Cutoff(0.5f, level) - 20000.0f) < 1.0f);
        assert(std::fabs(PlayerToneFilter::Cutoff(pitch, level) - endpoints[level]) < 1.0f);
        assert(std::fabs(PlayerToneFilter::Cutoff(35.0f, level) - endpoints[level]) < 1.0f);
        float previous = 20000;
        for (int step = 1; step <= 110; ++step)
        {
            const float current = PlayerToneFilter::Cutoff(std::exp2(step / 120.0f), level);
            assert(current <= previous && current >= endpoints[level] - 1);
            previous = current;
        }
        assert(std::fabs(PlayerToneFilter::Cutoff(std::sqrt(pitch), level) - std::sqrt(20000 * endpoints[level])) < 1);
        // No reset between levels: a live edit at unchanged pitch must update cached coefficients.
        for (const float frequency : {1000.0f, 4200.0f, 10000.0f})
        {
            const double gain = measure(filter, pitch, level, frequency);
            const double ratio = std::tan(3.14159265359 * frequency / 44100.0) / std::tan(3.14159265359 * endpoints[level] / 44100.0);
            const double expected = level == 0 ? 1.0 : 1.0 / std::sqrt(1.0 + std::pow(ratio, 4.0));
            assert(std::fabs(gain - expected) < 0.003);
        }
        filter.Reset();
        int16_t block[128];
        for (int i = 0; i < 128; ++i)
        {
            block[i] = i * 200 - 12000;
        }
        filter.Process(block, 128, level == 0 ? pitch : 1.0f, level, 44100.0f);
        for (int i = 0; i < 128; ++i)
        {
            assert(block[i] == i * 200 - 12000);
        }
    }
    assert(std::fabs(measure(filter, pitch, 0, 4200) - 1) < 0.00001);
    for (int b = 0; b < 300; ++b)
    {
        int16_t block[128];
        for (auto &sample : block)
        {
            sample = 1000;
        }
        filter.Process(block, 128, b % 2 ? 1.01f : 35.0f, b % 6, 44100.0f);
    }
    filter.Reset();
    int16_t silence[128] = {};
    filter.Process(silence, 128, pitch, 5, 44100.0f);
    for (auto sample : silence)
    {
        assert(sample == 0);
    }
}

""".replace('#include <cstdint>', '#include <cstdint>\n#include <initializer_list>')
player_header = (root / 'lib/AudioPlayer/AudioPlayer.h').read_text(encoding='utf-8')
player_code = (root / 'lib/AudioPlayer/AudioPlayer.cpp').read_text(encoding='utf-8')
fields = re.findall(r'^    uint8_t tone_filter_level(?:_wait)? = 0;', player_header, re.M)
assert len(fields) == 2
setter = re.search(r'^    void Set_tone_filter[^\n]+', player_header, re.M).group(0)
update = re.search(r'^void AudioPlayer::Update_tone_filter[^\n]+\n\{.*?^\}', player_code, re.M | re.S).group(0)
voice = "\nnamespace VoiceTest\n{\nstruct AudioPlayer\n{\n" + "\n".join(fields) + "\n" + setter + r"""
    int local_patch = 1, instrument_id = 2, patch_id_wait = 1, instrument_id_wait = 2;
    bool pending = true;
    bool Has_pending_note() const { return pending; }
    void Update_tone_filter(int patch, int instrument, uint8_t level);
};
""" + update + "\n}\n"
source = source.replace('int main()', voice + 'int main()')
source = source.replace('    const float pitch =', r"""    VoiceTest::AudioPlayer player;
    for (uint8_t level = 0; level <= 5; ++level)
    {
        player.Set_tone_filter(level);
        assert(player.tone_filter_level_wait == level);
        player.Update_tone_filter(1, 2, level);
        assert(player.tone_filter_level == level && player.tone_filter_level_wait == level);
    }
    player.patch_id_wait = 3;
    player.Update_tone_filter(1, 2, 2);
    assert(player.tone_filter_level == 2 && player.tone_filter_level_wait == 5);
    player.Update_tone_filter(3, 2, 3);
    assert(player.tone_filter_level == 2 && player.tone_filter_level_wait == 3);
    player.pending = false;
    player.Update_tone_filter(3, 2, 0);
    assert(player.tone_filter_level_wait == 3);
    player.Update_tone_filter(1, 7, 0);
    assert(player.tone_filter_level == 2);
    const float pitch =""")
compiler = shutil.which('g++') or 'C:/msys64/ucrt64/bin/g++.exe'
environment = os.environ.copy()
environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
with tempfile.TemporaryDirectory() as temp:
    folder = Path(temp)
    test = folder / 'test.cpp'
    test.write_text(source)
    exe = folder / 'test.exe'
    subprocess.run([compiler, '-std=c++20', '-O2', '-I', str(root / 'lib/AudioPlayer'), str(test), str(root / 'lib/AudioPlayer/PlayerToneFilter.cpp'), '-o', str(exe)], check=True, env=environment)
    subprocess.run([str(exe)], check=True, env=environment)
print('All HIFI levels: endpoints, curve, response, bypass, live edits and reset passed.')
