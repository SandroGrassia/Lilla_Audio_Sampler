/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 * Credits: François Best (https://github.com/FortySevenEffects/arduino_midi_library/issues/165)
 *
 */

#include "CaptureSources.h"
#include <MidiReader.h>

void MidiReader::Begin(void)
{
    // Extend the Teensy Serial1 RX buffer by 512 bytes; storage must remain valid for the driver's lifetime.
    static uint8_t midi_rx_extra_buffer[512];
    static bool midi_rx_buffer_attached = false;
    if (!midi_rx_buffer_attached)
    {
        Serial1.addMemoryForRead(midi_rx_extra_buffer, sizeof(midi_rx_extra_buffer));
        midi_rx_buffer_attached = true;
    }
    MIDI.begin(MIDI_CHANNEL_OMNI);
    MIDI.turnThruOff();
    Vibrato->Update_vibrato_array(127); // Shared full-depth waveform; each Player applies its channel's modulation amount.
}

void MidiReader::Start(void)
{
    Reset_keys_state();
    midi_message_received = 0; // no message
    midi_stop_flag = false;
}

void MidiReader::Collect_messages(void)
{
    batch.Clear();
    const bool learn_cc = Lilla_state == CC_SETTINGS;
    if ((learn_cc && display_wait) || (!learn_cc && midi_stop_flag))
    {
        return;
    }
    static_assert(midi::DefaultSettings::Use1ByteParsing, "The RX byte budget requires one-byte MIDI parsing.");
    const int available = Serial1.available(); // Snapshot: never chase bytes arriving during this IRQ.
    const int byte_limit = available < MidiInputBatch::Max_rx_bytes ? available : MidiInputBatch::Max_rx_bytes;
    const uint32_t started = micros();
    for (int bytes = 0; bytes < byte_limit && !batch.Full(); ++bytes)
    {
        if (static_cast<uint32_t>(micros() - started) >= MidiInputBatch::Max_parse_micros)
        {
            break;
        }
        if (!MIDI.read())
        {
            continue; // Partial message is retained by the library; false does not mean an empty UART.
        }
        const MidiInputMessage message = {static_cast<uint8_t>(MIDI.getType()), MIDI.getChannel(), MIDI.getData1(), MIDI.getData2()};
        if (learn_cc)
        {
            if (message.type == midi::ControlChange)
            {
                CC_midi_controller = message.data1;
                display_wait = true;
                break; // Consume exactly one learned CC; later bytes stay in the UART.
            }
        }
        else
        {
            batch.Push(message); // Capacity was checked before consuming the next byte.
        }
    }
}

void MidiReader::Update(void)
{
    Collect_messages();
    if (LOOP_learn_flag && Lilla_state == MIDI_LOOP && LOOP_learning_track > 0 && LOOP_elements > 0 && LOOP_learn_clock >= LOOP_time)
    {
        LOOP_learn_flag = false;
    }
    Players_Manager->Begin_midi_batch();
    for (uint8_t channel = 0; channel < 16; ++channel)
    {
        for (uint8_t control = 0; control < 3; ++control)
        {
            if (batch.dirty[channel][control])
            {
                Handle_message(batch.controls[channel][control]);
            }
        }
    }
    for (uint8_t event = 0; event < batch.count; ++event)
    {
        Handle_message(batch.ordered[event]);
    }
    Update_loops(); // Independent source and budget; ordered after external MIDI within this block.
    Players_Manager->End_midi_batch();
}

void MidiReader::Handle_message(const MidiInputMessage &message)
{
    if (Capture_learn_key && message.type == midi::NoteOn && message.data2 > 0)
    {
        if (Capture_learn_note < 0)
        {
            Capture_learn_note = message.data1;
        }
        return;
    }
    uint8_t velocity;
    float velocity_float;
    uint8_t midi_channel;
    uint8_t note_number;
    int controller;
    uint8_t midi_value;
    switch (message.type)
    {
    case midi::NoteOn:
    {
        midi_channel = message.channel - 1; // from 0 (MIDI CH. 1) to 15 (MIDI CH.16)
        note_number = message.data1;
        velocity = message.data2;

        // Test Midi Out
        /*
        Midi_out.NoteOn(note_number, velocity,  midi_channel + 1);
        Midi_out.NoteOff(note_number, velocity,  midi_channel + 1);
        */

        // Loop learning
        if ((Lilla_state == MIDI_LOOP) && LOOP_learn_flag && LOOP_elements < LOOP_EVENTS)
        {
            // Solo in caso di NoteOn
            if (LOOP_elements == 0)
            {
                LOOP_elements = 1;
                LOOP_last_event = 0;

                if (LOOP_learning_track == 0)
                {
                    // Chiede a Main l'accensione del primo led del metronomo
                    LOOP_metronomo_flag_IN[0] = true;
                }

                // Si azzera il contatempo
                LOOP_learn_clock = 0;
            }

            // Aggiorna il conteggio degli elementi
            else
            {
                ++LOOP_elements;
                ++LOOP_last_event;
            }

            // Memorizza il tempo
            if (LOOP_learning_track == 0)
            {
                LOOP_clock_memo = LOOP_learn_clock;
            }
            else
            {
                LOOP_clock_memo = LOOP_Clock() % LOOP_time;
            }

            LOOP_element[LOOP_learning_track][LOOP_last_event].time = LOOP_clock_memo;
            LOOP_element[LOOP_learning_track][LOOP_last_event].midi_channel = midi_channel;
            LOOP_element[LOOP_learning_track][LOOP_last_event].note_number = note_number;
            LOOP_element[LOOP_learning_track][LOOP_last_event].velocity = velocity;
            LOOP_element[LOOP_learning_track][LOOP_last_event].note_on = true;

            // Ultimo evento accettabile
            if (LOOP_elements >= LOOP_EVENTS)
            {
                LOOP_learn_flag = false;
                // Learning completion is handled by the main loop; do not print from the audio IRQ.
            }
        }

        if (tuning_tone_flag)
        {
            tuning_tone_last_note = note_number;
            Tone_generator->Frequency(pitch_from_note[note_number] * 261.63);
            Tone_generator->Amplitude(Volume_float[tuning_tone_volume]);
            Tone_generator->Start();
            TT_playing = true;
            TT_led_flag = true;
        }

        velocity_float = velocity / 127.0f;
        key_state[midi_channel][note_number] = true; // real key

        for (auto instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
        {
            if ((Patch[Patch_id].Instrument[instrument_id].used) && bitRead(map_instrument_for_note[midi_channel][note_number], instrument_id))
            {
                Players_Manager->Play_note(instrument_id, note_number, velocity_float, NO_TRACK);
            }
        }

        if (Lilla_state == MIDI_MONITOR && !display_wait)
        {
            midi_message_received = 1;      // note ON
            MM_midi_channel = midi_channel; // from 0 (MIDI CH. 1) to 15 (MIDI CH.16)
            MM_note_number = note_number;
            MM_velocity = velocity;
            display_wait = true;
        }
    }
    break;

    case midi::NoteOff:
    {
        midi_channel = message.channel - 1; // from 0 (MIDI CH. 1) to 15 (MIDI CH.16)
        note_number = message.data1;
        velocity = message.data2;

        // Loop learning
        if ((Lilla_state == MIDI_LOOP) && LOOP_learn_flag && LOOP_elements > 0 && LOOP_elements < LOOP_EVENTS)
        {
            // Aggiorna il conteggio degli elementi
            ++LOOP_elements;
            ++LOOP_last_event;

            // Memorizza il tempo
            if (LOOP_learning_track == 0)
            {
                LOOP_clock_memo = LOOP_learn_clock;
            }
            else
            {
                LOOP_clock_memo = LOOP_Clock() % LOOP_time;
            }

            LOOP_element[LOOP_learning_track][LOOP_last_event].time = LOOP_clock_memo;
            LOOP_element[LOOP_learning_track][LOOP_last_event].midi_channel = midi_channel;
            LOOP_element[LOOP_learning_track][LOOP_last_event].note_number = note_number;
            LOOP_element[LOOP_learning_track][LOOP_last_event].velocity = velocity;
            LOOP_element[LOOP_learning_track][LOOP_last_event].note_on = false;

            // Ultimo evento accettabile
            if (LOOP_elements >= LOOP_EVENTS)
            {
                LOOP_learn_flag = false;
                // // Learning completion is handled by the main loop; do not print from the audio IRQ.
            }
        }

        if (tuning_tone_flag && (tuning_tone_last_note == note_number))
        {
            Tone_generator->Stop();
            TT_playing = false;
            TT_led_flag = true;
        }

        key_state[midi_channel][note_number] = false; // real key
        Players_Manager->Multicast_stop_players_for_NoteOff(midi_channel, note_number, -1);

        if (Lilla_state == MIDI_MONITOR && !display_wait)
        {
            midi_message_received = 2;      // note OFF
            MM_midi_channel = midi_channel; // from 0 (MIDI CH. 1) to 15 (MIDI CH.16)
            MM_note_number = note_number;
            MM_velocity = velocity;
            display_wait = true;
            break;
        }
    }
    break;

    case midi::PitchBend:
    {
        // PRINT("PitchBend", "", (message.data2 << 7) + message.data1);
        midi_channel = message.channel - 1; // from 0 (MIDI CH. 1) to 15 (MIDI CH.16)
        pitch_bend_value[midi_channel] = ((message.data2 << 7) + message.data1) / 16384.0f + 0.5f;
        Players_Manager->Broadcast_pitch_bend(midi_channel, pitch_bend_value[midi_channel]);

        if (Lilla_state == MIDI_MONITOR && !display_wait)
        {
            midi_message_received = 3;      // pitch bend
            MM_midi_channel = midi_channel; // from 0 (MIDI CH. 1) to 15 (MIDI CH.16)
            MM_pitch_bend_least = message.data1;
            MM_pitch_bend_most = message.data2;
            display_wait = true;
            break;
        }
    }
    break;

    case midi::AfterTouchChannel: // Status = 1101nnnn Pressure value = 0vvvvvvv
    {
        midi_channel = message.channel - 1; // from 0 (MIDI CH. 1) to 15 (MIDI CH.16);
        midi_value = message.data1;
        after_touch_channel_value[midi_channel] = midi_value / 127.0f;

        for (auto instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
        {
            if ((Preset[instrument_id].midi_channel == midi_channel) && (Preset[instrument_id].Filter.use == 1) && (Preset[instrument_id].Filter.modulation == 4))
            {
                Players_Manager->Multicast_IF_index(instrument_id, Preset[instrument_id].Filter.index * after_touch_channel_value[midi_channel]);
            }
        }
        if (Lilla_state == MIDI_MONITOR && !display_wait)
        {
            midi_message_received = 7;      // After Touch Channel
            MM_midi_channel = midi_channel; // from 0 (MIDI CH. 1) to 15 (MIDI CH.16)
            MM_least_bits = midi_value;
            display_wait = true;
            break;
        }
    }
    break;

    case midi::ControlChange: // Status = 1011nnnn  Controller = 0ccccccc (da 0 a 120)  Value = 0vvvvvvv
    {
        // PRINT("ControlChange", "message.data1", message.data1);
        // PRINT("ControlChange", "message.data2", message.data2);
        midi_channel = message.channel - 1; // from 0 (MIDI CH. 1) to 15 (MIDI CH.16)
        controller = message.data1;
        midi_value = message.data2;

        if (controller == 123) // All sound off
        {
            // PRINT("All Note off", " data 1:", 123);
            Players_Manager->Multicast_all_notes_off(midi_channel);
        }

        else if (controller == 126) // All notes off
        {
            // PRINT("All Note off", " data 1:", 126);
            Players_Manager->Multicast_all_notes_off(midi_channel);
        }

        else if (controller == 1) // Modulation
        {
            // PRINT("Mod", "", midi_value);
            Players_Manager->Set_modulation(midi_channel, midi_value);
        }

        else if ((controller == CC_lowpass_filter_value) && (controller > 0)) // change the lowpass filter cut frequency
        {
            lowpass_target = midi_value * 0.31; // <= 39

            if (abs(lowpass_target - lowpass) > 0)
            {
                lowpass_flag = true;
                lowpass_direction = lowpass_target > lowpass;
                display_lowpass_flag = true;
            }
        }

        else
        {
            for(auto instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
            {
                if ((controller == CC_Sound_gain[instrument_id]) && (controller > 0))
                {
                    if (Patch[Patch_id].Instrument[instrument_id].used && (Get_midi_channel(Patch_id, instrument_id) == midi_channel))
                    {
                        Sound[Get_sound_id(Patch_id, instrument_id)].gain = (float)midi_value * 0.315; // 127 --> 40
                        Players_Manager->Update_Preset_volume(Patch_id, instrument_id, Patch_volume_gain(volume_patch));
                        Players_Manager->Multicast_volume_for_instrument_edit(instrument_id);

                        if (Lilla_state == PERFORMANCE)
                        {
                            instrument_volume_changed = instrument_id;
                            display_instrument_volume_flag = true;
                        }
                    }
                }
            }
        }

        if (Lilla_state == MIDI_MONITOR && !display_wait)
        {
            midi_message_received = 5;      // control change
            MM_midi_channel = midi_channel; // from 0 (MIDI CH. 1) to 15 (MIDI CH.16)
            MM_midi_controller = controller;
            MM_midi_value = midi_value;
            display_wait = true;
            break;
        }
    }
    break;

    case midi::AfterTouchPoly:
        if (Lilla_state == MIDI_MONITOR && !display_wait)
        {
            midi_message_received = 4;               // after touch poly
            MM_midi_channel = message.channel - 1; // from 0 (MIDI CH. 1) to 15 (MIDI CH.16)
            MM_least_bits = message.data1;
            MM_most_bits = message.data2;
            display_wait = true;
        }
        break;

    case midi::ProgramChange:
        if (Lilla_state == MIDI_MONITOR && !display_wait)
        {
            midi_message_received = 6;
            MM_midi_channel = message.channel - 1; // from 0 (MIDI CH. 1) to 15 (MIDI CH.16);
            MM_least_bits = message.data1;
            display_wait = true;
        }
        break;

    case midi::SystemExclusive:
        if (Lilla_state == MIDI_MONITOR && !display_wait)
        {
            midi_message_received = 8;               // System Exclusive
            MM_midi_channel = message.channel - 1; // from 0 (MIDI CH. 1) to 15 (MIDI CH.16);
            display_wait = true;
        }
        break;

    default:
        if (Lilla_state == MIDI_MONITOR && !display_wait)
        {
            display_wait = true;
        }
        break;
    }
}

void MidiReader::Update_loops(void)
{
    uint8_t velocity;
    float velocity_float;
    uint8_t midi_channel;
    uint8_t note_number;
    // Eseguito se ci sono track running
    if (Lilla_state == MIDI_LOOP || (Lilla_state_0 == MIDI_LOOP && (Lilla_state == DELAY_SETTINGS || Lilla_state == SOUND_EDIT || Lilla_state == INSTRUMENT_VCF || Lilla_state == MIXER || Lilla_state == SETUP || Lilla_state == CC_SETTINGS)))
    {
        unsigned long LOOP_Clock_frozen = LOOP_Clock(); // congela LOOP_Clock()

        // update metronomo
        if (LOOP_metronomo_run && (LOOP_Clock_frozen >= LOOP_metronomo.metro_time))
        {
            if (Lilla_state == MIDI_LOOP) // chiede a track() di aggiornare il metronomo
            {
                LOOP_metronomo_flag_IN[1] = true;
            }
            else
            {
                LOOP_metronomo.Update(false); // comanda lo switch del solo beat
                LOOP_metronomo.metro_time += LOOP_metronomo.Read_metro_delta_ms();
            }
        }

        // check notes to play/stop
        for (uint8_t pass = 0; pass < Loop_events_per_track; ++pass)
        {
            for (auto track = 0; track < TRACKS; ++track)
            {
                if (LOOP_track_run[track] && LOOP_events[track] > 0 && (LOOP_Clock_frozen >= LOOP_play_time[track]))
                {
                    uint32_t event = LOOP_play_event[track];

                    // Prima del primo evento, tutti i Player di track devono aver gia' ricevuto NoteOff, altrimenti gli vengono inviati
                    if (event == 0)
                    {
                        Players_Manager->Multicast_stop_players_for_loop_track(track);
                    }
                    midi_channel = LOOP_element[track][event].midi_channel;
                    note_number = constrain(LOOP_element[track][event].note_number + LOOP_pitch_int[track], 0, 127);
                    velocity = LOOP_element[track][event].velocity;
                    velocity_float = velocity / 127.0f;

                    if (LOOP_element[track][event].note_on)
                    {
                        if (tuning_tone_flag)
                        {
                            tuning_tone_last_note = note_number;
                            Tone_generator->Frequency(pitch_from_note[note_number] * 261.63);
                            Tone_generator->Amplitude(Volume_float[tuning_tone_volume]);
                            Tone_generator->Start();
                            TT_playing = true;
                            TT_led_flag = true;
                        }

                        for (auto instrument_id = 0; instrument_id < INSTRUMENTS; ++instrument_id)
                        {
                            if ((Patch[Patch_id].Instrument[instrument_id].used) && bitRead(map_instrument_for_note[midi_channel][note_number], instrument_id))
                            {
                                Players_Manager->Play_note(instrument_id, note_number, velocity_float, track);
                            }
                        }

                    }

                    else if (!LOOP_element[track][event].note_on)
                    {
                        if (tuning_tone_flag && (tuning_tone_last_note == note_number))
                        {
                            Tone_generator->Stop();
                            TT_playing = false;
                            TT_led_flag = true;
                        }

                        // for(auto i = 0; i < 127; ++i)
                        // {
                        //  Serial.print(key_state[0][i]);
                        //  Serial.print("-");
                        //  }
                        // Serial.println();

                        Players_Manager->Multicast_stop_players_for_NoteOff(midi_channel, note_number, track);
                    }

                    // Prossimo evento
                    LOOP_play_event[track] = (LOOP_play_event[track] + 1) % LOOP_events[track];
                    LOOP_play_time[track] = LOOP_Clock_time_from_virtual_time(LOOP_element[track][LOOP_play_event[track]].time);
                    // Serial.print("Prossimo evento tra ms virtuali:");
                    // Serial.println(LOOP_play_time[track] - LOOP_Clock_frozen);

                    /*
                    Serial.print("Si prosegue con l'evento:");
                    Serial.print(LOOP_play_event[track]);
                    Serial.print(" del track:");
                    Serial.print(track);
                    Serial.print(" tra ms virtuali:");
                    Serial.println(LOOP_play_time[track] - LOOP_Clock());
                    Serial.print("*************");
                    */
                }
            }
        }
    }
}
