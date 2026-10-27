/**
 * @file actor_sort.h
 * @brief Actor draw-order policies and the target-specific sort boundary.
 */

#ifndef UNSIGNED_ACTOR_SORT_H
#define UNSIGNED_ACTOR_SORT_H

typedef enum UActorSort {
    U_ACTOR_ORDER_Y_STABLE = 0,
    U_ACTOR_ORDER_Y_X_STABLE,
    U_ACTOR_ORDER_COUNT,
} UActorSort;

#include "core/types.h"

struct UActor;

/**
 * Stable insertion-sort of actor pointers by world Y, optionally then world X.
 *
 * Implemented by actor_sort.asm on Neo Geo and by the host mock in tests.
 * The routine deliberately receives only the pointer span and scalar policy.
 * `UActor::position` is the first field of UActor, with Vec2 laid out as
 * signed 16-bit x/y words; that layout is part
 * of the narrow C/ASM ABI.
 *
 * @pre `instances` contains `count >= 2` valid actor pointers.
 * @pre `order` is a valid UActorSort value.
 * @return non-zero when at least one pointer moved.
 */
u32 unsigned_actor_sort(struct UActor **instances, u32 count, u32 order);

#endif
