/**
 * @file panel.h
 * @brief Decorative panel widget.
 */

#ifndef UNSIGNED_DISPLAY_UI_WIDGET_PANEL_H
#define UNSIGNED_DISPLAY_UI_WIDGET_PANEL_H

#include "display/ui/element.h"

typedef struct UUIPanel {
    UUIElement element;
} UUIPanel;

/**
 * @brief Initializes the UI panel to a valid empty runtime state.
 *
 * @param panel Panel widget to initialize.
 * @param bounds Bounds used for layout, culling, collision, or effect calculations.
 * @param style UI style identifier resolved by the active theme.
 */
void unsigned_ui_panel_init(UUIPanel *panel, UUIRect bounds, UUIStyleId style);

#endif
