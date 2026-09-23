/**
 * @file attribute.h
 * @brief Character gameplay attribute storage.
 */

#ifndef UNSIGNED_GAMEPLAY_ATTRIBUTE_H
#define UNSIGNED_GAMEPLAY_ATTRIBUTE_H

#include "core/types.h"

typedef struct UGameplayAttribute {
    s16 base_value;
    s16 current_value;
    struct UGameplayAttribute *min_value;
    struct UGameplayAttribute *max_value;
    /** Optional notification receiving this attribute after current_value changes. */
    UCallbackFunc on_change;
} UGameplayAttribute;

/**
 * Set current_value and notify on_change once, after writing a different value.
 * Values are clamped to optional min/max attributes. Unchanged values are ignored.
 * Use this setter for runtime changes; direct initialization does not notify.
 * @pre `attribute` is valid.
 */
void unsigned_gameplay_attribute_set_current_value(UGameplayAttribute *attribute, s16 value);

/** Add a signed delta to current_value with saturation to s16 and the same min/max clamping rules.
 * @pre `attribute` is valid.
 */
void unsigned_gameplay_attribute_add_current_value(UGameplayAttribute *attribute, s16 delta);

#endif
