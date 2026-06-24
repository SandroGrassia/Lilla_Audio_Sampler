/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

/*
Userinterface.h version for LILLA PCB_2026_R1
*/

#pragma once

#include <Arduino.h>
#include "config.h"

// Encoders, Pushbuttons, Shift Register chips
static constexpr int ENCODERS = 17;
static constexpr int PUSHBUTTONS = 38;
static constexpr int UI_LEDS = 5;
static constexpr int SHIFTERS = 5;          // number of shifter chips
static constexpr int SHIFTER_CHANNELS = 16; // number of channels in a shifter

enum ShifterPort // (input/output) port id
{
    a0,
    a1,
    a2,
    a3,
    a4,
    a5,
    a6,
    a7,
    b0,
    b1,
    b2,
    b3,
    b4,
    b5,
    b6,
    b7
};

// Shifters ports connections
/*
static constexpr char *shifter0[16][2] =
{
    {"a0", "PB_16"},
    {"a1", "DT_16"},
    {"a2", "CLK_16"},
    {"a3", "DT_0"},
    {"a4", "CLK_0"},
    {"a5", "PB_0"},
    {"a6", "DT_1"},
    {"a7", "CLK_1"},
    {"b0", "CLK_2"},
    {"b1", "DT_2"},
    {"b2", "PB_1"},
    {"b3", "PB_2"},
    {"b4", "CLK_3"},
    {"b5", "DT_3"},
    {"b6", "PB_3"},
    {"b7", ""}
};

static constexpr char *shifter1[16][2] =
{
    {"a0", "PB_33"},
    {"a1", "PB_34"},
    {"a2", "PB_35"},
    {"a3", "PB_36"},
    {"a4", "PB_32"},
    {"a5", "LED_P12"},
    {"a6", "PB_29"},
    {"a7", "PB_28"},
    {"b0", "PB_31"},
    {"b1", "PB_30"},
    {"b2", "CLK_4"},
    {"b3", "DT_4"},
    {"b4", "PB_4"},
    {"b5", "DT_6"},
    {"b6", "CLK_6"},
    {"b7", "PB_6"}
};

static constexpr char *shifter2[16][2] =
{
    {"a0", "PB_21"},
    {"a1", "LED_P8"},
    {"a2", "PB_24"},
    {"a3", "PB_5"},
    {"a4", "DT_7"},
    {"a5", "CLK_7"},
    {"a6", "DT_5"},
    {"a7", "CLK_5"},
    {"b0", "PB_7"},
    {"b1", "PB_37"},
    {"b2", "PB_17"},
    {"b3", "PB_18"},
    {"b4", "PB_19"},
    {"b5", "PB_20"},
    {"b6", "PB_25"},
    {"b7", "LED_P9"}
};

static constexpr char *shifter3[16][2] =
{
    {"a0", "CLK_14"},
    {"a1", "DT_14"},
    {"a2", "PB_13"},
    {"a3", "DT_13"},
    {"a4", "CLK_13"},
    {"a5", "PB_12"},
    {"a6", "DT_12"},
    {"a7", "CLK_12"},
    {"b0", "PB_8"},
    {"b1", "CLK_8"},
    {"b2", "DT_8"},
    {"b3", "PB_11"},
    {"b4", "CLK_11"},
    {"b5", "DT_11"},
    {"b6", "PB_14"},
    {"b7", ""}
};

static constexpr char *shifter4[16][2] =
{
    {"a0", "PB_15"},
    {"a1", "CLK_15"},
    {"a2", "DT_15"},
    {"a3", "LED_P11"},
    {"a4", "PB_27"},
    {"a5", "DT_9"},
    {"a6", "CLK_9"},
    {"a7", "PB_9"},
    {"b0", "DT_10"},
    {"b1", "CLK_10"},
    {"b2", "PB_10"},
    {"b3", "PB_26"},
    {"b4", "LED_P10"},
    {"b5", ""},
    {"b6", "PB_22"},
    {"b7", "PB_23"}
};
*/

struct Encoder_physical_struct
{
    uint8_t shifter_id;
    ShifterPort DT_shifter_channel;
    ShifterPort CLK_shifter_channel;
};

