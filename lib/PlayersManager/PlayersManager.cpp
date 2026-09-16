#include "PlayersManager.h"
#include <algorithm>

namespace
{
/*
// Calibration: user-supplied BENCH_ALIGN capture, four individually timed reads per case at 600 MHz.
// Use exact_mean_us: exact requested range, source address mod 4 = 2, aligned RAM1 destination.
// Flash: seek+read (CPU source-cache state not applicable); PSRAM: memset+memcpy, cold source cache.
// RAM: tiled AudioTables RAM2 source, memcpy with cold source cache. No expanded/aligned-read results used.
// Cache preparation and data comparison are excluded, as are interpolation, bounds checks and other audio work.
// Retain a cumulative maximum per column so local measurement dips cannot make larger requests cheaper.
// These are four-read mean estimates, NOT worst-case guarantees; cold-cache preparation is only for the benchmark.
// Recalibrate after changing clock, reader, alignment, memory placement or cache/access pattern.
// Columns: samples, Flash_us, PSRAM_us, RAM2_us. Archived monotonic table; not compiled.
read_times = {
    {10, 6.040f, 0.938f, 0.149f},
    {20, 9.865f, 1.725f, 0.230f},
    {30, 13.766f, 2.070f, 0.297f},
    {40, 17.566f, 2.820f, 0.342f},
    {50, 21.398f, 3.464f, 0.409f},
    {60, 25.305f, 3.931f, 0.473f},
    {70, 29.135f, 4.600f, 0.555f},
    {80, 33.187f, 5.364f, 0.614f},
    {90, 36.775f, 5.702f, 0.660f},
    {100, 40.655f, 6.478f, 0.735f},
    {110, 44.845f, 6.805f, 0.787f},
    {120, 48.248f, 7.584f, 0.860f},
    {130, 52.030f, 8.224f, 0.914f},
    {140, 56.144f, 8.690f, 0.971f},
    {150, 59.742f, 9.350f, 1.059f},
    {160, 63.553f, 10.127f, 1.111f},
    {170, 67.628f, 10.432f, 1.150f},
    {180, 71.429f, 11.233f, 1.241f},
    {190, 75.115f, 11.559f, 1.304f},
    {200, 78.936f, 12.512f, 1.350f},
    {210, 82.752f, 13.001f, 1.420f},
    {220, 86.830f, 13.443f, 1.482f},
    {230, 90.603f, 14.105f, 1.555f},
    {240, 94.338f, 14.873f, 1.620f},
    {250, 98.138f, 15.206f, 1.662f},
    {260, 102.377f, 15.985f, 1.733f},
    {270, 105.871f, 16.312f, 1.785f},
    {280, 109.588f, 17.090f, 1.862f},
    {290, 115.428f, 17.737f, 1.913f},
    {300, 117.904f, 18.195f, 1.972f},
    {315, 123.528f, 20.865f, 2.081f},
    {330, 131.067f, 20.865f, 2.151f},
    {345, 134.742f, 20.942f, 2.278f},
    {360, 140.736f, 21.845f, 2.352f},
    {375, 146.655f, 22.703f, 2.479f},
    {390, 152.398f, 23.623f, 2.559f},
    {405, 159.559f, 24.585f, 2.660f},
    {420, 163.547f, 25.502f, 2.735f},
    {435, 169.615f, 26.345f, 2.842f},
    {450, 175.005f, 27.250f, 2.915f},
    {465, 181.140f, 28.232f, 3.040f},
    {480, 186.568f, 29.158f, 3.113f},
    {495, 192.778f, 29.765f, 3.203f},
    {500, 194.038f, 30.281f, 3.243f},
    {520, 204.072f, 31.365f, 3.350f},
    {540, 210.580f, 32.478f, 3.481f},
    {560, 217.384f, 33.910f, 3.624f},
    {580, 225.657f, 35.016f, 3.732f},
    {600, 234.949f, 36.119f, 3.863f},
    {620, 240.417f, 37.215f, 3.973f},
    {640, 250.161f, 38.659f, 4.123f},
    {660, 255.913f, 39.756f, 4.246f},
    {680, 263.520f, 40.877f, 4.350f},
    {700, 271.347f, 41.979f, 4.482f},
    {720, 280.931f, 43.412f, 4.624f},
    {740, 287.294f, 44.515f, 4.734f},
    {760, 296.295f, 45.625f, 4.863f},
    {780, 303.051f, 46.725f, 4.969f},
    {800, 311.217f, 48.170f, 5.113f},
    {820, 317.303f, 49.377f, 5.243f},
    {840, 325.138f, 50.381f, 5.350f},
    {860, 334.461f, 51.485f, 5.700f},
    {880, 342.819f, 52.932f, 5.700f},
    {900, 350.618f, 54.032f, 5.734f},
    {920, 358.408f, 55.141f, 5.866f},
    {940, 365.548f, 56.248f, 5.969f},
    {960, 372.937f, 57.691f, 6.114f},
    {980, 379.086f, 58.918f, 6.243f},
    {1000, 386.388f, 59.886f, 6.349f},
    {1020, 394.830f, 60.996f, 6.483f},
    {1040, 405.589f, 62.532f, 6.629f},
    {1060, 410.724f, 63.550f, 6.733f},
    {1080, 417.370f, 64.642f, 6.863f},
    {1100, 426.640f, 65.989f, 6.972f},
    {1120, 432.533f, 67.195f, 7.115f},
    {1140, 442.971f, 70.204f, 7.246f},
    {1160, 448.841f, 70.204f, 7.351f},
    {1180, 457.357f, 70.496f, 7.482f},
    {1200, 463.893f, 71.973f, 7.628f},
    {1220, 472.563f, 73.128f, 7.735f},
    {1240, 481.048f, 74.170f, 7.863f},
    {1260, 487.337f, 75.248f, 7.969f},
    {1280, 496.687f, 76.715f, 8.119f},
    {1300, 503.489f, 77.797f, 8.244f},
    {1320, 511.177f, 79.002f, 8.349f},
    {1340, 519.800f, 80.020f, 8.488f},
    {1360, 527.447f, 81.557f, 8.845f},
    {1380, 534.778f, 82.558f, 8.845f},
    {1400, 541.882f, 85.301f, 8.865f},
    {1420, 549.712f, 85.301f, 8.972f},
    {1440, 556.948f, 86.230f, 9.115f},
    {1460, 565.762f, 87.310f, 9.242f},
    {1480, 572.703f, 88.429f, 9.351f},
    {1500, 580.505f, 89.525f, 9.481f},
    {1520, 589.427f, 90.975f, 9.629f},
    {1540, 595.090f, 92.073f, 9.740f},
    {1560, 603.959f, 93.270f, 9.863f},
    {1580, 610.551f, 94.294f, 9.971f},
    {1600, 619.820f, 95.726f, 10.120f},
    {1620, 626.253f, 96.814f, 10.243f},
    {1640, 634.626f, 97.930f, 10.349f},
    {1660, 641.373f, 100.650f, 10.483f},
    {1680, 652.263f, 100.650f, 10.636f},
    {1700, 656.307f, 101.674f, 10.737f},
    {1720, 663.314f, 102.684f, 10.863f},
    {1740, 671.316f, 105.749f, 10.970f},
    {1760, 680.210f, 105.749f, 11.115f},
    {1780, 689.238f, 106.329f, 11.243f},
    {1800, 698.043f, 109.246f, 11.350f},
    {1820, 706.288f, 109.246f, 11.483f},
    {1840, 714.636f, 110.141f, 11.624f},
    {1860, 720.932f, 112.711f, 11.960f},
    {1880, 728.690f, 112.711f, 11.960f},
    {1900, 734.288f, 113.301f, 11.971f},
    {1920, 740.510f, 114.742f, 12.114f},
    {1940, 747.577f, 115.839f, 12.243f},
    {1960, 756.459f, 118.748f, 12.349f},
    {1980, 764.102f, 118.748f, 12.482f},
    {2000, 774.711f, 119.493f, 12.843f},
    {2020, 785.433f, 120.602f, 12.843f},
    {2040, 788.858f, 121.897f, 12.899f},
    {2060, 793.181f, 124.380f, 12.969f},
    {2080, 803.249f, 124.380f, 13.113f},
    {2100, 816.244f, 125.494f, 13.256f},
    {2120, 816.244f, 128.148f, 13.349f},
    {2140, 824.877f, 128.148f, 13.487f},
    {2160, 839.115f, 130.670f, 13.628f},
    {2180, 839.132f, 130.670f, 13.733f},
    {2200, 850.203f, 131.397f, 13.861f},
    {2220, 860.234f, 132.320f, 13.971f},
    {2240, 863.361f, 133.884f, 14.120f},
    {2260, 877.612f, 134.851f, 14.246f},
    {2280, 879.845f, 135.979f, 14.352f},
    {2300, 891.252f, 137.065f, 14.515f},
    {2320, 895.176f, 140.159f, 14.625f},
    {2340, 906.773f, 140.159f, 14.733f},
    {2360, 910.019f, 140.731f, 14.862f},
    {2380, 921.950f, 142.042f, 14.977f},
    {2400, 928.151f, 145.174f, 15.116f},
    {2420, 932.708f, 145.174f, 15.258f},
    {2440, 944.798f, 145.489f, 15.349f},
    {2460, 948.096f, 146.592f, 15.482f},
    {2480, 960.804f, 149.697f, 15.844f},
    {2500, 967.407f, 149.697f, 15.844f},
    {2520, 975.355f, 150.247f, 15.894f},
    {2540, 980.058f, 152.968f, 15.970f},
    {2560, 990.528f, 152.968f, 16.116f},
    {2580, 998.514f, 153.973f, 16.257f},
    {2600, 1006.421f, 155.075f, 18.093f},
    {2620, 1013.180f, 156.092f, 18.093f},
    {2640, 1020.263f, 157.523f, 18.093f},
    {2660, 1024.629f, 158.623f, 18.093f},
    {2680, 1035.736f, 159.743f, 18.093f},
    {2700, 1043.965f, 160.833f, 18.093f},
    {2720, 1052.324f, 162.277f, 18.093f},
    {2740, 1059.076f, 163.581f, 18.093f},
    {2760, 1067.633f, 164.580f, 18.093f},
    {2780, 1076.064f, 167.363f, 18.093f},
    {2800, 1082.291f, 167.363f, 18.093f},
    {2820, 1091.223f, 169.926f, 18.093f},
    {2840, 1096.737f, 170.997f, 18.093f},
    {2860, 1107.362f, 170.997f, 18.093f},
    {2880, 1111.886f, 171.806f, 18.118f},
    {2900, 1120.446f, 172.887f, 18.249f},
    {2920, 1126.125f, 175.808f, 18.394f},
    {2940, 1135.277f, 175.808f, 18.482f},
    {2960, 1145.632f, 178.146f, 18.623f},
    {2980, 1151.746f, 178.146f, 18.741f},
    {3000, 1160.732f, 178.757f, 18.863f},
    {3020, 1164.818f, 181.808f, 18.969f},
    {3040, 1175.051f, 181.808f, 19.113f},
    {3060, 1183.405f, 182.423f, 19.242f},
    {3080, 1188.764f, 183.613f, 19.357f},
    {3100, 1196.569f, 186.412f, 19.514f},
    {3120, 1207.923f, 186.412f, 19.625f},
    {3140, 1210.776f, 187.398f, 19.733f},
    {3160, 1221.580f, 188.281f, 21.604f},
    {3180, 1226.605f, 189.412f, 21.604f},
    {3200, 1238.976f, 192.602f, 21.604f},
    {3220, 1241.902f, 192.602f, 21.604f},
    {3240, 1254.071f, 193.205f, 21.604f},
    {3260, 1256.682f, 194.149f, 21.604f},
    {3280, 1267.102f, 195.709f, 21.604f},
    {3300, 1271.657f, 198.420f, 21.604f},
    {3320, 1282.553f, 198.420f, 21.604f},
    {3340, 1289.666f, 200.800f, 21.604f},
    {3360, 1297.104f, 200.800f, 21.604f},
    {3380, 1307.469f, 201.541f, 21.604f},
    {3400, 1315.703f, 202.727f, 21.604f},
    {3420, 1319.055f, 203.645f, 21.604f},
    {3440, 1328.550f, 205.180f, 21.642f},
    {3460, 1337.526f, 206.178f, 21.733f},
    {3480, 1343.838f, 209.116f, 21.866f},
    {3500, 1349.255f, 209.116f, 21.973f},
    {3520, 1359.881f, 211.409f, 22.115f},
    {3540, 1368.428f, 212.759f, 22.245f},
    {3560, 1373.185f, 212.759f, 22.356f},
    {3580, 1383.325f, 214.857f, 22.481f},
    {3600, 1390.836f, 216.758f, 22.628f},
    {3620, 1398.974f, 216.758f, 22.734f},
    {3640, 1404.899f, 218.400f, 22.863f},
    {3660, 1412.966f, 218.400f, 22.984f},
    {3680, 1423.483f, 219.348f, 23.115f},
    {3700, 1427.634f, 220.643f, 23.242f},
    {3720, 1437.493f, 221.548f, 23.355f},
    {3740, 1443.949f, 222.822f, 23.482f},
    {3760, 1451.787f, 224.288f, 23.629f},
    {3780, 1460.643f, 226.971f, 23.735f},
    {3800, 1467.558f, 227.996f, 23.860f},
    {3820, 1473.785f, 227.996f, 23.970f},
    {3840, 1482.966f, 228.856f, 24.120f},
    {3860, 1492.497f, 229.950f, 24.252f},
    {3880, 1499.250f, 231.267f, 24.378f},
    {3900, 1505.471f, 232.321f, 24.482f},
    {3920, 1513.742f, 233.746f, 24.633f},
    {3940, 1520.622f, 234.916f, 24.735f},
    {3960, 1527.898f, 236.044f, 24.863f},
    {3980, 1538.535f, 236.912f, 24.968f},
    {4000, 1543.267f, 240.012f, 25.116f},
    {4020, 1554.944f, 240.012f, 25.242f},
    {4040, 1560.420f, 240.732f, 25.360f},
    {4060, 1568.816f, 241.675f, 25.488f},
    {4080, 1574.373f, 243.319f, 25.624f},
    {4100, 1581.327f, 244.440f, 25.738f},
    {4120, 1590.707f, 245.512f, 26.077f},
    {4140, 1597.213f, 248.121f, 26.077f},
    {4160, 1607.616f, 249.653f, 26.118f},
    {4180, 1613.552f, 249.653f, 26.243f},
    {4200, 1619.410f, 251.703f, 26.348f},
    {4220, 1630.839f, 251.703f, 26.482f},
    {4240, 1635.333f, 252.616f, 26.624f},
    {4260, 1646.272f, 255.477f, 26.742f},
    {4280, 1652.060f, 255.477f, 26.896f},
    {4300, 1656.363f, 257.800f, 26.970f},
    {4320, 1668.966f, 257.800f, 27.120f},
    {4340, 1674.868f, 258.463f, 27.246f},
    {4360, 1681.809f, 261.365f, 27.350f},
    {4380, 1689.931f, 261.365f, 27.486f},
    {4400, 1696.425f, 262.138f, 27.622f},
    {4420, 1705.457f, 263.329f, 27.733f},
    {4440, 1714.118f, 266.166f, 27.867f},
    {4460, 1722.261f, 266.166f, 27.979f},
    {4480, 1728.563f, 268.656f, 28.117f},
    {4500, 1740.056f, 268.656f, 28.245f},
};
*/

// Least-squares fits to the original exact_mean_us measurements (before the table's cumulative maximum).
// PSRAM/RAM use cold source cache; Flash uses seek+read. Units: samples -> microseconds per operation.
// Validated at 600 MHz over 10..4500 samples. Mean estimates, not worst-case timing guarantees.
constexpr float Flash_read_time_us(uint32_t samples)
{
    return 2.2526f + 0.385575f * static_cast<float>(samples);
}

constexpr float Psram_read_time_us(uint32_t samples)
{
    return 0.4655f + 0.059608f * static_cast<float>(samples);
}

constexpr float Ram_read_time_us(uint32_t samples)
{
    return 0.1165f + 0.006258f * static_cast<float>(samples);
}
}

