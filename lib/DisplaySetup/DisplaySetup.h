/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#pragma once

#include "DisplayPrimitives.h"
#include "SharedSetup.h"

// Setup page.
class DisplaySetup
{
private:
    // SETUP selection coordinates in character columns/rows and frame widths.
    static constexpr int SET_column_label = 0;
    static constexpr float SET_column_row_Convert[2] = {0, 2};
    static constexpr float SET_column_row_Import[2] = {0, 3};
    static constexpr float SET_column_row_KeyStep[2] = {14, 5};
    static constexpr float SET_column_row_FirstOctave[2] = {13, 6};
    static constexpr float SET_column_row_ControlChange[2] = {0, 7};
    static constexpr float SET_column_row_Backup[2] = {0, 9};
    static constexpr float SET_column_row_Restore[2] = {0, 10};
    static constexpr float SET_column_row_FactoryReset[2] = {0, 11};

    static constexpr int SET_chars_Convert = 19;
    static constexpr int SET_chars_Import = 26;
    static constexpr int SET_chars_KeyStep = 12;
    static constexpr int SET_chars_FirstOctave = 2;
    static constexpr int SET_chars_ControlChange = 25;
    static constexpr int SET_chars_Backup = 20;
    static constexpr int SET_chars_Restore = 33;
    static constexpr int SET_chars_FactoryReset = 13;

    static constexpr int Convert_popup_lines = 4;
    static constexpr int Import_info_popup_lines = 4;

public:
    DisplaySetup() {}

    FLASHMEM void SETUP_show_SETUP_page(void);
    FLASHMEM void SETUP_show_Key_step_value(void);
    FLASHMEM void SETUP_show_First_octave_value(void);
    FLASHMEM void SETUP_show_frame(int8_t value);
    FLASHMEM void SETUP_show_convert_popup(void);
    FLASHMEM static void SETUP_show_read_progress(const char *filename); // Show reading, current conversion or indexing in a fixed-width three-line popup.
    FLASHMEM void SETUP_show_import_info_popup(void);
    FLASHMEM void SETUP_show_import_info_frame(int value);
    FLASHMEM void SETUP_show_convert_frame(int value);
};
