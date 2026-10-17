/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "ShiftRegisters.h"
#include <initializer_list>
#include <SPI.h>

// Four-second cosine cycle, starting at full brightness; duty cycle ranges from 40% to 100%.
const uint16_t ShiftRegisters::LED_BREATH_ON_US[LED_BREATH_STEPS] = {10000, 9994, 9976, 9947, 9906, 9853, 9789, 9714, 9629, 9533, 9427, 9312, 9187, 9054, 8912, 8763, 8607, 8445, 8277, 8104, 7927, 7746, 7562, 7376, 7188, 7000, 6812, 6624, 6438, 6254, 6073, 5896, 5723, 5555, 5393, 5237, 5088, 4946, 4813, 4688, 4573, 4467, 4371, 4286, 4211, 4147, 4094, 4053, 4024, 4006, 4000, 4006, 4024, 4053, 4094, 4147, 4211, 4286, 4371, 4467, 4573, 4688, 4813, 4946, 5088, 5237, 5393, 5555, 5723, 5896, 6073, 6254, 6438, 6624, 6812, 7000, 7188, 7376, 7562, 7746, 7927, 8104, 8277, 8445, 8607, 8763, 8912, 9054, 9187, 9312, 9427, 9533, 9629, 9714, 9789, 9853, 9906, 9947, 9976, 9994};

FLASHMEM
void ShiftRegisters::Begin(void)
{
    // Deselect both devices before enabling their shared hardware SPI bus.
    digitalWrite(SPI1_DISPLAY_CS, HIGH);
    pinMode(SPI1_DISPLAY_CS, OUTPUT);
    digitalWrite(SPI1_SHIFTERS_CS, HIGH);
    pinMode(SPI1_SHIFTERS_CS, OUTPUT);
    SPI1.setMOSI(SPI1_MOSI);
    SPI1.setSCK(SPI1_SCLK);
    SPI1.setMISO(SPI1_MISO);
    Start_SPI_for_shifters();
    Setup_physical_channels();
    Switch_all_leds(false);
}

FLASHMEM
void ShiftRegisters::Pulse_startup_leds(void)
{
    constexpr uint32_t pwm_period_us = 10000; // 100 Hz carrier.
    constexpr uint32_t pulse_period_us = 1000000;
    constexpr uint32_t duration_us = 2 * pulse_period_us;
    // One-second sinusoidal envelope: round(255 * (1 - cos(2 * pi * phase)) / 2).
    static constexpr uint8_t envelope[] = {0, 0, 1, 2, 4, 6, 9, 12, 16, 20, 24, 29, 35, 40, 46, 53, 59, 66, 73, 81, 88, 96, 104, 112, 119, 128, 136, 143, 151, 159, 167, 174, 182, 189, 196, 202, 209, 215, 220, 226, 231, 235, 239, 243, 246, 249, 251, 253, 254, 255, 255, 255, 254, 253, 251, 249, 246, 243, 239, 235, 231, 226, 220, 215, 209, 202, 196, 189, 182, 174, 167, 159, 151, 143, 136, 128, 119, 112, 104, 96, 88, 81, 73, 66, 59, 53, 46, 40, 35, 29, 24, 20, 16, 12, 9, 6, 4, 2, 1, 0};
    static_assert(sizeof(envelope) / sizeof(envelope[0]) == pulse_period_us / pwm_period_us);
    Switch_all_leds(false);
    bool output_on = false;
    const uint32_t start_us = micros();

    // Startup only: audio processing and display initialization have not started yet.
    while (true)
    {
        const uint32_t elapsed_us = micros() - start_us;
        if (elapsed_us >= duration_us)
        {
            break;
        }

        const uint32_t pulse_phase_us = (elapsed_us / pwm_period_us * pwm_period_us) % pulse_period_us;
        const uint8_t level = envelope[pulse_phase_us / pwm_period_us];
        const uint32_t on_us = static_cast<uint32_t>(level) * pwm_period_us / 255;
        const bool on = (elapsed_us % pwm_period_us) < on_us;
        if (on != output_on)
        {
            for (int led = 0; led < UI_LEDS; ++led)
            {
                Shifter[UI_leds[led].shifter_id].digitalWrite(UI_leds[led].shifter_channel, on ? LOW : HIGH);
            }
            output_on = on;
        }
    }

    Switch_all_leds(false);
}

