/**
 * @file collision_box.c
 * @brief Implements axis-aligned collision-box geometry.
 */

#include "physics/collision_box.h"

void unsigned_physics_collision_box_set_position(UCollisionBox *box, const Vec2 *position) {
    box->x = box->offset_x + position->x;
    box->y = box->offset_y + position->y;
}
