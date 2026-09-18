/**
 * @file element.h
 * @brief Common UI element state and focusability.
 */

#ifndef UNSIGNED_DISPLAY_UI_ELEMENT_H
#define UNSIGNED_DISPLAY_UI_ELEMENT_H

#include "display/ui/types.h"

typedef struct UUIElement {
    UUIElementType type;
    UUIRect bounds;
    UUIStyleId style;
    bool visible;
    bool enabled;
    bool focusable;
    bool focused;
} UUIElement;

/**
 * @brief Initializes the UI element to a valid empty runtime state.
 *
 * @param element UI element being configured, focused or rendered.
 * @param type Concrete UI element type used by renderer/theme dispatch.
 * @param bounds Bounds used for layout, culling, collision, or effect calculations.
 * @param style UI style identifier resolved by the active theme.
 * @param focusable Whether the element participates in menu focus navigation.
 */
void unsigned_ui_element_init(UUIElement *element, UUIElementType type, UUIRect bounds, UUIStyleId style, bool focusable);

/**
 * @brief Moves the element bounds without changing its size.
 *
 * @param element UI element being configured, focused or rendered.
 * @param x New screen-space left coordinate.
 * @param y New screen-space top coordinate.
 */
void unsigned_ui_element_set_position(UUIElement *element, s16 x, s16 y);

/**
 * @brief Replaces the width/height of the element bounds without moving its origin.
 *
 * @param element UI element being configured, focused or rendered.
 * @param width Width of the configured rectangle or element.
 * @param height Height of the configured rectangle or element.
 */
void unsigned_ui_element_set_size(UUIElement *element, u16 width, u16 height);

/**
 * @brief Changes visibility; hidden elements are also excluded from focus.
 *
 * @param element UI element being configured, focused or rendered.
 * @param visible Requested visibility state.
 */
void unsigned_ui_element_set_visible(UUIElement *element, bool visible);

/**
 * @brief Changes interaction state; disabled elements cannot receive menu focus.
 *
 * @param element UI element being configured, focused or rendered.
 * @param enabled Requested interaction-enabled state.
 */
void unsigned_ui_element_set_enabled(UUIElement *element, bool enabled);

/**
 * @brief Sets style on the UI element.
 *
 * @param element UI element being configured, focused or rendered.
 * @param style UI style identifier resolved by the active theme.
 */
void unsigned_ui_element_set_style(UUIElement *element, UUIStyleId style);

/**
 * @brief Returns whether the UI element can perform focus.
 *
 * @param element UI element being configured, focused or rendered.
 * @return true only when the element is visible, enabled and declared focusable.
 */
bool unsigned_ui_element_can_focus(const UUIElement *element);

#endif
