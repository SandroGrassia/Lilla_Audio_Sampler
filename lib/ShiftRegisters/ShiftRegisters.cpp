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
    Shifter[UI_leds[led].shifter_id].digitalWrite(UI_leds[led].shifter_channel, (on ? LOW : HIGH));
}

void ShiftRegisters::Set_monitored_encoders(const uint32_t &data)
{
    monitored_encoders = data;

    Serial.print("monitored_encoders, BIN: ");
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

void ShiftRegisters::Set_monitored_pushbuttons_switches(const uint64_t &data)
{
    monitored_pushbuttons_switches = data;

    Serial.print("monitored_pushbuttons_switches, BIN: ");
    Serial.println(monitored_pushbuttons_switches, BIN);

    for (auto i = 0; i < PUSHBUTTONS; ++i)
    {
        if (bitRead(monitored_pushbuttons_switches, i))
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
        if (idx < 32)
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
        if (idx < 64)
        {
            mask |= (uint64_t(1) << idx);
        }
    }
    return mask;
}

void ShiftRegisters::Init_context_sets(void)
{
    context_encoders[Start_context] = Make_encoders_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_Tempo, EN_PB_Loop, EN_PB_Track1, EN_PB_Track2,
                                                          EN_PB_Select, EN_PB_Track3, EN_PB_Track4, EN_PB_Value, EN_PB_From, EN_PB_Step, EN_PB_To, EN_PB_LineOutVol, EN_PB_PreListenVol});

    context_pushbuttons_switches[Start_context] = Make_pushbuttons_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_Tempo, EN_PB_Loop, EN_PB_Track1, EN_PB_Track2,
                                                                         EN_PB_Select, EN_PB_Track3, EN_PB_Track4, EN_PB_Value, EN_PB_From, EN_PB_Step, EN_PB_To, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                                         PB_S2, PB_S3, PB_S4, PB_S5, PB_S6, PB_S7, PB_S8, PB_Rec1, PB_Rec2, PB_Rec4, PB_Rec3, SW_TOOLS_Mixer, SW_TOOLS_Delay, SW_TOOLS_Setup, SW_TOOLS_Test, PB_Tools,
                                                                         SW_MODE_Sampler, SW_MODE_LiveSampler, SW_MODE_Performance, SW_MODE_MidiLoop, PB_S1});

    context_encoders[Performance_context] = Make_encoders_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                                EN_PB_Select, EN_PB_Value, EN_PB_From, EN_PB_To});

    context_pushbuttons_switches[Performance_context] = Make_pushbuttons_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                                               EN_PB_Select, EN_PB_Value, EN_PB_From, EN_PB_To, PB_S1, PB_S2, PB_S3, PB_S4, PB_S5, PB_S6, PB_S7, PB_S8,
                                                                               PB_Tools, SW_TOOLS_Mixer, SW_TOOLS_Delay, SW_TOOLS_Setup, SW_TOOLS_Test, SW_MODE_Sampler, SW_MODE_LiveSampler, SW_MODE_Performance, SW_MODE_MidiLoop});

    context_encoders[Sound_edit_context] = Make_encoders_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                               EN_PB_Select, EN_PB_Value, EN_PB_From, EN_PB_Step, EN_PB_To});

    context_pushbuttons_switches[Sound_edit_context] = Make_pushbuttons_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                                              EN_PB_Select, EN_PB_Value, EN_PB_From, EN_PB_Step, EN_PB_To, PB_S1, PB_S2, PB_S3, PB_S4, PB_S5, PB_S6, PB_S7, PB_S8,
                                                                              PB_Tools, SW_TOOLS_Mixer, SW_TOOLS_Delay, SW_TOOLS_Setup, SW_TOOLS_Test, SW_MODE_Sampler, SW_MODE_LiveSampler, SW_MODE_Performance, SW_MODE_MidiLoop});

    context_encoders[Instrument_Vcf_context] = Make_encoders_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                                   EN_PB_Select, EN_PB_Value});

    context_pushbuttons_switches[Instrument_Vcf_context] = Make_pushbuttons_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                                                  EN_PB_Select, EN_PB_Value, PB_S1, PB_S2, PB_S3, PB_S4, PB_S5, PB_S6, PB_S7, PB_S8,
                                                                                  PB_Tools, SW_TOOLS_Mixer, SW_TOOLS_Delay, SW_TOOLS_Setup, SW_TOOLS_Test, SW_MODE_Sampler, SW_MODE_LiveSampler, SW_MODE_Performance, SW_MODE_MidiLoop});

    context_encoders[Mixer_context] = Make_encoders_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                          EN_PB_Select, EN_PB_Value});
    context_pushbuttons_switches[Mixer_context] = Make_pushbuttons_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                                         EN_PB_Select, EN_PB_Value, PB_S1, PB_S2, PB_S3, PB_S4, PB_S5, PB_S6, PB_S7, PB_S8,
                                                                         PB_Tools, SW_TOOLS_Mixer, SW_TOOLS_Delay, SW_TOOLS_Setup, SW_TOOLS_Test, SW_MODE_Sampler, SW_MODE_LiveSampler, SW_MODE_Performance, SW_MODE_MidiLoop});

    context_encoders[Delay_settings_context] = Make_encoders_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                                   EN_PB_Select, EN_PB_Value});
    context_pushbuttons_switches[Delay_settings_context] = Make_pushbuttons_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                                                  EN_PB_Select, EN_PB_Value, PB_S1, PB_S2, PB_S3, PB_S4, PB_S5, PB_S6, PB_S7, PB_S8,
                                                                                  PB_Tools, SW_TOOLS_Mixer, SW_TOOLS_Delay, SW_TOOLS_Setup, SW_TOOLS_Test, SW_MODE_Sampler, SW_MODE_LiveSampler, SW_MODE_Performance, SW_MODE_MidiLoop});

    context_encoders[Live_Sampling_context] = Make_encoders_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                                  EN_PB_Select, EN_PB_Value, EN_PB_From, EN_PB_Step, EN_PB_To});
    context_pushbuttons_switches[Live_Sampling_context] = Make_pushbuttons_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                                                 EN_PB_Select, EN_PB_Value, EN_PB_From, EN_PB_Step, EN_PB_To, PB_S1, PB_S2, PB_S3, PB_S4, PB_S5, PB_S6, PB_S7, PB_S8,
                                                                                 PB_Tools, SW_TOOLS_Mixer, SW_TOOLS_Delay, SW_TOOLS_Setup, SW_TOOLS_Test, SW_MODE_Sampler, SW_MODE_LiveSampler, SW_MODE_Performance, SW_MODE_MidiLoop});

    context_encoders[Direct_Sampling_context] = Make_encoders_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                                    EN_PB_Select, EN_PB_Value});
    context_pushbuttons_switches[Direct_Sampling_context] = Make_pushbuttons_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                                                   EN_PB_Select, EN_PB_Value, PB_S1, PB_S2, PB_S3, PB_S4, PB_S5, PB_S6, PB_S7, PB_S8,
                                                                                   PB_Tools, SW_TOOLS_Mixer, SW_TOOLS_Delay, SW_TOOLS_Setup, SW_TOOLS_Test, SW_MODE_Sampler, SW_MODE_LiveSampler, SW_MODE_Performance, SW_MODE_MidiLoop});

    context_encoders[Midi_Monitor_context] = Make_encoders_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol});

    context_pushbuttons_switches[Midi_Monitor_context] = Make_pushbuttons_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                                                PB_Tools, SW_TOOLS_Mixer, SW_TOOLS_Delay, SW_TOOLS_Setup, SW_TOOLS_Test, SW_MODE_Sampler, SW_MODE_LiveSampler, SW_MODE_Performance, SW_MODE_MidiLoop});

    context_encoders[Midi_Loop_context] = Make_encoders_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                              EN_PB_Select, EN_PB_Value,
                                                              EN_PB_Tempo, EN_PB_Loop, EN_PB_Track1, EN_PB_Track2, EN_PB_Track3, EN_PB_Track4});

    context_pushbuttons_switches[Midi_Loop_context] = Make_pushbuttons_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                                             EN_PB_Select, EN_PB_Value,
                                                                             EN_PB_Tempo, EN_PB_Loop, EN_PB_Track1, EN_PB_Track2, EN_PB_Track3, EN_PB_Track4,
                                                                             PB_Rec1, PB_Rec2, PB_Rec3, PB_Rec4,
                                                                             PB_Tools, SW_TOOLS_Mixer, SW_TOOLS_Delay, SW_TOOLS_Setup, SW_TOOLS_Test, SW_MODE_Sampler, SW_MODE_LiveSampler, SW_MODE_Performance, SW_MODE_MidiLoop});

    context_encoders[Setup_context] = Make_encoders_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                          EN_PB_Select, EN_PB_Value});
    context_pushbuttons_switches[Setup_context] = Make_pushbuttons_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                                         EN_PB_Select, EN_PB_Value,
                                                                         PB_Tools, SW_TOOLS_Mixer, SW_TOOLS_Delay, SW_TOOLS_Setup, SW_TOOLS_Test, SW_MODE_Sampler, SW_MODE_LiveSampler, SW_MODE_Performance, SW_MODE_MidiLoop});

    context_encoders[Control_Change_context] = Make_encoders_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                                   EN_PB_Select, EN_PB_Value});
    context_pushbuttons_switches[Control_Change_context] = Make_pushbuttons_mask({EN_PB_TuningTone, EN_PB_Resolution, EN_PB_Downsampling, EN_PB_Cutoff, EN_PB_LineOutVol, EN_PB_PreListenVol,
                                                                                  EN_PB_Select, EN_PB_Value,
                                                                                  PB_Tools, SW_TOOLS_Mixer, SW_TOOLS_Delay, SW_TOOLS_Setup, SW_TOOLS_Test, SW_MODE_Sampler, SW_MODE_LiveSampler, SW_MODE_Performance, SW_MODE_MidiLoop});
}