FLASHMEM
void ShiftRegisters::Start_SPI_for_shifters(void)
{
    for (auto i = 0; i < SHIFTERS; ++i)
    {
        Shifter[i].begin_SPI(SPI1_SHIFTERS_CS, &SPI1, SHIFTER_ADDRESS[i]);

        // Needed??
        Shifter[i].enableAddrPins();
        Shifter[i].enableAddrPins();
    }
}

void ShiftRegisters::Reset_shifters_channels(void)
{
    for (auto i = 0; i < SHIFTERS; ++i)
    {
        shifter_channel_value[Old][i] = 0b1111111111111111;
    }
}

void ShiftRegisters::Reset_monitored_channels(void)
{
    for (auto i = 0; i < SHIFTERS; ++i)
    {
        monitored_channels[i] = 0;
    }
}

FLASHMEM
void ShiftRegisters::Setup_physical_channels(void)
{
    // Set all channels as INPUT with internal PULLUP (100kohm)
    for (auto i = 0; i < SHIFTERS; ++i)
    {
        for (auto j = 0; j < 16; ++j)
        {
            Shifter[i].pinMode(j, INPUT_PULLUP);
        }
    }

    // Set UI_LEDS channels as OUTPUT
    for (auto i = 0; i < UI_LEDS; ++i)
    {
        Shifter[UI_leds[i].shifter_id].pinMode(UI_leds[i].shifter_channel, OUTPUT);
    }
}

void ShiftRegisters::Switch_all_leds(bool on)
{
    for (int led = LED_Rec_1; led <= LED_Tools; ++led)
    {
        Switch_led(static_cast<LedNames>(led), on);
    }
}

void ShiftRegisters::Switch_led(LedNames led, bool on)
{
    if (led < 0 || led >= UI_LEDS)
    {
        Serial.println(F("Switch_led(int led, bool on) ERROR! Invalid led."));
        return;
    }

    if (led < PWM_LEDS)
    {
        if (on && led_requested_on[led])
        {
            return;
        }
        led_requested_on[led] = on;
        led_output_on[led] = on;
        led_cycle_start_us[led] = micros();
    }

    Shifter[UI_leds[led].shifter_id].digitalWrite(UI_leds[led].shifter_channel, (on ? LOW : HIGH));
}

void ShiftRegisters::Update_led_pwm(void)
{
    const uint32_t now_us = micros();
    for (int led = 0; led < PWM_LEDS; ++led)
    {
        if (!led_requested_on[led])
        {
            continue;
        }

        const uint32_t elapsed_us = now_us - led_cycle_start_us[led];
        const uint32_t phase_us = elapsed_us % LED_BREATH_PERIOD_US;
        // Advance the epoch each cycle to keep unsigned micros() rollover safe.
        led_cycle_start_us[led] += elapsed_us - phase_us;
        const uint32_t step = phase_us / (LED_BREATH_PERIOD_US / LED_BREATH_STEPS);
        const bool on = (phase_us % LED_PWM_PERIOD_US) < LED_BREATH_ON_US[step];
        if (on != led_output_on[led])
        {
            Shifter[UI_leds[led].shifter_id].digitalWrite(UI_leds[led].shifter_channel, (on ? LOW : HIGH));
            led_output_on[led] = on;
        }
    }
}

FLASHMEM
void ShiftRegisters::Set_monitored_encoders(void)
{
    Serial.print(F("monitored_encoders, BIN: "));
    Serial.println(monitored_encoders, BIN);

    for (auto i = 0; i < ENCODERS; ++i)
    {
        if (bitRead(monitored_encoders, i))
        {
            const auto shifter_id = encoder_physical[i].shifter_id;
            bitWrite(monitored_channels[shifter_id], encoder_physical[i].DT_shifter_channel, 1);
            bitWrite(monitored_channels[shifter_id], encoder_physical[i].CLK_shifter_channel, 1);
        }
    }
}

