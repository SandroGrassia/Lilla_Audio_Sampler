/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#pragma once

#include <Arduino.h>
#include "SharedDelay.h"
#include "SharedVFS.h"
#include <EEPROM.h>
#include <SD.h>
#include <FS.h>
#include "config.h"
#include "Functions.h"
#include "GlobalFRAM.h"
#include "LillaFRAM_2x512.h"

class ArchivingManager
{
private:
#define AUDIO_BOARD_SDCARD 10

    // Locations of settings data in emulated EEPROM memory (dimension T4.1: 4284 byte)
    static constexpr int EEPROM_BYTES = 4284;          // Teensy 4.1 EEPROM total byte
    static constexpr int LOCATION_PATCH = 0;           // location in EEPROM of Patch[0]   --> 24 patches_number x 90 bytes = 2160 byte
    static constexpr int LOCATION_RECORDING = 2160;    // location in EEPROM of Recording[0] --> 30 recordings x 4 bytes = 120 byte
    static constexpr int LOCATION_SOUND = 2280;        // location in EEPROM of Sound[0]     --> 85 sounds x 22 bytes = 2200 byte
    static constexpr int LOCATION_OPTIMIZATION = 4235; // Profile index 0..2: 16, 12 or 10 file voices, with source-dependent pitch limits.
    static constexpr int LOCATION_FIRST_OCTAVE = 4236; // 1 byte   int8_t -2 --> 0
    static constexpr int LOCATION_DELAY = 4237;        // 17 byte
    static constexpr int LOCATION_CC_SETTINGS = 4254;  // 31 byte

    // Decode little-endian values from the EEPROM snapshot.
    static uint16_t Read_EEPROM_uint16(const uint8_t *source, size_t address);
    static uint32_t Read_EEPROM_uint32(const uint8_t *source, size_t address);

    template <class T>
    int Eeprom_writeAnything(const size_t, const T &value);
    template <class T>
    int Eeprom_readAnything(const size_t, T &value); // il valore di ritorno e' value (che viene modificato)

    void Eeprom_write_uint8_t(const size_t address, const uint8_t &value);
    void Eeprom_write_int8_t(const size_t address, const int8_t &value);
    void Eeprom_read_uint8_t(const size_t address, uint8_t &destination);
    void Eeprom_read_int8_t(const size_t address, int8_t &destination);
    uint16_t Get_location_of_Sound(const uint8_t &sound_id);
    uint16_t GET_location_of_Patch(const uint8_t &patch_id);

    String Filename_patch_Delay(const int patch_id);
    String Filename_Patch(const int patch_id);
    String Filename_Sound(const int patch_id, const int instrument_id);

    static constexpr size_t PATCH_PATH_SIZE = 32;
    bool Is_SD_inserted(void);
    void Copy_Delay_data_from_RAM_to_SD(File &file);
    void Copy_Delay_data_from_Eeprom_to_SD(File &file);
    void Copy_Delay_data_from_SD_to_Eeprom(File &file);
    void Print_Delay_data_reading_from_Eeprom(void);

    void Copy_Delay_data_from_SD_to_RAM(File &file);
    void Copy_Patch_from_RAM_to_SD(const int patch_id, File &file);
    void Copy_Patch_from_SD_to_RAM(const int patch_id, File &file);
    void Copy_Sound_from_RAM_to_SD(const int patch_id, const int instrument_id, File &file);
    void Copy_Sound_from_SD_to_RAM(const int patch_id, const int instrument_id, const int sound_id, File &file);

    static constexpr uint16_t FILEHEADER_VERSION = 0;

    struct FileHeader
    {
        uint16_t version;
        uint16_t payloadSize;
    };

    struct EEPROM_Instrument_filter_data_struct // 5 bytes
    {
        uint8_t data;           // bit0:use  bit1,2,3:modulation  bit4,5:filter_type
        uint8_t pivot;          // 0 --> 30 filter frequency/note frequency
        uint8_t resonance;      // 0 --> 40
        uint8_t index;          // 1 --> 20
        uint8_t frequency_time; // 0 --> 20
    };

    struct EEPROM_Instrument_struct // 11 bytes
    {
        bool used;
        uint8_t sound_id;
        uint8_t root_key;
        uint8_t from_note;
        uint8_t to_note;
        uint8_t info;                                // bit0: precedence, bit1: lock
        EEPROM_Instrument_filter_data_struct Filter; // 5 bytes
    };

