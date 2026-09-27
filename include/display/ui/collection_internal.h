/**
 * @file collection_internal.h
 * @brief Shared compact-list operations for UI element owners.
 */

#ifndef UNSIGNED_DISPLAY_UI_COLLECTION_INTERNAL_H
#define UNSIGNED_DISPLAY_UI_COLLECTION_INTERNAL_H

#include "display/ui/element.h"

/** Append one element to caller-owned compact UI storage. */
static inline void unsigned_ui_collection_add(UUIElement **items, u8 *count, UUIElement *element) {
    items[*count] = element;
    ++(*count);
}

/** Remove one known member while preserving the order of the remaining elements. */
static inline u8 unsigned_ui_collection_remove(UUIElement **items, u8 count, UUIElement *element) {
    u8 index = 0u;
    while (items[index] != element) {
        ++index;
    }

    for (u8 i = index; i + 1u < count; ++i) {
        items[i] = items[i + 1u];
    }
    return (u8)(count - 1u);
}

#endif
