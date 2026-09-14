/**
 * @file movement.h
 * @brief Generic world-position movement constraints.
 *
 * Spatial constraints belong to the movement/physics layer rather than actor state: actors
 * only store a world position, while callers decide whether a given movement is constrained.
 */

#ifndef UNSIGNED_PHYSICS_MOVEMENT_H
#define UNSIGNED_PHYSICS_MOVEMENT_H

#include "core/types.h"

typedef struct UMovementBounds {
    s16 min_x;
    s16 max_x;
    s16 min_y;
    s16 max_y;
} UMovementBounds;

/** Return whether bounds describe a non-inverted rectangle. */
bool unsigned_physics_movement_bounds_valid(const UMovementBounds *bounds);

/** Clamp a world position to optional movement bounds. NULL/invalid bounds leave it unchanged. */
void unsigned_physics_movement_constrain(Vec2 *position, const UMovementBounds *bounds);

#endif