bool PlayersManager::Get_read_time_us(ReadSource source, uint32_t samples, float &time_us)
{
    time_us = INFINITY; // A failed estimate must never look like a free operation if a caller ignores the status.
    if (source != ReadSource::Flash && source != ReadSource::Psram && source != ReadSource::Ram)
    {
        return false;
    }
    if (samples == 0)
    {
        time_us = 0.0f;
        return true;
    }
    if (samples > 4500)
    {
        return false;
    }
    const uint32_t measured_samples = samples < 10 ? 10 : samples; // Preserve the conservative minimum for nonzero requests below the measured range.
    switch (source)
    {
        case ReadSource::Flash:
            time_us = Flash_read_time_us(measured_samples);
            break;
        case ReadSource::Psram:
            time_us = Psram_read_time_us(measured_samples);
            break;
        case ReadSource::Ram:
            time_us = Ram_read_time_us(measured_samples);
            break;
    }
    return true;
}

void PlayersManager::Set_ADSR_ptr(AudioADSR *ptr)
{
    ADSR = ptr;
}

AudioTables::Pointers PlayersManager::Get_playback_tables(uint8_t instrument_id)
{
    if (instrument_id >= INSTRUMENTS || Audio_tables_ptr == nullptr || !AudioTables::Needs_tables(Preset[instrument_id]))
    {
        return {};
    }
    return Audio_tables_ptr->Get_active_pointers(instrument_id, Preset[instrument_id]);
}

void PlayersManager::MX_multicast_change_routing(int instrument_id)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if ((Player_ptr[player].Read_instrument() == instrument_id) && Player_ptr[player].isPlaying())

        {
            // configure Router_L, Router_R input/output routing_table
            if (Delay_values.instrument_route[instrument_id])
            {
                Router_L_ptr->routing_table[player] = 0;
                Router_R_ptr->routing_table[player] = 0;
            }
            else
            {
                Router_L_ptr->routing_table[player] = 1;
                Router_R_ptr->routing_table[player] = 1;
            }

            Router_L_ptr->routing_MX[player] = MX_routing_source[instrument_id];
            Router_R_ptr->routing_MX[player] = MX_routing_source[instrument_id];
        }
    }
}

int PlayersManager::Count_sample_voices(void) const
{
    int count = 0;
    for (int player = 0; player < PLAYERS; ++player)
    {
        count += Player_ptr[player].Uses_sample_voice();
    }
    return count;
}

int PlayersManager::Select_player_for_note(uint8_t instrument_id, uint8_t note_number, int track)
{
    // Se e' una nota appartenente ad un track: track >= -1
    // Altrimenti: track = NO_TRACK

    int8_t id_player = -1;
    bool finished = false;
    const bool needs_sample = !Preset[instrument_id].use_Wavetable && Preset[instrument_id].file < FIRST_LIVE_SAMPLING_FILE;
    const int voice_limit = OPTIMIZATION_VOICES[optimization];
    // A reduced profile takes effect after the old voices finish their fast release.
    if (needs_sample && Count_sample_voices() > voice_limit)
    {
        return -1;
    }

    // Caso NoteOn da tastiera reale (track == NO_TRACK) if a Player is_playing with same patch_id, instrument_id and note_number, and track, this Player must be taken
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if ((Player_ptr[player].Assigned_patch() == Patch_id) && (Player_ptr[player].Assigned_note() == note_number) && (Player_ptr[player].Assigned_instrument() == instrument_id) && Player_ptr[player].isPlaying() && Player_ptr[player].Assigned_track() == track) // isPlaying() significa !idle
        {
            id_player = player;
            finished = true;
            // PRINT("Play the same note - ", "Stop the same Player:", id_player);
            break;
        }
    }

    if (!finished && needs_sample) // File voices share the profile budget regardless of their current storage.
    {
        Update_players_stistics();

        if (Count_sample_voices() < voice_limit) // a Player can be used
        {
            // 1) if there is a Player !isPlaying, it can be used
            id_player = Find_free_player();
            finished = id_player >= 0;

            if (!finished)
            {
                // 2) if there is a Player playing an Instrument of a different Patch, this Player can be used
                id_player = Find_other_patch_player(false, false);
                finished = id_player >= 0;
            }

            if (!finished)
            {
                // 3) look for a Player !power_on and playing of the SAME INSTRUMENT: choose the OLDEST
                id_player = Smartfind_oldest_player(instrument_id, false, true); // SMARTFIND_oldest_player(uint8_t instrument_id, bool power_on, bool playing)
                if (id_player >= 0)
                {
                    finished = true;
                    // PRINT("Play from Flash - case 2", "Found available Player:", id_player);
                }
            }

            if (!finished)
            {
                // 4) look for a Player !power_on of ANY INSTRUMENT NOT protected: choose the OLDEST
                id_player = Simplefind_oldest_player(false); // SIMPLEFIND_oldest_player_power_on(bool power_on)
                if (id_player >= 0)
                {
                    finished = true;
                    // PRINT("Play from Flash - case 3", "Found available Player:", id_player);
                }
            }
            if (!finished)
            {
                // 5) look for a Player power_on and playing of the SAME INSTRUMENT: choose the OLDEST
                id_player = Smartfind_oldest_player(instrument_id, true, true); // SMARTFIND_oldest_player(uint8_t instrument_id, bool power_on, bool playing)
                if (id_player >= 0)
                {
                    finished = true;
                    // PRINT("Play from Flash - case 4", "Found available Player:", id_player);
                }
            }
            if (!finished && Preset[instrument_id].precedence)
            {
                // 6) look for a Player power_on of ANY INSTRUMENT NOT protected: choose the OLDEST
                id_player = Simplefind_oldest_player(true); // SIMPLEFIND_oldest_player(bool power_on)
                if (id_player >= 0)
                {
                    finished = true;
                    // PRINT("Play from Flash - case 5", "Found available Player:", id_player);
                }
            }
        }

        // Only an existing Flash/cache slot can be reused at the profile limit.
        else
        {
            // 5A) there is a Player flash_mode from a different Patch and playing
            id_player = Find_other_patch_player(true, false);
            finished = id_player >= 0;

            if (!finished)
            {
                // 6) there is a Player playing the SAME INSTRUMENT: choose the OLDEST
                id_player = Smartfind_oldest_player(instrument_id, false, true); // SMARTFIND_oldest_player(uint8_t instrument_id, bool power_on, bool playing)
                if (id_player >= 0)
                {
                    finished = true;
                    // PRINT("Play from Flash - case 7", "Found available Player:", id_player);
                }
            }

            if (!finished)
            {
                // 7)  there is a Player fading (state = 2) ANY INSTRUMENT NOT protected flash_mode: choose the OLDEST
                id_player = Simplefind_oldest_sample_player(false, true);
                if (id_player >= 0)
                {
                    finished = true;
                    // PRINT("Play from Flash - case 9", "Found available Player:", id_player);
                }
            }

            if (!finished)
            {
                // 8)  there is a Player playing (state = 1) ANY INSTRUMENT NOT protected flash_mode: choose the OLDEST
                id_player = Simplefind_oldest_sample_player(true, true);
                if (id_player >= 0)
                {
                    finished = true;
                    // PRINT("Play from Flash - case 10", "Found available Player:", id_player);
                }
            }
        }
    }

    if (!finished && !needs_sample) // AudioTables wavetables and Live Sampler keep the independent memory allocation path.
    {
        // 1) look for a Player NOT used
        Update_players_stistics();

        if (players_playing < PLAYERS) // there is at least ONE player that can be taken
        {
            id_player = Find_free_player();
            finished = id_player >= 0;
        }

        else // all Player are playing
        {
            // 2) look for a Player playing an Instrument of a different Patch: this Player can be taken
            id_player = Find_other_patch_player(false, true);
            finished = id_player >= 0;

            if (!finished)
            {
                // 3) look for a Player !power_on (ADSR "Release" phase, or idle) and playing of the SAME INSTRUMENT: choose the OLDEST
                id_player = Smartfind_oldest_player(instrument_id, false, true); // SMARTFIND_oldest_player(uint8_t instrument_id, bool power_on, bool playing)
                if (id_player >= 0)
                {
                    finished = true;
                    // PRINT("Play from RAM - case 3", "Found available Player:", id_player);
                }
            }

            if (!finished)
            {
                // 4) look for a Player !power_on of ANY INSTRUMENT NOT protected: choose the OLDEST
                id_player = Simplefind_oldest_player(false); // SIMPLEFIND_oldest_player(bool power_on)
                if (id_player >= 0)
                {
                    finished = true;
                    // PRINT("Play from RAM - case 4", "Found available Player:", id_player);
                }
            }

            if (!finished)
            {
                // 5) look for a player power_on and playing of the SAME INSTRUMENT: choose the OLDEST
                id_player = Smartfind_oldest_player(instrument_id, true, true); // SMARTFIND_oldest_player(uint8_t instrument_id, bool power_on, bool playing)
                if (id_player >= 0)
                {
                    finished = true;
                    // PRINT("Play from RAM - case 5", "Found available Player:", id_player);
                }
            }
        }

        if (!finished && Preset[instrument_id].precedence)
        {
            // 6) look for a Player power_on of ANY INSTRUMENT NOT protected: choose the OLDEST
            id_player = Simplefind_oldest_player(true); // SIMPLEFIND_oldest_player(bool power_on)
            if (id_player >= 0)
            {
                finished = true;
                // PRINT("Play from RAM - case 6", "Found available Player:", id_player);
            }
        }
    }

    // A same-note candidate may be a wavetable while all file slots are reserved.
    if (finished && needs_sample && !Player_ptr[id_player].Uses_sample_voice())
    {
        Update_players_stistics();
        if (Count_sample_voices() >= voice_limit)
        {
            id_player = Simplefind_oldest_sample_player(false, true);
            if (id_player < 0)
            {
                id_player = Simplefind_oldest_sample_player(true, true);
            }
            finished = id_player >= 0;
        }
    }
    return finished ? id_player : -1;
}

