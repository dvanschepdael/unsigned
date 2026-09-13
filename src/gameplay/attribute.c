/**
 * @file attribute.c
 * @brief Gameplay attribute updates and change notification.
 */

#include "gameplay/attribute.h"

void unsigned_gameplay_attribute_set_current_value(UGameplayAttribute *attribute, s16 value) {
    if (attribute == NULL || attribute->current_value == value) {
        return;
    }

    attribute->current_value = value;
    if (attribute->on_update != NULL) {
        attribute->on_update(attribute);
    }
}
