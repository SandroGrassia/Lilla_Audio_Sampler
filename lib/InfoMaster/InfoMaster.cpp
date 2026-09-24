/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "CaptureSources.h"
#include "InfoMaster.h"

// Samples in mono recording or in Left channel's recording
int InfoMaster::DS_recording_samples(int recording)
{
    int first_packet = Recording[recording].first_packet;
    int packets = Recording[recording].packets;
    bool stereo = Recording[recording].stereo;
    SerialFlashFile rawfile;

    if (packets < 1 || first_packet < 0 || first_packet >= VFS_PACKETS_MAX)
    {
        return 0;
    }

    int value = 0; // bytes

    // Conta i campioni di tutti i packet tranne l'ultimo
    if (packets > 1)
    {
        value += (packets - 1) * PACKET_DIM;
    }

    // Quindi aggiungi l'ultimo packet
    // Look for the last packet of recording
    if (!stereo)
    {
        rawfile = SerialFlash.open(name_packet[first_packet + packets - 1]);
    }
    else
        // 0 1 2 3 4 5 6 7 8 9 10 11
        //         L R L R L R
        rawfile = SerialFlash.open(name_packet[first_packet + 2 * (packets - 1)]); // 4 + 2*(3 - 1) = 8

    // Verify if packet is empty
    uint16_t sample_value;
    int sample = 0;

    for (sample = (PACKET_DIM - 2); sample >= 0; sample -= 2)
    {
        rawfile.seek(sample);
        rawfile.read(&sample_value, 2); // read 2 bytes (1 sample)
        if (sample_value != 0xFFFF)
        {
            break;
        }
    }
    if (sample < 0)
    {
        sample = 0;
    }

    value += sample + 1;
    rawfile.close();

    Serial.print("InfoMaster - DS_recording_samples: ");
    Serial.println(value);

    return (value >> 1); // return samples_available;
}

int InfoMaster::Raw_file_samples(int file_id)
{
    const auto *capture = Capture_find(file_id);
    if (capture != nullptr)
    {
        return capture->audio.samples;
    }
    int result;
    LillaSerialFlashFile rawfile; // SerialFlashFile rawfile;
    rawfile.fast_open(file_id);   // rawfile = SerialFlash.open(name_file[file_id]);

    if (!rawfile)
    {
        return 0;
    }
    else
    {
        result = rawfile.size()/sizeof(int16_t);
        result = result < PATCH_CACHE_ARRAY_SAMPLES? result : PATCH_CACHE_ARRAY_SAMPLES; 
        rawfile.close();
        return result; // return samples_available;
    }
}

