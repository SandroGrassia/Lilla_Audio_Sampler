/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.com
*/

#pragma once

#if defined(LILLA_READ_BENCHMARK)
class PatchCacheManager;
class AudioTables;

namespace ReadBenchmark
{
// Call from the control loop only; allowed excludes recording and metadata replacement.
void Run(PatchCacheManager &cache, AudioTables &tables, bool allowed);
}
#endif
