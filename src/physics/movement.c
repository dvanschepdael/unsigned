/**
 * @file movement.c
 * @brief Implements generic world-position movement constraints.
 */

#include "physics/movement.h"

#include "core/math/math.h"

bool unsigned_physics_movement_bounds_valid(const UMovementBounds *bounds) {
    return bounds != NULL && bounds->min_x <= bounds->max_x && bounds->min_y <= bounds->max_y;
}

void unsigned_physics_movement_constrain(Vec2 *position, const UMovementBounds *bounds) {
    if (position == NULL || !unsigned_physics_movement_bounds_valid(bounds)) {
        return;
    }

    position->x = unsigned_math_clamp_s16(position->x, bounds->min_x, bounds->max_x);
    position->y = unsigned_math_clamp_s16(position->y, bounds->min_y, bounds->max_y);
}
