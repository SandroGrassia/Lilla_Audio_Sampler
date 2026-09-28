/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#pragma once

#include <Arduino.h>
#include <AudioStream.h>
#include <SerialFlash.h>
#include <spi_interrupt.h>
#include <Wire.h>
#include <utility/dspinst.h>
#include "config.h"
#include "LillaSerialFlash.h"
#include "SharedElements.h"
#include "AudioVCF.h"
#include "WaveLFO.h"
#include "StereoLiveSampler.h"
#include "SharedLiveSampler.h"
#include "PlayersStatistics.h"
#include "Functions.h"
#include "AudioADSR.h"
#include "AudioTables.h"
#include "PlayerReadDiagnostics.h"
#include "PlayerReadBudget.h"

class AudioPlayer : public AudioStream
{
private:
    static constexpr int BASKET_DIM = 4500;    // la dimensione deve essere non inferiore a AUDIO_BLOCK_SAMPLES * MAX_PITCH_WAVETABLE
    static_assert(BASKET_DIM >= AUDIO_BLOCK_SAMPLES * MAX_PITCH_WAVETABLE + 2);
    static_assert(MAX_PITCH_PSRAM <= MAX_PITCH_WAVETABLE && MAX_PITCH_FLASH <= MAX_PITCH_WAVETABLE);
    static int16_t samples_basket[BASKET_DIM]; // cache array unico per samples copiati dai Player
    int16_t block[AUDIO_BLOCK_SAMPLES];
    int16_t budget_tail_sample = 0; // Last rendered mono sample for a continuous source-free retirement ramp.
    bool budget_tail_pending = false; // Emit one fading cached block before the replacement renders.
    bool rendered_block_valid = false; // Never read an uninitialized output block when retiring a new voice.
    float budget_tail_pan_L = 0.0f; // Preserve outgoing routing gains across a replacement start.
    float budget_tail_pan_R = 0.0f; // Preserve outgoing routing gains across a replacement start.
    uint8_t mix_samples = 32;
    uint8_t identity;
    enum PlayerStates
    {
        IDLE,        // no output available
        RUNNING,     // running, no stop request
        FADING,      //  stop requested, output is falling down
        IDLE_REQUEST // last update, than go to IDLE
    };

    int state;
    bool patch_release_pending = false;
    void Enforce_cycle_deadline(void);
    uint32_t patch_release_started = 0;
    static constexpr uint32_t QUICK_RELEASE_TIME = 2000; // Maximum lifetime in milliseconds of outgoing patch voices.
    static constexpr uint32_t PATCH_RELEASE_BLOCK_MS = (1000u * AUDIO_BLOCK_SAMPLES + static_cast<uint32_t>(AUDIO_SAMPLE_RATE) - 1u) / static_cast<uint32_t>(AUDIO_SAMPLE_RATE);
    static_assert(QUICK_RELEASE_TIME > 2u * PATCH_RELEASE_BLOCK_MS);
    LillaSerialFlashFile rawfile; // SerialFlashFile rawfile;
    int file_id;
    uint32_t samples_counter; // starting from play()

    volatile bool power_on = false; // play() ONLY can set power_on = true; the stop event (Release_note(), ADSR_gain=0, end of file) coming for first sets power_on=false:
    volatile bool idle = true;