    struct EEPROM_Patch_struct // 90 bytes
    {
        bool used; // 0:deleted  1:active
        uint8_t instruments;
        EEPROM_Instrument_struct Instrument[8]; // 8X11= 88 bytes
    } __attribute__((__packed__));              // https://cs50.stackexchange.com/questions/22297/i-am-getting-an-unexpected-sizeof-error
    EEPROM_Patch_struct EEPROM_Patch;
    static constexpr int SIZE_OF_EEPROM_PATCH = sizeof(EEPROM_Patch);

    struct alignas(4) FRAM_Instrument_filter_struct // 8 byte
    {
        uint8_t use;
        uint8_t type;
        uint8_t pivot;
        uint8_t resonance;
        uint8_t modulation;
        uint8_t index;
        uint8_t frequency_time;
        uint8_t reserved;
    };

    struct alignas(4) FRAM_Instrument_struct // 16 byte
    {
        uint8_t used;
        uint8_t root_key;
        uint16_t sound_id;
        uint8_t from_note;
        uint8_t to_note;
        uint8_t precedence;
        uint8_t lock;

        FRAM_Instrument_filter_struct Filter;
    };

    struct alignas(4) FRAM_Patch_delay_struct // 28 byte
    {
        uint16_t samples;
        int16_t samples_LR;
        uint16_t modulation_phase_LR;
        uint16_t loop_gain;
        uint8_t instrument_route[INSTRUMENTS];
        uint8_t modulation_source;
        uint8_t modulation_depth;
        uint8_t modulation_frequency;
        uint8_t reserved[9];
    };

    struct alignas(4) FRAM_Patch_struct // 192 byte
    {
        uint8_t used;
        uint8_t instruments;
        uint8_t reserved[30]; // Spazio per futuri metadati di Patch.
        FRAM_Instrument_struct Instrument[INSTRUMENTS];
        FRAM_Patch_delay_struct Delay;
        uint32_t crc32; // CRC-32/ISO-HDLC of bytes 0..187, written after the payload.
    };

    struct alignas(4) FRAM_Sound_struct // 32 byte
    {
        uint32_t A;
        uint32_t B;
        uint16_t file;
        uint16_t Noclick;
        uint8_t used;
        uint8_t mode;
        int8_t pitch;
        int8_t pan;
        uint8_t midi_channel;
        uint8_t attack_type;
        uint8_t attack;
        uint8_t decay;
        uint8_t sustain;
        uint8_t release;
        uint8_t gain;
        uint8_t reserved[5];
        uint32_t crc32; // CRC-32/ISO-HDLC of bytes 0..27, written after the payload.
    };

    struct alignas(4) FRAM_Recording_struct // 8 byte
    {
        uint16_t first_packet;
        uint16_t packets;
        uint8_t stereo;
        uint8_t consistent;
        uint8_t reserved[2];
    };

    struct alignas(4) FRAM_CC_settings_struct // 16 byte
    {
        uint8_t sound_gain[INSTRUMENTS];
        uint8_t lowpass_filter;
        uint8_t reserved[7];
    };

    struct alignas(4) FRAM_System_struct // 256 byte
    {
        uint8_t optimization;
        int8_t first_octave;
        uint8_t reserved_alignment[2];
        FRAM_CC_settings_struct CC_settings;
        uint8_t reserved[236];
    };

    /*

    capacità totale: 131.072 byte (128 KiB);

    Intervallo	    Dimensione	Contenuto
    0x00000–0x000FF	256	        Header
    0x00100–0x096FF	38.400	    200 Patch
    0x09700–0x0FAFF	25.600	    800 Sound
    0x0FB00–0x0FBEF	240	        30 Recording
    0x0FBF0–0x0FBFF	16	        Allineamento riservato
    0x0FC00–0x0FCFF	256	        System
    0x0FD00–0x1FFFF	66.304	    Libero

    FRAM 0: 768 byte liberi
    FRAM 1: 65.536 byte liberi
    Totale: 66.304 byte = 64,75 KiB

    intervalli liberi
    FRAM 0: 0x0FD00–0x0FFFF
    FRAM 1: 0x10000–0x1FFFF
    */

    static constexpr uint16_t FRAM_PATCHES = 200;
    static constexpr uint16_t FRAM_SOUNDS = 800;

    static constexpr uint32_t FRAM_HEADER_ADDRESS = 0x00000;
    static constexpr uint32_t FRAM_HEADER_BYTES = 0x00100;

