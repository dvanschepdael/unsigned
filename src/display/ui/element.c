/**
 * @file element.c
 * @brief Implements common UI element state and focusability.
 */

#include "display/ui/element.h"

void unsigned_ui_element_init(UUIElement *element, UUIElementType type, UUIRect bounds, UUIStyleId style, bool focusable) {
    if (element == NULL) {
        return;
    }

    *element = (UUIElement){
        .type = type,
        .bounds = bounds,
        .style = style,
        .visible = true,
        .enabled = true,
        .focusable = focusable,
    };
}

void unsigned_ui_element_set_position(UUIElement *element, s16 x, s16 y) {
    if (element == NULL) {
        return;
    }

    element->bounds.x = x;
    element->bounds.y = y;
}

void unsigned_ui_element_set_size(UUIElement *element, u16 width, u16 height) {
    if (element == NULL) {
        return;
    }

    element->bounds.width = width;
    element->bounds.height = height;
}

void unsigned_ui_element_set_visible(UUIElement *element, bool visible) {
    if (element == NULL) {
        return;
    }

    element->visible = visible;
    if (!visible) {
        element->focused = false;
    }
}

void unsigned_ui_element_set_enabled(UUIElement *element, bool enabled) {
    if (element == NULL) {
        return;
    }

    element->enabled = enabled;
    if (!enabled) {
        element->focused = false;
    }
}

void unsigned_ui_element_set_style(UUIElement *element, UUIStyleId style) {
    if (element == NULL) {
        return;
    }

    element->style = style;
}

bool unsigned_ui_element_can_focus(const UUIElement *element) {
    return element != NULL && element->visible && element->enabled && element->focusable;
}
