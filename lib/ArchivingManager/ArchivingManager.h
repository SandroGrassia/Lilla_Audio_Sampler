/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#pragma once

#include <Arduino.h>
#include "SharedDelay.h"
#include "SharedVFS.h"
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

    String Filename_Patch(const int patch_id);
    String Filename_Sound(const int patch_id, const int instrument_id);

    static constexpr size_t PATCH_PATH_SIZE = 32;
    bool Is_SD_inserted(void);
    bool Copy_Patch_from_RAM_to_SD(const int patch_id, File &file);
    bool Copy_Patch_from_SD_to_RAM(const int patch_id, File &file);
    void Copy_Sound_from_RAM_to_SD(const int patch_id, const int instrument_id, File &file);
    void Copy_Sound_from_SD_to_RAM(const int patch_id, const int instrument_id, const int sound_id, File &file);

    static constexpr uint16_t FILEHEADER_VERSION = 0;

    struct FileHeader
    {
        uint16_t version;
        uint16_t payloadSize;
    };

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

    struct alignas(4) FRAM_Recording_struct // 12 byte
    {
        uint16_t first_packet;
        uint16_t packets;
        uint8_t stereo;
        uint8_t consistent;
        uint8_t reserved[2];
        uint32_t crc32; // CRC-32/ISO-HDLC of bytes 0..7, written after the payload.
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
        uint8_t key_step;
        uint8_t reserved_alignment;
        FRAM_CC_settings_struct CC_settings;
        uint8_t reserved[232];
        uint32_t crc32; // CRC-32/ISO-HDLC of bytes 0..251, including CC_settings, written after the payload.
    };

    /*

    capacità totale: 131.072 byte (128 KiB);

    Intervallo	    Dimensione	Contenuto
    0x00000–0x000FF	256	        Header
    0x00100–0x096FF	38.400	    200 Patch
    0x09700–0x0FAFF	25.600	    800 Sound
    0x0FB00–0x0FC67	360	        30 Recording
    0x0FC68–0x0FCFF	152	        Allineamento riservato
    0x0FD00–0x0FDFF	256	        System
    0x0FE00–0x1FFFF	66.048	    Libero

    FRAM 0: 512 byte liberi
    FRAM 1: 65.536 byte liberi
    Totale: 66.048 byte = 64,5 KiB

    intervalli liberi
    FRAM 0: 0x0FE00–0x0FFFF
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
    static constexpr uint32_t FRAM_RECORDING_BYTES = RECORDINGS * sizeof(FRAM_Recording_struct); // 0x00168

    static constexpr uint32_t FRAM_SYSTEM_ADDRESS = 0x0FD00;
    static constexpr uint32_t FRAM_SYSTEM_BYTES = sizeof(FRAM_System_struct); // 0x00100

    static constexpr uint32_t FRAM_FIRST_FREE_ADDRESS = FRAM_SYSTEM_ADDRESS + FRAM_SYSTEM_BYTES; // 0x0FE00

    struct alignas(4) FRAM_Backup_header_struct
    {
        uint8_t magic[8];
        uint16_t version;
        uint16_t header_bytes;
        uint32_t payload_bytes;
        uint32_t payload_crc32;
    };
    static constexpr uint16_t FRAM_BACKUP_VERSION = 2;

    static_assert(sizeof(FRAM_Patch_struct) == 192);
    static_assert(sizeof(FRAM_Sound_struct) == 32);
    static_assert(offsetof(FRAM_Sound_struct, crc32) == 28);

    static_assert(sizeof(FRAM_Instrument_filter_struct) == 8);
    static_assert(sizeof(FRAM_Instrument_struct) == 16);
    static_assert(sizeof(FRAM_Patch_delay_struct) == 28);
    static_assert(sizeof(FRAM_Recording_struct) == 12);
    static_assert(offsetof(FRAM_Recording_struct, crc32) == 8);
    static_assert(sizeof(FRAM_CC_settings_struct) == 16);
    static_assert(sizeof(FRAM_System_struct) == 256);
    static_assert(offsetof(FRAM_System_struct, key_step) == 2);
    static_assert(offsetof(FRAM_System_struct, CC_settings) == 4);
    static_assert(offsetof(FRAM_System_struct, crc32) == 252);
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

    byte Save_CC_lowpass_filter(const int CC_lowpass_filter);
    byte Read_CC_lowpass_filter(uint8_t &CC_lowpass_filter);
    byte Save_CC_settings(const uint8_t sound_gain[INSTRUMENTS], uint8_t lowpass_filter);
    byte Read_CC_settings(uint8_t sound_gain[INSTRUMENTS], uint8_t &lowpass_filter);
    byte Save_optimization(const uint8_t optimization);
    byte Read_optimization(uint8_t &optimization);
    byte Save_key_step(const uint8_t key_step);
    byte Read_key_step(uint8_t &key_step);
    byte Save_CC_Sound_gain(const uint8_t instrument_id, const uint8_t CC_Sg_instrument);
    byte Read_CC_Sound_gain(const uint8_t instrument_id, uint8_t &CC_Sg_instrument);
    byte Read_Delay(uint8_t patch_id, Delay_data_struct &Delay_data);
    byte Save_Delay(uint8_t patch_id, const Delay_data_struct &Delay_data);
    byte Read_first_octave(int8_t &first_octave);
    byte Save_first_octave(const int8_t first_octave);
    byte Save_Sound(const int sound_id);
    bool Validate_Sound_AB_file_raw(uint32_t sound_id);
    byte Read_Sound(const int sound_id);
    byte Save_Patch(const int patch_id);
    byte Read_Patch(const int patch_id);
    byte Save_DS_Recording(const int recording);
    byte Read_DS_Recording(const int recording);
    bool Copy_Patch_from_RAM_to_SD(const int patch_id);
    bool Copy_Patch_from_SD_to_RAM(const int patch_id);

    bool Copy_Sound_from_RAM_to_SD(const int patch_id, const int instrument_id);
    bool Copy_Sound_from_SD_to_RAM(const int patch_id, const int instrument_id, const int sound_id);

    bool Save_Patch_from_RAM_to_SD(const int patch_id);
    bool Resume_Patch_from_SD_to_RAM(const int patch_id);

    byte Factory_reset_FRAM(bool publish_ready = true);
    byte Check_FRAM_archive();
    byte Set_FRAM_archive_state(uint32_t state);
    bool Verify_FRAM_backup(File &file);
    bool Export_FRAM_backup();
    static constexpr uint32_t RESTORE_IN_PROGRESS = 0x52535452;
    static constexpr uint32_t ARCHIVE_READY = 0x52454144;
    bool Save_FRAM_backup(File &file);
    bool Restore_FRAM_backup(File &file);
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
    struct FRAM_Recording_repair_report
    {
        uint8_t cleared_recordings = 0;
        uint8_t failed_id = UINT8_MAX;
    };
    // Startup only. CRC-corrupt Recording metadata is replaced with an empty, consistent record.
    byte Repair_Recordings_in_FRAM(FRAM_Recording_repair_report &report);
    struct FRAM_System_repair_report
    {
        bool defaulted = false;
    };
    // Startup only. A CRC-corrupt System record is replaced with safe defaults.
    byte Repair_System_in_FRAM(FRAM_System_repair_report &report);
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
