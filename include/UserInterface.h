/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#pragma once

#include <Arduino.h>
#include "config.h"

// Encoders, Pushbuttons, Shift Register chips
static constexpr int ENCODERS = 26;
static constexpr int PUSHBUTTONS = 36;
static constexpr int SHIFTERS = 6;          // number of shifter chips
static constexpr int SHIFTER_CHANNELS = 16; // number of channels in a shifter

enum ShifterPort // (input) port id
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

struct Encoder_physical_struct
{
    uint8_t shifter_id;
    ShifterPort DT_shifter_channel;
    ShifterPort CLK_shifter_channel;
};

static constexpr Encoder_physical_struct encoder_physical[ENCODERS] = {
    {2, b7, a0}, // Encoder 0
    {2, a3, a2}, // Encoder 1
    {2, a6, a5}, // Encoder 2
    {1, a0, a1}, // Encoder 3
    {1, a4, a3}, // Encoder 4
    {1, a7, a6}, // Encoder 5
    {0, a3, a2}, // Encoder 6
    {0, a6, a5}, // Encoder 7
    {2, b4, b5}, // Encoder 8
    {5, a7, a6}, // Encoder 9

    {2, b3, b2}, // Encoder 10

    {1, b5, b7}, // Encoder 11
    {1, b4, b3}, // Encoder 12
    {1, b1, b0}, // Encoder 13
    {0, b5, b4}, // Encoder 14
    {0, b2, b1}, // Encoder 15
    {5, b5, b7}, // Encoder 16
    {5, b4, b3}, // Encoder 17
    {5, b1, b0}, // Encoder 18
    {4, a0, a1}, // Encoder 19
    {4, a3, a2}, // Encoder 20
    {4, a6, a7}, // Encoder 21
    {3, a2, a3}, // Encoder 22
    {3, a5, a6}, // Encoder 23
    {3, b5, b4}, // Encoder 24
    {3, b2, b1}  // Encoder 25
};

// Pushbuttons
struct Pushbutton_physical_struct
{
    uint8_t shifter_id;
    ShifterPort shifter_channel;
};

static constexpr Pushbutton_physical_struct pushbutton_physical[PUSHBUTTONS] = {
    {2, a1}, // Pushbutton 0
    {2, a4}, // Pushbutton 1
    {2, a7}, // Pushbutton 2
    {1, a2}, // Pushbutton 3
    {1, a5}, // Pushbutton 4
    {0, a1}, // Pushbutton 5
    {0, a4}, // Pushbutton 6
    {0, a7}, // Pushbutton 7
    {2, b6}, // Pushbutton 8
    {2, b1}, // Pushbutton 9

    {2, b0}, // Pushbutton 10

    {1, b6}, // Pushbutton 11
    {1, b2}, // Pushbutton 12
    {0, b6}, // Pushbutton 13
    {0, b3}, // Pushbutton 14
    {0, b0}, // Pushbutton 15
    {5, b6}, // Pushbutton 16
    {5, a5}, // Pushbutton 17
    {5, b2}, // Pushbutton 18

    {4, b7}, // Pushbutton 19

    {4, a5}, // Pushbutton 20
    {3, a1}, // Pushbutton 21
    {3, a4}, // Pushbutton 22
    {3, a7}, // Pushbutton 23
    {3, b3}, // Pushbutton 24
    {3, b0}, // Pushbutton 25
    {4, b6}, // Pushbutton 26
    {4, b5}, // Pushbutton 27
    {4, b4}, // Pushbutton 28
    {4, b3}, // Pushbutton 29
    {4, b2}, // Pushbutton 30
    {4, b1}, // Pushbutton 31
    {4, b0}, // Pushbutton 32
    {4, a4}, // Pushbutton 33
    {3, b7}, // Pushbutton 34
    {3, b6}  // Pushbutton 35
};

struct EncoderPushbutton
{
    int encoder_id;
    int pushbutton_id;
};

