/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "ShiftRegisters.h"
#include <initializer_list>

void ShiftRegisters::Start_SPI_for_shifters(void)
{
    for (auto i = 0; i < SHIFTERS; ++i)
    {
        Shifter[i].begin_SPI(SPI1_SHIFTERS_CS, SPI1_SCLK, SPI1_MISO, SPI1_MOSI, SHIFTER_ADDRESS[i]); // (int8_t cs_pin, int8_t sck_pin, int8_t miso_pin, int8_t mosi_pin, uint8_t _hw_addr = 0x00);

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
    monitored_shifters = 0;
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
    for (auto led = 0; led < UI_LEDS; ++led)
    {
        Switch_led(led, on);
    }
}

void ShiftRegisters::Switch_led(int led, bool on)
{
    if (led < 0 || led >= UI_LEDS)
    {
        Serial.println(F("Switch_led(int led, bool on) ERROR! Invalid led."));
        return;
    }

    Shifter[UI_leds[led].shifter_id].digitalWrite(UI_leds[led].shifter_channel, (on ? LOW : HIGH));
}

void ShiftRegisters::Set_monitored_encoders(const uint32_t &data)
{
    monitored_encoders = data;

    Serial.print(F("monitored_encoders, BIN: "));
    Serial.println(monitored_encoders, BIN);

    for (auto i = 0; i < ENCODERS; ++i)
    {
        if (bitRead(monitored_encoders, i))
        {
            const auto shifter_id = encoder_physical[i].shifter_id;
            bitWrite(monitored_shifters, shifter_id, 1);
            bitWrite(monitored_channels[shifter_id], encoder_physical[i].DT_shifter_channel, 1);
            bitWrite(monitored_channels[shifter_id], encoder_physical[i].CLK_shifter_channel, 1);
        }
    }
}

void ShiftRegisters::Set_monitored_switches(const uint8_t &data)
{
    monitored_switches = data;

    Serial.print(F("monitored_switches, BIN: "));
    Serial.println(monitored_switches, BIN);

    for (auto i = 0; i < SWITCHES; ++i)
    {
        if (bitRead(monitored_switches, i))
        {
            const auto shifter_id = switch_physical[i].shifter_id;
            bitWrite(monitored_shifters, shifter_id, 1);
            bitWrite(monitored_channels[shifter_id], switch_physical[i].A_pin_shifter_channel, 1);
            bitWrite(monitored_channels[shifter_id], switch_physical[i].B_pin_shifter_channel, 1);
            bitWrite(monitored_channels[shifter_id], switch_physical[i].C_pin_shifter_channel, 1);
            bitWrite(monitored_channels[shifter_id], switch_physical[i].D_pin_shifter_channel, 1);
        }
    }
}

void ShiftRegisters::Set_monitored_pushbuttons(const uint64_t &data)
{
    monitored_pushbuttons = data;

    Serial.print(F("monitored_pushbuttons, BIN: "));
    Serial.println(monitored_pushbuttons, BIN);

    for (auto i = 0; i < PUSHBUTTONS; ++i)
    {
        if (bitRead(monitored_pushbuttons, i))
        {
            const auto shifter_id = pushbutton_physical[i].shifter_id;
            bitWrite(monitored_shifters, shifter_id, 1);
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

void ShiftRegisters::Init_context_sets(void)
{
    const auto all_switches = Make_switches_mask({SwitchTools, SwitchModes});

    context_encoders[Start_context] = Make_encoders_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_Tempo, EN_PB_Loop, EN_PB_Track1, EN_PB_Track2,
                                                          EN_PB_Select, EN_PB_Track3, EN_PB_Track4, EN_PB_Value, EN_PB_From, EN_PB_Step, EN_PB_To, EN_PB_LineOutVol, EN_PB_PreListenVol});

    context_pushbuttons[Start_context] = Make_pushbuttons_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_Tempo, EN_PB_Loop, EN_PB_Track1, EN_PB_Track2,
                                                                EN_PB_Select, EN_PB_Track3, EN_PB_Track4, EN_PB_Value, EN_PB_From, EN_PB_Step, EN_PB_To, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                                PB_S2, PB_S3, PB_S4, PB_S5, PB_S6, PB_S7, PB_S8, PB_Rec1, PB_Rec2, PB_Rec4, PB_Rec3, PB_Tools, PB_S1});
    context_switches[Start_context] = all_switches;

    context_encoders[Performance_context] = Make_encoders_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                                EN_PB_Select, EN_PB_Value, EN_PB_From, EN_PB_To});

    context_pushbuttons[Performance_context] = Make_pushbuttons_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                                      EN_PB_Select, EN_PB_Value, EN_PB_From, EN_PB_To, PB_S1, PB_S2, PB_S3, PB_S4, PB_S5, PB_S6, PB_S7, PB_S8, PB_Tools});
    context_switches[Performance_context] = all_switches;

    context_encoders[Sound_edit_context] = Make_encoders_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                               EN_PB_Select, EN_PB_Value, EN_PB_From, EN_PB_Step, EN_PB_To});

    context_pushbuttons[Sound_edit_context] = Make_pushbuttons_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                                     EN_PB_Select, EN_PB_Value, EN_PB_From, EN_PB_Step, EN_PB_To, PB_S1, PB_S2, PB_S3, PB_S4, PB_S5, PB_S6, PB_S7, PB_S8, PB_Tools});
    context_switches[Sound_edit_context] = all_switches;

    context_encoders[Instrument_Vcf_context] = Make_encoders_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                                   EN_PB_Select, EN_PB_Value});

    context_pushbuttons[Instrument_Vcf_context] = Make_pushbuttons_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                                         EN_PB_Select, EN_PB_Value, PB_S1, PB_S2, PB_S3, PB_S4, PB_S5, PB_S6, PB_S7, PB_S8, PB_Tools});
    context_switches[Instrument_Vcf_context] = all_switches;

    context_encoders[Mixer_context] = Make_encoders_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                          EN_PB_Select, EN_PB_Value});
    context_pushbuttons[Mixer_context] = Make_pushbuttons_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                                EN_PB_Select, EN_PB_Value, PB_S1, PB_S2, PB_S3, PB_S4, PB_S5, PB_S6, PB_S7, PB_S8, PB_Tools});
    context_switches[Mixer_context] = all_switches;

    context_encoders[Delay_settings_context] = Make_encoders_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                                   EN_PB_Select, EN_PB_Value});
    context_pushbuttons[Delay_settings_context] = Make_pushbuttons_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                                         EN_PB_Select, EN_PB_Value, PB_S1, PB_S2, PB_S3, PB_S4, PB_S5, PB_S6, PB_S7, PB_S8, PB_Tools});
    context_switches[Delay_settings_context] = all_switches;

    context_encoders[Live_Sampling_context] = Make_encoders_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                                  EN_PB_Select, EN_PB_Value, EN_PB_From, EN_PB_Step, EN_PB_To});
    context_pushbuttons[Live_Sampling_context] = Make_pushbuttons_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                                        EN_PB_Select, EN_PB_Value, EN_PB_From, EN_PB_Step, EN_PB_To, PB_S1, PB_S2, PB_S3, PB_S4, PB_S5, PB_S6, PB_S7, PB_S8, PB_Tools});
    context_switches[Live_Sampling_context] = all_switches;

    context_encoders[Direct_Sampling_context] = Make_encoders_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                                    EN_PB_Select, EN_PB_Value});
    context_pushbuttons[Direct_Sampling_context] = Make_pushbuttons_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                                          EN_PB_Select, EN_PB_Value, PB_S1, PB_S2, PB_S3, PB_S4, PB_S5, PB_S6, PB_S7, PB_S8, PB_Tools});
    context_switches[Direct_Sampling_context] = all_switches;

    context_encoders[Midi_Monitor_context] = Make_encoders_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol});
    context_pushbuttons[Midi_Monitor_context] = Make_pushbuttons_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol, PB_Tools});
    context_switches[Midi_Monitor_context] = all_switches;

    context_encoders[Midi_Loop_context] = Make_encoders_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                              EN_PB_Select, EN_PB_Value, EN_PB_Tempo, EN_PB_Loop, EN_PB_Track1, EN_PB_Track2, EN_PB_Track3, EN_PB_Track4});
    context_pushbuttons[Midi_Loop_context] = Make_pushbuttons_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                                    EN_PB_Select, EN_PB_Value, PB_Tools, EN_PB_Tempo, EN_PB_Loop, EN_PB_Track1, EN_PB_Track2, EN_PB_Track3, EN_PB_Track4,
                                                                    PB_Rec1, PB_Rec2, PB_Rec3, PB_Rec4});
    context_switches[Midi_Loop_context] = all_switches;

    context_encoders[Setup_context] = Make_encoders_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                          EN_PB_Select, EN_PB_Value});
    context_pushbuttons[Setup_context] = Make_pushbuttons_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol, EN_PB_Select, EN_PB_Value, PB_Tools});
    context_switches[Setup_context] = all_switches;

    context_encoders[Control_Change_context] = Make_encoders_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                                   EN_PB_Select, EN_PB_Value});
    context_pushbuttons[Control_Change_context] = Make_pushbuttons_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol, EN_PB_Select, EN_PB_Value, PB_Tools});
    context_switches[Control_Change_context] = all_switches;
}

