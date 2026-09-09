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
} UGameplayAttribute;

typedef struct UGameplayAttributeContainer {
    u8 count;
    u8 capacity;
    UGameplayAttribute *instances;
} UGameplayAttributeContainer;

#endif
