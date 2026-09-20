/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <ILI9341_t3n.h>
#include "DisplayPrimitives.h"
#include "SharedSampler.h"
#include "SharedElements.h"
#include "SharedVFS.h"
#include "GlobalInfoMaster.h"
#include "GraphicElements.h"

// Handles all display rendering for the Direct Sampler page on the ILI9341 TFT.
// Responsibilities:
//   - Full page layout: title, memory info, recording description, IO diagram.
//   - Stereo VU-meter bars with green-to-red gradient.
//   - Menu row: dynamic item list and highlight frame.
//   - Recording status: REC blink, elapsed seconds, volume and gain values.
//   - Popup messages: delete advice, conversion options, export options.
class DisplaySampler
{
private:
    // page canvas top-left corner (pixel coordinates) — change these to remap the whole page
    static constexpr int DS_CANVAS_X = 0;
    static constexpr int DS_CANVAS_Y = 0;

    // row anchors (character-grid rows)
    static constexpr float DS_ROW_MENU = 1;
    static constexpr float DS_ROW_MEMORY = 4;    // AUDIO MEMORY / FREE FOR ... section
    static constexpr float DS_ROW_RECORDING = 8; // RECORDING description section

    // label positions [col, row] — UPPERCASE = static label text
    static constexpr float DS_column_row_SAMPLER[2] = {0, 0};
    static constexpr float DS_column_row_VOLUME_LABEL[2] = {41, 0};
    static constexpr float DS_column_row_AUDIO_MEMORY[2] = {0, DS_ROW_MEMORY};
    static constexpr float DS_column_row_FREE_RECORDINGS[2] = {0, DS_ROW_MEMORY + 1};
    static constexpr float DS_column_row_FREE_RAW_FILES[2] = {0, DS_ROW_MEMORY + 2};
    static constexpr float DS_column_row_RECORDING[2] = {0, DS_ROW_RECORDING};
    static constexpr float DS_column_row_RECORDING_LED[2] = {1.5, DS_ROW_RECORDING}; // indented when LED is drawn on the left
    static constexpr float DS_column_row_FILE_MONO[2] = {0, DS_ROW_RECORDING + 1};
    static constexpr float DS_column_row_FILE_RIGHT[2] = {0, DS_ROW_RECORDING + 2}; // stereo only
    static constexpr float DS_column_row_LENGTH[2] = {0, DS_ROW_RECORDING + 2};     // mono; stereo uses DS_ROW_LENGTH_STEREO
    static constexpr float DS_column_row_PLEASE_WAIT[2] = {3.5, DS_ROW_RECORDING};
    static constexpr float DS_column_row_POPUP_LINE_0[2] = {0, DS_ROW_RECORDING};
    static constexpr float DS_column_row_POPUP_LINE_1[2] = {0, DS_ROW_RECORDING + 1};
    static constexpr float DS_column_row_POPUP_LINE_2[2] = {0, DS_ROW_RECORDING + 2};

    // value positions [col, row] — lowercase = updatable value
    static constexpr float DS_column_row_volume[2] = {48, 0};
    static constexpr float DS_column_row_recording[2] = {12, DS_ROW_RECORDING};
    static constexpr float DS_column_row_available_memory[2] = {22, DS_ROW_MEMORY + 1};
    static constexpr float DS_column_row_raw_available_memory[2] = {21, DS_ROW_MEMORY + 2};
    static constexpr float DS_column_row_length[2] = {7, DS_ROW_RECORDING + 2}; // row overridden for stereo

    // dynamic row offsets for length and volume (depend on mono/stereo/no-recording state)
    static constexpr float DS_ROW_LENGTH_MONO = DS_ROW_RECORDING + 2;
    static constexpr float DS_ROW_LENGTH_STEREO = DS_ROW_RECORDING + 3;
    static constexpr float DS_ROW_VOLUME_NONE = DS_ROW_RECORDING + 2; // no recording selected
    static constexpr float DS_ROW_VOLUME_MONO = DS_ROW_RECORDING + 3;
    static constexpr float DS_ROW_VOLUME_STEREO = DS_ROW_RECORDING + 4;