int16_t *InfoMaster::Sound_620_samples_array(int file_id, uint32_t A, uint32_t B)
{
    for (auto i = 0; i < 2 * WAVEBOARD_WIDTH; ++i)
    {
        samples_620_array[i] = 0;
    }

    if (B < A)
    {
        return samples_620_array;
    }

    // (B - A + 1) >= 100

    const uint32_t span = B - A + 1;

    if (span <= static_cast<uint32_t>(WAVEBOARD_WIDTH))
    {
        // In questo ramo span <= WAVEBOARD_WIDTH e quindi entra interamente in samples_basket[BASKET_INFO].
        const int samples_to_read = static_cast<int>(span);

        // Protegge da un eventuale errore o lettura incompleta.
        memset(samples_basket, 0, samples_to_read * sizeof(samples_basket[0]));

        // Una sola lettura Flash invece di WAVEBOARD_WIDTH letture.
        Read_samples(file_id, samples_basket, static_cast<int>(A), samples_to_read);

        // Mappa esattamente:
        // colonna 0                      -> campione A
        // colonna WAVEBOARD_WIDTH - 1    -> campione B
        const float position_step = static_cast<float>(span - 1) / static_cast<float>(WAVEBOARD_WIDTH - 1);

        for (int i = 0; i < WAVEBOARD_WIDTH; ++i)
        {
            const float position = position_step * i;
            const uint32_t lower = static_cast<uint32_t>(position);
            const uint32_t upper = (lower + 1 < span) ? lower + 1 : lower;
            const float fraction = position - static_cast<float>(lower);
            const float interpolated = samples_basket[lower] + (samples_basket[upper] - samples_basket[lower]) * fraction;
            const int16_t value = static_cast<int16_t>(lroundf(interpolated));
            samples_620_array[i] = value > 0 ? value : 0;
            samples_620_array[i + WAVEBOARD_WIDTH] = value < 0 ? value : 0;
        }

        return samples_620_array;
    }

    if (span <= BASKET_INFO)
    {
        // Protegge da un eventuale errore o lettura incompleta.
        memset(samples_basket, 0, span * sizeof(samples_basket[0]));

        // Un'unica lettura per tutta la finestra [A, B].
        Read_samples(file_id, samples_basket, static_cast<int>(A), span);

        for (int i = 0; i < WAVEBOARD_WIDTH; ++i)
        {
            // Suddivisione intera ed esatta dell'intervallo.
            const uint32_t first = (static_cast<uint32_t>(i) * span) / WAVEBOARD_WIDTH;
            const uint32_t last_excluded = (static_cast<uint32_t>(i + 1) * span) / WAVEBOARD_WIDTH;

            int16_t max_pos = 0;
            int16_t min_neg = 0;

            for (uint32_t j = first; j < last_excluded; ++j)
            {
                const int16_t value = samples_basket[j];

                if (value > max_pos)
                {
                    max_pos = value;
                }
                else if (value < min_neg)
                {
                    min_neg = value;
                }
            }

            samples_620_array[i] = max_pos;
            samples_620_array[i + WAVEBOARD_WIDTH] = min_neg;
        }
        return samples_620_array;
    }

    // Se la scansione completa richiederebbe più di 310 letture, viene effettuata una sola lettura per ogni colonna.
    if (span > FULL_SCAN_LIMIT)
    {
        for (int pixel = 0; pixel < WAVEBOARD_WIDTH; ++pixel)
        {
            // Intervallo completo rappresentato dalla colonna.
            const uint32_t first = static_cast<uint32_t>((static_cast<uint64_t>(pixel) * span) / WAVEBOARD_WIDTH);

            const uint32_t last_excluded = static_cast<uint32_t>((static_cast<uint64_t>(pixel + 1) * span) / WAVEBOARD_WIDTH);
            const uint32_t pixel_span = last_excluded - first;
            const uint32_t samples_to_read = pixel_span < BASKET_INFO ? pixel_span : BASKET_INFO;

            // Se l'intervallo è più grande del basket, seleziona una finestra centrata per evitare una preferenza sistematica verso il suo inizio.
            const uint32_t read_offset = first + ((pixel_span - samples_to_read) / 2);

            // Protegge da un eventuale errore o lettura incompleta.
            memset(samples_basket, 0, samples_to_read * sizeof(samples_basket[0]));

            Read_samples(file_id, samples_basket, static_cast<int>(A + read_offset), samples_to_read);

            int16_t max_pos = 0;
            int16_t min_neg = 0;

            for (uint32_t j = 0; j < samples_to_read; ++j)
            {
                const int16_t sample = samples_basket[j];

                if (sample > max_pos)
                {
                    max_pos = sample;
                }
                else if (sample < min_neg)
                {
                    min_neg = sample;
                }
            }

            samples_620_array[pixel] = max_pos;
            samples_620_array[pixel + WAVEBOARD_WIDTH] = min_neg;
        }
        return samples_620_array;
    }

    // Case BASKET_INFO < span <= FULL_SCAN_LIMIT --> block sequential reading, using all samples_basket elements.
    uint32_t processed = 0;
    int pixel = 0;
    uint32_t next_pixel_offset = static_cast<uint32_t>((static_cast<uint64_t>(pixel + 1) * span) / WAVEBOARD_WIDTH);

    while (processed < span)
    {
        const uint32_t remaining = span - processed;

        const int samples_to_read = static_cast<int>(remaining < BASKET_INFO ? remaining : BASKET_INFO);

        // Protegge da un eventuale errore o lettura incompleta.
        memset(samples_basket, 0, samples_to_read * sizeof(samples_basket[0]));

        Read_samples(file_id, samples_basket, static_cast<int>(A + processed), samples_to_read);

        for (int j = 0; j < samples_to_read; ++j)
        {
            const uint32_t sample_offset = processed + static_cast<uint32_t>(j);

            // Avanza alla colonna alla quale appartiene il campione.
            while (pixel < WAVEBOARD_WIDTH - 1 && sample_offset >= next_pixel_offset)
            {
                ++pixel;
                next_pixel_offset = static_cast<uint32_t>((static_cast<uint64_t>(pixel + 1) * span) / WAVEBOARD_WIDTH);
            }

            const int16_t sample = samples_basket[j];

            if (sample > samples_620_array[pixel])
            {
                samples_620_array[pixel] = sample;
            }
            else if (sample < samples_620_array[pixel + WAVEBOARD_WIDTH])
            {
                samples_620_array[pixel + WAVEBOARD_WIDTH] = sample;
            }
        }
        processed += static_cast<uint32_t>(samples_to_read);
    }
    return samples_620_array;
}

// Live Sampling
void InfoMaster::LS_restart_antiflicker(void)
{
    LS_antiflicker = true;
}