void PlayersManager::Play_note(uint8_t instrument_id, uint8_t note_number, float velocity_float, int track) // after receiving a NoteOn command
{
    if (instrument_id >= INSTRUMENTS || !Preset[instrument_id].active)
    {
        return;
    }
    const AudioTables::Pointers table_pointers = Get_playback_tables(instrument_id);
    // Reject an unavailable sound before allocating a voice or changing its envelope.
    if (AudioTables::Needs_tables(Preset[instrument_id]) && table_pointers.bank_mask == 0)
    {
        return;
    }

    const int id_player = Select_player_for_note(instrument_id, note_number, track);
    if (id_player < 0)
    {
        return; // All eligible voices are occupied or reserved; never overwrite a different pending note.
    }

    Player_booked[id_player] = true;

    if (Player_ptr[id_player].State() > 0 && !restart_Player[id_player]) // sta inviando sample, cioè !idle
    {
        restart_Player[id_player] = true;
        ++players_to_restart;
    }

    Player_ptr[id_player].Write_precedence(Preset[instrument_id].precedence);
    Player_ptr[id_player].Write_midi_channel(Preset[instrument_id].midi_channel);
    Player_ptr[id_player].Set_modulation(modulation_value[Preset[instrument_id].midi_channel]);
    Player_ptr[id_player].Set_source(Preset[instrument_id].source);

    ADSR[id_player].Setup(Preset[instrument_id].attack, Preset[instrument_id].decay, Preset[instrument_id].sustain, Preset[instrument_id].release, Preset[instrument_id].attack_type);

    /*
    Serial.print("id_player: ");
    Serial.print(id_player);
    Serial.print(" ");
    Serial.print(Preset[instrument_id].attack);
    Serial.print(" ");
    Serial.print( Preset[instrument_id].decay);
    Serial.print(" ");
    Serial.print(Preset[instrument_id].sustain);
    Serial.print(" ");
    Serial.print(Preset[instrument_id].release);
    Serial.print(" ");
    Serial.println(Preset[instrument_id].attack_type);
    */

    Player_ptr[id_player].Write_loop_track(track);
    if (track >= 0)
    {
        Player_ptr[id_player].Set_volume(LOOP_volume[track] * Preset[instrument_id].volume);
    }
    else
    {
        Player_ptr[id_player].Set_volume(Preset[instrument_id].volume);
    }
    Player_ptr[id_player].Set_pan(Preset[instrument_id].pan);
    Player_ptr[id_player].Set_pitch(Preset[instrument_id].pitch);

    Player_ptr[id_player].Main_settings(Preset[instrument_id].mode, Preset[instrument_id].A, Preset[instrument_id].B, Preset[instrument_id].Noclick, Preset[instrument_id].use_Wavetable, table_pointers.noclick, table_pointers.wavetable, table_pointers.bank_mask);

    if (!Preset[instrument_id].lock)
    {
        Player_ptr[id_player].Set_effects(resolution_value[resolution], downsampling);
        Player_ptr[id_player].Set_pitch_bend(pitch_bend_value[Preset[instrument_id].midi_channel]);
    }
    else
    {
        Player_ptr[id_player].Set_effects(16.0, 0);
        Player_ptr[id_player].Set_pitch_bend(1.0);
    }

    // VCF and its LFO_0
    if (Preset[instrument_id].Filter.use == 1)
    {
        if (Preset[instrument_id].Filter.modulation == 4) // LFO wave "sine" modulates VCF and MIDI After touch modulates index
        {
            Player_ptr[id_player].Connect_VCF(true, Preset[instrument_id].Filter.type, Preset[instrument_id].Filter.pivot, Preset[instrument_id].Filter.resonance, true);                                                                                                              // void Connect_VCF(bool use, int type, float pivot, float resonance, bool modulated)
            Player_ptr[id_player].Connect_LFO_TO_VCF(Preset[instrument_id].Filter.modulation, Preset[instrument_id].Filter.index * after_touch_channel_value[Preset[instrument_id].midi_channel], Preset[instrument_id].Filter.periodic, Preset[instrument_id].Filter.frequency_time); // Connect_LFO_TO_VCF(uint8_t modulation, float index, uint8_t periodic, float frequency_time)
        }
        else if (Preset[instrument_id].Filter.modulation > 0) // LFO modulates VCF
        {
            Player_ptr[id_player].Connect_VCF(true, Preset[instrument_id].Filter.type, Preset[instrument_id].Filter.pivot, Preset[instrument_id].Filter.resonance, true);                                              // void Connect_VCF(bool use, int type, float pivot, float resonance, bool modulated)
            Player_ptr[id_player].Connect_LFO_TO_VCF(Preset[instrument_id].Filter.modulation, Preset[instrument_id].Filter.index, Preset[instrument_id].Filter.periodic, Preset[instrument_id].Filter.frequency_time); // Connect_LFO_TO_VCF(uint8_t modulation, float index, uint8_t periodic, float frequency_time)
        }
        else // VCF is not modulated
        {
            Player_ptr[id_player].Connect_VCF(true, Preset[instrument_id].Filter.type, Preset[instrument_id].Filter.pivot, Preset[instrument_id].Filter.resonance, false); // void Connect_VCF(bool use, int type, float pivot, float resonance, bool modulated)
        }
    }
    else
    {
        Player_ptr[id_player].Connect_VCF(false, 0, 20000, 1, false);
    }

    /*
    PLAY note; il Player setta:
    power_on = true
    idle = false
    (se interrogato Player[player].is_playing == true);
    */
    Player_ptr[id_player].Get_ready_to_play(pitch_from_note[note_number + 60 - Patch[Patch_id].Instrument[instrument_id].root_key], velocity_float, Patch_id, instrument_id, Preset[instrument_id].sound_id, note_number);

    // calcola update_time e trasmetti il valore al Player
    int update_time;
    if (Lilla_state == LIVE_SAMPLING)
    {
        update_time = 9.067 * Player_ptr[id_player].Read_pitch() + 35.3;
    }
    else
    {
        if (Preset[instrument_id].use_Wavetable)
        {
            update_time = 2.7 * Player_ptr[id_player].Read_pitch() + 31.0;
        }
        else
        {
            update_time = 48.56 * Player_ptr[id_player].Read_pitch() + 48.31;
        }
    }
    if (Preset[instrument_id].Filter.use == 1)
    {
        update_time += 15;
    }

    Player_ptr[id_player].Write_update_time(update_time);

    // Gestione del Delay
    // configura su Router_L e Router_R input/output le routing_table
    if (Delay_values.instrument_route[instrument_id])
    {
        Router_L_ptr->routing_table[id_player] = 0;
        Router_R_ptr->routing_table[id_player] = 0;
    }
    else
    {
        Router_L_ptr->routing_table[id_player] = 1;
        Router_R_ptr->routing_table[id_player] = 1;
    }

    Router_L_ptr->routing_MX[id_player] = MX_routing_source[instrument_id];
    Router_R_ptr->routing_MX[id_player] = MX_routing_source[instrument_id];

    // display Players activity
    if (false)
    {
        for (auto player = 0; player < PLAYERS; ++player)
        {
            if (Player_ptr[player].isPoweredOn())
            {
                Serial.print(Player_ptr[player].Read_note());
                Serial.print("/");
                Serial.print(Player_ptr[player].Read_instrument());
            }
            else
            {
                Serial.print("NN");
            }
            Serial.print(" ");
        }
        Serial.println();
    }
}

void PlayersManager::Release_Player_noteOff(uint8_t player, int track) // after receiving a NoteOff command
{
    Player_ptr[player].Release_note();
    Player_ptr[player].Write_time_stamp(millis());

    // display Players activity
    if (false)
    {
        for (auto player = 0; player < PLAYERS; ++player)
        {
            if (Player_ptr[player].isPoweredOn())
            {
                Serial.print(Player_ptr[player].Read_note());
                Serial.print("/");
                Serial.print(Player_ptr[player].Read_instrument());
            }
            else
                Serial.print("NN");
            Serial.print(" ");
        }
        Serial.println();
    }
}

void PlayersManager::Begin_midi_batch(void)
{
    midi_batch_active = true;
    Reset_booked_and_restart_player();
    Reset_players_to_restart();
    for (int player = 0; player < PLAYERS; ++player)
    {
        Player_booked[player] = Player_ptr[player].Has_pending_note();
    }
}

void PlayersManager::End_midi_batch(void)
{
    // Rebuild from actual pending starts, including requests retained after audio allocation failure.
    Reset_players_to_restart();
    for (int player = 0; player < PLAYERS; ++player)
    {
        restart_Player[player] = Player_ptr[player].Needs_restart_mix();
        if (restart_Player[player])
        {
            ++players_to_restart;
        }
    }
    if (players_to_restart > 0)
    {
        Calculate_and_set_mix_samples();
    }
    midi_batch_active = false;
}

