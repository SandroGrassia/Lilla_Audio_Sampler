/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "DisplayPrimitives.h"
#include "SharedDS.h"
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

    // VU-meter bar geometry (pixel coordinates)
    static constexpr int DS_VUMETER_BAR_X        = 210;
    static constexpr int DS_VUMETER_BAR_Y        = 175;
    static constexpr int DS_VUMETER_BAR_DISTANCE = 6;
    static constexpr int DS_VUMETER_BAR_DX       = 12;  // horizontal distance between L and R bar centre lines
    static constexpr int DS_START_Y = DS_VUMETER_BAR_Y + 13;
    static constexpr int DS_START_X = DS_VUMETER_BAR_X - 7;

    int DS_frame_menu_position_0 = 0;    // last highlighted menu position, used to erase the previous frame
    int DS_VU_meter_value_old[2] = {0, 0}; // previous bar height for each channel, used for incremental redraw

    // Returns the RGB565 colour for a VU-meter bar brick at the given normalised level (0.0 – 1.0).
    uint16_t DS_calc_bar_color(float value);

public:
    DisplaySampler() {}

    // Draws the full Direct Sampler page (title, memory info, IO diagram, recording description).
    void DS_page(int recording);

    // Draws the LINE-IN / SAMPLER / LINE-OUT IO diagram and initialises the VU-meter.
    void DS_sampler_IO(void);

    // Incrementally redraws one VU-meter bar channel. channel: 0 = L, 1 = R. value: 0 – BAR_ELEMENTS.
    void DS_bar(int channel, int value);

    // Shows or hides the LINE-OUT arrow and label.
    void DS_line_out(bool visible);

    // Shows or hides the SAMPLER box and its arrow.
    void DS_sampler_frame(bool visible);

    // Draws the RECORD label in red (blinking) or dim red (off).
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

    // Draws the full recording description block (number, file names, length). Pass led=true to also refresh volume.
    void DS_Recording_description(int recording, bool led);

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
};
