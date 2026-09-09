/**
 * @file input.h
 * @brief Controller state sampling and edge detection.
 */

#ifndef UNSIGNED_INPUT_H
#define UNSIGNED_INPUT_H

#include "core/types.h"

#define UNSIGNED_INPUT_BUTTONS 10
#define UNSIGNED_INPUT_DEFAULT_HOLD_FRAMES 20u

enum { U_INPUT_PLAYER_CAPACITY = 4 };

typedef enum UInputButton {
    U_INPUT_BUTTON_UP = (1u << 0),
    U_INPUT_BUTTON_DOWN = (1u << 1),
    U_INPUT_BUTTON_LEFT = (1u << 2),
    U_INPUT_BUTTON_RIGHT = (1u << 3),
    U_INPUT_BUTTON_A = (1u << 4),
    U_INPUT_BUTTON_B = (1u << 5),
    U_INPUT_BUTTON_C = (1u << 6),
    U_INPUT_BUTTON_D = (1u << 7),
    U_INPUT_BUTTON_START = (1u << 8),
    U_INPUT_BUTTON_SELECT = (1u << 9),
} UInputButton;

typedef enum UInputTrigger {
    /** Repeated hold pulse derived from the per-button hold counters. */
    U_INPUT_TRIGGER_HOLD,
    /** True every frame while all buttons in the binding mask are physically down. */
    U_INPUT_TRIGGER_DOWN,
    /** One-frame rising edge. */
    U_INPUT_TRIGGER_PRESSED,
    /** One-frame falling edge. */
    U_INPUT_TRIGGER_RELEASED,
} UInputTrigger;

typedef u16 UInputMask;

typedef struct UInputState {
    UInputMask hold;
    UInputMask down;
    UInputMask pressed;
    UInputMask released;
} UInputState;

typedef struct UInputController {
    u8 buttons[UNSIGNED_INPUT_BUTTONS];
    UInputMask buttons_down;
    UInputState state;
} UInputController;

typedef struct UInputManager {
    UInputController players[U_INPUT_PLAYER_CAPACITY];
    u8 player_count;
    u8 hold_frames;
} UInputManager;

/**
 * @brief Initializes controller runtime state for a fixed number of local players and the default hold threshold.
 *
 * @param input Input manager to initialize.
 * @param player_count Number of active player controllers; must be 1..U_INPUT_PLAYER_CAPACITY.
 * @return true when input/player_count are valid; false otherwise.
 */
bool unsigned_input_manager_init(UInputManager *input, u8 player_count);

/**
 * @brief Advances the input controller by one scheduled engine frame.
 *
 * @param controller Controller state used to drive player or UI input.
 * @param buttons_down Raw controller button mask sampled for the current frame.
 * @param hold_frames Per-button frame counters used to derive repeat/hold input state.
 */
void unsigned_input_controller_tick(UInputController *controller, UInputMask buttons_down, u8 hold_frames);

#endif