void ShiftRegisters::Set_context(LillaContext context)
{
    Set_monitored_encoders_pushbuttons(context_encoders[context], context_pushbuttons_switches[context]);
}

void ShiftRegisters::Set_monitored_encoders_pushbuttons(const uint32_t &enc, const uint64_t &pb)
{
    Reset_monitored_channels();
    Set_monitored_encoders(enc);
    Set_monitored_pushbuttons_switches(pb);

    if (true)
    {
        Serial.println("Monitored shift register chips channeles");
        Serial.println("b7  b6  b5  b4  b3  b2  b1  b0  a7  a6  a5  a4  a3  a2  a1  a0");
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
                    const auto encoder = Shifter_channel_to_encoder_pushbutton[shifter_id][channel].encoder_id;

                    // Serial.print("encoder: ");
                    // Serial.println(encoder);

                    const auto pushbutton = Shifter_channel_to_encoder_pushbutton[shifter_id][channel].pushbutton_id;
                    if (encoder > -1)
                    {
                        const int DT_channel = encoder_physical[encoder].DT_shifter_channel;
                        const int CLK_channel = encoder_physical[encoder].CLK_shifter_channel;
                        Encoders_manager.Transmit_DT_CLK(encoder, bitRead(shifter_channel_value[Last][shifter_id], DT_channel), bitRead(shifter_channel_value[Last][shifter_id], CLK_channel));

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
                    else if (pushbutton > -1)
                    {
                        switch (pushbutton)
                        {
                        case SW_TOOLS_Mixer:
                        case SW_TOOLS_Delay:
                        case SW_TOOLS_Setup:
                        case SW_TOOLS_Test:
                        {
                            uint8_t value =
                                bitRead(shifter_channel_value[Last][shifter_id], pushbutton_physical[SW_TOOLS[0]].shifter_channel) |
                                (bitRead(shifter_channel_value[Last][shifter_id], pushbutton_physical[SW_TOOLS[1]].shifter_channel) << 1) |
                                (bitRead(shifter_channel_value[Last][shifter_id], pushbutton_physical[SW_TOOLS[2]].shifter_channel) << 2) |
                                (bitRead(shifter_channel_value[Last][shifter_id], pushbutton_physical[SW_TOOLS[3]].shifter_channel) << 3);

                            Switches_manager.Transmit_contacts(SwitchTools, value);

                            // All Tools switch contacts were handled together; clear them to avoid reprocessing another changed contact in this scan.
                            for (auto i = 0; i < SWITCH_TOOLS_CONTACTS; ++i)
                            {
                                bitWrite(shifter_channel_value[Filtered][shifter_id], pushbutton_physical[SW_TOOLS[i]].shifter_channel, 0);
                            }
                        }
                        break;

                        case SW_MODE_Sampler:
                        case SW_MODE_LiveSampler:
                        case SW_MODE_Performance:
                        case SW_MODE_MidiLoop:
                        {
                            uint8_t value =
                                bitRead(shifter_channel_value[Last][shifter_id], pushbutton_physical[SW_MODES[0]].shifter_channel) |
                                (bitRead(shifter_channel_value[Last][shifter_id], pushbutton_physical[SW_MODES[1]].shifter_channel) << 1) |
                                (bitRead(shifter_channel_value[Last][shifter_id], pushbutton_physical[SW_MODES[2]].shifter_channel) << 2) |
                                (bitRead(shifter_channel_value[Last][shifter_id], pushbutton_physical[SW_MODES[3]].shifter_channel) << 3);

                            Switches_manager.Transmit_contacts(SwitchModes, value);

                            // All Modes switch contacts were handled together; clear them to avoid reprocessing another changed contact in this scan.
                            for (auto i = 0; i < SWITCH_MODES_CONTACTS; ++i)
                            {
                                bitWrite(shifter_channel_value[Filtered][shifter_id], pushbutton_physical[SW_MODES[i]].shifter_channel, 0);
                            }
                        }
                        break;

                        default:
                            Pushbuttons_manager.Transmit_position(pushbutton, bitRead(shifter_channel_value[Last][shifter_id], channel));
                            break;
                        }
                    }
                }
            }
        }
    }
}
