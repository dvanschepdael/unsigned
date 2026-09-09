/**
 * @file collision_box.c
 * @brief Implements axis-aligned collision-box geometry.
 */

#include "physics/collision_box.h"

void unsigned_physics_collision_box_set_position(UCollisionBox *box, const Vec2 *position) {
    if (box == NULL || position == NULL) {
        return;
    }

    box->x = box->offset_x + position->x;
    box->y = box->offset_y + position->y;
}

bool unsigned_physics_collision_box_intersects(const UCollisionBox *a, const UCollisionBox *b) {
    if (a == NULL || b == NULL) {
        return false;
    }

    return unsigned_physics_collision_box_intersects_fast(a, b);
}