// Physical location of each rotary encoder in the shifter matrix.
// The array index is the logical encoder ID.
// Each entry contains the shifter chip ID and the two channels connected to
// the encoder quadrature signals.
//
// Format:
//   {shifter_id, DT_shifter_channel, CLK_shifter_channel}
static constexpr Encoder_physical_struct encoder_physical[ENCODERS] = {
    {0, a3, a4}, // Encoder 0  -> DT_0,  CLK_0
    {0, a6, a7}, // Encoder 1  -> DT_1,  CLK_1
    {0, b1, b0}, // Encoder 2  -> DT_2,  CLK_2
    {0, b5, b4}, // Encoder 3  -> DT_3,  CLK_3
    {1, b3, b2}, // Encoder 4  -> DT_4,  CLK_4
    {2, a6, a7}, // Encoder 5  -> DT_5,  CLK_5
    {1, b5, b6}, // Encoder 6  -> DT_6,  CLK_6
    {2, a4, a5}, // Encoder 7  -> DT_7,  CLK_7
    {3, b2, b1}, // Encoder 8  -> DT_8,  CLK_8
    {4, a5, a6}, // Encoder 9  -> DT_9,  CLK_9
    {4, b0, b1}, // Encoder 10 -> DT_10, CLK_10
    {3, b5, b4}, // Encoder 11 -> DT_11, CLK_11
    {3, a6, a7}, // Encoder 12 -> DT_12, CLK_12
    {3, a3, a4}, // Encoder 13 -> DT_13, CLK_13
    {3, a1, a0}, // Encoder 14 -> DT_14, CLK_14
    {4, a2, a1}, // Encoder 15 -> DT_15, CLK_15
    {0, a1, a2}  // Encoder 16 -> DT_16, CLK_16
};

// Pushbuttons
struct Pushbutton_physical_struct
{
    uint8_t shifter_id;
    ShifterPort shifter_channel;
};

// Physical location of each pushbutton in the shifter matrix.
// The array index is the logical pushbutton ID.
// Each entry contains the shifter chip ID and the channel where that pushbutton is connected.
//
// Format:
//   {shifter_id, shifter_channel}
static constexpr Pushbutton_physical_struct pushbutton_physical[PUSHBUTTONS] = {
    {0, a5}, // Pushbutton 0  -> PB_0
    {0, b2}, // Pushbutton 1  -> PB_1
    {0, b3}, // Pushbutton 2  -> PB_2
    {0, b6}, // Pushbutton 3  -> PB_3

    {1, b4}, // Pushbutton 4  -> PB_4
    {2, a3}, // Pushbutton 5  -> PB_5
    {1, b7}, // Pushbutton 6  -> PB_6
    {2, b0}, // Pushbutton 7  -> PB_7

    {3, b0}, // Pushbutton 8  -> PB_8
    {4, a7}, // Pushbutton 9  -> PB_9
    {4, b2}, // Pushbutton 10 -> PB_10
    {3, b3}, // Pushbutton 11 -> PB_11

    {3, a5}, // Pushbutton 12 -> PB_12
    {3, a2}, // Pushbutton 13 -> PB_13
    {3, b6}, // Pushbutton 14 -> PB_14
    {4, a0}, // Pushbutton 15 -> PB_15

    {0, a0}, // Pushbutton 16 -> PB_16
    {2, b2}, // Pushbutton 17 -> PB_17
    {2, b3}, // Pushbutton 18 -> PB_18
    {2, b4}, // Pushbutton 19 -> PB_19

    {2, b5}, // Pushbutton 20 -> PB_20
    {2, a0}, // Pushbutton 21 -> PB_21
    {4, b6}, // Pushbutton 22 -> PB_22
    {4, b7}, // Pushbutton 23 -> PB_23

    {2, a2}, // Pushbutton 24 -> PB_24
    {2, b6}, // Pushbutton 25 -> PB_25
    {4, b3}, // Pushbutton 26 -> PB_26
    {4, a4}, // Pushbutton 27 -> PB_27

    {1, a7}, // Pushbutton 28 -> PB_28
    {1, a6}, // Pushbutton 29 -> PB_29
    {1, b1}, // Pushbutton 30 -> PB_30
    {1, b0}, // Pushbutton 31 -> PB_31

    {1, a4}, // Pushbutton 32 -> PB_32
    {1, a0}, // Pushbutton 33 -> PB_33
    {1, a1}, // Pushbutton 34 -> PB_34
    {1, a2}, // Pushbutton 35 -> PB_35
    {1, a3}, // Pushbutton 36 -> PB_36
    {2, b1}  // Pushbutton 37 -> PB_37
};