void PlayersManager::Set_modulation(uint8_t midi_channel, uint8_t value)
{
    if (midi_channel >= 16)
    {
        return;
    }
    modulation_value[midi_channel] = value;
    for (int player = 0; player < PLAYERS; ++player)
    {
        if (Player_ptr[player].Read_midi_channel() == midi_channel)
        {
            Player_ptr[player].Set_modulation(value);
        }
    }
}

void PlayersManager::Reset_booked_and_restart_player(void)
{
    // Player_booked serve ad evitare che gli Instrument che sono attivati da NoteOn non competano sullo stesso Player
    // players_to_restart, se risultera' >0, richiede il calcolo dei vari valori di samples (mix_samples_for_Player[player])
    // che ciascun Player da riavviare (restart_Player[player] == true) dovra' utilizzare
    for (auto player = 0; player < PLAYERS; ++player)
    {
        Player_booked[player] = false;
        restart_Player[player] = false;
    }
}

bool PlayersManager::Get_restart_player(int player)
{
    return restart_Player[player];
}

void PlayersManager::Cancel_restart_player(int player)
{
    restart_Player[player] = false;
}

bool PlayersManager::Can_select_player(int player, const Player_filter &filter) const
{
    if (Player_booked[player] || Player_ptr[player].Has_pending_note())
    {
        return false; // Only the explicit same-note retrigger path may replace a pending request.
    }
    if (filter.instrument >= 0 && Player_ptr[player].Read_instrument() != filter.instrument)
    {
        return false;
    }
    if (filter.powered >= 0 && Player_ptr[player].isPoweredOn() != static_cast<bool>(filter.powered))
    {
        return false;
    }
    if (filter.playing >= 0 && Player_ptr[player].isPlaying() != static_cast<bool>(filter.playing))
    {
        return false;
    }
    if (filter.unprotected && Player_ptr[player].Read_precedence())
    {
        return false;
    }
    if (filter.sample_only && !Player_ptr[player].Uses_sample_voice())
    {
        return false;
    }
    if (filter.other_patch && Player_ptr[player].Read_patch_wait() == Patch_id)
    {
        return false;
    }
    return true;
}

int PlayersManager::Find_player(const Player_filter &filter, Selection_order order)
{
    int result = -1;
    unsigned long oldest = 0;
    for (int player = 0; player < PLAYERS; ++player)
    {
        if (!Can_select_player(player, filter))
        {
            continue;
        }
        if (order == Selection_order::First)
        {
            return player;
        }
        const unsigned long timestamp = Player_ptr[player].Read_time_stamp();
        if (order == Selection_order::Last || result < 0 || timestamp < oldest)
        {
            result = player;
            oldest = timestamp; // Strict comparison preserves the lowest index when timestamps tie.
        }
    }
    return result;
}

int PlayersManager::Find_free_player(void)
{
    Player_filter filter;
    filter.playing = 0;
    return Find_player(filter, Selection_order::First);
}

int PlayersManager::Find_other_patch_player(bool sample_only, bool prefer_last)
{
    Player_filter filter;
    filter.other_patch = true;
    filter.sample_only = sample_only;
    filter.playing = sample_only ? 1 : -1;
    return Find_player(filter, prefer_last ? Selection_order::Last : Selection_order::First);
}

int PlayersManager::Smartfind_oldest_player(uint8_t instrument_id, bool power_on, bool playing)
{
    Player_filter filter;
    filter.instrument = instrument_id;
    filter.powered = power_on;
    filter.playing = playing;
    return Find_player(filter, Selection_order::Oldest);
}

int PlayersManager::Simplefind_oldest_player(bool power_on) // "precedence" instruments are EXCLUDED
{
    Player_filter filter;
    filter.powered = power_on;
    filter.unprotected = true;
    return Find_player(filter, Selection_order::Oldest);
}

int PlayersManager::Simplefind_oldest_sample_player(bool power_on, bool playing) // "precedence" instruments are EXCLUDED
{
    Player_filter filter;
    filter.powered = power_on;
    filter.playing = playing;
    filter.unprotected = true;
    filter.sample_only = true;
    return Find_player(filter, Selection_order::Oldest);
}

void PlayersManager::Change_from_key(int patch_id, int instrument_id, int from_key_new)
{
    if (from_key_new > Patch[patch_id].Instrument[instrument_id].from_note)
    {
        for (auto player = 0; player < PLAYERS; ++player)
        {
            if (Player_ptr[player].Read_note() < from_key_new && Player_ptr[player].isPoweredOn() && Player_ptr[player].Read_instrument() == instrument_id)
            {
                Release_player(player);
            }
        }
    }
    Patch[patch_id].Instrument[instrument_id].from_note = from_key_new;
    Update_map_Instrument_for_notes(Patch[patch_id].Instrument[instrument_id].from_note, Patch[patch_id].Instrument[instrument_id].to_note, instrument_id);
}

void PlayersManager::Change_to_key(int patch_id, int instrument_id, int to_key_new)
{
    if (to_key_new < Patch[patch_id].Instrument[instrument_id].to_note)
    {
        for (auto player = 0; player < PLAYERS; ++player)
        {
            if (Player_ptr[player].Read_note() > to_key_new && Player_ptr[player].isPoweredOn() && Player_ptr[player].Read_instrument() == instrument_id)
            {
                Release_player(player);
            }
        }
    }
    Patch[patch_id].Instrument[instrument_id].to_note = to_key_new;
    Update_map_Instrument_for_notes(Patch[patch_id].Instrument[instrument_id].from_note, Patch[patch_id].Instrument[instrument_id].to_note, instrument_id);
}

void PlayersManager::Multicast_change_players_notes(int patch_id, int instrument_id) // usato quando si cambia la root_key
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if (Player_ptr[player].isPoweredOn() && Player_ptr[player].Read_instrument() == instrument_id)
        {
            Player_ptr[player].Set_note(pitch_from_note[Player_ptr[player].Read_note() + 60 - Patch[patch_id].Instrument[instrument_id].root_key]);
        }
    }
}

void PlayersManager::Multicast_release_players(int sound_id)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if (Player_ptr[player].Read_sound_id() == sound_id)
        {
            Release_player(player);
        }
    }
}

void PlayersManager::Broadcast_volume(void)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if (Player_ptr[player].isPlaying())
        {
            Player_ptr[player].Update_volume(Preset[Player_ptr[player].Read_instrument()].volume);
        }
    }
}

void PlayersManager::Multicast_volume_for_instrument_edit(int instrument_id)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if ((Player_ptr[player].Read_instrument() == instrument_id) && Player_ptr[player].isPlaying())
        {
            Player_ptr[player].Update_volume(Preset[instrument_id].volume);
        }
    }
}

void PlayersManager::Multicast_pitch_for_sound_edit(int instrument_id)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if ((Player_ptr[player].Read_instrument() == instrument_id) && Player_ptr[player].isPlaying())
        {
            Player_ptr[player].Set_pitch(Preset[instrument_id].pitch);
        }
    }
}

void PlayersManager::Multicast_pan(int instrument_id)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if ((Player_ptr[player].Read_instrument() == instrument_id) && Player_ptr[player].isPlaying())
        {
            Player_ptr[player].Update_pan(Preset[instrument_id].pan);
        }
    }
}

void PlayersManager::Multicast_effects(float resolution, uint8_t downsampling)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if (Player_ptr[player].isPlaying() && !Preset[Player_ptr[player].Read_instrument()].lock)
        {
            Player_ptr[player].Set_effects(resolution, downsampling);
        }
    }
}

void PlayersManager::Broadcast_reset_effect(float resolution, uint8_t downsampling, int effect)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if (effect == 0) // reset resolution
        {
            Player_ptr[player].Set_effects(16.0, downsampling);
        }

        else if (effect == 1) // reset downsampling
        {
            Player_ptr[player].Set_effects(resolution, 1);
        }

        else
        {
            return;
        }
    }
}