static constexpr EncoderPushbutton Shifter_channel_to_encoder_pushbutton[SHIFTERS][SHIFTER_CHANNELS] = {
    /* ===================== SHIFTER 0 ===================== */
    {/* a0 */ {-1, -1},
     /* a1 */ {-1, 5},
     /* a2 */ {6, -1},
     /* a3 */ {6, -1},
     /* a4 */ {-1, 6},
     /* a5 */ {7, -1},
     /* a6 */ {7, -1},
     /* a7 */ {-1, 7},
     /* b0 */ {-1, 15},
     /* b1 */ {15, -1},
     /* b2 */ {15, -1},
     /* b3 */ {-1, 14},
     /* b4 */ {14, -1},
     /* b5 */ {14, -1},
     /* b6 */ {-1, 13},
     /* b7 */ {-1, -1}},

    /* ===================== SHIFTER 1 ===================== */
    {/* a0 */ {3, -1},
     /* a1 */ {3, -1},
     /* a2 */ {-1, 3},
     /* a3 */ {4, -1},
     /* a4 */ {4, -1},
     /* a5 */ {-1, 4},
     /* a6 */ {5, -1},
     /* a7 */ {5, -1},
     /* b0 */ {13, -1},
     /* b1 */ {13, -1},
     /* b2 */ {-1, 12},
     /* b3 */ {12, -1},
     /* b4 */ {12, -1},
     /* b5 */ {11, -1},
     /* b6 */ {-1, 11},
     /* b7 */ {11, -1}},

    /* ===================== SHIFTER 2 ===================== */
    {/* a0 */ {0, -1},
     /* a1 */ {-1, 0},
     /* a2 */ {1, -1},
     /* a3 */ {1, -1},
     /* a4 */ {-1, 1},
     /* a5 */ {2, -1},
     /* a6 */ {2, -1},
     /* a7 */ {-1, 2},
     /* b0 */ {-1, 10},
     /* b1 */ {-1, 9},
     /* b2 */ {10, -1},
     /* b3 */ {10, -1},
     /* b4 */ {8, -1},
     /* b5 */ {8, -1},
     /* b6 */ {-1, 8},
     /* b7 */ {0, -1}},

    /* ===================== SHIFTER 3 ===================== */
    {/* a0 */ {-1, -1},
     /* a1 */ {-1, 21},
     /* a2 */ {22, -1},
     /* a3 */ {22, -1},
     /* a4 */ {-1, 22},
     /* a5 */ {23, -1},
     /* a6 */ {23, -1},
     /* a7 */ {-1, 23},
     /* b0 */ {-1, 25},
     /* b1 */ {25, -1},
     /* b2 */ {25, -1},
     /* b3 */ {-1, 24},
     /* b4 */ {24, -1},
     /* b5 */ {24, -1},
     /* b6 */ {-1, 35},
     /* b7 */ {-1, 34}},

    /* ===================== SHIFTER 4 ===================== */
    {/* a0 */ {19, -1},
     /* a1 */ {19, -1},
     /* a2 */ {20, -1},
     /* a3 */ {20, -1},
     /* a4 */ {-1, 33},
     /* a5 */ {-1, 20},
     /* a6 */ {21, -1},
     /* a7 */ {21, -1},
     /* b0 */ {-1, 32},
     /* b1 */ {-1, 31},
     /* b2 */ {-1, 30},
     /* b3 */ {-1, 29},
     /* b4 */ {-1, 28},
     /* b5 */ {-1, 27},
     /* b6 */ {-1, 26},
     /* b7 */ {-1, 19}},

    /* ===================== SHIFTER 5 ===================== */
    {/* a0 */ {-1, -1},
     /* a1 */ {-1, -1},
     /* a2 */ {-1, -1},
     /* a3 */ {-1, -1},
     /* a4 */ {-1, -1},
     /* a5 */ {-1, 17},
     /* a6 */ {9, -1},
     /* a7 */ {9, -1},
     /* b0 */ {18, -1},
     /* b1 */ {18, -1},
     /* b2 */ {-1, 18},
     /* b3 */ {17, -1},
     /* b4 */ {17, -1},
     /* b5 */ {16, -1},
     /* b6 */ {-1, 16},
     /* b7 */ {16, -1}}};

// UI devices
constexpr int EN_PB_TuningTone = 7;
constexpr int EN_PB_Resolution = 0;
constexpr int EN_PB_Downsampling = 8;
constexpr int EN_PB_Tempo = 5;
constexpr int EN_PB_Loop = 6;
constexpr int EN_PB_Track1 = 13;
constexpr int EN_PB_Track2 = 14;
constexpr int EN_PB_Track3 = 21;
constexpr int EN_PB_Track4 = 22;
constexpr int EN_PB_Track[4] = {EN_PB_Track1, EN_PB_Track2, EN_PB_Track3, EN_PB_Track4};
constexpr int EN_PB_Select = 25;
constexpr int EN_PB_Value = 24;
constexpr int EN_PB_PreListenVol = 18;
constexpr int EN_PB_From = 1;
constexpr int EN_PB_Step = 2;
constexpr int EN_PB_To = 3;
constexpr int EN_PB_LineOutVol = 17;
constexpr int SEL_Mixer = 27;       // PB_Shift +
constexpr int SEL_Delay = 28;       // PB_Shift +
constexpr int SEL_Setup = 33;       // PB_Shift +
constexpr int SEL_Test = 31;        // PB_Shift +
constexpr int SEL_Sampler = 30;     // PB_Shift +
constexpr int SEL_LiveSampler = 29; // PB_Shift +
constexpr int SEL_Performance = 26; // PB_Shift +
constexpr int SEL_MidiLoop = 32;    // PB_Shift +
constexpr int PB_SwitchTo = 35;
constexpr int PB_Shift = 35;
constexpr int PB_Rec1 = 9;
constexpr int PB_Rec2 = 10;
constexpr int PB_Rec3 = 11;
constexpr int PB_Rec4 = 12;
constexpr int PB_S1 = 26;
constexpr int PB_S2 = 27;
constexpr int PB_S3 = 28;
constexpr int PB_S4 = 29;
constexpr int PB_S5 = 30;
constexpr int PB_S6 = 31;
constexpr int PB_S7 = 32;
constexpr int PB_S8 = 33;