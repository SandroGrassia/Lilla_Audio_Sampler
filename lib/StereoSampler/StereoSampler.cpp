/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

// StereoSampler.h registra sul "virtual file system" (VFS) realizato nella Flash memory

#include "StereoSampler.h"

void StereoSampler::Begin(void)
{
    recording = false;
}
bool StereoSampler::Start(int from_packet, int last_packet, int recording_id_in, bool stereo_in)
{
    AudioStartUsingSPI();
    recording_id = recording_id_in;
    stereo_flag = stereo_in;

    // packet:  0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18
    // free:    * * * * 1 2 3 4 5 6 7  8  9  10 11 12 13 14 15
    // result:  * * * * L R L R L R L  R  L  R  L  R  L  R

    packet_open_L = from_packet; // 4

    if (stereo_flag)
    {
        packet_open_R = packet_open_L + 1; // 2
        if ((last_packet - from_packet + 1) % 2 == 0)
        {
            last_packet_L = last_packet - 1;
        }
        else // 13
        {
            last_packet_L = last_packet - 2;
        }
        last_packet_R = last_packet_L + 1; // 14
    }
    else
    {
        last_packet_L = last_packet;
    }

    Recording[recording_id].first_packet = from_packet;
    Recording[recording_id].packets = 0;
    Recording[recording_id].consistent = false; // consistent will be set TRUE in Main code, after stop()
    increment_packets_flag = true;

    Packet_L.packet_fast_open(packet_open_L);
    // Serial.print(F("SSampler - First packet Left/Mono is: "));


    if (stereo_flag)
    {
        Packet_R.packet_fast_open(packet_open_R);
        // Serial.print("SSampler - First packet Right is: ");

    }

    attack_gain_flag = true;
    samples_counter = Slope_samples;
    decay_gain_flag = false;
    countdown_flag = false;
    book_stop_flag = false;
    storage_error = 0;
    missing_blocks_L = 0;
    missing_blocks_R = 0;
    recording = true;
    // Main only: let the audio IRQ finish the three-block attack before another UI command.
    delay(20);
    return true;
}