// Loop Rec LED
struct UI_LEDs_physical_struct
{
    uint8_t shifter_id;
    ShifterPort shifter_channel;
};

static constexpr UI_LEDs_physical_struct UI_leds[UI_LEDS] = {
    {2, a1}, // LED_P8 (LED_Rec_1)
    {2, b7}, // LED_P9 (LED_Rec_2)
    {4, a3}, // LED_P11 (LED_Rec_3)
    {4, b4}, // LED_P10 (LED_Rec_4)
    {1, a5}, // LED_P12 (LED_Tools)
};

struct EncoderPushbutton
{
    int encoder_id;
    int pushbutton_id;
};

// Lookup table that maps each physical shifter channel to the UI control connected to it.
// The first index selects the shifter chip, and the second index selects the channel
// inside that shifter (a0...a7, b0...b7).
//
// Each entry contains:
//   {encoder_id, pushbutton_id}
//
// Use -1 when no encoder or pushbutton is assigned to that channel.
// Channels used by other devices, such as LEDs, are also marked as {-1, -1}.
static constexpr EncoderPushbutton Shifter_channel_to_encoder_pushbutton[SHIFTERS][SHIFTER_CHANNELS] = {
    /* ===================== SHIFTER 0 ===================== */
    {/* a0 */ {-1, 16}, // PB_16
     /* a1 */ {16, -1}, // DT_16
     /* a2 */ {16, -1}, // CLK_16
     /* a3 */ {0, -1},  // DT_0
     /* a4 */ {0, -1},  // CLK_0
     /* a5 */ {-1, 0},  // PB_0
     /* a6 */ {1, -1},  // DT_1
     /* a7 */ {1, -1},  // CLK_1
     /* b0 */ {2, -1},  // CLK_2
     /* b1 */ {2, -1},  // DT_2
     /* b2 */ {-1, 1},  // PB_1
     /* b3 */ {-1, 2},  // PB_2
     /* b4 */ {3, -1},  // CLK_3
     /* b5 */ {3, -1},  // DT_3
     /* b6 */ {-1, 3},  // PB_3
     /* b7 */ {-1, -1}},

    /* ===================== SHIFTER 1 ===================== */
    {/* a0 */ {-1, 33}, // PB_33
     /* a1 */ {-1, 34}, // PB_34
     /* a2 */ {-1, 35}, // PB_35
     /* a3 */ {-1, 36}, // PB_36
     /* a4 */ {-1, 32}, // PB_32
     /* a5 */ {-1, -1}, // ***** LED_P12 (LED_Tools)
     /* a6 */ {-1, 29}, // PB_29
     /* a7 */ {-1, 28}, // PB_28
     /* b0 */ {-1, 31}, // PB_31
     /* b1 */ {-1, 30}, // PB_30
     /* b2 */ {4, -1},  // CLK_4
     /* b3 */ {4, -1},  // DT_4
     /* b4 */ {-1, 4},  // PB_4
     /* b5 */ {6, -1},  // DT_6
     /* b6 */ {6, -1},  // CLK_6
     /* b7 */ {-1, 6}}, // PB_6

    /* ===================== SHIFTER 2 ===================== */
    {/* a0 */ {-1, 21},  // PB_21
     /* a1 */ {-1, -1},  // ***** LED_P8 (LED_Rec_1)
     /* a2 */ {-1, 24},  // PB_24
     /* a3 */ {-1, 5},   // PB_5
     /* a4 */ {7, -1},   // DT_7
     /* a5 */ {7, -1},   // CLK_7
     /* a6 */ {5, -1},   // DT_5
     /* a7 */ {5, -1},   // CLK_5
     /* b0 */ {-1, 7},   // PB_7
     /* b1 */ {-1, 37},  // PB_37
     /* b2 */ {-1, 17},  // PB_17
     /* b3 */ {-1, 18},  // PB_18
     /* b4 */ {-1, 19},  // PB_19
     /* b5 */ {-1, 20},  // PB_20
     /* b6 */ {-1, 25},  // PB_25
     /* b7 */ {-1, -1}}, // ***** LED_P9 (LED_Rec_2)

    /* ===================== SHIFTER 3 ===================== */
    {/* a0 */ {14, -1}, // CLK_14
     /* a1 */ {14, -1}, // DT_14
     /* a2 */ {-1, 13}, // PB_13
     /* a3 */ {13, -1}, // DT_13
     /* a4 */ {13, -1}, // CLK_13
     /* a5 */ {-1, 12}, // PB_12
     /* a6 */ {12, -1}, // DT_12
     /* a7 */ {12, -1}, // CLK_12
     /* b0 */ {-1, 8},  // PB_8
     /* b1 */ {8, -1},  // CLK_8
     /* b2 */ {8, -1},  // DT_8
     /* b3 */ {-1, 11}, // PB_11
     /* b4 */ {11, -1}, // CLK_11
     /* b5 */ {11, -1}, // DT_11
     /* b6 */ {-1, 14}, // PB_14
     /* b7 */ {-1, -1}},

    /* ===================== SHIFTER 4 ===================== */
    {/* a0 */ {-1, 15}, // PB_15
     /* a1 */ {15, -1}, // CLK_15
     /* a2 */ {15, -1}, // DT_15
     /* a3 */ {-1, -1}, // ***** LED_P11 (LED_Rec_3)
     /* a4 */ {-1, 27}, // PB_27
     /* a5 */ {9, -1},  // DT_9
     /* a6 */ {9, -1},  // CLK_9
     /* a7 */ {-1, 9},  // PB_9
     /* b0 */ {10, -1}, // DT_10
     /* b1 */ {10, -1}, // CLK_10
     /* b2 */ {-1, 10}, // PB_10
     /* b3 */ {-1, 26}, // PB_26
     /* b4 */ {-1, -1}, // ***** LED_P10 (LED_Rec_4)
     /* b5 */ {-1, -1},
     /* b6 */ {-1, 22}, // PB_22
     /* b7 */ {-1, 23}} // PB_23
};