    static constexpr uint32_t FRAM_PATCH_ADDRESS = FRAM_HEADER_ADDRESS + FRAM_HEADER_BYTES; // 0x00100
    static constexpr uint32_t FRAM_PATCH_BYTES = FRAM_PATCHES * sizeof(FRAM_Patch_struct);  // 0x09600

    static constexpr uint32_t FRAM_SOUND_ADDRESS = FRAM_PATCH_ADDRESS + FRAM_PATCH_BYTES; // 0x09700
    static constexpr uint32_t FRAM_SOUND_BYTES = FRAM_SOUNDS * sizeof(FRAM_Sound_struct); // 0x06400

    static constexpr uint32_t FRAM_RECORDING_ADDRESS = FRAM_SOUND_ADDRESS + FRAM_SOUND_BYTES;    // 0x0FB00
    static constexpr uint32_t FRAM_RECORDING_BYTES = RECORDINGS * sizeof(FRAM_Recording_struct); // 0x000F0

    static constexpr uint32_t FRAM_SYSTEM_ADDRESS = 0x0FC00;
    static constexpr uint32_t FRAM_SYSTEM_BYTES = sizeof(FRAM_System_struct); // 0x00100

    static constexpr uint32_t FRAM_FIRST_FREE_ADDRESS = FRAM_SYSTEM_ADDRESS + FRAM_SYSTEM_BYTES; // 0x0FD00

    static_assert(sizeof(FRAM_Patch_struct) == 192);
    static_assert(sizeof(FRAM_Sound_struct) == 32);
    static_assert(offsetof(FRAM_Sound_struct, crc32) == 28);

    static_assert(sizeof(FRAM_Instrument_filter_struct) == 8);
    static_assert(sizeof(FRAM_Instrument_struct) == 16);
    static_assert(sizeof(FRAM_Patch_delay_struct) == 28);
    static_assert(sizeof(FRAM_Recording_struct) == 8);
    static_assert(sizeof(FRAM_CC_settings_struct) == 16);
    static_assert(sizeof(FRAM_System_struct) == 256);
    static_assert(offsetof(FRAM_System_struct, CC_settings) == 4);
    static_assert(offsetof(FRAM_Patch_struct, Instrument) == 32);
    static_assert(offsetof(FRAM_Patch_struct, Delay) == 160);
    static_assert(offsetof(FRAM_Patch_struct, crc32) == 188);

    static_assert(FRAM_RECORDING_ADDRESS + FRAM_RECORDING_BYTES <= FRAM_SYSTEM_ADDRESS);
    static_assert(FRAM_FIRST_FREE_ADDRESS <= LillaFRAM_2x512::TOTAL_SIZE);

    uint32_t FRAM_Get_patch_address(uint8_t patch_id);
    uint32_t FRAM_Get_instrument_address(uint8_t patch_id, uint8_t instrument_id);
    uint32_t FRAM_Get_filter_address(uint8_t patch_id, uint8_t instrument_id);
    uint32_t FRAM_Get_delay_address(uint8_t patch_id);
    uint32_t FRAM_Get_sound_address(uint16_t sound_id);
    uint32_t FRAM_Get_recording_address(uint8_t recording_id);
    uint32_t FRAM_Get_CC_settings_address();

public:
    ArchivingManager(void) {}

    void Save_CC_lowpass_filter(const int CC_lowpass_filter);
    void Read_CC_lowpass_filter(uint8_t &CC_lowpass_filter);
    void Save_optimization(const uint8_t optimization);
    void Read_optimization(uint8_t &optimization);
    void Save_CC_Sound_gain(const uint8_t instrument_id, const uint8_t CC_Sg_instrument);
    void Read_CC_Sound_gain(const uint8_t instrument_id, uint8_t &CC_Sg_instrument);
    void Copy_patch_Delay_data_from_Eeprom_to_Ram(Delay_data_struct &Delay_data);
    void Save_Delay_to_Eeprom(const Delay_data_struct &Delay_data);
    void Read_first_octave(int8_t &first_octave);
    void Save_first_octave(const int8_t first_octave);
    void Save_Sound(const int sound_id);
    bool Validate_Sound_AB_file_raw(uint32_t sound_id);
    void Read_Sound(const int sound_id);
    void Save_Patch(const int patch_id);
    void Read_Patch(const int patch_id);
    void Save_DS_Recording(const int recording);
    void Save_DS_Recording(const int &recording, const EEPROM_VFS_Recording &EEPROM_Rec_recording);
    void Read_DS_Recording(const int &recording, EEPROM_VFS_Recording &EEPROM_Rec_recording);
    void Save_setup_file(File &file);              // File e' l'oggetto file incluso in FS.h
    void Copy_setup_from_Eeprom_to_SD(File &file); // File e' l'oggetto file incluso in FS.h
    void Reset_EEPROM(void);
    void Print_EEPROM_content(void);