void StereoSampler::Stop_and_wait(void)
{
    if (recording)
    {
        book_stop_flag = true;
        while (recording)
        {
            delayMicroseconds(50);
        }
    }
}
void StereoSampler::update(void)
{
    // only update if we're recording
    if (!recording)
    {
        return;
    }

    if (packet_open_L == last_packet_L && !countdown_flag)
    {
        countdown_blocks = PACKET_BLOCKS - Slope_blocks; // 256 - 3 = 253
        countdown_flag = true;
        Serial.println(F("*** SS Last packet!! ***"));
    }

    // automatic stop triggered at the last block of the last packet available
    if (countdown_flag == true && countdown_blocks == 1)
    {
        countdown_flag = false;
        book_stop_flag = true;
        Serial.println(F("*** SS countdown_blocks == 1!! ***"));
    }
    else if (countdown_flag == true && countdown_blocks > 1)
    {
        --countdown_blocks;
        // Serial.print(F("SS countdown_blocks: "));
        // Serial.println(countdown_blocks);
    }

    if (book_stop_flag)
    {
        book_stop_flag = false;
        if (!decay_gain_flag)
        {
            decay_gain_flag = true;
            samples_counter = Slope_samples;
        }
    }

    audio_block_t *in_block_L = receiveReadOnly(0);
    audio_block_t *in_block_R = receiveReadOnly(1);
    static const int16_t silence[AUDIO_BLOCK_SAMPLES] = {};
    const int16_t *samples_L = in_block_L != nullptr ? in_block_L->data : silence;
    const int16_t *samples_R = in_block_R != nullptr ? in_block_R->data : silence;
    if (in_block_L == nullptr)
    {
        missing_blocks_L = missing_blocks_L + 1;
    }
    if (in_block_R == nullptr)
    {
        missing_blocks_R = missing_blocks_R + 1;
    }

    // update .packets, update FAT table
    if (increment_packets_flag)
    {
        ++Recording[recording_id].packets;
        const byte result = Archive.Save_DS_Recording(recording_id);

        if (result != LillaFRAM_2x512::ERROR_0)
        {
            storage_error = result;
            stop();
            if (in_block_L != nullptr)
            {
                release(in_block_L);
            }
            if (in_block_R != nullptr)
            {
                release(in_block_R);
            }
            return;
        }

        VFS_FAT_table[packet_open_L] = recording_id;
        if (stereo_flag)
        {
            VFS_FAT_table[packet_open_R] = recording_id;
        }

        increment_packets_flag = false;
    }

    if (stereo_flag)
    {
        if (!attack_gain_flag && !decay_gain_flag)
        {
            // direcly copy from AudioQueue.s to Packet.s
            Packet_L.write(samples_L, 256);
            Packet_R.write(samples_R, 256);
        }

        else if (attack_gain_flag)
        {
            memcpy(RAM_buffer_L, samples_L, 256);
            memcpy(RAM_buffer_R, samples_R, 256);

            for (auto sample = 0; sample < AUDIO_BLOCK_SAMPLES; ++sample)
            {
                *(RAM_buffer_L_ptr + sample) = (float)*(RAM_buffer_L_ptr + sample) * (1.0 - samples_counter / F_Slope_samples);
                *(RAM_buffer_R_ptr + sample) = (float)*(RAM_buffer_R_ptr + sample) * (1.0 - samples_counter / F_Slope_samples);
                --samples_counter;
            }

            Packet_L.write(RAM_buffer_L, 256);
            Packet_R.write(RAM_buffer_R, 256);

            if (samples_counter == 0)
            {
                attack_gain_flag = false;
            }
        }

        else if (decay_gain_flag)
        {
            memcpy(RAM_buffer_L, samples_L, 256);
            memcpy(RAM_buffer_R, samples_R, 256);

            for (auto sample = 0; sample < AUDIO_BLOCK_SAMPLES; ++sample)
            {
                *(RAM_buffer_L_ptr + sample) = (float)*(RAM_buffer_L_ptr + sample) * (samples_counter / F_Slope_samples);
                *(RAM_buffer_R_ptr + sample) = (float)*(RAM_buffer_R_ptr + sample) * (samples_counter / F_Slope_samples);
                --samples_counter;
            }
            Packet_L.write(RAM_buffer_L, 256);
            Packet_R.write(RAM_buffer_R, 256);

            if (samples_counter == 0)
            {
                decay_gain_flag = false;
                Serial.println(F("*** SS Decay stereo terminato ***"));
                stop();
            }
        }
    }

    // mono recording
    else
    {
        if (!attack_gain_flag && !decay_gain_flag)
        {
            // copy blocks from AudioQueue.s to RAM_buffer.s
            memcpy(RAM_buffer_L, samples_L, 256);
            memcpy(RAM_buffer_R, samples_R, 256);

            // calculate average values
            for (auto sample = 0; sample < AUDIO_BLOCK_SAMPLES; ++sample)
            {
                *(RAM_buffer_L_ptr + sample) = (*(RAM_buffer_L_ptr + sample) + *(RAM_buffer_R_ptr + sample)) / 2;
            }

            // copy block from RAM_buffer_L to Packet_L
            Packet_L.write(RAM_buffer_L, 256);
        }

        else if (attack_gain_flag)
        {
            // copy blocks from AudioQueue.s to RAM_buffer.s
            memcpy(RAM_buffer_L, samples_L, 256);
            memcpy(RAM_buffer_R, samples_R, 256);

            // calculate average values
            for (auto sample = 0; sample < AUDIO_BLOCK_SAMPLES; ++sample)
            {
                *(RAM_buffer_L_ptr + sample) = (1.0 - samples_counter / F_Slope_samples) * (float)(*(RAM_buffer_L_ptr + sample) + *(RAM_buffer_R_ptr + sample)) / 2.0;
                --samples_counter;
            }

            // copy block from RAM_buffer_L to Packet_L
            Packet_L.write(RAM_buffer_L, 256);

            if (samples_counter == 0)
            {
                attack_gain_flag = false;
            }
        }

        else if (decay_gain_flag)
        {
            // copy blocks from AudioQueue.s to RAM_buffer.s
            memcpy(RAM_buffer_L, samples_L, 256);
            memcpy(RAM_buffer_R, samples_R, 256);

            // calculate average values
            for (auto sample = 0; sample < AUDIO_BLOCK_SAMPLES; ++sample)
            {
                *(RAM_buffer_L_ptr + sample) = (samples_counter / F_Slope_samples) * (float)(*(RAM_buffer_L_ptr + sample) + *(RAM_buffer_R_ptr + sample)) / 2.0;
                samples_counter--;
            }

            // copy block from RAM_buffer_L to Packet_L
            Packet_L.write(RAM_buffer_L, 256);

            if (samples_counter == 0)
            {
                decay_gain_flag = false;
                stop();
            }
        }
    }

    if (recording && Packet_L.position() == PACKET_DIM) // Do not open another packet after the fade has completed.
    {
        if (stereo_flag)
        {
            Packet_L.close();
            Packet_R.close();
            int next_packet_L = packet_open_L + 2;

            if (next_packet_L <= last_packet_L)
            {
                packet_open_L = next_packet_L;
                Packet_L.packet_fast_open(packet_open_L);
                packet_open_R = packet_open_R + 2;
                Packet_R.packet_fast_open(packet_open_R);

                increment_packets_flag = true;

            }

            // Packets finished!
            else
            {
                stop();
            }
        }

        else
        {
            Packet_L.close();
            int next_packet_L = packet_open_L + 1;

            if (next_packet_L <= last_packet_L)
            {
                packet_open_L = next_packet_L;
                Packet_L.packet_fast_open(packet_open_L);
                increment_packets_flag = true;
            }

            // Packets finished!
            else
            {
                stop();
            }
        }
    }
    if (in_block_L != nullptr)
    {
        release(in_block_L);
    }
    if (in_block_R != nullptr)
    {
        release(in_block_R);
    }
    return;
}

bool StereoSampler::Is_recording(void)
{
    return recording;
}

void StereoSampler::stop(void)
{
    Serial.println(F("SSampler - Stop received!"));
    if (recording)
    {
        // cancel new packet registration if required
        if (increment_packets_flag)
        {
            increment_packets_flag = false;
        }

        Packet_L.close();

        if (stereo_flag)
        {
            Packet_R.close();
        }

        book_stop_flag = false;
        attack_gain_flag = false;
        decay_gain_flag = false;
        AudioStopUsingSPI();
        recording = false;
    }
}
