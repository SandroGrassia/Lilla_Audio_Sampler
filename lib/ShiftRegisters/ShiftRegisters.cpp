/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "ShiftRegisters.h"
#include <initializer_list>
#include <SPI.h>

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

    Shifter[UI_leds[led].shifter_id].digitalWrite(UI_leds[led].shifter_channel, (on ? LOW : HIGH));
}

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

void ShiftRegisters::Init_controller_masks(void)
{
    monitored_encoders = Make_encoders_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_Tempo, EN_PB_Loop, EN_PB_Track1, EN_PB_Track2,
                                             EN_PB_Select, EN_PB_Track3, EN_PB_Track4, EN_PB_Value, EN_PB_From, EN_PB_Step, EN_PB_To, EN_PB_LineOutVol, EN_PB_PreListenVol});

    monitored_pushbuttons = Make_pushbuttons_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_Tempo, EN_PB_Loop, EN_PB_Track1, EN_PB_Track2,
                                                   EN_PB_Select, EN_PB_Track3, EN_PB_Track4, EN_PB_Value, EN_PB_From, EN_PB_Step, EN_PB_To, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                   PB_S2, PB_S3, PB_S4, PB_S5, PB_S6, PB_S7, PB_S8, PB_Rec1, PB_Rec2, PB_Rec4, PB_Rec3, PB_Tools, PB_S1});
    monitored_switches = Make_switches_mask({SwitchTools, SwitchModes});
}

void ShiftRegisters::Monitor_all_controllers(void)
{
    Set_monitored_encoders_pushbuttons_switches();
}

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
