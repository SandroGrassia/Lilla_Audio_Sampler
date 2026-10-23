"""Exercise production WAV conversion, sample-rate changes, failure paths and an optional real WAV."""
from pathlib import Path
import os
import shutil
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[2]
fixture_path = root / 'scripts/tests/user_audio_import_regression.py'
namespace = {'__file__': str(fixture_path), '__name__': 'wav_fixture'}
exec(compile(fixture_path.read_text().split("compiler=shutil.which")[0], str(fixture_path), 'exec'), namespace)
checks = r'''
#include <fstream>
#include <iterator>
#include <limits>

AudioBytes wav_source(uint16_t encoding, uint16_t bits, uint16_t channels, uint32_t rate, const std::vector<double> &mono, bool extensible)
{
    AudioBytes format, data, chunks;
    audio_integer(format, extensible ? 0xFFFE : encoding, 2, false);
    audio_integer(format, channels, 2, false);
    audio_integer(format, rate, 4, false);
    audio_integer(format, rate * channels * bits / 8, 4, false);
    audio_integer(format, channels * bits / 8, 2, false);
    audio_integer(format, bits, 2, false);
    if (extensible)
    {
        audio_integer(format, 22, 2, false);
        audio_integer(format, bits, 2, false);
        audio_integer(format, channels == 1 ? 4 : 3, 4, false);
        audio_integer(format, encoding, 4, false);
        format.insert(format.end(), {0, 0, 0x10, 0, 0x80, 0, 0, 0xAA, 0, 0x38, 0x9B, 0x71});
    }
    for (double sample : mono)
    {
        for (uint16_t channel = 0; channel < channels; ++channel)
        {
            if (encoding == 3 && bits == 64)
            {
                uint64_t value;
                memcpy(&value, &sample, sizeof(value));
                audio_integer(data, value & UINT32_MAX, 4, false);
                audio_integer(data, value >> 32, 4, false);
            }
            else if (encoding == 3)
            {
                const float value = sample;
                uint32_t encoded;
                memcpy(&encoded, &value, sizeof(value));
                audio_integer(data, encoded, 4, false);
            }
            else if (bits == 8)
            {
                data.push_back(static_cast<uint8_t>(std::lround(sample * 128) + 128));
            }
            else
            {
                const int64_t value = std::llround(sample * static_cast<double>(uint64_t{1} << (bits - 1)));
                audio_integer(data, static_cast<uint32_t>(value), bits / 8, false);
            }
        }
    }
    audio_chunk(chunks, "fmt ", format, false);
    audio_chunk(chunks, "data", data, false);
    audio_chunk(chunks, "LIST", {'I', 'N', 'F', 'O'}, false);
    return audio_container(chunks, false);
}

std::vector<int16_t> decode(const AudioBytes &wav, uint32_t block)
{
    reset();
    SD.add("/AUDIO_FILES/test.wav", wav);
    File input = SD.open("/AUDIO_FILES/test.wav");
    WavImport decoder;
    uint32_t bytes = 0;
    uint16_t channels = 0;
    assert(decoder.Open(input, bytes, channels) && channels == 1);
    std::vector<int16_t> output(bytes / 2);
    for (uint32_t frame = 0; frame < output.size();)
    {
        const uint32_t count = std::min(block, static_cast<uint32_t>(output.size()) - frame);
        assert(decoder.Read(output.data() + frame, count));
        frame += count;
    }
    assert(SD.bytes("/AUDIO_FILES/test.wav") == wav);
    return output;
}

void check_formats()
{
    const std::vector<double> levels = {-0.75, -0.5, -0.25, 0, 0.25, 0.5, 0.75};
    for (uint16_t encoding : {1, 3})
    {
        for (uint16_t bits : {8, 16, 24, 32, 64})
        {
            if ((encoding == 1 && bits == 64) || (encoding == 3 && bits < 32))
            {
                continue;
            }
            for (uint16_t channels : {1, 2})
            {
                for (bool extensible : {false, true})
                {
                    auto wav = wav_source(encoding, bits, channels, 44100, levels, extensible);
                    const auto output = decode(wav, 3);
                    assert(output.size() == levels.size());
                    for (size_t i = 0; i < levels.size(); ++i)
                    {
                        assert(output[i] == levels[i] * 32768);
                    }
                    reset();
                    SD.add("/AUDIO_FILES/test.wav", wav);
                    const auto report = run();
                    assert(report.copied == 1 && report.errors == 0 && report.indexed == 1);
                    assert(SD.bytes("/AUDIO_FILES/test.raw").size() == output.size() * 2);
                    assert(memcmp(SD.bytes("/AUDIO_FILES/test.raw").data(), output.data(), output.size() * 2) == 0);
                }
            }
        }
    }
    assert(decode(wav_source(3, 64, 1, 44100, {-2, -1, 1, 2}, false), 1) == std::vector<int16_t>({-32768, -32768, 32767, 32767}));
}

void check_rates()
{
    constexpr double pi = 3.14159265358979323846;
    for (uint32_t rate : {8000, 11025, 12345, 16000, 22050, 32000, 44100, 48000, 88200, 96000, 176400, 192000})
    {
        std::vector<double> tone(rate / 20);
        for (size_t i = 0; i < tone.size(); ++i)
        {
            tone[i] = 0.5 * std::sin(2 * pi * 1000 * i / rate);
        }
        const auto wav = wav_source(1, 24, 2, rate, tone, true);
        const auto output = decode(wav, 257);
        assert(output.size() == (tone.size() * 44100 + rate - 1) / rate);
        assert(output == decode(wav, 1));
        double error = 0;
        for (size_t i = 400; i < output.size() - 400; ++i)
        {
            const double difference = output[i] - 16384 * std::sin(2 * pi * 1000 * i / 44100);
            error += difference * difference;
        }
        assert(std::sqrt(error / (output.size() - 800)) < 8);
        if (rate > 44100)
        {
            for (size_t i = 0; i < tone.size(); ++i)
            {
                tone[i] = 0.5 * std::sin(2 * pi * (rate == 48000 ? 23500 : 30000) * i / rate);
            }
            const auto rejected = decode(wav_source(3, 32, 1, rate, tone, false), 17);
            double energy = 0;
            for (size_t i = 400; i < rejected.size() - 400; ++i)
            {
                energy += static_cast<double>(rejected[i]) * rejected[i];
            }
            assert(std::sqrt(energy / (rejected.size() - 800)) < 50);
        }
    }
}

void check_failures()
{
    for (uint32_t rate : {7999, 192001})
    {
        reset();
        SD.add("/AUDIO_FILES/test.wav", wav_source(1, 16, 1, rate, {0.5, -0.5}, false));
        assert(run().errors == 1 && !SD.exists("/AUDIO_FILES/test.raw"));
    }
    for (double invalid : {std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()})
    {
        reset();
        SD.add("/AUDIO_FILES/test.wav", wav_source(3, 64, 1, 48000, {0.5, invalid}, false));
        assert(run().errors == 1 && !SD.exists("/AUDIO_FILES/test.raw"));
    }
    reset();
    SD.add("/AUDIO_FILES/test.wav", wav_source(1, 32, 2, 96000, std::vector<double>(1000, 0.5), false));
    File input = SD.open("/AUDIO_FILES/test.wav");
    WavImport decoder;
    uint32_t bytes = 0;
    uint16_t channels = 0;
    array_allocation_limit = 1;
    assert(!decoder.Open(input, bytes, channels));
    array_allocation_limit = SIZE_MAX;
    assert(decoder.Open(input, bytes, channels));
    read_budget = 10;
    int16_t sample;
    assert(!decoder.Read(&sample, 1));
    read_budget = -1;
    std::cout << "PASS: PCM/float, extensible WAV, exact levels, stereo, 8-192 kHz, duration, block continuity, antialias, clipping, allocation and I/O failures\n";
}

int main(int argc, char **argv)
{
    check_formats();
    check_rates();
    check_failures();
    if (argc > 1)
    {
        std::ifstream input(argv[1], std::ios::binary);
        assert(input);
        AudioBytes wav((std::istreambuf_iterator<char>(input)), {});
        auto output = decode(wav, 127);
        WavImport::Info info;
        File file = SD.open("/AUDIO_FILES/test.wav");
        assert(WavImport::Inspect(file, info));
        assert(info.encoding == 1 && info.bits == 16 && info.rate == 44100 && info.channels == 2);
        assert(output.size() == info.frames);
        for (size_t i = 0; i < output.size(); ++i)
        {
            int16_t pair[2];
            memcpy(pair, wav.data() + info.offset + 4 * i, sizeof(pair));
            assert(output[i] == (static_cast<int32_t>(pair[0]) + pair[1]) / 2);
        }
        std::cout << "PASS: supplied WAV, complete stereo-to-mono conversion, exact samples and original preserved\n";
    }
}
'''
compiler = shutil.which('g++') or 'C:/msys64/ucrt64/bin/g++.exe'
with tempfile.TemporaryDirectory(prefix='lilla-wav-') as directory:
    cpp = Path(directory) / 'wav.cpp'
    exe = Path(directory) / 'wav.exe'
    cpp.write_text(namespace['fixture'] + checks, encoding='utf-8')
    subprocess.run([compiler, '-std=c++20', '-O2', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    environment = os.environ.copy()
    environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
    subprocess.run([str(exe), *sys.argv[1:]], check=True, env=environment)
