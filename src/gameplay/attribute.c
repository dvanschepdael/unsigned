/**
 * @file attribute.c
 * @brief Gameplay attribute updates and change notification.
 */

#include "gameplay/attribute.h"

#include "core/math/math.h"

void unsigned_gameplay_attribute_set_current_value(UGameplayAttribute *attribute, s16 value) {
    if (attribute->min_value != NULL && value < attribute->min_value->current_value) {
        value = attribute->min_value->current_value;
    }

    if (attribute->max_value != NULL && value > attribute->max_value->current_value) {
        value = attribute->max_value->current_value;
    }

    if (attribute->current_value == value) {
        return;
    }

    attribute->current_value = value;
    if (attribute->on_change != NULL) {
        attribute->on_change(attribute);
    }
}

void unsigned_gameplay_attribute_add_current_value(UGameplayAttribute *attribute, s16 delta) {
    if (delta == 0) {
        return;
    }

    const s16 next = unsigned_math_saturate_s16((s32)attribute->current_value + delta);
    unsigned_gameplay_attribute_set_current_value(attribute, next);
}
