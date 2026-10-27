/**
 * @file label.h
 * @brief Text label widget.
 */

#ifndef UNSIGNED_DISPLAY_UI_WIDGET_LABEL_H
#define UNSIGNED_DISPLAY_UI_WIDGET_LABEL_H

#include "display/ui/element.h"

typedef struct UUILabel {
    UUIElement element;
    const char *text;
} UUILabel;

/**
 * @brief Initializes the UI label to a valid empty runtime state.
 *
 * @param label Label text/widget to configure.
 * @param bounds Bounds used for layout, culling, collision, or effect calculations.
 * @param style UI style identifier resolved by the active theme.
 * @param text Text content consumed by formatting or rendering.
 */
void unsigned_ui_label_init(UUILabel *label, UUIRect bounds, UUIStyleId style, const char *text);

/**
 * @brief Sets text on the UI label.
 *
 * @param label Label text/widget to configure.
 * @param text Text content consumed by formatting or rendering.
 */
void unsigned_ui_label_set_text(UUILabel *label, const char *text);

#endif