void PlayersManager::Update_all_Preset(int patch_id, float volume_patch)
{
    for (uint8_t instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
    {
        Preset[instrument_id] = {};
        if (Patch[patch_id].Instrument[instrument_id].used)
        {
            Update_Preset(patch_id, instrument_id, volume_patch);
        }
    }
    Cache_manager_ptr->Set_required_files(Preset);
    Refresh_cache_sources();
}

void PlayersManager::Update_all_Preset_volume(int patch_id, float volume_patch)
{
    for (auto instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
    {
        if (Patch[patch_id].Instrument[instrument_id].used)
        {
            Update_Preset_volume(patch_id, instrument_id, volume_patch);
        }
    }
}

Preset_struct PlayersManager::Build_Preset(int patch_id, int instrument_id, float volume_patch)
{
    Preset_struct result = {};
    result.active = true;
    const uint16_t sound_id = Get_sound_id(patch_id, instrument_id);

    result.volume = MX_mute[instrument_id] ? 0.0f : volume_patch * Volume_float[Sound[sound_id].gain];
    result.pan = Sound[sound_id].pan;
    result.sound_id = sound_id;
    result.file = Sound[sound_id].file;
    result.source = Cache_manager_ptr->Get_source(result.file);
    result.midi_channel = Get_midi_channel(patch_id, instrument_id);
    result.pitch = Calc_pitch(Sound[sound_id].pitch);
    result.mode = Sound[sound_id].mode;
    result.A = Sound[sound_id].A;
    result.B = Sound[sound_id].B;
    result.use_Wavetable = (result.B - result.A + 1) <= BLOCK_MIN;
    result.Noclick = Sound[sound_id].Noclick;
    result.attack_type = bitRead(Sound[sound_id].data, 0);
    result.attack = Calc_attack(Sound[sound_id].attack);
    result.decay = Calc_decay(Sound[sound_id].decay);
    result.sustain = Calc_sustain(Sound[sound_id].sustain);
    result.release = Calc_release(Sound[sound_id].release);
    result.precedence = Patch[patch_id].Instrument[instrument_id].precedence;
    result.lock = Patch[patch_id].Instrument[instrument_id].lock;

    result.Filter.use = Patch[patch_id].Instrument[instrument_id].Filter.use;
    result.Filter.type = Patch[patch_id].Instrument[instrument_id].Filter.type;

    const float value = Patch[patch_id].Instrument[instrument_id].Filter.pivot / 10.0f;
    result.Filter.pivot = 20.0f * pow(2.0f, value);
    result.Filter.resonance = (5.0f + Patch[patch_id].Instrument[instrument_id].Filter.resonance) / 5.0f;
    result.Filter.index = Patch[patch_id].Instrument[instrument_id].Filter.index / 20.0f;
    result.Filter.modulation = Patch[patch_id].Instrument[instrument_id].Filter.modulation;

    if (result.Filter.modulation == 3 || result.Filter.modulation == 4)
    {
        result.Filter.periodic = 1;
        result.Filter.frequency_time = Patch[patch_id].Instrument[instrument_id].Filter.frequency_time * Patch[patch_id].Instrument[instrument_id].Filter.frequency_time / 40.0f;
    }
    else
    {
        result.Filter.periodic = 0;
        result.Filter.frequency_time = Patch[patch_id].Instrument[instrument_id].Filter.frequency_time / 8.0f;
    }

    return result;
}

bool PlayersManager::Build_presets_snapshot(int patch_id, float volume_patch, Preset_struct (&presets)[INSTRUMENTS], uint16_t &tables_mask)
{
    tables_mask = 0;

    if (patch_id < 0 || patch_id > PATCHES_MAX)
    {
        return false;
    }

    for (uint8_t instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
    {
        presets[instrument_id] = {};

        if (!Patch[patch_id].Instrument[instrument_id].used)
        {
            continue;
        }

        presets[instrument_id] = Build_Preset(patch_id, instrument_id, volume_patch);

        // Live Sampler instruments do not require AudioTables storage.
        if (presets[instrument_id].file < FIRST_LIVE_SAMPLING_FILE)
        {
            tables_mask |= static_cast<uint16_t>(1u << instrument_id);
        }
    }

    return true;
}

bool PlayersManager::Activate_prepared_presets(const Preset_struct (&presets)[INSTRUMENTS])
{
    if (Audio_tables_ptr == nullptr)
    {
        return false;
    }

    if (!Audio_tables_ptr->Activate_prepared())
    {
        return false;
    }

    // Publish the matching presets before audio interrupts are restored.
    for (uint8_t instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
    {
        Preset[instrument_id] = presets[instrument_id];
    }
    Cache_manager_ptr->Set_required_files(Preset);
    Refresh_cache_sources();
    Refresh_audio_table_references();
    return true;
}

void PlayersManager::Update_Preset(int patch_id, int instrument_id, float volume_patch)
{
    Preset[instrument_id] = Build_Preset(patch_id, instrument_id, volume_patch);
}

void PlayersManager::Update_Preset_volume(int patch_id, int instrument_id, float volume_patch)
{
    if (MX_mute[instrument_id])
    {
        Preset[instrument_id].volume = 0.0;
    }
    else
        Preset[instrument_id].volume = volume_patch * Volume_float[Sound[Get_sound_id(patch_id, instrument_id)].gain];
}

void PlayersManager::Update_Preset_pan(int patch_id, int instrument_id)
{
    Preset[instrument_id].pan = Sound[Get_sound_id(patch_id, instrument_id)].pan;
}

void PlayersManager::Update_Preset_sound_id(int patch_id, int instrument_id)
{
    Preset[instrument_id].sound_id = Get_sound_id(patch_id, instrument_id);
}

void PlayersManager::Update_Preset_file(int patch_id, int instrument_id)
{
    Preset[instrument_id].file = Sound[Get_sound_id(patch_id, instrument_id)].file;
    Preset[instrument_id].source = Cache_manager_ptr->Get_source(Preset[instrument_id].file);
}

void PlayersManager::Update_Preset_midi_channel(int patch_id, int instrument_id)
{
    Preset[instrument_id].midi_channel = Get_midi_channel(patch_id, instrument_id);
}

void PlayersManager::Update_Preset_pitch(int patch_id, int instrument_id)
{
    Preset[instrument_id].pitch = Calc_pitch(Sound[Get_sound_id(patch_id, instrument_id)].pitch);
}

void PlayersManager::Update_Preset_mode(int patch_id, int instrument_id)
{
    Preset[instrument_id].mode = Sound[Get_sound_id(patch_id, instrument_id)].mode;
}

void PlayersManager::Update_Preset_A_B_Wavetable(int patch_id, int instrument_id)
{
    Preset[instrument_id].A = Sound[Get_sound_id(patch_id, instrument_id)].A;
    Preset[instrument_id].B = Sound[Get_sound_id(patch_id, instrument_id)].B;
    Preset[instrument_id].use_Wavetable = (Preset[instrument_id].B - Preset[instrument_id].A + 1) <= BLOCK_MIN;
}

void PlayersManager::Update_Preset_Noclick(int patch_id, int instrument_id)
{
    Preset[instrument_id].Noclick = Sound[Get_sound_id(patch_id, instrument_id)].Noclick;
}

void PlayersManager::Update_Preset_attack_type(int patch_id, int instrument_id)
{
    Preset[instrument_id].attack_type = bitRead(Sound[Get_sound_id(patch_id, instrument_id)].data, 0);
}

void PlayersManager::Update_Preset_attack(int patch_id, int instrument_id)
{
    Preset[instrument_id].attack = Calc_attack(Sound[Get_sound_id(patch_id, instrument_id)].attack);
}

void PlayersManager::Update_Preset_decay(int patch_id, int instrument_id)
{
    Preset[instrument_id].decay = Calc_decay(Sound[Get_sound_id(patch_id, instrument_id)].decay);
}

void PlayersManager::Update_Preset_sustain(int patch_id, int instrument_id)
{
    Preset[instrument_id].sustain = Calc_sustain(Sound[Get_sound_id(patch_id, instrument_id)].sustain);
}

void PlayersManager::Update_Preset_release(int patch_id, int instrument_id)
{
    Preset[instrument_id].release = Calc_release(Sound[Get_sound_id(patch_id, instrument_id)].release);
}

void PlayersManager::Update_Preset_precedence(int patch_id, int instrument_id)
{
    Preset[instrument_id].precedence = Patch[patch_id].Instrument[instrument_id].precedence;
}

void PlayersManager::Update_Preset_lock(int patch_id, int instrument_id)
{
    Preset[instrument_id].lock = Patch[patch_id].Instrument[instrument_id].lock;
}

void PlayersManager::Multicast_IF_update_filter_type(int instrument_id)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if ((Player_ptr[player].Read_instrument() == instrument_id) && Player_ptr[player].isPlaying())
        {
            if (Preset[instrument_id].Filter.use == 1)
            {
                if (Preset[instrument_id].Filter.modulation == 4) // LFO wave "sine" modulates VCF and MIDI After touch modulates index
                {
                    Player_ptr[player].Connect_VCF(true, Preset[instrument_id].Filter.type, Preset[instrument_id].Filter.pivot, Preset[instrument_id].Filter.resonance, true);                                                                                                              // void Connect_VCF(bool use, int type, float pivot, float resonance, bool modulated)
                    Player_ptr[player].Connect_LFO_TO_VCF(Preset[instrument_id].Filter.modulation, Preset[instrument_id].Filter.index * after_touch_channel_value[Preset[instrument_id].midi_channel], Preset[instrument_id].Filter.periodic, Preset[instrument_id].Filter.frequency_time); // Connect_LFO_TO_VCF(uint8_t modulation, float index, uint8_t periodic, float frequency_time)
                }
                else if (Preset[instrument_id].Filter.modulation > 0) // LFO modulates VCF
                {
                    Player_ptr[player].Connect_VCF(true, Preset[instrument_id].Filter.type, Preset[instrument_id].Filter.pivot, Preset[instrument_id].Filter.resonance, true);                                              // void Connect_VCF(bool use, int type, float pivot, float resonance, bool modulated)
                    Player_ptr[player].Connect_LFO_TO_VCF(Preset[instrument_id].Filter.modulation, Preset[instrument_id].Filter.index, Preset[instrument_id].Filter.periodic, Preset[instrument_id].Filter.frequency_time); // Connect_LFO_TO_VCF(uint8_t modulation, float index, uint8_t periodic, float frequency_time)
                }
                else // VCF is not modulated
                {
                    Player_ptr[player].Connect_VCF(true, Preset[instrument_id].Filter.type, Preset[instrument_id].Filter.pivot, Preset[instrument_id].Filter.resonance, false); // void Connect_VCF(bool use, int type, float pivot, float resonance, bool modulated)
                }
            }
            else
                Player_ptr[player].Connect_VCF(false, 0, 20000, 1, false);

            Player_ptr[player].Start_VCF();
        }
    }
}

void PlayersManager::Update_IF_resonance(int patch_id, int instrument_id)
{
    Update_Preset_IF_resonance(patch_id, instrument_id);
    if (Preset[instrument_id].Filter.use == 1)
    {
        Multicast_IF_resonance(instrument_id);
    }
}

void PlayersManager::Multicast_IF_pivot(int instrument_id)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if (((Player_ptr + player)->Read_instrument() == instrument_id) && (Player_ptr + player)->isPlaying())
        {
            (Player_ptr + player)->Update_VCF_pivot(Preset[instrument_id].Filter.pivot);
        }
    }
}

void PlayersManager::Multicast_IF_frequency_filter(int instrument_id)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if (((Player_ptr + player)->Read_instrument() == instrument_id) && (Player_ptr + player)->isPlaying())
        {
            if ((Preset[instrument_id].Filter.modulation != 0) && (Preset[instrument_id].Filter.modulation != 4)) // Connect_LFO_TO_VCF(uint8_t modulation, float index, uint8_t periodic, float frequency_time)
            {
                (Player_ptr + player)->Connect_LFO_TO_VCF(Preset[instrument_id].Filter.modulation, Preset[instrument_id].Filter.index, Preset[instrument_id].Filter.periodic, Preset[instrument_id].Filter.frequency_time);
            }

            else // Connect_LFO_TO_VCF(uint8_t modulation, float index, uint8_t periodic, float frequency_time)
            {
                (Player_ptr + player)->Connect_LFO_TO_VCF(Preset[instrument_id].Filter.modulation, Preset[instrument_id].Filter.index * after_touch_channel_value[Preset[instrument_id].midi_channel], Preset[instrument_id].Filter.periodic, Preset[instrument_id].Filter.frequency_time);
            }
        }
    }
}

void PlayersManager::Multicast_IF_resonance(int instrument_id)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if (((Player_ptr + player)->Read_instrument() == instrument_id) && (Player_ptr + player)->isPlaying())
        {
            (Player_ptr + player)->Update_VCF_resonance(Preset[instrument_id].Filter.resonance);
        }
    }
}

void PlayersManager::Update_Preset_IF(int patch_id, int instrument_id)
{
    Preset[instrument_id].Filter.use = Patch[patch_id].Instrument[instrument_id].Filter.use;
    Preset[instrument_id].Filter.type = Patch[patch_id].Instrument[instrument_id].Filter.type; // 0 -> 3

    float value = Patch[patch_id].Instrument[instrument_id].Filter.pivot / 10.0f;                                        // 0 --> 100  0 --> 10
    Preset[instrument_id].Filter.pivot = 20.0f * pow(2.0f, value);                                                       //  20 --> 20048
    Preset[instrument_id].Filter.resonance = (5.0f + Patch[patch_id].Instrument[instrument_id].Filter.resonance) / 5.0f; // 0 --> 40
    Preset[instrument_id].Filter.index = Patch[patch_id].Instrument[instrument_id].Filter.index / 20.0f;                 // 0 --> 20 : 0 --> 1.0
    Update_Preset_IF_modulation(patch_id, instrument_id);
}

void PlayersManager::Update_Preset_IF_resonance(int patch_id, int instrument_id)
{
    Preset[instrument_id].Filter.resonance = (5.0f + Patch[patch_id].Instrument[instrument_id].Filter.resonance) / 5.0f; // 0 --> 40
}

void PlayersManager::Update_Preset_IF_filter_type(int patch_id, int instrument_id)
{
    Preset[instrument_id].Filter.type = Patch[patch_id].Instrument[instrument_id].Filter.type; // 0 -> 3
}

void PlayersManager::Update_Preset_IF_modulation(int patch_id, int instrument_id)
{
    Preset[instrument_id].Filter.modulation = Patch[patch_id].Instrument[instrument_id].Filter.modulation; // 0 -> 4
    if (Preset[instrument_id].Filter.modulation == 3 || Preset[instrument_id].Filter.modulation == 4)
    {
        Preset[instrument_id].Filter.periodic = 1;
        Preset[instrument_id].Filter.frequency_time = Patch[patch_id].Instrument[instrument_id].Filter.frequency_time * Patch[patch_id].Instrument[instrument_id].Filter.frequency_time / 40.0f; // 0 --> 40
    }
    else
    {
        Preset[instrument_id].Filter.periodic = 0;
        Preset[instrument_id].Filter.frequency_time = Patch[patch_id].Instrument[instrument_id].Filter.frequency_time / 8.0f; // 0 --> 40
    }
}

void PlayersManager::Update_Preset_IF_index(int patch_id, int instrument_id)
{
    Update_Preset_IF(patch_id, instrument_id);
    if (Preset[instrument_id].Filter.use == 1)
    {
        if (Preset[instrument_id].Filter.modulation == 4)
        {
            Multicast_IF_index(instrument_id, Preset[instrument_id].Filter.index * after_touch_channel_value[Preset[instrument_id].midi_channel]);
        }
        else if (Preset[instrument_id].Filter.modulation != 0)
        {
            Multicast_IF_index(instrument_id, Preset[instrument_id].Filter.index);
        }
    }
}