    /*
    power_on and idle

    Get_ready_to_play() sets power_on=true, than Player waits for next update() to start playing (idle=false);
    When Player starts playing, idle is set idle=false;
    The stop event coming for first sets power_on=false:
        - Release_note() has less priority, it only sets power_on=false
        - ADSR_gain=0 immediately sets power_on=false, idle=true
        - same End-Of-File event

    ** Release_note() comes first:
    play     ______!________________________________
    Release_note ____________________%______________

    power_on ______|-----------------%______________
    idle     ------|_______________________|--------

    ADSR_gain _____|***********************|________
    EOF      ____________________________________!__

    state   0000000111111111111111111222222000000000



    ** ADSR_gain=0 comes first:
    play     ______!________________________________
    Release_note _____________________!_____________

    power_on ______|--------------%_________________
    idle     ------|______________|-----------------

    ADSR_gain______|**************%_________________
    EOF      ____________________________________!__

    state   0000000111111111111111100000000000000000



    ** End-Of-File comes first:
    play     ______!________________________________
    Release_note ____________________!______________

    power_on ______|----------%_____________________
    idle     ------|__________|---------------------

    ADSR    _______|*************************|______
    EOF      _________________%_____________________

    state   0000000111111111110000000000000000000000

    */

    float *vibrato_array_ptr;
    float vibrato_array_element_float = 0;
    uint8_t *vibrato_array_last_element_ptr;
    float pitch_vibrato = 1.0;

    // ADSR
    AudioADSR *ADSR = nullptr;

    // MIDI_LOOP
    int track;
    int track_wait;

    // VCF
    bool VCF_connect = false;
    bool VCF_modulated = false;
    float VCF_frequency_pivot = 5000.0;
    float VCF_central_frequency;
    float VCF_frequency_array[4];
    int8_t VCF_pivot_steps = 0;
    float VCF_pivot_grain;
    void Send_LFO_to_VCF(void);
    int LFO_modulation = 0;
    float LFO_index = 1.0;
    uint8_t LFO_periodic = 0;
    float LFO_seconds = 1.0;
    int LFO_index_steps = 0;
    float LFO_index_grain = 0;

    // check performance
    elapsedMicros local_timer;

    // Edit variables
    uint8_t mode_player_E;
    int16_t *Wavetable_E_ptr;
    int16_t *Noclick_E_ptr;
    int A_Flash_sample_E;
    int B_Flash_sample_E;
    int C_Flash_sample_E;
    int B_Flash_sample_shifted_E;
    uint16_t delta_Noclick_E;
    bool use_Wavetable_E;
    int32_t Wavetable_length_E;
    int32_t Flash_first_RAM_sample_E;
    float pitch_limit_E;

    // Operation variables
    bool precedence = false;
    int midi_channel = 0;
    int local_patch = 0;
    int instrument_id = 0;
    int sound_id = 0;
    int note = 0;
    unsigned long time_stamp = 0;
    int update_time;

    float pitch = 0.0;
    float pitch_note;
    float velocity_gain; // 0 <= velocity_gain <= 1.0
    float pitch_based_gain_correction;
    float pitch_tune = 1.0;
    float pitch_limit;
    float pitch_bend = 1.0;

    float ADSR_gain;
    float volume_gain = 1.0;
    float K_volume_gain; // [Gain/sample]
    uint32_t JV0;
    int8_t pan_int;
    float pan_gain_L;
    float pan_gain_R;
    float K_pan_gain_L, K_pan_gain_R;
    uint32_t JP0;
    float K_resolution_step = 1;
    uint8_t downsampling;
    int16_t stored_sample;
    uint8_t sliding;
    float raw_first_value_cache[AUDIO_BLOCK_SAMPLES];
    float fast_stop_gain;
    uint8_t mode_player;
    int A_Flash_sample;
    int B_Flash_sample;
    int B_Flash_sample_0;
    int C_Flash_sample;
    int B_Flash_sample_shifted;
    float a_sample;
    float b_sample;
    float a_first_sample;
    float initial_index_offset;
    int16_t *Wavetable_ptr;
    bool use_Wavetable_old;
    int16_t INT_Wavetable_B_sample;
    int16_t INT_Wavetable_C_sample;
    int32_t Wavetable_length;
    int32_t Flash_first_RAM_sample;
    int16_t *Noclick_ptr;
    uint16_t delta_Noclick = 0;
    
    // AudioTables: banco corrente, prossima partenza e modifica in attesa.
    uint8_t tables_bank_mask = 0;
    uint8_t tables_bank_mask_wait = 0;
    uint8_t tables_bank_mask_E = 0;