    void Copy_patch_Delay_data_from_Eeprom_to_SD(const int patch_id);
    void Copy_patch_Delay_data_from_RAM_to_SD(const int patch_id);
    bool Copy_patch_Delay_data_from_SD_to_RAM(const int patch_id);
    bool Copy_patch_Delay_data_from_SD_to_Eeprom(const int patch_id);
    void Print_patch_Delay_file_reading_from_SD(const int patch_id);
    bool Delete_patch_Delay_data_in_SD(const int patch_id);

    bool Copy_Patch_from_RAM_to_SD(const int patch_id);
    bool Copy_Patch_from_SD_to_RAM(const int patch_id);

    bool Copy_Sound_from_RAM_to_SD(const int patch_id, const int instrument_id);
    bool Copy_Sound_from_SD_to_RAM(const int patch_id, const int instrument_id, const int sound_id);

    int GET_location_of_DS_Recording(const int &recording);

    bool Save_Patch_from_RAM_to_SD(const int patch_id);
    bool Resume_Patch_from_SD_to_RAM(const int patch_id);

    // Initialize FRAM and stop metadata writers before calling. EEPROM is never changed.
    // Failure leaves a partial archive: retry before enabling the FRAM runtime backend.
    byte Migrate_EEPROM_to_FRAM();
    struct FRAM_Repair_report
    {
        uint16_t cleared_patches = 0;
        uint16_t cleared_sounds = 0;
        uint16_t defaulted_sounds = 0;
        uint16_t failed_id = UINT16_MAX;
        bool failed_sound = false;
    };
    // Startup only, before runtime loading. Repairs FRAM without changing the RAM model.
    byte Repair_Patch_Sound_in_FRAM(FRAM_Repair_report &report);
    // Startup only: clears runtime arrays on failure and reports the failing record ID.
    byte Load_Patch_Sound_from_FRAM(uint16_t &failed_id, bool &failed_sound);
    static constexpr byte FRAM_ERROR_VERIFY = 13;
    static constexpr byte FRAM_ERROR_SOURCE = 14;
    static constexpr byte FRAM_ERROR_CRC = 12; // Stored Patch or Sound checksum does not match its payload.

    // Patch reads validate the CRC before publishing data; writes calculate and store the CRC last.
    byte FRAM_Write_patch(uint8_t patch_id, const FRAM_Patch_struct &source);
    byte FRAM_Read_patch(uint8_t patch_id, FRAM_Patch_struct &destination);

    // Nested accesses validate the containing Patch; nested writes save it again with a new CRC.
    byte FRAM_Write_instrument(uint8_t patch_id, uint8_t instrument_id, const FRAM_Instrument_struct &source);
    byte FRAM_Read_instrument(uint8_t patch_id, uint8_t instrument_id, FRAM_Instrument_struct &destination);

    byte FRAM_Write_filter(uint8_t patch_id, uint8_t instrument_id, const FRAM_Instrument_filter_struct &source);
    byte FRAM_Read_filter(uint8_t patch_id, uint8_t instrument_id, FRAM_Instrument_filter_struct &destination);

    byte FRAM_Write_delay(uint8_t patch_id, const FRAM_Patch_delay_struct &source);
    byte FRAM_Read_delay(uint8_t patch_id, FRAM_Patch_delay_struct &destination);

    // Sound reads validate the CRC before publishing data; writes calculate and store the CRC last.
    byte FRAM_Write_sound(uint16_t sound_id, const FRAM_Sound_struct &source);
    byte FRAM_Read_sound(uint16_t sound_id, FRAM_Sound_struct &destination);

    byte FRAM_Write_recording(uint8_t recording_id, const FRAM_Recording_struct &source);
    byte FRAM_Read_recording(uint8_t recording_id, FRAM_Recording_struct &destination);

    byte FRAM_Write_CC_settings(const FRAM_CC_settings_struct &source);
    byte FRAM_Read_CC_settings(FRAM_CC_settings_struct &destination);

    byte FRAM_Write_system(const FRAM_System_struct &source);
    byte FRAM_Read_system(FRAM_System_struct &destination);
};
