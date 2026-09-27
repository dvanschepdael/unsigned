/**
 * @file collision_box.h
 * @brief Axis-aligned collision-box geometry.
 */

#ifndef UNSIGNED_PHYSICS_COLLISION_BOX_H
#define UNSIGNED_PHYSICS_COLLISION_BOX_H

#include "core/types.h"

typedef struct UCollisionBox {
    s16 x;
    s16 y;
    s16 w;
    s16 h;
    s16 offset_x;
    s16 offset_y;
} UCollisionBox;

/**
 * @brief Tests whether two gameplay hit regions overlap with positive area.
 *
 * @pre `a` and `b` are valid collision boxes.
 * @return true when both boxes have positive area and overlap on both axes.
 */
static inline bool unsigned_physics_collision_box_intersects(const UCollisionBox *a, const UCollisionBox *b) {
    return a->w > 0 && a->h > 0 && b->w > 0 && b->h > 0 && (s32)a->x < (s32)b->x + b->w && (s32)a->x + a->w > b->x && (s32)a->y < (s32)b->y + b->h && (s32)a->y + a->h > b->y;
}

/**
 * @brief Places a collision box at a world position while preserving its local offsets and size.
 *
 * @param box Collision box whose world-space x/y coordinates are updated.
 * @param position World-space origin to which offset_x/offset_y are applied.
 * @pre `box` and `position` are valid.
 */
void unsigned_physics_collision_box_set_position(UCollisionBox *box, const Vec2 *position);

#endif
