"""Exercise production routing with every allocation failure combination and missing inputs."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
production = '\n'.join(re.sub(r'^#(?:include|pragma).*$', '', (ROOT / path).read_text(encoding='utf-8'), flags=re.M) for path in ('lib/Router_16x3/Router_16x3.h', 'lib/Router_16x3/Router_16x3.cpp'))
prefix = r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <cstdio>
constexpr int AUDIO_BLOCK_SAMPLES = 128;
struct audio_block_t { alignas(8) int16_t data[128] = {}; bool held = false; };
uint32_t signed_add_16_and_16(uint32_t a, uint32_t b)
{
    int16_t x[2], y[2];
    memcpy(x, &a, 4);
    memcpy(y, &b, 4);
    for (int i = 0; i < 2; ++i)
    {
        x[i] = std::clamp(int(x[i]) + y[i], -32768, 32767);
    }
    memcpy(&a, x, 4);
    return a;
}
class AudioStream
{
public:
    audio_block_t inputs[16], outputs[3];
    audio_block_t *pending[16] = {};
    int allocation_mask = 0, allocations = 0, transmitted = 0;
    int16_t result[3][128] = {};
    AudioStream(int, audio_block_t **) {}
    virtual void update() = 0;
    audio_block_t *allocate()
    {
        const int index = allocations++;
        assert(index < 3 && !outputs[index].held);
        if ((allocation_mask & (1 << index)) == 0)
        {
            return nullptr;
        }
        outputs[index].held = true;
        return &outputs[index];
    }
    audio_block_t *receiveReadOnly(int index)
    {
        auto *block = pending[index];
        pending[index] = nullptr;
        return block;
    }
    void release(audio_block_t *block) { assert(block != nullptr && block->held); block->held = false; }
    void transmit(audio_block_t *block, int channel)
    {
        assert(block != nullptr && block->held);
        ++transmitted;
        memcpy(result[channel], block->data, sizeof(block->data));
    }
};
'''
checks = r'''
int main()
{
    Router_16x3 router;
    for (int output_mask = 0; output_mask < 8; ++output_mask)
    {
        for (int input_mask : {0, 1, 0xaaaa, 0xffff})
        {
            router.allocations = 0;
            router.transmitted = 0;
            router.allocation_mask = output_mask;
            int expected[3] = {};
            for (int in = 0; in < 16; ++in)
            {
                const int sample = (in + 1) * 100;
                router.routing_MX[in] = in % 4;
                router.routing_table[in] = in % 2;
                std::fill_n(router.inputs[in].data, 128, sample);
                router.inputs[in].held = (input_mask & (1 << in)) != 0;
                router.pending[in] = router.inputs[in].held ? &router.inputs[in] : nullptr;
                if (router.inputs[in].held)
                {
                    if (router.routing_MX[in] > 1)
                    {
                        expected[router.routing_table[in]] += sample;
                    }
                    if (router.routing_MX[in] == 1 || router.routing_MX[in] == 3)
                    {
                        expected[2] += sample;
                    }
                }
            }
            router.update();
            assert(router.transmitted == (output_mask == 7 ? 3 : 0));
            for (int in = 0; in < 16; ++in)
            {
                assert(!router.inputs[in].held && router.pending[in] == nullptr);
            }
            for (int out = 0; out < 3; ++out)
            {
                assert(!router.outputs[out].held);
                if (output_mask == 7)
                {
                    for (int value : router.result[out])
                    {
                        assert(value == expected[out]);
                    }
                }
            }
        }
    }
    std::puts("PASS: Router allocation failures, missing inputs, routing sums, release ownership and recovery");
}
'''
compiler = shutil.which('g++') or r'C:\msys64\ucrt64\bin\g++.exe'
environment = os.environ.copy()
environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
with tempfile.TemporaryDirectory(prefix='lilla-router-') as directory:
    cpp = Path(directory) / 'test.cpp'
    exe = Path(directory) / 'test.exe'
    cpp.write_text(prefix + production + checks, encoding='utf-8', newline='\r\n')
    subprocess.run([compiler, '-std=c++20', '-O2', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True, env=environment)
    subprocess.run([str(exe)], check=True, env=environment)