    // PSRAM operation
    AudioFileSource source_now;
    AudioFileSource source_wait;
    bool spi_in_use = false;
    void Close_source(void); // Close the current Flash handle and release its SPI lease exactly once.

    // wait variables
    int file_id_wait;
    float volume_gain_wait;
    int patch_id_wait;
    int instrument_id_wait;
    int sound_id_wait;
    int note_wait;
    float pitch_note_wait;    // 0.5=half speed 1=original speed 2=double speed
    float velocity_gain_wait; // 0 <= velocity_gain <= 1.0
    float pitch_tune_wait;
    float pitch_bend_wait;
    uint8_t mode_player_wait;
    int pan_int_wait;
    float pan_gain_L_wait;
    float pan_gain_R_wait;
    int A_Flash_sample_wait;
    int B_Flash_sample_wait;
    int C_Flash_sample_wait;
    int B_Flash_sample_shifted_wait;
    float a_first_sample_wait;
    int16_t *Wavetable_wait_ptr;
    int16_t *Noclick_wait_ptr;
    int delta_Noclick_wait;
    int Wavetable_length_wait;
    int Flash_first_RAM_sample_wait;
    float pitch_limit_wait;
    bool use_Wavetable_wait;
    int K_resolution_step_wait;
    int downsampling_wait;

    // flags
    bool warmup_for_play_again_flag = false; // quando si riceve Get_ready_to_play ma il Player è !idle, il Player non puo' partire immediatamente
    bool restart_flag = false;
    bool pending_note_released = false; // A NoteOff for the replacement must survive until Start_playing().
    float modulation_depth = 0.0f;
    bool main_settings_editing_flag = false;
    bool vibrato_flag = false;
    bool pitch_tune_flag = false;
    bool pitch_bend_flag = false;
    bool resolution_flag_wait = false;
    bool downsampling_flag_wait = false;
    bool volume_flag = false;
    bool volume_gain_warmup_flag = false;
    bool pan_flag = false;
    bool pan_gain_warmup_flag = false;
    bool set_effects_flag = false;
    bool resolution_flag = false;
    bool downsampling_flag = false;
    bool use_Wavetable = false;

    // DIRECT_SAMPLIG
    int recording;
    bool recording_flag = false;
    bool stereo_flag = false;
    int packet_delta = 0;
    int first_packet = 0;

    // LIVE_SAMPLING
    bool LS_flag = false;
    inline static volatile bool live_unrecorded_notice = false; // Segnala al loop principale un tentativo di riproduzione di audio non ancora registrato nel buffer Live Sampler.
    bool live_forward_end = false; // Fade the final block when FWD reaches never-recorded samples.
    bool live_forward_empty = false;
    int16_t live_forward_last_sample = 0;
    int16_t *FIFO;
    int FIFO_dim;

    inline static bool read_diagnostics_enabled = false;
    PlayerReadDiagnostics read_diagnostics;
    void Record_read(PlayerReadSource source, int count, uint16_t flags = 0);
    void Harvest_samples(void); // Optional diagnostics wrap the unchanged source harvest paths.
    void Update_pitch(void);
    void Update_volume_gain(void);
    void Update_pan_gain(void);
    void Start_playing(void); // Acquire the prepared source and replace the old note without leaking its SPI lease.

    bool Harvest_live_forward_end(void); // Read only valid first-pass samples and hold the last value for the fade.
    void Fade_live_forward_end(void);
    void Flash_memory_harvest(void);
    int Loop_period(int first, int last, int crossfade, uint8_t mode) const; // Return the sample period for forward, reverse or ping-pong loops.
    bool Fill_loop_samples(int16_t *destination, int count, int phase, int first, int last, int crossfade, uint8_t mode, const int16_t *noclick); // Caller supplies a valid buffer, positive count and geometry with a positive Loop_period(). Fill through the active sample reader.
    void Loop_memory_harvest(void); // Fill repeated file loops from Flash/cache without reading beyond A/B or NoClick.
    void Wavetable_harvest(void);
    void Read_samples(int16_t *destination, int first_sample, int total_samples); // Read logical samples from Flash, a complete cache or the live circular buffer.

