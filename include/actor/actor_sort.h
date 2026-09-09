/**
 * @file actor_sort.h
 * @brief Actor draw-order policies.
 */

#ifndef UNSIGNED_ACTOR_ORDER_H
#define UNSIGNED_ACTOR_ORDER_H

typedef enum UActorSort {
    U_ACTOR_ORDER_Y_STABLE = 0,
    U_ACTOR_ORDER_Y_X_STABLE,
    U_ACTOR_ORDER_COUNT,
} UActorSort;

#endif
