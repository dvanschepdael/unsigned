/**
 * @file panel.c
 * @brief Implements decorative panel widget.
 */

#include "display/ui/widget/panel.h"

void unsigned_ui_panel_init(UUIPanel *panel, UUIRect bounds, UUIStyleId style) {
    if (panel == NULL) {
        return;
    }

    unsigned_ui_element_init(&panel->element, U_UI_ELEMENT_PANEL, bounds, style, false);
}