    float Mirror(float pivot, float value);
    void Append_reversed(int16_t *target_ptr, uint16_t first_index, int16_t *source_ptr, uint16_t N);
    void Append(int16_t *target_ptr, uint16_t first_index, int16_t *source_ptr, uint16_t N);

    bool myLED = false;
    int led_instrument_id = 0;
    int led_track = -1; // Captured at registration; -1 identifies the Performance counter.
    void My_LED(bool on);

public:
    AudioPlayer(void) : AudioStream(0, NULL)
    {
        begin();
    }

    static void Enable_read_diagnostics(bool enabled) { read_diagnostics_enabled = enabled; }
    const PlayerReadDiagnostics &Get_read_diagnostics(void) const { return read_diagnostics; } // Read from audio IRQ or with audio IRQ disabled.
    PlayerReadBudget::Plan Get_read_plan(bool pending = false, bool edited = false) const; // Capture source geometry without reading samples; caller owns the audio IRQ.
    float Reserved_read_us(void) const; // Maximum current/pending/edited full-block cost, with idle voices excluded.
    float Current_read_us(uint32_t output_samples) const; // Cost of an outgoing crossfade segment.
    void Retire_for_read_budget(void); // Release source ownership now and fade previously computed audio without further source reads.
    void Set_edit_mix_samples(uint8_t value); // Override only a pending edit crossfade before its next render.
    bool Has_pending_edit(void) const { return main_settings_editing_flag; } // Expose edit transitions to the shared read-budget planner.
    static bool Take_live_unrecorded_notice(void); // Consume from the main loop with audio interrupts disabled by the caller.
    void begin(void);
    virtual void update(void);
    int State(void);

    // Settati alla creazione di ciascuna istanza
    void Set_identity(uint8_t value);
    AudioVCF *VCF_ptr = nullptr;
    WaveLFO *LFO_ptr = nullptr;
    StereoLiveSampler *LiveSampler_ptr = nullptr;        // meglio static!
    PlayersStatistics *Players_statistics_ptr = nullptr; // meglio static!

    // Received when Live Sampler buffer is created
    int16_t *LS_buffer_mono_ptr = nullptr;
    int16_t *LS_buffer_L_ptr = nullptr;
    int16_t *LS_buffer_R_ptr = nullptr;

    // Received after ADS moduls
    void Set_ADSR_ptr(AudioADSR *ptr);

    // received before playing
    void Connect_VCF(bool use, int type, float pivot, float resonance, bool modulated);
    void Connect_LFO_TO_VCF(uint8_t modulation, float index, uint8_t periodic, float frequency_time); // used by Midi_Reader
    void Start_VCF(void);
    void Update_VCF_pivot(float new_pivot);
    void Update_LFO_index(float new_index);
    void Update_VCF_resonance(float resonance);

    bool isPlaying(void);
    bool isPoweredOn(void);
    bool Has_pending_note(void) const { return state != IDLE && (warmup_for_play_again_flag || restart_flag); }
    bool Needs_restart_mix(void) const { return state != IDLE && warmup_for_play_again_flag; }
    int Assigned_note(void) const { return Has_pending_note() ? note_wait : note; }
    int Assigned_track(void) const { return Has_pending_note() ? track_wait : track; }
    int Assigned_instrument(void) const { return Has_pending_note() ? instrument_id_wait : instrument_id; }
    int Assigned_patch(void) const { return Has_pending_note() ? patch_id_wait : local_patch; }
    void Set_modulation(uint8_t value);
    void set_file(int file_id_in); // Select a Flash or Live Sampler file for the next note.
    void Set_source(const AudioFileSource &source); // Attach the prepared source before configuring the next note.
    void Refresh_cached_source(const AudioFileSource &source); // Promote identical data and refresh current/pending pitch limits; call with audio interrupts disabled.
    bool Uses_flash(void) const; // Reserve Flash capacity for current playback and queued starts or edits.
    bool Uses_sample_voice(void) const; // Reserve one shared Flash/cache voice across current playback, queued starts and edits.
    bool Fast_stop_using_cache(uint16_t cache_mask); // Fade a current cache reader after pending edits and restarts complete.

