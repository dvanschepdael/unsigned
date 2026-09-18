/**
 * @file layout.c
 * @brief Implements simple deterministic UI element layout.
 */

#include "display/ui/layout.h"

void unsigned_ui_layout_linear(UUIElement *const *elements, u8 count, s16 x, s16 y, UUILayoutDirection direction, u16 spacing) {
    s16 current_x = x;
    s16 current_y = y;

    if (elements == NULL) {
        return;
    }

    for (u8 i = 0; i < count; ++i) {
        UUIElement *element = elements[i];

        if (element == NULL) {
            continue;
        }

        element->bounds.x = current_x;
        element->bounds.y = current_y;

        if (direction == U_UI_LAYOUT_HORIZONTAL) {
            current_x += (s16)(element->bounds.width + spacing);
        } else {
            current_y += (s16)(element->bounds.height + spacing);
        }
    }
}