// Encoders an pushbuttons (and switch positions) names
enum EnPbNames : int
{
    EN_PB_TuningTone = 0,
    EN_PB_Resolution = 1,
    EN_PB_Downsampling = 2,
    EN_PB_Cutoff = 3,
    EN_PB_Tempo = 4,
    EN_PB_Loop = 5,
    EN_PB_Track1 = 6,
    EN_PB_Track2 = 7,
    EN_PB_Select = 8,
    EN_PB_Track3 = 9,
    EN_PB_Track4 = 10,
    EN_PB_Value = 11,
    EN_PB_From = 12,
    EN_PB_Step = 13,
    EN_PB_To = 14,
    EN_PB_LineOutVol = 15,
    EN_PB_PreListenVol = 16,
    PB_S2 = 17,
    PB_S3 = 18,
    PB_S4 = 19,
    PB_S5 = 20,
    PB_S6 = 21,
    PB_S7 = 22,
    PB_S8 = 23,
    PB_Rec1 = 24,
    PB_Rec2 = 25,
    PB_Rec4 = 26,
    PB_Rec3 = 27,
    PB_Mixer = 28,
    PB_Delay = 29,
    PB_Setup = 30,
    PB_Test = 31,
    PB_Tools = 32,
    PB_Sampler = 33,
    PB_LiveSampler = 34,
    PB_Performance = 35,
    PB_MidiLoop = 36,
    PB_S1 = 37
};

constexpr EnPbNames EN_PB_Track[TRACKS] = {EN_PB_Track1, EN_PB_Track2, EN_PB_Track3, EN_PB_Track4};
constexpr EnPbNames PB_Rec[TRACKS] = {PB_Rec1, PB_Rec2, PB_Rec3, PB_Rec4};

// LEDs name
enum LedNames : int
{
    LED_Rec_1 = 0,
    LED_Rec_2 = 1,
    LED_Rec_3 = 2,
    LED_Rec_4 = 3,
    LED_Tools = 4
};