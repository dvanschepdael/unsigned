/**
 * @file input.h
 * @brief UI navigation/action input snapshot.
 */

#ifndef UNSIGNED_DISPLAY_UI_INPUT_H
#define UNSIGNED_DISPLAY_UI_INPUT_H

#include <stdbool.h>

#include "input/input.h"

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

/** Translate the standard Neo Geo/controller navigation mapping from a sampled input state. */
void unsigned_ui_input_from_controller(UUIInput *ui_input, const UInputState *input);

#endif
