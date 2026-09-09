/**
 * @file input.c
 * @brief Implements controller state sampling and edge detection.
 */

#include "input/input.h"

bool unsigned_input_manager_init(UInputManager *input, u8 player_count) {
    if (input == NULL || player_count == 0u || player_count > U_INPUT_PLAYER_CAPACITY) {
        return false;
    }

    *input = (UInputManager){
        .player_count = player_count,
        .hold_frames = UNSIGNED_INPUT_DEFAULT_HOLD_FRAMES,
    };
    return true;
}

void unsigned_input_controller_tick(UInputController *controller, UInputMask buttons_down, u8 hold_frames) {
    if (controller == NULL) {
        return;
    }

    UInputMask previous_buttons = controller->buttons_down;
    UInputMask pressed = buttons_down & (UInputMask)~previous_buttons;
    UInputMask released = previous_buttons & (UInputMask)~buttons_down;

    controller->state.down = buttons_down;
    controller->state.pressed = pressed;
    controller->state.released = released;
    controller->state.hold = 0u;

    UInputMask button_mask = 1u;
    for (u8 button = 0u; button < UNSIGNED_INPUT_BUTTONS; ++button, button_mask = (UInputMask)(button_mask << 1u)) {
        u8 frames = controller->buttons[button];

        if ((buttons_down & button_mask) != 0u) {
            if (frames < hold_frames) {
                ++frames;
                controller->buttons[button] = frames;
            }
            if (frames >= hold_frames) {
                controller->state.hold |= button_mask;
            }
        } else {
            controller->buttons[button] = 0u;
        }
    }

    controller->buttons_down = buttons_down;
}
