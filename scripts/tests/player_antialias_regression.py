"""Exercise the production source-domain IIR: alias rejection, DC, block/seek continuity and bounds."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
CODE = r"""
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>
#include "PlayerAntialias.h"

constexpr double Pi = 3.14159265358979323846;

std::vector<float> render(const std::vector<int16_t> &source, float start, float pitch, int outputs, bool local)
{
    PlayerAntialias::Settings filter;
    PlayerAntialias::Configure(filter, pitch);
    const int first = local ? static_cast<int>(floorf(start)) - filter.history : 0;
    const int count = static_cast<int>(floorf(start + (outputs - 1) * pitch)) - first + 2;
    assert(first >= 0 && first + count <= static_cast<int>(source.size()));
    if (local && outputs <= 128)
    {
        assert(count <= PlayerAntialias::Basket_capacity);
    }
    std::vector<float> result(outputs + 2, 1234567.0f);
    PlayerAntialias::Render(source.data() + first, count, start - first, pitch, outputs, filter, result.data() + 1);
    assert(result.front() == 1234567.0f && result.back() == 1234567.0f);
    return std::vector<float>(result.begin() + 1, result.end() - 1);
}

std::vector<double> reference_filter(const std::vector<int16_t> &source, const PlayerAntialias::Settings &settings)
{
    double x1[2] = {}, x2[2] = {}, y1[2] = {}, y2[2] = {};
    std::vector<double> result(source.size());
    for (int i = 0; i < static_cast<int>(source.size()); ++i)
    {
        double value = source[i];
        for (int section = 0; section < 2; ++section)
        {
            const auto &c = settings.sections[section];
            const double output = c.b0 * (value + 2 * x1[section] + x2[section]) - c.a1 * y1[section] - c.a2 * y2[section];
            x2[section] = x1[section];
            x1[section] = value;
            y2[section] = y1[section];
            y1[section] = output;
            value = output;
        }
        result[i] = source[i] + settings.mix * (value - source[i]);
    }
    return result;
}

double reference_sample(const std::vector<double> &filtered, float start, float pitch, int output, int history)
{
    const int first = static_cast<int>(floorf(start)) - history;
    const float position = (start - first) + output * pitch;
    const int index = static_cast<int>(position);
    return filtered[first + index] + (position - index) * (filtered[first + index + 1] - filtered[first + index]);
}

double amplitude(const std::vector<float> &samples, double frequency)
{
    double re = 0, im = 0;
    for (int i = 0; i < static_cast<int>(samples.size()); ++i)
    {
        const double angle = 2 * Pi * frequency * i;
        re += samples[i] * std::cos(angle);
        im += samples[i] * std::sin(angle);
    }
    return 2 * std::hypot(re, im) / samples.size();
}

int main()
{
    PlayerAntialias::Settings bypass;
    PlayerAntialias::Configure(bypass, 1);
    assert(bypass.mix == 0 && bypass.history == 0);
    PlayerAntialias::Configure(bypass, 0.75f);
    assert(bypass.mix == 0);
    for (const float pitch : {1.001f, 1.125f, 1.25f, 1.5f, 2.0f, 4.0f, 8.0f, 16.0f, 35.0f})
    {
        PlayerAntialias::Settings filter;
        PlayerAntialias::Configure(filter, pitch);
        assert(filter.mix > 0 && (pitch < 1.25f || filter.mix == 1));
        const auto cached = filter;
        PlayerAntialias::Configure(filter, pitch);
        assert(cached.sections[0].b0 == filter.sections[0].b0);
        std::vector<int16_t> source(90000, 12000);
        const auto dc = render(source, 1024.25f, pitch, 128, true);
        for (const float value : dc)
        {
            assert(std::abs(value - 12000) < 2);
        }
        uint32_t random = 123456;
        for (int i = 0; i < static_cast<int>(source.size()); ++i)
        {
            random = random * 1664525u + 1013904223u;
            source[i] = static_cast<int16_t>(static_cast<int>(random >> 16) - 32768);
        }
        const float start = 2048.25f;
        const auto reference = reference_filter(source, filter);
        double maximum_error = 0;
        for (int block = 0; block < 8; ++block)
        {
            const auto local = render(source, start + block * 128 * pitch, pitch, 128, true);
            for (int i = 0; i < 128; ++i)
            {
                maximum_error = std::max(maximum_error, std::abs(local[i] - reference_sample(reference, start + block * 128 * pitch, pitch, i, filter.history)));
            }
        }
        assert(maximum_error < 2); // Compare to an independent double-precision, continuously running direct-form-I filter.
        // Independent seeks/crossfade segments are reconstructed from their own preceding source samples.
        for (const float seek : {10000.5f, 5000.75f, 30000.25f})
        {
            const auto local = render(source, seek, pitch, 32, true);
            for (int i = 0; i < 32; ++i)
            {
                assert(std::abs(reference_sample(reference, seek, pitch, i, filter.history) - local[i]) < 2);
            }
        }
        if (pitch < 1.5f)
        {
            continue;
        }
        for (const bool alias : {false, true})
        {
            const double output_frequency = alias ? 0.6875 : 0.0625;
            const double frequency = output_frequency / pitch;
            for (int i = 0; i < static_cast<int>(source.size()); ++i)
            {
                source[i] = static_cast<int16_t>(std::lround(12000 * std::sin(2 * Pi * frequency * i)));
            }
            const auto wave = render(source, 1024, pitch, 2048, false);
            const double gain = amplitude(wave, alias ? 1 - output_frequency : output_frequency) / 12000;
            if (alias)
            {
                assert(gain < 0.15);
                std::cout << "Alias rejection at " << pitch << "x: " << -20 * std::log10(gain) << " dB\n";
            }
            else
            {
                assert(gain > 0.96 && gain < 1.01);
            }
        }
    }
    const uint8_t enabled = 50 | PlayerAntialias::Enable_flag;
    assert(!PlayerAntialias::Enabled(0, 1.5f));
    assert(!PlayerAntialias::Enabled(50, 1.5f));
    assert(!PlayerAntialias::Enabled(PlayerAntialias::Enable_flag, 1.5f));
    for (float base : {1.9f, 2.0f, 2.1f})
    {
        for (float bend : {0.75f, 1.0f, 1.5f})
        {
            const bool active = PlayerAntialias::Enabled(enabled, base);
            PlayerAntialias::Settings filter;
            PlayerAntialias::Configure(filter, active ? base * bend : 1.0f);
            assert(active == (base <= 2.0f));
            assert((filter.mix > 0.0f) == active);
            if (active)
            {
                assert(filter.pitch == base * bend);
            }
        }
    }
    std::cout << "PASS: source IIR, alias rejection and base-pitch eligibility independent of bend\n";
}
"""
compiler = shutil.which("g++") or r"C:\msys64\ucrt64\bin\g++.exe"
with tempfile.TemporaryDirectory(prefix="player_antialias_") as folder:
    cpp = Path(folder) / "test.cpp"
    exe = Path(folder) / "test.exe"
    cpp.write_text(CODE, encoding="utf-8", newline="\r\n")
    subprocess.run([compiler, "-std=c++20", "-O2", "-Wall", "-Wextra", "-Werror", "-I", str(ROOT / "lib/AudioPlayer"), str(cpp), str(ROOT / "lib/AudioPlayer/PlayerAntialias.cpp"), "-o", str(exe)], check=True)
    environment = os.environ.copy()
    environment["PATH"] = str(Path(compiler).parent) + os.pathsep + environment.get("PATH", "")
    subprocess.run([str(exe)], check=True, env=environment)
