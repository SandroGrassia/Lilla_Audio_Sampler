/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#include "DisplayDiagnostics.h"
#include "SharedElements.h"

FLASHMEM
void DisplayDiagnostics::Midi_monitor_page(void)
{
    tft.fillScreen(ILI9341_BLACK);

    Backgorund_red(0, 0, 12); // Display.Board(float col, float row, int chars)
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(0));
    tft.setTextColor(ILI9341_WHITE);
    tft.print("MIDI MONITOR");

    tft.setTextColor(TEXT_COLOR);
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(3));
    tft.print("MIDI CHANNEL");
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(4));
    tft.print("MESSAGE");
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(5));
    tft.print("NOTE-NUMBER");
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(6));
    tft.print("VELOCITY");
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(7));
    tft.print("VALUE");
    tft.setCursor(display_coordinate_x(0), display_coordinate_y(8));
    tft.print("NUMBER");
}

FLASHMEM
void DisplayDiagnostics::Midi_monitor_frame(void)
{
    Frame_by_col_row(0, 1, 6, true);
}

FLASHMEM
void DisplayDiagnostics::Midi_monitor_data(uint8_t incoming_midi_channel, uint8_t incoming_midi_message, int8_t incoming_note_number, int8_t incoming_velocity, int32_t incoming_midi_value, int8_t incoming_number)
{
    const char message_name[12][20] = {{"NoteOn"}, {"NoteOff"}, {"PitchBend"}, {"AfterTouchPoly"}, {"ControlChange"}, {"ProgramChange"}, {"AfterTouchChange"}, {"SystemExclusive"}, {"Unknown"}};

    tft.setTextColor(ILI9341_YELLOW);
    Cancel_text_reset_cursor(display_coordinate_x(13), display_coordinate_y(3), 2);
    tft.print(incoming_midi_channel + 1);

    Cancel_text_reset_cursor(display_coordinate_x(8), display_coordinate_y(4), 16);
    tft.print(message_name[incoming_midi_message]);

    Cancel_text(display_coordinate_x(12), display_coordinate_y(5), 9);
    if (incoming_note_number >= 0)
    {
        tft.setCursor(display_coordinate_x(12), display_coordinate_y(5));
        tft.print(note_name[incoming_note_number % 12]);
        tft.print((int)(incoming_note_number / 12) + first_octave);
    }

    Cancel_text(display_coordinate_x(9), display_coordinate_y(6), 9);
    if (incoming_velocity >= 0)
    {
        tft.setCursor(display_coordinate_x(9), display_coordinate_y(6));
        tft.print(incoming_velocity);
    }

    Cancel_text(display_coordinate_x(6), display_coordinate_y(7), 9);
    if (incoming_midi_value >= 0)
    {
        tft.setCursor(display_coordinate_x(6), display_coordinate_y(7));
        tft.print(incoming_midi_value);
    }

    Cancel_text(display_coordinate_x(7), display_coordinate_y(8), 9);
    if (incoming_number >= 0)
    {
        tft.setCursor(display_coordinate_x(7), display_coordinate_y(8));
        tft.print(incoming_number);
    }
}

FLASHMEM
void DisplayDiagnostics::Encoder_pushbutton_test_board(void)
{
    L_POPUP = display_coordinate_x(32);
    H_POPUP = display_coordinate_y(6) - 4; // 64 pixel
    Y_POPUP = Centered_element_top(H_POPUP);
    X_POPUP = Centered_element_left(L_POPUP);

    tft.fillRoundRect(X_POPUP, Y_POPUP, L_POPUP, H_POPUP, 4, ILI9341_WHITE);

    tft.setTextColor(ILI9341_RED);
    tft.setCursor(X_POPUP + display_coordinate_x(1), Y_POPUP + 4);
    tft.print(F("PHYSICAL CONTROLS TEST"));
}

void DisplayDiagnostics::Encoder_pushbutton_test_result(const int device, const int element, const int value)
{
    Encoder_pushbutton_test_board();

    struct Result
    {
        int device;
        int element;
        int value;
    };
    static Result memo[5] = {};

    memo[4] = memo[3];
    memo[3] = memo[2];
    memo[2] = memo[1];
    memo[1] = memo[0];
    memo[0] = {device, element, value};

    tft.setTextColor(ILI9341_BLACK);

    for (auto i = 0; i < 5; ++i)
    {
        tft.setCursor(X_POPUP + display_coordinate_x(1), Y_POPUP + display_coordinate_y(5 - i));
        if (memo[i].device == 1)
        {
            tft.print("encoder ");
            tft.print(memo[i].element);
            tft.print(" value ");
            tft.print(memo[i].value);
        }
        if (memo[i].device == 2)
        {
            tft.print("pushbutton ");
            tft.print(memo[i].element);
            tft.print(" pressed");
        }
        if (memo[i].device == 3)
        {
            tft.print("switch ");
            tft.print(memo[i].element);
            tft.print(" value ");
            tft.print(memo[i].value);
        }
    }
}
