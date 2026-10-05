 /*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 */

#pragma once

#include <Arduino.h>
#include <SerialFlash.h>
#include "SharedElements.h"
#include "SharedVFS.h"
#include "config.h"

class LillaSerialFlashFile : public SerialFlashFile
{
    // Questa classe estende SerialFlashFile includendo un metodo per per l'apertura rapida (fast_open)
    // di un oggetto (rawfile). Utilizzo:
    // Lilla_SerialFlashFile rawfile;
    // rawfile.fast_open(id_file);
    //
    // Per ridefinire l'oggetto come SerialFlashFile:
    // SerialFlashFile rawfile1 = rawfile;

public:
    LillaSerialFlashFile(void) : SerialFlashFile() {} // Costruttore che chiama il costruttore della classe base

    // funzione di apertura da usare al posto di SerialFlash.open(nome_file)
    void fast_open(int id_file);
    void packet_fast_open(int id_packet);
    static bool Read_audio_samples(int file_id, int16_t *destination, int first_sample, int samples_count); // Read the requested Flash samples completely; the caller provides buffer capacity and protects shared SPI access.
    static bool Read_audio_samples_background(int file_id, int16_t *destination, int first_sample, int samples_count); // Main-loop reads: reserve SPI before audio can start and release the bus every 128 samples.
};

class FlashFileRegisterParser
{
    // questa classe contiene il database utilizzato da Lilla_SerialFlashFile::fast_open
private:
    static uint32_t address_array[RAW_FILES + PACKETS];
    static uint32_t length_array[RAW_FILES + PACKETS];
    static uint16_t dirindex_array[RAW_FILES + PACKETS];

public:
    static void Read_all_file_data();
    static uint32_t address(int id_file);
    static uint32_t length(int id_file);
    static uint16_t dirindex(int id_file);
};
