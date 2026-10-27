/**
 * @file button.h
 * @brief Focusable push-button widget.
 */

#ifndef UNSIGNED_DISPLAY_UI_WIDGET_BUTTON_H
#define UNSIGNED_DISPLAY_UI_WIDGET_BUTTON_H

#include "display/ui/element.h"

typedef struct UUIButton UUIButton;
typedef void (*UUIButtonPressedCallback)(UUIButton *button, void *context);

typedef struct UUIButtonConfig {
    UUIRect bounds;
    UUIStyleId style;
    const char *text;
    UUIButtonPressedCallback on_pressed;
    void *context;
} UUIButtonConfig;

struct UUIButton {
    UUIElement element;
    const char *text;
    UUIButtonPressedCallback on_pressed;
    void *context;
};

/**
 * @brief Initializes the UI button to a valid empty runtime state.
 *
 * @param button Button widget to initialize, rename or activate.
 * @param config Caller-owned initialization values consumed during the call.
 */
void unsigned_ui_button_init(UUIButton *button, const UUIButtonConfig *config);

/**
 * @brief Processes the UI button.
 *
 * @param button Button whose focus, input or callback state is updated.
 */
void unsigned_ui_button_press(UUIButton *button);

#endif