// Live Sampling
int16_t *InfoMaster::LS_620_samples_array(int file_id, int A_window_sample, int B_window_sample)
{
    int A_window_sample_local = A_window_sample;
    int samples_to_read;  // numero di campioni da leggere per una riga verticale della window
    int samples_per_line; // numero di campioni associati ad una riga verticale della window

    if (file_id == FIRST_LIVE_SAMPLING_FILE)
    {
        FIFO = LS_buffer_mono_ptr;
        FIFO_dim = LS_CACHE_MONO_SAMPLES; // samples
    }
    else if (file_id == FIRST_LIVE_SAMPLING_FILE + 1)
    {
        FIFO = LS_buffer_L_ptr;
        FIFO_dim = LS_CACHE_STEREO_SAMPLES; // samples
    }
    else
    {
        FIFO = LS_buffer_R_ptr;
        FIFO_dim = LS_CACHE_STEREO_SAMPLES; // samples
    }

    /*
        "samples_per_line" e' il numero di campioni associati ad 1 riga verticale della window; con ceil si garantisce la copertura della window contando
        per ogni riga della window lo stesso numero di campioni; esempio:
        B_window_sample - A_window_sample + 1 == 48
        WAVE_WIDTH_F == 10
        samples_per_line = ceil(48/10) = 5


                    0                                                                                                                             (LS_buffer_dim -1)
                    |.....................................................................................................................................|
                    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxQPxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
                                                  A_window_sample                               B_window_sample
                                                        |                                              |
        window                                          ........................C.......................
        intervalli                                      00000111112222233333444445555566666777778888899999

    */

    samples_per_line = ceil((B_window_sample - A_window_sample + 1) / WAVEBOARD_WIDTH_F);

    // non si possono prelevare piu' di BASKET_INFO campioni; si definisce percio' samples_to_read
    samples_to_read = (samples_per_line <= BASKET_INFO ? samples_per_line : BASKET_INFO);

    // Serial.print("samples_to_read: ");
    // Serial.println(samples_to_read);

    /*
    Si devono eseguire WAVEBOARD_WIDTH (tante quante le linee verticali della window) letture, ciascuna di samples_to_read campioni, avanzando di samples_per_line elementi su
    LS_cache_Mono o LS_cache_L/R.

    Se LS_XY_locked == false e samples_per_line costante, si richiede che ad ogni refresh della wave gli stessi campioni siano sempre raggruppati nello stesso ciclo di lettura;
    in questo modo si garantisce che i valori visualizzati non saltino in continuazione (flickering).

    Esempio:
    (B_window_sample - A_window_sample + 1) == 50
    WAVE_WIDTH == 7
    samples_per_line = ceil(50/7) = 8
    BASKET_INFO == 4
    samples_to_read = (8 <= BASKET_INFO ? samples_per_line : BASKET_INFO) = 4

                0                                                                                                                             (LS_buffer_dim -1)
                |.....................................................................................................................................|
    tempo0      xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxQPxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
                                              A_window_sample                                  B_window_sample
                                                    |                                                |
    window0                                         ........................C.........................
    lettura0                                       0000----1111----2222----3333----4444----5555----6666----

    tempo1      xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxQPxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
                                                         A_window_sample                                  B_window_sample
                                                               |                                                |
    window1                                                    ........................C.........................
    lettura1                                                  0000----1111----2222----3333----4444----5555----6666----

    tempo2      xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxQPxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
                                                                 A_window_sample                                  B_window_sample
                                                                       |                                                |
    window2                                                            ........................C.........................
    lettura2                                                          0000----1111----2222----3333----4444----5555----6666----
    */

    if (!LS_XY_lock)
    {
        if (LS_antiflicker)
        {
            last_A_window_sample = A_window_sample_local;
            last_samples_per_line = samples_per_line;
            LS_antiflicker = false;
        }

        if (samples_per_line == last_samples_per_line)
        {
            if (A_window_sample_local >= last_A_window_sample)
            {
                // si avanza finché si raggiunge il primo elemento
                // - multiplo di samples_per_line a partire da last_A_window_sample
                // - che non supera A_window_sample

                A_window_sample_local = last_A_window_sample;
                while ((A_window_sample_local + samples_per_line) < A_window_sample)
                {
                    A_window_sample_local += samples_per_line;
                }
            }
            else
            {
                A_window_sample_local = last_A_window_sample;
                while (A_window_sample_local > A_window_sample)
                {
                    A_window_sample_local -= samples_per_line;
                }
                if (A_window_sample_local < 0)
                {
                    A_window_sample_local += FIFO_dim;
                }
            }

            last_A_window_sample = A_window_sample_local;
        }
    }

    // si effettuano WAVEBOARD_WIDTH prelievi
    for (auto i = 0; i < WAVEBOARD_WIDTH; ++i)
    {
        // si avanza di samples_per_line (per poi prelevare samples_to_read campioni)
        float delta = samples_per_line * i;
        int16_t max_pos = 0;
        int16_t min_neg = 0;

        Read_samples(file_id, samples_basket, A_window_sample_local + delta, samples_to_read);

        for (auto j = 0; j < samples_to_read; ++j)
        {
            if (samples_basket[j] > max_pos)
            {
                max_pos = samples_basket[j];
            }
            else if (samples_basket[j] < min_neg)
            {
                min_neg = samples_basket[j];
            }
        }

        samples_620_array[i] = max_pos;
        samples_620_array[i + WAVEBOARD_WIDTH] = min_neg;
    }

    return samples_620_array;
}

