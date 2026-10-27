/**
 * @file element.c
 * @brief Implements common UI element state and focusability.
 */

#include "display/ui/element.h"

void unsigned_ui_element_init(UUIElement *element, UUIElementType type, UUIRect bounds, UUIStyleId style, bool focusable) {

    *element = (UUIElement){
        .type = type,
        .bounds = bounds,
        .style = style,
        .visible = true,
        .enabled = true,
        .focusable = focusable,
    };
}

void unsigned_ui_element_set_visible(UUIElement *element, bool visible) {

    element->visible = visible;
    if (!visible) {
        element->focused = false;
    }
}

void unsigned_ui_element_set_enabled(UUIElement *element, bool enabled) {

    element->enabled = enabled;
    if (!enabled) {
        element->focused = false;
    }
}

bool unsigned_ui_element_can_focus(const UUIElement *element) {
    return element->visible && element->enabled && element->focusable;
}
