/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

/*
Lilla PCB2025_R2
This model includes 6 Shift Registers (Shifters_manager) MCP23S17 (SPI communication)
- each Shifter address matches with its id (0 to 6)
- Shiters' channels are configured with INPUT_PULLUP
- each Encoder (DT,CLK,PB) is connected to a single Shifter
*/

/*
At startup
- main.cpp creates the Encoders_manager and Pushbuttons_manager objects
- main.cpp creates the ShiftRegisters object (Shifters_manager) sending the &Encoders pointer
- Shifters_manager creates the Shifter[SHIFTERS] ojbects
- Shifters_manager calls Init_controller_masks() from the constructor to populate the global controller bitmasks

For each loop():
- main.cpp calls Update()
- Shifters_manager calls the shifters and reads the status of the DT/CLK and "PB" pins of the monitored_encoders and monitored_pushbuttons_switches
- Shifters_manager send the filterd data to EncordersObj and Pushbuttons_manager
- main.cpp calls  EncordersObj and Pushbuttons_manager ...

All controllers are monitored in every Lilla_state, so state changes do not rebuild the monitored encoder/pushbutton/switch set.
*/

#pragma once

#include <Arduino.h>
#include "config.h"
#include <Adafruit_MCP23X17.h> // Shifters
#include "UserInterface.h"
#include "Encoders.h"
#include "Pushbuttons.h"
#include "Switches.h"


class ShiftRegisters
{
private:
    static constexpr uint8_t SHIFTER_ADDRESS[SHIFTERS] = {0x20, 0x21, 0x22, 0x23, 0x24};

    Adafruit_MCP23X17 Shifter[SHIFTERS]; // Physical shift registers
    Encoders &Encoders_manager;
    Pushbuttons &Pushbuttons_manager;
    Switches &Switches_manager;

    static constexpr int PWM_LEDS = 4;
    static constexpr uint32_t LED_PWM_PERIOD_US = 10000; // 100 Hz carrier.
    static constexpr uint32_t LED_BREATH_PERIOD_US = 4000000;
    static constexpr int LED_BREATH_STEPS = 100;
    static const uint16_t LED_BREATH_ON_US[LED_BREATH_STEPS];
    bool led_requested_on[PWM_LEDS] = {};
    bool led_output_on[PWM_LEDS] = {};
    uint32_t led_cycle_start_us[PWM_LEDS] = {};

    // Cooperative PWM: called only by Update() in the main loop, never by an ISR.
    void Update_led_pwm(void);

    static constexpr int STATES = 4;
    enum Type
    {
        Last,
        Old,
        Changed,
        Filtered
    };

    uint16_t shifter_channel_value[STATES][SHIFTERS]; // Used for caching Shifter[shifter_id].readGPIOAB(); 0: old read; 1: last read; 2: filtered (old EXOR last)
    
    // Variable used as lookup table
    uint32_t monitored_encoders;    // initial ENCODERS (17) bits, from 0 to 16, correspond to encoders (all other bits are ignored): 0b 00000000 0000000X XXXXXXXX XXXXXXXX  -  X=1: encoder monitored, X=0: encoder excluded
    uint64_t monitored_pushbuttons; // initial PUSHBUTTONS (38) bits, from 0 to 37, correspond to pushbuttons (all other bits are ignored): 0b 00000000 00000000 00000000  00XXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX  - X=1: pushbutton monitored, X=0: pushbutton excluded
    uint8_t monitored_switches; // initial SWITCHES (2) bits, from 0 to 1, correspond to switches (all other bits are ignored): 0b 000000XX  - X=1: switch monitored, X=0: switch excluded

    uint16_t monitored_channels[SHIFTERS];

    // Setup physical shifter
    FLASHMEM void Start_SPI_for_shifters(void);
    FLASHMEM void Setup_physical_channels(void);

    // Initializes the previous-scan cache as all channels open/high, matching the MCP23S17 input pull-up idle state.
    void Reset_shifters_channels(void);

    // Clears the active shifter/channel monitoring masks before rebuilding them for the current UI context.
    void Reset_monitored_channels(void);

    // Reads the full 16-bit GPIO state from one physical MCP23S17 shifter into the latest-scan cache.
    void Read_channels(const int &id);

    // Detects which channels changed since the previous scan, keeps only the monitored ones, then promotes the last scan to the old state.
    void Filter_channels_changed_values(const int &id);

    FLASHMEM void Set_monitored_encoders(void);
    FLASHMEM void Set_monitored_pushbuttons(void);
    FLASHMEM void Set_monitored_switches(void);
    FLASHMEM void Set_monitored_encoders_pushbuttons_switches(void);
    FLASHMEM void Init_controller_masks(void);

public:
    ShiftRegisters(Encoders &EncsObj, Pushbuttons &PbsObj, Switches &SwcObj) : Encoders_manager(EncsObj), Pushbuttons_manager(PbsObj), Switches_manager(SwcObj)
    {
        Reset_shifters_channels();
        Init_controller_masks();
    }

    FLASHMEM void Begin(void);
    FLASHMEM void Pulse_startup_leds(void);
    FLASHMEM void Monitor_all_controllers(void);
    void Update(void);
    void Switch_all_leds(bool on);
    void Switch_led(LedNames led, bool on);
};