    // character widths for Cancel_text / Cancel_text_reset_cursor calls
    static constexpr int DS_chars_volume = 4;
    static constexpr int DS_chars_recording = 4; // Allow the NONE label to fit with the standard frame padding.
    static constexpr int DS_chars_available_memory = 10;
    static constexpr int DS_chars_raw_available_memory = 6;
    static constexpr int DS_chars_length = 7;

    // Recording controls use the existing character-grid row numbering.
    static constexpr float DS_ROW_GAIN = 9;
    static constexpr float DS_ROW_LEVEL_L = 10;
    static constexpr float DS_ROW_LEVEL_R = 11;
    static constexpr float DS_COLUMN_GAIN = 14;
    static constexpr int DS_VUMETER_BAR_X = 52;
    static constexpr int DS_VUMETER_BAR_HEIGHT = 8;
    static constexpr int DS_VUMETER_STEP_WIDTH = 2;
    bool DS_recording_controls_visible = false;

    int DS_frame_menu_position_0 = 0;      // last highlighted menu position, used to erase the previous frame
    int DS_VU_meter_value_old[2] = {0, 0}; // previous bar height for each channel, used for incremental redraw

    // Returns the RGB565 colour for a VU-meter bar brick at the given normalised level (0.0 – 1.0).
    uint16_t DS_calc_bar_color(float value);

public:
    DisplaySampler() {}

    // Clears the upper area and draws the title and volume; callers restore the menu and pointer.
    void DS_page_upper(void);

    // Draws memory, recording and IO information without touching the upper area.
    void DS_page_lower(int recording);

    // Shows recording controls only during input monitoring and recording.
    void DS_set_recording_controls(bool visible);
    bool DS_has_recording_controls(void) const { return DS_recording_controls_visible; }
    void DS_sampler_IO(void);

    // Incrementally redraws one VU-meter bar channel. channel: 0 = L, 1 = R. value: 0 – BAR_ELEMENTS.
    void DS_bar(int channel, int value);

    // Draws the RECORDING label in red (blinking) or dim red (off).
    void DS_sampler_txt(bool color);

    // Redraws the free-recording-memory value (seconds available for new recordings).
    void DS_available_memory(void);

    // Redraws the free-raw-file-memory value (seconds available for RAW files).
    void DS_raw_available_memory(void);

    // Clears the recording description area on screen.
    void DS_hide_recording(void);

    // Shows or hides the 'PLEASE WAIT' delete advice.
    void DS_advice_delete(bool value);

    // Shows or hides the 'UNABLE TO CREATE RAW FILE' advice with the reason (DS_export: 0 = no space, -1 = no name).
    void DS_advice_no_conversion(int DS_export, bool value);

    // Draws the conversion-to-RAW options panel showing source and destination file names.
    void DS_conversion_options(int file_L_RAW, int file_R_RAW, int DS_export);

    // Draws the SD export options panel showing source file names and available targets.
    void DS_export_options(int file_L_RAW, int file_R_RAW, int DS_export);

    // Draws the recording description and LED space; update_header allows refreshing the header volume.
    void DS_Recording_description(int recording, bool led, bool update_header = true);

    // Draws the static LENGTH label and value for the current recording.
    void DS_recording_seconds(void);

    // Redraws only the elapsed-seconds value during an active recording.
    void DS_update_recording_seconds(float value);

    // Draws the VOLUME label and value at the row that matches the current recording layout.
    void DS_volume(void);

    // Redraws only the volume value in the header row. adj=true uses yellow (being edited), false uses white.
    void DS_update_volume(bool adj = true);

    // Redraws the LINE-IN gain value.
    void DS_show_gain(void);

    // Redraws the dynamic menu row, recalculating positions from the active Menu_DS[] entries.
    void DS_menu(void);

    // Erases the previous menu frame and draws a new one at the given position, updating choice_DS_menu.
    void DS_frame_menu(int position);

    // Draws the 'STOP SAMPLING?' confirmation popup.
    void DS_confirm_EXIT_from_DS(void);

    // Pointer
    void DS_show_pointer_frame(const DS_pointer_struct pointer, const bool show);
};
