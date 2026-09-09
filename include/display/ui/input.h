/**
 * @file input.h
 * @brief UI navigation/action input snapshot.
 */

#ifndef UNSIGNED_DISPLAY_UI_INPUT_H
#define UNSIGNED_DISPLAY_UI_INPUT_H

#include <stdbool.h>

typedef struct UUIInput {
    bool navigate_previous_pressed;
    bool navigate_next_pressed;
    bool value_previous_pressed;
    bool value_next_pressed;
    bool confirm_pressed;
    bool cancel_pressed;
} UUIInput;

/**
 * @brief Clears the UI input state without freeing caller-owned storage.
 *
 * @param input Input snapshot sampled or consumed by this API.
 */
void unsigned_ui_input_clear(UUIInput *input);

#endif