FLASHMEM
void ShiftRegisters::Set_monitored_switches(void)
{
    Serial.print(F("monitored_switches, BIN: "));
    Serial.println(monitored_switches, BIN);

    for (auto i = 0; i < SWITCHES; ++i)
    {
        if (bitRead(monitored_switches, i))
        {
            const auto shifter_id = switch_physical[i].shifter_id;
            bitWrite(monitored_channels[shifter_id], switch_physical[i].A_pin_shifter_channel, 1);
            bitWrite(monitored_channels[shifter_id], switch_physical[i].B_pin_shifter_channel, 1);
            bitWrite(monitored_channels[shifter_id], switch_physical[i].C_pin_shifter_channel, 1);
            bitWrite(monitored_channels[shifter_id], switch_physical[i].D_pin_shifter_channel, 1);
        }
    }
}

FLASHMEM
void ShiftRegisters::Set_monitored_pushbuttons(void)
{
    Serial.print(F("monitored_pushbuttons, BIN: "));
    Serial.println(monitored_pushbuttons, BIN);

    for (auto i = 0; i < PUSHBUTTONS; ++i)
    {
        if (bitRead(monitored_pushbuttons, i))
        {
            const auto shifter_id = pushbutton_physical[i].shifter_id;
            bitWrite(monitored_channels[shifter_id], pushbutton_physical[i].shifter_channel, 1);
        }
    }
}

static uint32_t Make_encoders_mask(std::initializer_list<int> list)
{
    uint32_t mask = 0;
    for (int idx : list)
    {
        if (idx < ENCODERS)
        {
            bitWrite(mask, idx, 1);
        }
    }
    return mask;
}

static uint64_t Make_pushbuttons_mask(std::initializer_list<int> list)
{
    uint64_t mask = 0;
    for (int idx : list)
    {
        if (idx < PUSHBUTTONS)
        {
            mask |= (uint64_t(1) << idx);
        }
    }
    return mask;
}

static uint8_t Make_switches_mask(std::initializer_list<int> list)
{
    uint8_t mask = 0;
    for (int idx : list)
    {
        if (idx < SWITCHES)
        {
            mask |= (uint8_t(1) << idx);
        }
    }
    return mask;
}

FLASHMEM
void ShiftRegisters::Init_controller_masks(void)
{
    monitored_encoders = Make_encoders_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_Tempo, EN_PB_Loop, EN_PB_Track1, EN_PB_Track2,
                                             EN_PB_Select, EN_PB_Track3, EN_PB_Track4, EN_PB_Value, EN_PB_From, EN_PB_Step, EN_PB_To, EN_PB_LineOutVol, EN_PB_PreListenVol});

    monitored_pushbuttons = Make_pushbuttons_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_Tempo, EN_PB_Loop, EN_PB_Track1, EN_PB_Track2,
                                                   EN_PB_Select, EN_PB_Track3, EN_PB_Track4, EN_PB_Value, EN_PB_From, EN_PB_Step, EN_PB_To, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                   PB_S2, PB_S3, PB_S4, PB_S5, PB_S6, PB_S7, PB_S8, PB_Rec1, PB_Rec2, PB_Rec4, PB_Rec3, PB_Tools, PB_S1});
    monitored_switches = Make_switches_mask({SwitchTools, SwitchModes});
}

FLASHMEM
void ShiftRegisters::Monitor_all_controllers(void)
{
    Set_monitored_encoders_pushbuttons_switches();
}

FLASHMEM
void ShiftRegisters::Set_monitored_encoders_pushbuttons_switches()
{
    Reset_monitored_channels();
    Set_monitored_encoders();
    Set_monitored_pushbuttons();
    Set_monitored_switches();

    if (true)
    {
        Serial.println(F("Monitored shift register chips channeles"));
        Serial.println(F("b7  b6  b5  b4  b3  b2  b1  b0  a7  a6  a5  a4  a3  a2  a1  a0"));
        
        for (auto i = 0; i < SHIFTERS; ++i)
        {
            for (auto j = (SHIFTER_CHANNELS - 1); j >= 0; --j)
            {
                Serial.print(bitRead(monitored_channels[i], j));
                Serial.print("   ");
            }
            Serial.println();
        }
    }
}

