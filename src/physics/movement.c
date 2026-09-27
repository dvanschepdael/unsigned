/**
 * @file movement.c
 * @brief Implements generic world-position movement constraints.
 */

#include "physics/movement.h"

#include "core/math/math.h"

void unsigned_physics_movement_constrain(Vec2 *position, const UMovementBounds *bounds) {
    position->x = unsigned_math_clamp_s16(position->x, bounds->min_x, bounds->max_x);
    position->y = unsigned_math_clamp_s16(position->y, bounds->min_y, bounds->max_y);
}