void PlayersManager::Multicast_IF_index(int instrument_id, float value)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if ((Player_ptr[player].Assigned_instrument() == instrument_id) && Player_ptr[player].isPlaying())
        {
            Player_ptr[player].Update_LFO_index(value);
        }
    }
}

void PlayersManager::Multicast_main_settings_editing(int patch_id, int instrument_id)
{
    const AudioTables::Pointers table_pointers = Get_playback_tables(instrument_id);
    if (AudioTables::Needs_tables(Preset[instrument_id]) && table_pointers.bank_mask == 0)
    {
        return;
    }
    uint8_t players_to_cross_mix = 0;
    uint8_t mix_samples_for_Player[PLAYERS] = {0};
    bool cross_mix_Player[PLAYERS] = {0};

    for (auto player = 0; player < PLAYERS; ++player)
    {
        mix_samples_for_Player[player] = 0;
        cross_mix_Player[player] = false;
    }

    for (auto player = 0; player < PLAYERS; ++player)
    {
        if (Player_ptr[player].Apply_preset_edit(patch_id, instrument_id, Preset[instrument_id], table_pointers))
        {
            ++players_to_cross_mix;
            cross_mix_Player[player] = true;
        }
    }

    if (players_to_cross_mix > 0)
    {
        bool finished = false;
        int mix_micros_span = Get_span_for_all_cross_mix();
        // Serial.print("We have ");
        // Serial.print(mix_micros_span);
        // Serial.println(" micros available for ALL cross_mix.");

        bool old_use_wavetable[PLAYERS];
        float old_pitch_of_Player[PLAYERS];
        float standard_pitch_of_Player[PLAYERS];

        // calculate the standardized "flash-play" pitch: pitch of RAM-playing Players will be normalized so they can be traated as Flash-playing Players
        for (auto player = 0; player < PLAYERS; ++player)
        {
            if (cross_mix_Player[player])
            {
                old_use_wavetable[player] = Player_ptr[player].Read_use_Wavetable();
                old_pitch_of_Player[player] = Player_ptr[player].Read_pitch();

                if (old_use_wavetable[player])
                {
                    standard_pitch_of_Player[player] = old_pitch_of_Player[player] * 0.07113;
                }
                else
                {
                    standard_pitch_of_Player[player] = old_pitch_of_Player[player];
                }
                // Serial.print("Player:");
                // Serial.print(player);
                // Serial.print(" has standard_pitch:");
                // Serial.println(standard_pitch_of_Player[player]);
            }
        }
        // Serial.println();

        // find highest-pitch / highest-time Player
        int max_cross_mix_time;
        uint8_t worst_Player = 0;
        float max_pitch = 0;
        for (auto player = 0; player < PLAYERS; ++player)
        {
            if (cross_mix_Player[player])
            {
                if (standard_pitch_of_Player[player] > max_pitch)
                {
                    max_pitch = standard_pitch_of_Player[player];
                    worst_Player = player;
                }
            }
        }
        // Serial.print("Worst Player (with highest pitch) is:");
        // Serial.print(worst_Player);
        // Serial.print(" with pitch:");
        // Serial.println(max_pitch);
        // Serial.println();

        // 1) Try to set mix_samples = 64 for ALL Players
        max_cross_mix_time = Get_cross_mix_time(worst_Player, 64);
        // Serial.print("Try Case 1 - max_cross_mix_time (micros) is:");
        // Serial.println(max_cross_mix_time);
        // Serial.println();

        if (mix_micros_span > (max_cross_mix_time * players_to_cross_mix))
        {
            // Serial.println("Case 1 is verified --> transmit mix_samples = 64 to Players to cross_mix.");
            for (auto player = 0; player < PLAYERS; ++player)
            {
                if (cross_mix_Player[player])
                {
                    cross_mix_Player[player] = false;
                    players_to_cross_mix--;
                    mix_samples_for_Player[player] = 32;
                    Player_ptr[player].Set_mix_samples(mix_samples_for_Player[player]);

                    mix_micros_span -= Get_cross_mix_time(player, 32); // Get_cross_mix_time(uint8_t player, uint16_t mix_samples)
                    // Serial.print("mix_sample 64 is given to Player ");
                    // Serial.print(player);
                    // Serial.print("; theorical update time is:");
                    // Serial.println(Get_cross_mix_time(player, 32) + Player_ptr[player].update_time);
                }
            }
            finished = true;
        }

        // 2) Try to set mix_samples = 48 for ALL Players
        if (!finished)
        {
            max_cross_mix_time = Get_cross_mix_time(worst_Player, 48);
            // Serial.print("Try Case 2 - max_cross_mix_time (micros) is:");
            // Serial.println(max_cross_mix_time);
            // Serial.println();

            if (mix_micros_span > (max_cross_mix_time * players_to_cross_mix))
            {
                // Serial.println("Case 2 is verified --> transmit mix_samples = 48 to Players to cross_mix.");
                for (auto player = 0; player < PLAYERS; ++player)
                {
                    if (cross_mix_Player[player])
                    {
                        cross_mix_Player[player] = false;
                        players_to_cross_mix--;
                        mix_samples_for_Player[player] = 48;
                        Player_ptr[player].Set_mix_samples(mix_samples_for_Player[player]);

                        mix_micros_span -= Get_cross_mix_time(player, 48); // Get_cross_mix_time(uint8_t player, uint16_t mix_samples)

                        if (false)
                        {
                            Serial.print("mix_sample 48 is given to Player ");
                            Serial.print(player);
                            Serial.print("; theorical update time is:");
                            Serial.println(Get_cross_mix_time(player, 48) + Player_ptr[player].Read_update_time());
                        }
                    }
                }
                finished = true;
            }
        }

        // 3) Try to set mix_samples = 32 for ALL Players
        if (!finished)
        {
            max_cross_mix_time = Get_cross_mix_time(worst_Player, 32);
            // Serial.print("Try Case 3 - max_cross_mix_time (micros) is:");
            // Serial.println(max_cross_mix_time);
            // Serial.println();

            if (mix_micros_span > (max_cross_mix_time * players_to_cross_mix))
            {
                // Serial.println("Case 3 is verified --> transmit mix_samples = 32 to Players to cross_mix.");
                for (auto player = 0; player < PLAYERS; ++player)
                {
                    if (cross_mix_Player[player])
                    {
                        cross_mix_Player[player] = false;
                        players_to_cross_mix--;
                        mix_samples_for_Player[player] = 32;
                        Player_ptr[player].Set_mix_samples(mix_samples_for_Player[player]);

                        mix_micros_span -= Get_cross_mix_time(player, 32); // Get_cross_mix_time(uint8_t player, uint16_t mix_samples)

                        if (false)
                        {
                            Serial.print("mix_sample 32 is given to Player ");
                            Serial.print(player);
                            Serial.print("; theorical update time is:");
                            Serial.println(Get_cross_mix_time(player, 32) + Player_ptr[player].Read_update_time());
                        }
                    }
                }
                finished = true;
            }
        }

        if (!finished)
        {
            // 4: Try to assign minimum  mix_samples to Players
            for (auto player = 0; player < PLAYERS; ++player)
            {
                if (cross_mix_Player[player])
                {
                    if (standard_pitch_of_Player[player] <= 0.7)
                    {
                        cross_mix_Player[player] = false;
                        players_to_cross_mix--;
                        mix_samples_for_Player[player] = 64;
                        Player_ptr[player].Set_mix_samples(mix_samples_for_Player[player]);
                        mix_micros_span -= Get_cross_mix_time(player, 64);

                        if (false)
                        {
                            Serial.print("mix_sample 64 is given to Player ");
                            Serial.print(player);
                            Serial.print("; theorical update time is:");
                            Serial.println(Get_cross_mix_time(player, 64) + Player_ptr[player].Read_update_time());
                        }
                    }
                    else if (standard_pitch_of_Player[player] <= 0.8)
                    {
                        cross_mix_Player[player] = false;
                        players_to_cross_mix--;
                        mix_samples_for_Player[player] = 48;
                        Player_ptr[player].Set_mix_samples(mix_samples_for_Player[player]);
                        mix_micros_span -= Get_cross_mix_time(player, 48);

                        if (false)
                        {
                            Serial.print("mix_sample 48 is given to Player ");
                            Serial.print(player);
                            Serial.print("; theorical update time is:");
                            Serial.println(Get_cross_mix_time(player, 48) + Player_ptr[player].Read_update_time());
                        }
                    }
                    else if (standard_pitch_of_Player[player] <= 1.0)
                    {
                        cross_mix_Player[player] = false;
                        players_to_cross_mix--;
                        mix_samples_for_Player[player] = 32;
                        Player_ptr[player].Set_mix_samples(mix_samples_for_Player[player]);
                        mix_micros_span -= Get_cross_mix_time(player, 32);

                        if (false)
                        {
                            Serial.print("mix_sample 32 is given to Player ");
                            Serial.print(player);
                            Serial.print("; theorical update time is:");
                            Serial.println(Get_cross_mix_time(player, 32) + Player_ptr[player].Read_update_time());
                        }
                    }
                    else if (standard_pitch_of_Player[player] <= 1.2)
                    {
                        cross_mix_Player[player] = false;
                        players_to_cross_mix--;
                        mix_samples_for_Player[player] = 24;
                        Player_ptr[player].Set_mix_samples(mix_samples_for_Player[player]);
                        mix_micros_span -= Get_cross_mix_time(player, 24);

                        if (false)
                        {
                            Serial.print("mix_sample 24 is given to Player ");
                            Serial.print(player);
                            Serial.print("; theorical update time is:");
                            Serial.println(Get_cross_mix_time(player, 24) + Player_ptr[player].Read_update_time());
                        }
                    }
                    else if (standard_pitch_of_Player[player] <= 1.4)
                    {
                        cross_mix_Player[player] = false;
                        players_to_cross_mix--;
                        mix_samples_for_Player[player] = 16;
                        Player_ptr[player].Set_mix_samples(mix_samples_for_Player[player]);
                        mix_micros_span -= Get_cross_mix_time(player, 16);

                        if (false)
                        {
                            Serial.print("mix_sample 16 is given to Player ");
                            Serial.print(player);
                            Serial.print("; theorical update time is:");
                            Serial.println(Get_cross_mix_time(player, 16) + Player_ptr[player].Read_update_time());
                        }
                    }
                    if (players_to_cross_mix == 0)
                    {
                        finished = true;
                        break;
                    }
                    if (mix_micros_span <= 10)
                    {
                        // Serial.print("Available time is finished :( Players to examin are:");
                        // Serial.println(players_to_cross_mix);
                        break;
                    }
                }
            }
        }

        if (!finished)
        {
            for (auto player = 0; player < PLAYERS; ++player)
            {
                if (cross_mix_Player[player])
                {
                    cross_mix_Player[player] = false;
                    players_to_cross_mix--;
                    mix_samples_for_Player[player] = 5;
                    Player_ptr[player].Set_mix_samples(mix_samples_for_Player[player]);
                    // Serial.print("Mix_sample 5 is given to Player ");
                    // Serial.println(player);
                }
            }
        }

        // Serial.print("FINISHED - Midi_reader: mix_samples calculation required (micros):");
        // Serial.println(micros() - execution_time);
        // Serial.println();
    }
}

bool PlayersManager::Get_use_Wavetable(int sound_id)
{
    return ((Sound[sound_id].B - Sound[sound_id].A + 1) <= BLOCK_MIN); // BLOCK_MIN is defined in Lilla_player.h
}