    void Set_volume(float volume_gain_value);
    void Update_volume(float volume_gain_value);
    void Set_pan(int pan_int_value);
    void Set_effects(float resolution_exp, uint8_t downsampling_in);
    void Set_pitch(float pitch_tune_in);
    void Set_note(float pitch_note_in);
    void Set_pitch_bend(float pitch_bend_in);
    void Write_loop_track(int track);
    void Update_pan(float pan_int_value);
    void Set_vibrato_pointers(float *p_vibrato_array_in, uint8_t *p_vibrato_array_last_element_in);
    void Set_vibrato_flag(bool value);
    void Write_update_time(uint16_t value);
    void Set_mix_samples(uint8_t value);

    void Main_settings(uint8_t mode_in, int A_value_in, int B_value_in, uint16_t delta_Noclick_in, bool use_Wavetable_in, int16_t *p_Noclick_in, int16_t *p_Wavetable_in, uint8_t tables_bank_mask_in = 0); // Prepare the next start from precomputed preset parameters and table references. 
    void Get_ready_to_play(float pitch_note_in, float velocity_in, int patch_in, uint8_t instrument_in, uint16_t sound_id_in, uint8_t note_in); // setta una serie di valori e flag, individuati col suffisso "wait", utilizzati alla successiva partenza/ripartenza del Player, comandata da update()
    void Main_settings_editing(uint8_t mode_in, int A_value_in, int B_value_in, uint16_t delta_Noclick_in, bool use_Wavetable_in, int16_t *p_Noclick_in, int16_t *p_Wavetable_in, uint8_t tables_bank_mask_in = 0); // Queue a crossmixed edit of the current note using the supplied tables.
    
    void Release_note(void); // release note, fires ADSR "release"
    void Release_patch(int patch_id); // Cancel outgoing queued notes and bound current patch voices, including voices already fading; call with audio interrupts disabled.
    void Fast_stop(void); 
    bool Fast_stop_using_tables(uint8_t banks_mask); // Request a fast stop when current playback uses one of the specified banks. Call from the audio IRQ or with audio interrupts disabled.

    float Read_pitch(void);
    int Read_loop_track(void);
    int Read_update_time(void);
    bool Read_precedence(void);
    int Read_patch_wait(void);
    int Read_local_patch(void);
    int Read_midi_channel(void);
    int Read_instrument(void);
    int Read_sound_id(void);
    int Read_note(void);
    unsigned long Read_time_stamp(void);
    int Read_use_Wavetable(void);

    void Write_midi_channel(int value);
    void Write_precedence(bool value);
    void Write_time_stamp(unsigned long value);

    // PSRAM cache management
    uint16_t Get_cache_reference_mask(void); // Protect current audio, queued starts and edits that keep using the current source.
    
    // AudioTables: chiamare nell'IRQ audio oppure con IRQ audio disabilitati.
    uint8_t Get_tables_reference_mask(void); // Include current playback, pending starts and pending edits.
    void Refresh_audio_table_references(AudioTables &tables); // Move equivalent table references without restarting playback; call with audio interrupts disabled.
    bool Apply_preset_edit(int patch, int instrument, const Preset_struct &preset, const AudioTables::Pointers &tables); // Update matching current and pending notes; return whether the current note needs a crossmix.
};
