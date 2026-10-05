"""Production filename registry with an in-memory, fault-injecting FRAM for host tests."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
REGISTRY_FIXTURE = r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>
#define DMAMEM
#ifndef FLASHMEM
#define FLASHMEM
#endif
struct LillaFRAM_2x512
{
    static constexpr uint8_t ERROR_0 = 0;
    std::vector<uint8_t> memory = std::vector<uint8_t>(131072, 0xFF);
    int budget = -1;
    uint8_t readArray(uint32_t address, uint32_t count, uint8_t *destination)
    {
        assert(address + count <= memory.size());
        memcpy(destination, memory.data() + address, count);
        return 0;
    }
    uint8_t writeArray(uint32_t address, uint32_t count, uint8_t *source)
    {
        assert(address + count <= memory.size());
        const uint32_t accepted = budget < 0 ? count : std::min(count, static_cast<uint32_t>(budget));
        memcpy(memory.data() + address, source, accepted);
        if (budget >= 0)
        {
            budget -= accepted;
        }
        return accepted == count ? 0 : 2;
    }
} LillaFram;
'''
for path in ("lib/FileNameRegistry/FileNameRegistry.h", "lib/FileNameRegistry/FileNameRegistry.cpp"):
    source = (ROOT / path).read_text(encoding="utf-8")
    REGISTRY_FIXTURE += re.sub(r'^#(?:include "|pragma)[^\n]*\n', '', source, flags=re.M)

REGISTRY_FIXTURE += r'''
void reset_registry()
{
    LillaFram.budget = -1;
    std::fill(LillaFram.memory.begin(), LillaFram.memory.end(), 0xFF);
    FileNameRegistry::Load();
    FileNameRegistry::Clear();
    assert(FileNameRegistry::Save());
}
'''