int8_t PlayersManager::Find_oldest_player(int instrument_id, bool power_on, bool playing)
{
    unsigned long time_min = 0;
    int8_t result = 0;
    for (auto player_ext = 0; player_ext < PLAYERS; ++player_ext)
    {
        if ((Player_ptr[player_ext].Read_instrument() == instrument_id) && (Player_ptr[player_ext].isPoweredOn() == power_on) && (Player_ptr[player_ext].isPlaying() == playing))
        {
            time_min = Player_ptr[player_ext].Read_time_stamp();
            result = player_ext;
            for (auto player = 0; player < PLAYERS; ++player) // look for the player playing for the longest time
            {
                if (((Player_ptr + player)->Read_instrument() == instrument_id) && ((Player_ptr + player)->isPoweredOn() == power_on) && ((Player_ptr + player)->isPlaying() == playing) && ((Player_ptr + player)->Read_time_stamp() < time_min))
                {
                    time_min = (Player_ptr + player)->Read_time_stamp();
                    result = player;
                }
            }
            return result;
        }
    }
    return -1;
}

void PlayersManager::Release_player(int player, int track) // after receiving a NoteOff command
{
    (Player_ptr + player)->Release_note();
    (Player_ptr + player)->Write_time_stamp(millis());
}

bool PlayersManager::Verify_if_stop_players(int patch_id, int instrument_id) // when EDITING a SOUND, each time B or A change it's MANDATORY to test if SOME Players MUST be stopped
{
    uint8_t players_critical = 0;
    uint8_t players_to_stop = 0;
    int8_t index = 0;

    if ((OPTIMIZATION_VOICES[optimization] < PLAYERS) && Preset[instrument_id].use_Wavetable && Sound[Get_sound_id(patch_id, instrument_id)].file < FIRST_LIVE_SAMPLING_FILE && !Get_use_Wavetable(Get_sound_id(patch_id, instrument_id))) // A wavetable edit starts sharing the file voice budget, whether cached or not.
    {
        // Count file voices plus this patch/instrument's voices that will leave AudioTables.
        for (auto player = 0; player < PLAYERS; ++player)
        {
            if ((Player_ptr + player)->isPlaying() && (((Player_ptr + player)->Read_local_patch() == patch_id && (Player_ptr + player)->Read_instrument() == instrument_id) || (Player_ptr + player)->Uses_sample_voice()))
            {
                players_critical++;
            }
        }

        if (players_critical > OPTIMIZATION_VOICES[optimization]) // players in excess MUST be stopped BEFORE UPDATING A and B
        {
            players_to_stop = players_critical - OPTIMIZATION_VOICES[optimization];

            // 1) look for a Player !power_on and playing "instrument_id": choose the OLDEST
            while (players_to_stop > 0)
            {
                index = Find_oldest_player(instrument_id, false, true); // Find_oldest_player(uint8_t instrument_id, bool power_on, bool playing)
                if (index == -1)
                {
                    break;
                }
                else
                {
                    (Player_ptr + index)->Fast_stop();
                    players_to_stop--;
                }
            }

            // 2) look for a Player power_on playing "instrument_id": choose the OLDEST
            while (players_to_stop > 0)
            {
                index = Find_oldest_player(instrument_id, true, true); // Find_oldest_player(uint8_t instrument_id, bool power_on, bool playing)
                if (index == -1)
                {
                    break;
                }
                else
                {
                    (Player_ptr + index)->Fast_stop();
                    (Player_ptr + index)->Write_time_stamp(millis());
                    players_to_stop--;
                }
            }
            return true; // some Player have been stopped
        }
        return false; // no need to stop any Player
    }
    return false; // no need to stop any Player
}

void PlayersManager::Release_player(int player) // after receiving a NoteOff command
{
    (Player_ptr + player)->Release_note();
    (Player_ptr + player)->Write_time_stamp(millis());
}

void PlayersManager::Release_all_players_for_instrument(int instrument_id) // after receiving a NoteOff command
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if ((Player_ptr + player)->isPoweredOn() && (Player_ptr + player)->Read_instrument() == instrument_id)
        {
            Release_player(player);
        }
    }
}

void PlayersManager::Release_all_players_for_instrument_solo(int instrument_id)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if ((Player_ptr + player)->isPlaying() && ((Player_ptr + player)->Read_instrument() != instrument_id))
        {

            if ((Player_ptr + player)->isPoweredOn())
            {
                (Player_ptr + player)->Fast_stop();
            }
        }
        // S_Map_one_Instrument_for_all_notes(instrument_id); // E' PALESEMENTE UN ERRORE!!!
    }
}

void PlayersManager::Release_all_players(void)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        Release_player(player);
    }
}

void PlayersManager::Release_softly_all_players(int patch_id) // Bound every outgoing voice and queued restart by QUICK_RELEASE_TIME.
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        Player_ptr[player].Release_patch(patch_id); // Include existing release tails and cancel pending notes belonging to the outgoing patch.
    }
}

void PlayersManager::Stop_all_players(void) // meglio Fast_stop...
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        (Player_ptr + player)->Fast_stop(); // Attiva il Release (ADSR) con tempo di caduta pari a 10 samples
        (Player_ptr + player)->Write_time_stamp(millis());
    }
}

uint16_t PlayersManager::Fast_stop_players_using_tables(uint8_t banks_mask)
{
    static_assert(PLAYERS <= 16);

    uint16_t stopped_players_mask = 0;

    for (uint8_t player_id = 0; player_id < PLAYERS; ++player_id)
    {
        if (Player_ptr[player_id].Fast_stop_using_tables(banks_mask))
        {
            Player_ptr[player_id].Write_time_stamp(millis());
            stopped_players_mask |= static_cast<uint16_t>(1u << player_id);
        }
    }

    return stopped_players_mask;
}

void PlayersManager::Release_all_players_loop(void)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if ((Player_ptr + player)->isPoweredOn() && ((Player_ptr + player)->Read_loop_track() >= 0))
        {
            Release_player(player);
        }
    }
}

void PlayersManager::Release_all_players_loop(int track)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if ((Player_ptr + player)->isPoweredOn() && ((Player_ptr + player)->Read_loop_track() == track))
        {
            Release_player(player);
        }
    }
}

float PlayersManager::Get_cross_mix_time(int player, int mix_samples) // restituisce il tempo (us) che si impiega a leggere i campioni che costituiscono il mix_samples, dal vecchio file e col vecchio pitch.
{
    if ((Player_ptr + player)->Read_use_Wavetable()) // how Player read OLD FILE
    {
        return (mix_samples * (0.03 * (Player_ptr + player)->Read_pitch() + 0.2));
    }
    else
    {
        return (mix_samples * (0.4 * (Player_ptr + player)->Read_pitch() + 0.2));
    }
}

int PlayersManager::Get_span_for_all_cross_mix(void) // ALERT: can be used if Play_note() HAS BEEN ALREADY SENT!
{
    int value = 0;

    for (auto player = 0; player < PLAYERS; ++player)
    {
        value += Player_ptr[player].Read_update_time();
    }

    return (AUDIO_PLAYER_DEADLINE_US - value - (midi_batch_active ? static_cast<int>(audio_update_time_micros) : 0)); // Share the emergency deadline; include preparation only for the current audio batch.
}

void PlayersManager::Multicast_reset_pitch_bend_effects(int instrument_id)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if (Player_ptr[player].Read_instrument() == instrument_id)
        {
            Player_ptr[player].Set_pitch_bend(1.0);
            Player_ptr[player].Set_effects(16.0, 0);
        }
    }
}

void PlayersManager::Broadcast_pitch_bend(int midi_channel, float value)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if (Player_ptr[player].isPlaying() && !Preset[Player_ptr[player].Assigned_instrument()].lock && Preset[Player_ptr[player].Assigned_instrument()].midi_channel == midi_channel) // if(!bitRead(Patch[patch_id].Instrument[Player_ptr[player].instrument_id].info, 1) && (Get_midi_channel(patch_id, Player_ptr[player].instrument_id) == midi_channel))
        {
            Player_ptr[player].Set_pitch_bend(value);
        }
    }
}

void PlayersManager::Broadcast_restore_pitch_bend_and_effects(int midi_channel, float value)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if (!Preset[Player_ptr[player].Read_instrument()].lock && Preset[Player_ptr[player].Read_instrument()].midi_channel == midi_channel) // if(!bitRead(Patch[patch_id].Instrument[Player_ptr[player].instrument_id].info, 1))
        {
            Player_ptr[player].Set_pitch_bend(value);
            Player_ptr[player].Set_effects(resolution_value[resolution], downsampling);
        }
    }
}

void PlayersManager::Multicast_volume_for_MIDI_LOOP_running(int track, float value)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if ((Player_ptr[player].Read_loop_track() == track) && Player_ptr[player].isPlaying())
        {
            Player_ptr[player].Update_volume(value * Preset[Player_ptr[player].Read_instrument()].volume);
        }
    }
}

void PlayersManager::Update_players_stistics(void)
{
    players_using_Wavetable = 0;
    players_using_Flash = 0;
    players_using_Psram = 0;

    for (auto player = 0; player < PLAYERS; ++player)
    {
        if (Player_ptr[player].isPlaying())
        {
            if (Player_ptr[player].Uses_flash())
            {
                ++players_using_Flash;
            }
            else if (Player_ptr[player].Read_use_Wavetable())
            {
                ++players_using_Wavetable;
            }
            else
            {
                ++players_using_Psram;
            }
        }
    }
    players_playing = players_using_Wavetable + players_using_Flash + players_using_Psram;
}

int PlayersManager::Get_players_playing(void)
{
    return players_playing;
}

int PlayersManager::Get_players_using_Flash(void)
{
    return players_using_Flash;
}

int PlayersManager::Get_players_using_Wavetable(void)
{
    return players_using_Wavetable;
}

