/**
 * @file layout.h
 * @brief Simple deterministic UI element layout.
 */

#ifndef UNSIGNED_DISPLAY_UI_LAYOUT_H
#define UNSIGNED_DISPLAY_UI_LAYOUT_H

#include "display/ui/element.h"
#include "display/ui/types.h"

/**
 * @brief Computes the UI linear layout without drawing it.
 *
 * @param elements Ordered UI element array to lay out.
 * @param count Number of participating entries.
 * @param x Screen-space X position assigned to the first element.
 * @param y Screen-space Y position assigned to the first element.
 * @param direction Horizontal or vertical layout direction.
 * @param spacing Pixel spacing inserted between consecutive elements.
 */
void unsigned_ui_layout_linear(UUIElement *const *elements, u8 count, s16 x, s16 y, UUILayoutDirection direction, u16 spacing);

#endif
