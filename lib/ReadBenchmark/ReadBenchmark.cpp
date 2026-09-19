#include "ReadBenchmark.h"

#if defined(LILLA_READ_BENCHMARK)
#include <Arduino.h>
#include <Audio.h>
#include <spi_interrupt.h>
#include <array>
#include <cstdlib>
#include <cstring>
#include "PatchCacheManager.h"
#include "AudioTables.h"

void ReadBenchmark::Run(PatchCacheManager &cache, AudioTables &tables, bool allowed)
{
    static constexpr auto sizes = []()
    {
        std::array<uint16_t, 244> values{};
        size_t index = 0;
        for (uint16_t samples = 10; samples <= 300; samples += 10)
        {
            values[index++] = samples;
        }
        for (uint16_t samples = 315; samples < 500; samples += 15)
        {
            values[index++] = samples;
        }
        values[index++] = 500; // Include the boundary explicitly: the previous step ends at 495.
        for (uint16_t samples = 520; samples <= 4500; samples += 20)
        {
            values[index++] = samples;
        }
        return values;
    }();
    static_assert(sizes.front() == 10 && sizes[29] == 300 && sizes[42] == 495 && sizes[43] == 500 && sizes[44] == 520 && sizes.back() == 4500);
    static constexpr uint8_t reads_per_case = 4;
    static constexpr uint32_t capacity = sizes.back() + 2;
    static constexpr uint32_t source_capacity = sizes.back() + 80; // Padding for four fixed starts, alignment and the final expanded read.
    DMAMEM static uint64_t total_cycles[4][2][244][2];
    alignas(32) static int16_t destination[2][capacity]; // Both rows are 4-byte aligned in RAM1; useful expanded data starts at index 1.
    static volatile uint32_t checksum = 0;
    if (!allowed)
    {
        Serial.println(F("BENCH_ALIGN: unavailable during recording/conversion or metadata replacement"));
        return;
    }
    int16_t *ram_source = static_cast<int16_t *>(malloc(source_capacity * sizeof(int16_t)));
    if (ram_source == nullptr)
    {
        Serial.println(F("BENCH_ALIGN: insufficient RAM2 for source buffer"));
        return;
    }
    Serial.println(F("BENCH_ALIGN: four reads per method/case; exact versus expanded 4-byte-aligned read; audio suspended"));
    Serial.flush();
    const bool audio_enabled = NVIC_IS_ENABLED(IRQ_SOFTWARE) != 0;
    AudioNoInterrupts();
    const AudioFileSource source = cache.Get_source(0);
    const int16_t *table = nullptr;
    for (uint8_t instrument = 0; instrument < INSTRUMENTS; ++instrument)
    {
        const AudioTables::Pointers pointers = tables.Get_active_pointers(instrument);
        if (pointers.wavetable != nullptr)
        {
            table = pointers.wavetable;
            break;
        }
    }
    LillaSerialFlashFile file;
    file.fast_open(0);
    const bool ready = source.storage == Psram && source.psram_ptr != nullptr && source.samples >= source_capacity && file && file.size() / sizeof(int16_t) >= source_capacity && table != nullptr;
    bool read_ok = ready;
    bool data_ok = true;
    if (ready)
    {
        for (uint32_t i = 0; i < source_capacity; ++i)
        {
            ram_source[i] = table[i % WavetableManager::FULL_WAVETABLE_DIM];
        }
        ARM_DEMCR |= ARM_DEMCR_TRCENA;
        ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
        AudioStartUsingSPI();
        for (uint8_t kind = 0; kind < 4 && read_ok && data_ok; ++kind)
        {
            const int16_t *memory = kind == 3 ? ram_source : source.psram_ptr;
            const uint32_t base = kind == 0 ? 0 : ((32u - (reinterpret_cast<uintptr_t>(memory) & 31u)) & 31u) / sizeof(int16_t);
            const uint8_t cache_cases = kind == 0 ? 1 : 2;
            for (uint8_t cache = 0; cache < cache_cases && read_ok && data_ok; ++cache)
            {
                for (size_t c = 0; c < sizes.size() && read_ok && data_ok; ++c)
                {
                    total_cycles[kind][cache][c][0] = 0;
                    total_cycles[kind][cache][c][1] = 0;
                    const uint32_t samples = sizes[c];
                    const uint32_t expanded_samples = (samples + 2u) & ~1u; // Include the preceding sample and round the length to a multiple of four bytes.
                    for (uint8_t reading = 0; reading < reads_per_case && read_ok && data_ok; ++reading)
                    {
                        const uint32_t aligned_first = base + static_cast<uint32_t>(reading) * 16u; // Fixed cache-line-aligned starts independent of sample count.
                        for (uint8_t turn = 0; turn < 2 && read_ok; ++turn)
                        {
                            const uint8_t method = turn ^ (reading & 1u); // Alternate method order to avoid always measuring one first.
                            const uint32_t first = aligned_first + (method == 0 ? 1u : 0u);
                            const uint32_t count = method == 0 ? samples : expanded_samples;
                            const size_t bytes = count * sizeof(int16_t);
                            if (kind != 0)
                            {
                                // Write back dirty data before invalidating: never discard cached PSRAM/RAM writes.
                                // Prepare the same expanded source span before EACH timed method, outside the measurement.
                                arm_dcache_flush_delete(const_cast<int16_t *>(memory + aligned_first), expanded_samples * sizeof(int16_t));
                                if (cache == 1)
                                {
                                    memcpy(destination[method], memory + aligned_first, expanded_samples * sizeof(int16_t));
                                    asm volatile("dsb" ::: "memory"); // Force the untimed prefetch to finish before measuring a warm-source copy.
                                }
                            }
                            asm volatile("dsb" ::: "memory");
                            const uint32_t start = ARM_DWT_CYCCNT;
                            if (kind == 0)
                            {
                                file.seek(first * sizeof(int16_t));
                                if (file.read(destination[method], bytes) != bytes)
                                {
                                    read_ok = false;
                                }
                            }
                            else if (kind == 2)
                            {
                                memset(destination[method], 0, bytes);
                                asm volatile("" ::: "memory");
                                memcpy(destination[method], memory + first, bytes);
                            }
                            else
                            {
                                memcpy(destination[method], memory + first, bytes);
                            }
                            asm volatile("dsb" ::: "memory");
                            total_cycles[kind][cache][c][method] += static_cast<uint32_t>(ARM_DWT_CYCCNT - start);
                        }
                        if (read_ok)
                        {
                            data_ok = memcmp(destination[0], destination[1] + 1, samples * sizeof(int16_t)) == 0; // Compare only the requested samples, outside the timed interval.
                            checksum = checksum + static_cast<uint16_t>(destination[0][0]) + static_cast<uint16_t>(destination[0][samples - 1]);
                        }
                    }
                }
            }
        }
        AudioStopUsingSPI();
    }
    file.close();
    if (audio_enabled)
    {
        AudioInterrupts();
    }
    free(ram_source);
    if (!ready)
    {
        Serial.println(F("BENCH_ALIGN: select a patch with 0.raw cached in PSRAM, >=4580 samples, and an active AudioTables bank; send b to retry"));
        return;
    }
    if (!read_ok || !data_ok)
    {
        Serial.println(read_ok ? F("BENCH_ALIGN: requested samples differ; results discarded") : F("BENCH_ALIGN: short Flash read; results discarded"));
        return;
    }
    const char *names[] = {"FLASH_seek_read", "PSRAM_copy", "PSRAM_zero_copy", "RAM2_copy"};
    const double cycles_per_us = static_cast<double>(F_CPU_ACTUAL) / 1000000.0;
    Serial.println(F("BENCH_ALIGN: exact source address mod 4 = 2; expanded source address mod 4 = 0; both destinations aligned in RAM1"));
    Serial.println(F("BENCH_ALIGN: cache preparation and data comparison excluded; cold/warm refer to source CPU cache; Flash cache=n/a; other IRQs enabled"));
    Serial.println(F("BENCH_ALIGN: expanded useful data begins at destination+1; downstream Player processing and file-edge handling are not measured"));
    Serial.println(F("source,cache,requested_samples,pitch_equivalent,expanded_samples,exact_mean_us,aligned_mean_us,saved_us"));
    for (uint8_t kind = 0; kind < 4; ++kind)
    {
        const uint8_t cache_cases = kind == 0 ? 1 : 2;
        for (uint8_t cache = 0; cache < cache_cases; ++cache)
        {
            for (size_t c = 0; c < sizes.size(); ++c)
            {
                const double exact = static_cast<double>(total_cycles[kind][cache][c][0]) / reads_per_case / cycles_per_us;
                const double aligned = static_cast<double>(total_cycles[kind][cache][c][1]) / reads_per_case / cycles_per_us;
                Serial.printf("%s,%s,%u,%.6f,%u,%.3f,%.3f,%.3f\n", names[kind], kind == 0 ? "n/a" : (cache == 0 ? "cold" : "warm"), static_cast<unsigned int>(sizes[c]), static_cast<double>(sizes[c]) / 128.0, static_cast<unsigned int>((sizes[c] + 2u) & ~1u), exact, aligned, exact - aligned);
            }
        }
    }
    Serial.printf("BENCH_ALIGN: complete; requested samples match; checksum=%lu\n", static_cast<unsigned long>(checksum));
}
#endif