void PlayersManager::Calculate_and_set_mix_samples(void)
{
    uint8_t mix_samples_for_Player[PLAYERS] = {0};

    for (auto player = 0; player < PLAYERS; ++player)
    {
        mix_samples_for_Player[player] = 0;
    }

    bool finished = false;

    int mix_micros_span = Get_span_for_all_cross_mix(); // IMP! Get_span_for_all_cross_mix() CAN be called ONLY if Play_note() HAS BEEN ALREADY CALLED!
    // Serial.print("We have ");
    // Serial.print(mix_micros_span);
    // Serial.println(" micros available for ALL cross_mix.");

    bool old_use_wavetable[PLAYERS];
    float old_pitch_of_Player[PLAYERS];
    float standard_pitch_of_Player[PLAYERS];

    // Calculate the standardized "flash-play" pitch: pitch of RAM-playing Players will be normalized so they can be traated as Flash-playing Players
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if (Get_restart_player(player))
        {
            old_use_wavetable[player] = Player_ptr[player].Read_use_Wavetable();
            old_pitch_of_Player[player] = Player_ptr[player].Read_pitch();

            if (old_use_wavetable[player])
            {
                standard_pitch_of_Player[player] = old_pitch_of_Player[player] * 0.07113;
            }
            else
            {
                standard_pitch_of_Player[player] = old_pitch_of_Player[player];
            }

            if (false)
            {
                Serial.print("Player:");
                Serial.print(player);
                Serial.print(" has standard_pitch:");
                Serial.println(standard_pitch_of_Player[player]);
            }
        }
    }
    // Serial.println();

    // find highest-pitch / highest-time Player
    int max_cross_mix_time;
    uint8_t worst_Player = 0;
    float max_pitch = 0;

    for (auto player = 0; player < PLAYERS; ++player)
    {
        if (Get_restart_player(player))
        {
            if (standard_pitch_of_Player[player] > max_pitch)
            {
                max_pitch = standard_pitch_of_Player[player];
                worst_Player = player;
            }
        }
    }

    if (false)
    {
        Serial.print("Worst Player (with highest pitch) is:");
        Serial.print(worst_Player);
        Serial.print(" with pitch:");
        Serial.println(max_pitch);
        Serial.println();
    }

    // 1) Try to set mix_samples = 64 for ALL Players
    max_cross_mix_time = Get_cross_mix_time(worst_Player, 64);
    // Serial.print("Try Case 1 - max_cross_mix_time (micros) is:");
    // Serial.println(max_cross_mix_time);
    // Serial.println();

    if (mix_micros_span > (max_cross_mix_time * players_to_restart))
    {
        // Serial.println("Case 1 is verified --> transmit mix_samples = 64 to Players to restart.");
        for (auto player = 0; player < PLAYERS; ++player)
        {
            if (Get_restart_player(player))
            {
                Cancel_restart_player(player);
                players_to_restart--;
                mix_samples_for_Player[player] = 64;
                Player_ptr[player].Set_mix_samples(mix_samples_for_Player[player]);

                mix_micros_span -= Get_cross_mix_time(player, 64);

                if (false)
                {
                    Serial.print("mix_sample 64 is given to Player ");
                    Serial.print(player);
                    Serial.print("; theorical update time is:");
                    Serial.println(Get_cross_mix_time(player, 64) + Player_ptr[player].Read_update_time());
                }
            }
        }
        finished = true;
    }

    // 2) Try to set mix_samples = 48 for ALL Players
    if (!finished)
    {
        max_cross_mix_time = Get_cross_mix_time(worst_Player, 48);
        // Serial.print("Try Case 2 - max_cross_mix_time (micros) is:");
        // Serial.println(max_cross_mix_time);
        // Serial.println();

        if (mix_micros_span > (max_cross_mix_time * players_to_restart))
        {
            // Serial.println("Case 2 is verified --> transmit mix_samples = 48 to Players to restart.");
            for (auto player = 0; player < PLAYERS; ++player)
            {
                if (Get_restart_player(player))
                {
                    Cancel_restart_player(player);
                    players_to_restart--;
                    mix_samples_for_Player[player] = 48;
                    Player_ptr[player].Set_mix_samples(mix_samples_for_Player[player]);

                    mix_micros_span -= Get_cross_mix_time(player, 48);
                    if (false)
                    {
                        Serial.print("mix_sample 48 is given to Player ");
                        Serial.print(player);
                        Serial.print("; theorical update time is:");
                        Serial.println(Get_cross_mix_time(player, 48) + Player_ptr[player].Read_update_time());
                    }
                }
            }
            finished = true;
        }
    }

    // 3) Try to set mix_samples = 32 for ALL Players
    if (!finished)
    {
        max_cross_mix_time = Get_cross_mix_time(worst_Player, 32);
        // Serial.print("Try Case 3 - max_cross_mix_time (micros) is:");
        // Serial.println(max_cross_mix_time);
        // Serial.println();

        if (mix_micros_span > (max_cross_mix_time * players_to_restart))
        {
            // Serial.println("Case 3 is verified --> transmit mix_samples = 32 to Players to restart.");
            for (auto player = 0; player < PLAYERS; ++player)
            {
                if (Get_restart_player(player))
                {
                    Cancel_restart_player(player);
                    players_to_restart--;
                    mix_samples_for_Player[player] = 32;
                    Player_ptr[player].Set_mix_samples(mix_samples_for_Player[player]);

                    mix_micros_span -= Get_cross_mix_time(player, 32);

                    if (false)
                    {
                        Serial.print("mix_sample 32 is given to Player ");
                        Serial.print(player);
                        Serial.print("; theorical update time is:");
                        Serial.println(Get_cross_mix_time(player, 32) + Player_ptr[player].Read_update_time());
                    }
                }
            }
            finished = true;
        }
    }

    if (!finished)
    {
        // 4: Try to assign minimum  mix_samples to Players
        const uint8_t first_player = restart_mix_first_player;
        restart_mix_first_player = (restart_mix_first_player + 1) % PLAYERS;
        for (int offset = 0; offset < PLAYERS; ++offset)
        {
            const int player = (first_player + offset) % PLAYERS;
            if (Get_restart_player(player))
            {
                if (standard_pitch_of_Player[player] <= 0.7 && Get_cross_mix_time(player, 64) <= mix_micros_span)
                {
                    Cancel_restart_player(player);
                    players_to_restart--;
                    mix_samples_for_Player[player] = 64;
                    Player_ptr[player].Set_mix_samples(mix_samples_for_Player[player]);
                    mix_micros_span -= Get_cross_mix_time(player, 64);

                    if (false)
                    {
                        Serial.print("mix_sample 64 is given to Player ");
                        Serial.print(player);
                        Serial.print("; theorical update time is:");
                        Serial.println(Get_cross_mix_time(player, 64) + Player_ptr[player].Read_update_time());
                    }
                }
                else if (standard_pitch_of_Player[player] <= 0.8 && Get_cross_mix_time(player, 48) <= mix_micros_span)
                {
                    Cancel_restart_player(player);
                    players_to_restart--;
                    mix_samples_for_Player[player] = 48;
                    Player_ptr[player].Set_mix_samples(mix_samples_for_Player[player]);
                    mix_micros_span -= Get_cross_mix_time(player, 48);

                    if (false)
                    {
                        Serial.print("mix_sample 48 is given to Player ");
                        Serial.print(player);
                        Serial.print("; theorical update time is:");
                        Serial.println(Get_cross_mix_time(player, 48) + Player_ptr[player].Read_update_time());
                    }
                }
                else if (standard_pitch_of_Player[player] <= 1.0 && Get_cross_mix_time(player, 32) <= mix_micros_span)
                {
                    Cancel_restart_player(player);
                    players_to_restart--;
                    mix_samples_for_Player[player] = 32;
                    Player_ptr[player].Set_mix_samples(mix_samples_for_Player[player]);
                    mix_micros_span -= Get_cross_mix_time(player, 32);

                    if (false)
                    {
                        Serial.print("mix_sample 32 is given to Player ");
                        Serial.print(player);
                        Serial.print("; theorical update time is:");
                        Serial.println(Get_cross_mix_time(player, 32) + Player_ptr[player].Read_update_time());
                    }
                }
                else if (standard_pitch_of_Player[player] <= 1.2 && Get_cross_mix_time(player, 24) <= mix_micros_span)
                {
                    Cancel_restart_player(player);
                    players_to_restart--;
                    mix_samples_for_Player[player] = 24;
                    Player_ptr[player].Set_mix_samples(mix_samples_for_Player[player]);
                    mix_micros_span -= Get_cross_mix_time(player, 24);

                    if (false)
                    {
                        Serial.print("mix_sample 24 is given to Player ");
                        Serial.print(player);
                        Serial.print("; theorical update time is:");
                        Serial.println(Get_cross_mix_time(player, 24) + Player_ptr[player].Read_update_time());
                    }
                }
                else if (standard_pitch_of_Player[player] <= 1.4 && Get_cross_mix_time(player, 16) <= mix_micros_span)
                {
                    Cancel_restart_player(player);
                    players_to_restart--;
                    mix_samples_for_Player[player] = 16;
                    Player_ptr[player].Set_mix_samples(mix_samples_for_Player[player]);
                    mix_micros_span -= Get_cross_mix_time(player, 16);

                    if (false)
                    {
                        Serial.print("mix_sample 16 is given to Player ");
                        Serial.print(player);
                        Serial.print("; theorical update time is:");
                        Serial.println(Get_cross_mix_time(player, 16) + Player_ptr[player].Read_update_time());
                    }
                }
                if (players_to_restart == 0)
                {
                    finished = true;
                    break;
                }
                if (mix_micros_span <= 10)
                {
                    // Serial.print("Available time is finished :( Players to examin are:");
                    // Serial.println(players_to_restart);
                    break;
                }
            }
        }
    }

    if (!finished)
    {
        for (auto player = 0; player < PLAYERS; ++player)
        {
            if (Get_restart_player(player))
            {
                Cancel_restart_player(player);
                players_to_restart--;
                mix_samples_for_Player[player] = 0;
                Player_ptr[player].Set_mix_samples(mix_samples_for_Player[player]);
                // Serial.print("Mix_sample ZERO is given to Player ");
                // Serial.println(player);
            }
        }
    }
}

void PlayersManager::Reset_players_to_restart(void)
{
    players_to_restart = 0;
}

int PlayersManager::Get_players_to_restart(void)
{
    return players_to_restart;
}

void PlayersManager::Multicast_update_vibrato(int midi_channel, bool vibrato_active)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if (Player_ptr[player].Read_midi_channel() == midi_channel)
        {
            Player_ptr[player].Set_vibrato_flag(vibrato_active);
        }
    }
}

void PlayersManager::Multicast_all_notes_off(int midi_channel)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if (Player_ptr[player].isPoweredOn() && Player_ptr[player].Read_midi_channel() == midi_channel)
        {
            Release_Player_noteOff(player);
        }
    }
}

void PlayersManager::Multicast_stop_players_for_loop_track(int track)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if (Player_ptr[player].isPoweredOn() && Player_ptr[player].Assigned_track() == track)
        {
            Release_Player_noteOff(player, track);
        }
    }
}

void PlayersManager::Multicast_stop_players_for_NoteOff(int midi_channel, int note_number, int track)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        if (Player_ptr[player].isPoweredOn() && Player_ptr[player].Assigned_note() == note_number && Player_ptr[player].Read_midi_channel() == midi_channel && Player_ptr[player].Assigned_track() == track)
        {
            Release_Player_noteOff(player, track);
        }
    }
}

void PlayersManager::Broadcast_FIFO_stereo(int16_t *LS_buffer_L_ptr, int16_t *LS_buffer_R_ptr)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        Player_ptr[player].LS_buffer_L_ptr = LS_buffer_L_ptr;
        Player_ptr[player].LS_buffer_R_ptr = LS_buffer_R_ptr;
    }
}

void PlayersManager::Broadcast_FIFO_mono(int16_t *LS_buffer_mono_ptr)
{
    for (auto player = 0; player < PLAYERS; ++player)
    {
        Player_ptr[player].LS_buffer_mono_ptr = LS_buffer_mono_ptr;
    }
}

uint8_t PlayersManager::Refresh_audio_table_references(void)
{
    uint8_t referenced_banks = 0;
    for (uint8_t player_id = 0; player_id < PLAYERS; ++player_id)
    {
        if (Audio_tables_ptr != nullptr)
        {
            Player_ptr[player_id].Refresh_audio_table_references(*Audio_tables_ptr);
        }
        referenced_banks |= Player_ptr[player_id].Get_tables_reference_mask();
    }
    return referenced_banks;
}

int PlayersManager::Get_players_using_Psram(void)
{
    return players_using_Psram;
}

void PlayersManager::Refresh_cache_sources(void)
{
    for (auto &preset : Preset)
    {
        if (!preset.active)
        {
            continue;
        }
        preset.source = Cache_manager_ptr->Get_source(preset.file);
        if (preset.source.storage != Psram)
        {
            continue;
        }
        for (uint8_t player = 0; player < PLAYERS; ++player)
        {
            Player_ptr[player].Refresh_cached_source(preset.source);
        }
    }
}

uint16_t PlayersManager::Get_cache_reference_mask(void)
{
    uint16_t mask = 0;
    for (uint8_t player = 0; player < PLAYERS; ++player)
    {
        mask |= Player_ptr[player].Get_cache_reference_mask();
    }
    return mask;
}

uint16_t PlayersManager::Fast_stop_players_using_cache(uint16_t mask)
{
    uint16_t stopped = 0;
    for (uint8_t player = 0; player < PLAYERS; ++player)
    {
        if (Player_ptr[player].Fast_stop_using_cache(mask))
        {
            stopped |= static_cast<uint16_t>(1u << player);
        }
    }
    return stopped;
}