void ShiftRegisters::Read_channels(const int &shifter_id)
{
    shifter_channel_value[Last][shifter_id] = Shifter[shifter_id].readGPIOAB();
}

void ShiftRegisters::Filter_channels_changed_values(const int &shifter_id)
{
    // XOR marks each channel whose current state differs from the previous scan.
    shifter_channel_value[Changed][shifter_id] = shifter_channel_value[Last][shifter_id] ^ shifter_channel_value[Old][shifter_id];

    // Keep only changed channels that belong to monitored devices.
    shifter_channel_value[Filtered][shifter_id] = shifter_channel_value[Changed][shifter_id] & monitored_channels[shifter_id];

    shifter_channel_value[Old][shifter_id] = shifter_channel_value[Last][shifter_id];
    // Serial.println(shifter_channel_value[Filter][shifter_id], BIN);
}

void ShiftRegisters::Update(void)
{
    Update_led_pwm();

    // Parse all shifter registers
    for (auto shifter_id = 0; shifter_id < SHIFTERS; ++shifter_id)
    {
        Read_channels(shifter_id); // read physical channels
        Filter_channels_changed_values(shifter_id);

        if (false)
        {
            if (shifter_channel_value[Filtered][shifter_id] != 0)
            {
                Serial.print("shifter_id: ");
                Serial.println(shifter_id);
                Serial.print(shifter_channel_value[Old][shifter_id], BIN);
                Serial.print(" - ");
                Serial.println(shifter_channel_value[Last][shifter_id], BIN);
            }
        }

        // Parse channels and transmit changes
        for (auto channel = 0; channel < SHIFTER_CHANNELS; ++channel)
        {
            if (bitRead(shifter_channel_value[Filtered][shifter_id], channel))
            {
                const auto &device = Shifter_channel_to_device[shifter_id][channel];

                switch (device.device_type)
                {
                case Encoder:
                {
                    const int DT_channel = encoder_physical[device.device_id].DT_shifter_channel;
                    const int CLK_channel = encoder_physical[device.device_id].CLK_shifter_channel;
                    Encoders_manager.Transmit_DT_CLK(device.device_id, bitRead(shifter_channel_value[Last][shifter_id], DT_channel), bitRead(shifter_channel_value[Last][shifter_id], CLK_channel));

                    // Both encoder channels are handled together; clear them to avoid a second transmission when the loop reaches the other changed channel.
                    bitWrite(shifter_channel_value[Filtered][shifter_id], DT_channel, 0);
                    bitWrite(shifter_channel_value[Filtered][shifter_id], CLK_channel, 0);
                }
                break;

                case Pushbutton:
                {
                    Pushbuttons_manager.Transmit_position(device.device_id, bitRead(shifter_channel_value[Last][shifter_id], channel));
                }
                break;

                case Switch:
                {
                    const int A_channel = switch_physical[device.device_id].A_pin_shifter_channel;
                    const int B_channel = switch_physical[device.device_id].B_pin_shifter_channel;
                    const int C_channel = switch_physical[device.device_id].C_pin_shifter_channel;
                    const int D_channel = switch_physical[device.device_id].D_pin_shifter_channel;

                    auto value = bitRead(shifter_channel_value[Last][shifter_id], A_channel) |
                                 (bitRead(shifter_channel_value[Last][shifter_id], B_channel) << 1) |
                                 (bitRead(shifter_channel_value[Last][shifter_id], C_channel) << 2) |
                                 (bitRead(shifter_channel_value[Last][shifter_id], D_channel) << 3);

                    bitWrite(shifter_channel_value[Filtered][shifter_id], A_channel, 0);
                    bitWrite(shifter_channel_value[Filtered][shifter_id], B_channel, 0);
                    bitWrite(shifter_channel_value[Filtered][shifter_id], C_channel, 0);
                    bitWrite(shifter_channel_value[Filtered][shifter_id], D_channel, 0);

                    Switches_manager.Transmit_contacts(device.device_id, value);
                }
                break;

                case NoDevice:
                break;

                default:
                    break;
                }
            }
        }
    }
}
