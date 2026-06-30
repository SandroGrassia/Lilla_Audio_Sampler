/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#pragma once

#include <Arduino.h>
#include "config.h"
#include "UserInterface.h"

class Pushbuttons
{
private:
    enum State
    {
        down, // 0
        up    // 1
    };
    struct NextStateAndOutput
    {
        State next_state;
        bool changed;
        bool restart_timer;
    };
    
    // Pushbutton debounce/state transition table. Rows are indexed by the current sampled position: 0 = pressed, 1 = released.
    // Columns are the previous stable state; each cell returns the next state, the one-shot press output, and the debounce timer restart flag.
    static constexpr NextStateAndOutput matrix[2][2] =
        {
  //               old: down           old: up
            {{down, false, false}, {down, true, true}}, // new: down
            {{up, false, true}, {up, false, false}}     // new: up
    };
    // Graph:
    // https://magjac.com/graphviz-visual-editor/?dot=digraph%7B%0Anode%20%5Bstyle%3Dfilled%20fontname%3D%22arial%22%20fontsize%3D24%20width%3D0%5D%3B%0Aedge%20%5Barrowhead%3Dopen%20fontsize%3D12%20alingment%3Dleft%20fontname%3D%22arial%20narrow%22%20fillcolor%3D%22%23ffffff%22%5D%3B%0A%20%20%20%0A%20%20%20%20down%20%5Bshape%3D%22circle%22%20style%3D%22filled%22%20fillcolor%3D%22%23ffffff%22%5D%0A%20%20%20%20up%20%5Bshape%3D%22circle%22%20style%3D%22filled%22%20fillcolor%3D%22%23ffffff%22%5D%0A%0A%20%20%20%20%0A%20%20%20%20down%20-%3E%20down%20%5Blabel%20%3D%20%220%22%20fillcolor%3D%22%23000000%22%20color%3D%22%23000000%22%5D%0A%20%20%20%20down%20-%3E%20up%20%5Blabel%20%3D%20%221%2Frestart_timer%22%20fillcolor%3D%22%23000000%22%20color%3D%22%23000000%22%5D%0A%20%20%20%20%0A%20%20%20%20up%20-%3E%20up%20%5Blabel%20%3D%20%221%22%20fillcolor%3D%22%23000000%22%20color%3D%22%23000000%22%5D%0A%20%20%20%20up%20-%3E%20down%20%5Blabel%20%3D%20%220%2Ftrue%2Frestart_timer%22%20fillcolor%3D%22%23000000%22%20color%3D%22%23000000%22%5D%0A%0A%0A%7D
    
    static constexpr int PAUSE_PB = 50; // milliseconds

    State state[PUSHBUTTONS]; // 0: pushbutton is actually pressed  1: pushbutton is actually released
    bool changed[PUSHBUTTONS]; // output goes to true at the first pressed event, than returns to false
    uint32_t timer[PUSHBUTTONS];

public:
    Pushbuttons()
    {
        Reset();
    }

    void Transmit_position(const uint8_t &pushbutton, const uint8_t &position); // position -->  0: pressed, 1: released
    bool Get_state(const uint8_t &pushbutton);
    bool Get_change(const uint8_t &pushbutton);
    void Reset(void);
};
