/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#pragma once

#include "DisplayPrimitives.h"
#include "SharedElements.h"
#include "SharedPerformance.h"

class DisplayPerformance
{
private:
    void Note(const int note_number);

    static constexpr float P_column_Instrument_frame = 3;
    static constexpr float P_chars_width_Instrument_frame = 49;

    static constexpr float P_row_Instrument_title = 5;
    static constexpr float P_column_SOUND_title = 0.5;
    static constexpr float P_column_LOCK_title = 7;
    static constexpr float P_column_P_title = 12.5;
    static constexpr float P_column_MIDI_title = 15;
    static constexpr float P_column_ROOT_K_title = 20.5;
    static constexpr float P_column_FROM_K_title = 28;
    static constexpr float P_column_TO_K_title = 36.5;
    static constexpr float P_column_PAN_title = 43;
    static constexpr float P_column_GAIN_title = 47.5;

    static constexpr float P_pixel_x_LED = 8; // posizione led pagina Performance
    static constexpr float P_column_Sound = 3;

    // pointer
    static constexpr int P_chars_instrument_element[8] = {1, 1, 2, 4, 4, 4, 2, 4};                       // Lock, Precedence,....., Gain
    static constexpr float P_column_instrument_element[8] = {8.5, 12.5, 16, 21.5, 28.5, 36.5, 43, 47.5}; // Lock, Precedence,....., Gain

    int P_Instrument_pixels_y(int position);

    // TUNING TONE
    static constexpr float TT_Instrument_INDENT_X0 = 0.5; // indentatura dell'header nella Performance (in caratteri) a sinistra
    static constexpr float TT_Instrument_SPACE_X = 1.5;   // spaziatura (in caratteri) tra due titoli dell'header nella Performance

    // Both patch dialogs use this centered layout.
    static constexpr int L_POPUP = 106;
    static constexpr int H_POPUP = 47;
    static constexpr int X_POPUP = Centered_element_left(L_POPUP);
    static constexpr int Y_POPUP = Centered_element_top(H_POPUP);
    static constexpr int Y_POPUP_TXT = 10;
    static constexpr int Y_POPUP_OPT = 30;

public:
    DisplayPerformance() {}

    void Led_PERFORMANCE_instrument(int instrument_id, bool on);
    void Led_tuning_tone(int patch_id);
    void P_show_pointer_frame(P_field_description_struct value, bool show);
    void P_show_PERFORMANCE_page(bool change_patch, bool change_vol);
    void P_Patch_header(bool change_patch, bool change_vol);
    void P_show_Performance_menu(void);
    void P_Confirm_patch_change_popup(void);
    void P_Confirm_patch_change_popup_frame(int value);
    void P_Confirm_patch_delete_popup(void);
    void P_Confirm_patch_delete_popup_frame(int value);
    void P_show_Instruments_header(void);
    void P_show_all_instruments(int patch_id);
    void P_show_Instrument_description(int patch_id, int instrument_id, bool editing);
    void P_show_Sound_number(int instrument_id, bool editing);
    void P_show_Lock_value(int patch_id, int instrument_id, bool editing);
    void P_show_Precedence_value(int patch_id, int instrument_id, bool editing);
    void P_show_Midi_value(int patch_id, int instrument_id, bool editing);
    void P_show_RootKey_value(int patch_id, int instrument_id, bool editing);
    void P_show_FromKey_value(int patch_id, int instrument_id, bool editing);
    void P_show_ToKey_value(int patch_id, int instrument_id, bool editing);
    void P_show_Pan_value(int patch_id, int instrument_id, bool editing);
    void P_show_Gain_value(int patch_id, int instrument_id, bool editing);
    void P_delete_instrument_by_position(int position);
    void P_show_delete_Instrument_frame(float line, bool show);
    void P_show_TuningTone_instrument(int patch_id);
    void P_show_gain_TuningTone(int patch_id);
};