void InfoMaster::Read_samples(int file_id, int16_t *destination, int seek_in, int samples_in) // samples_in <= BASKET_INFO
{
    if (Capture_find(file_id) != nullptr)
    {
        Capture_read(file_id, destination, seek_in, samples_in);
        return;
    }
    int first_byte;
    int total_bytes = samples_in * 2;
    byte *destination_byte = (byte *)destination;
    LillaSerialFlashFile rawfile; // SerialFlashFile rawfile;
    int first_packet;

    // .raw files
    if (file_id < FIRST_RECORDING_FILE)
    {
        first_byte = seek_in * 2;
        rawfile.fast_open(file_id); // rawfile = SerialFlash.open(filename);
        if (!rawfile)
        {
            return;
        }

        rawfile.seek(first_byte);
        rawfile.read(destination_byte, total_bytes);
        rawfile.close();
    }

    // Direct Sampling
    // .rec files; indicano solo una registrazione, i samples sono contenuti nei Packet (registrati con Direct Sampler)
    else if (file_id < FIRST_LIVE_SAMPLING_FILE)
    {
        first_byte = seek_in * 2;
        int recording = (file_id - FIRST_RECORDING_FILE) / 2;
        bool file_L_flag = ((file_id - FIRST_RECORDING_FILE) % 2 == 0); // 0.rec, 2.rec, 4.rec

        if (file_L_flag)
        {
            first_packet = Recording[recording].first_packet;
        }
        else
        {
            first_packet = Recording[recording].first_packet + 1;
        }

        int packet_delta = first_byte >> 16;
        int local_first_byte = first_byte % PACKET_DIM; // updated

        // Serial.print("needed_packet is: ");
        // Serial.println(needed_packet);

        rawfile.packet_fast_open(first_packet + packet_delta); // rawfile = SerialFlash.open(name_packet[first_packet + packet_delta]);
        if (!rawfile)
        {
            return;
        }

        // Serial.print(F("1 - Packet played is: "));
        // Serial.println(name_packet[first_packet + packet_delta]);

        int local_last_byte = local_first_byte + total_bytes - 1;

        if (local_last_byte < PACKET_DIM) // 1 only Packet is needed
        {
            // timer = 0;
            rawfile.seek(local_first_byte);
            rawfile.read(destination_byte, total_bytes);
            rawfile.close();
            // Serial.println(timer);
        }
        else // 2 Packets are needed - with T41@600MHz adds 40us
        {
            // timer = 0;
            int first_part = PACKET_DIM - local_first_byte;
            int second_part = total_bytes - first_part;

            rawfile.seek(local_first_byte);
            rawfile.read(destination_byte, first_part);
            rawfile.close();

            packet_delta += 2;
            rawfile.packet_fast_open(first_packet + packet_delta); // rawfile = SerialFlash.open(name_packet[first_packet + packet_delta]);
            if (!rawfile)
            {
                return;
            }

            rawfile.seek(0);
            rawfile.read(destination_byte + first_part, second_part);
            rawfile.close();

            // Serial.print(F("2 - Packet played is: "));
            // Serial.println(name_packet[first_packet + packet_delta]);
        }
    }

    // Live Sampling
    else
    {
        if (seek_in > FIFO_dim - 1)
        {
            seek_in -= FIFO_dim;
        }

        int last_sample = seek_in + samples_in - 1;

        if (last_sample <= FIFO_dim - 1)
        {
            memcpy(destination, (FIFO + seek_in), total_bytes);
        }

        else
        {
            int first_part_samples = FIFO_dim - seek_in; // lenght in samples
            int second_part_bytes = total_bytes - 2 * first_part_samples;
            memcpy(destination, (FIFO + seek_in), 2 * first_part_samples);
            memcpy(destination + first_part_samples, FIFO, second_part_bytes);
        }
    }
}