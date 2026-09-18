/**
 * @file button.h
 * @brief Focusable push-button widget.
 */

#ifndef UNSIGNED_DISPLAY_UI_WIDGET_BUTTON_H
#define UNSIGNED_DISPLAY_UI_WIDGET_BUTTON_H

#include "display/ui/element.h"

typedef struct UUIButton UUIButton;
typedef void (*UUIButtonPressedCallback)(UUIButton *button, void *context);

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
 * @param bounds Bounds used for layout, culling, collision, or effect calculations.
 * @param style UI style identifier resolved by the active theme.
 * @param text Text content consumed by formatting or rendering.
 * @param on_pressed Callback invoked when the button is activated.
 * @param context Opaque caller context passed back to callbacks.
 */
void unsigned_ui_button_init(UUIButton *button, UUIRect bounds, UUIStyleId style, const char *text, UUIButtonPressedCallback on_pressed, void *context);

/**
 * @brief Sets text on the UI button.
 *
 * @param button Button widget to initialize, rename or activate.
 * @param text Text content consumed by formatting or rendering.
 */
void unsigned_ui_button_set_text(UUIButton *button, const char *text);

/**
 * @brief Processes the UI button.
 *
 * @param button Button whose focus, input or callback state is updated.
 */
void unsigned_ui_button_press(UUIButton *button);

#endif
