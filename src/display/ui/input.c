/**
 * @file input.c
 * @brief Implements UI navigation/action input snapshot.
 */

#include "display/ui/input.h"

#include <stddef.h>

void unsigned_ui_input_clear(UUIInput *input) {
    if (input == NULL) {
        return;
    }

    *input = (UUIInput){ 0 };
}

void unsigned_ui_input_from_controller(UUIInput *ui_input, const UInputState *input) {
    if (ui_input == NULL) {
        return;
    }

    unsigned_ui_input_clear(ui_input);
    if (input == NULL) {
        return;
    }

    ui_input->navigate_previous_pressed = (input->pressed & U_INPUT_BUTTON_UP) != 0u;
    ui_input->navigate_next_pressed = (input->pressed & U_INPUT_BUTTON_DOWN) != 0u;
    ui_input->value_previous_pressed = (input->pressed & U_INPUT_BUTTON_LEFT) != 0u;
    ui_input->value_next_pressed = (input->pressed & U_INPUT_BUTTON_RIGHT) != 0u;
    ui_input->confirm_pressed = (input->pressed & (U_INPUT_BUTTON_A | U_INPUT_BUTTON_START)) != 0u;
    ui_input->cancel_pressed = (input->pressed & U_INPUT_BUTTON_B) != 0u;
}