void ShiftRegisters::Set_context(LillaContext context)
{
    if (context < 0 || context >= LILLA_CONTEXTS)
    {
        Serial.println(F("Set_context(LillaContext context) ERROR! Invalid context."));
        return;
    }

    Set_monitored_encoders_pushbuttons_switches(context_encoders[context], context_pushbuttons[context], context_switches[context]);
}

void ShiftRegisters::Set_monitored_encoders_pushbuttons_switches(const uint32_t &enc, const uint64_t &pb, const uint8_t &sw)
{
    Reset_monitored_channels();
    Set_monitored_encoders(enc);
    Set_monitored_pushbuttons(pb);
    Set_monitored_switches(sw);

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

    // Keep only changed channels that belong to monitored encoders and pushbuttons.
    shifter_channel_value[Filtered][shifter_id] = shifter_channel_value[Changed][shifter_id] & monitored_channels[shifter_id];

    shifter_channel_value[Old][shifter_id] = shifter_channel_value[Last][shifter_id];
    // Serial.println(shifter_channel_value[Filter][shifter_id], BIN);
}

void ShiftRegisters::Update(void)
{
    // Parse all shifter registers
    for (auto shifter_id = 0; shifter_id < SHIFTERS; ++shifter_id)
    {
        // Read only the monitored shift registers
        if (bitRead(monitored_shifters, shifter_id))
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
                    const auto encoder_id = Shifter_channel_to_encoder_pushbutton_switch[shifter_id][channel].encoder_id;
                    const auto pushbutton_id = Shifter_channel_to_encoder_pushbutton_switch[shifter_id][channel].pushbutton_id;
                    const auto switch_id = Shifter_channel_to_encoder_pushbutton_switch[shifter_id][channel].switch_id;

                    if (encoder_id > -1)
                    {
                        const int DT_channel = encoder_physical[encoder_id].DT_shifter_channel;
                        const int CLK_channel = encoder_physical[encoder_id].CLK_shifter_channel;
                        Encoders_manager.Transmit_DT_CLK(encoder_id, bitRead(shifter_channel_value[Last][shifter_id], DT_channel), bitRead(shifter_channel_value[Last][shifter_id], CLK_channel));

                        // Both encoder channels are handled together; clear them to avoid a second transmission when the loop reaches the other changed channel.
                        bitWrite(shifter_channel_value[Filtered][shifter_id], DT_channel, 0);
                        bitWrite(shifter_channel_value[Filtered][shifter_id], CLK_channel, 0);

                        /*
                        Serial.print("DT_channel/CLK_channel: ");
                        Serial.print(DT_channel);
                        Serial.print(" / ");
                        Serial.println(CLK_channel);

                        Serial.print("DT/CLK: ");
                        Serial.print(bitRead(shifter_channel_value[Last][shifter_id], DT_channel));
                        Serial.print(" / ");
                        Serial.println(bitRead(shifter_channel_value[Last][shifter_id], CLK_channel));
                        */
                    }
                    else if (pushbutton_id > -1)
                    {
                        Pushbuttons_manager.Transmit_position(pushbutton_id, bitRead(shifter_channel_value[Last][shifter_id], channel));
                    }

                    else if (switch_id > -1)
                    {
                        const int A_channel = switch_physical[switch_id].A_pin_shifter_channel;
                        const int B_channel = switch_physical[switch_id].B_pin_shifter_channel;
                        const int C_channel = switch_physical[switch_id].C_pin_shifter_channel;
                        const int D_channel = switch_physical[switch_id].D_pin_shifter_channel;

                        auto value = bitRead(shifter_channel_value[Last][shifter_id], A_channel) |
                                     (bitRead(shifter_channel_value[Last][shifter_id], B_channel) << 1) |
                                     (bitRead(shifter_channel_value[Last][shifter_id], C_channel) << 2) |
                                     (bitRead(shifter_channel_value[Last][shifter_id], D_channel) << 3);

                        bitWrite(shifter_channel_value[Filtered][shifter_id], A_channel, 0);
                        bitWrite(shifter_channel_value[Filtered][shifter_id], B_channel, 0);
                        bitWrite(shifter_channel_value[Filtered][shifter_id], C_channel, 0);
                        bitWrite(shifter_channel_value[Filtered][shifter_id], D_channel, 0);

                        Switches_manager.Transmit_contacts(switch_id, value);
                    }
                }
            }
        }
    }
}
