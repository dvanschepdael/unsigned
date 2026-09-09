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

typedef struct UCollisionBoxContainer {
    u8 count;
    u8 capacity;
    const UCollisionBox **instances;
    bool overflowed;
} UCollisionBoxContainer;

/**
 * @brief Tests two already validated axis-aligned boxes for strict area overlap.
 *
 * @details This inline fast path does not perform NULL checks; use unsigned_physics_collision_box_intersects() for defensive callers.
 *
 * @param a First non-NULL collision box.
 * @param b Second non-NULL collision box.
 * @return true when both boxes have positive area and overlap on both axes; false otherwise.
 */
static inline bool unsigned_physics_collision_box_intersects_fast(const UCollisionBox *a, const UCollisionBox *b) {
    return a->w > 0 && a->h > 0 && b->w > 0 && b->h > 0 && (s32)a->x < (s32)b->x + b->w && (s32)a->x + a->w > b->x && (s32)a->y < (s32)b->y + b->h && (s32)a->y + a->h > b->y;
}

/**
 * @brief Places a collision box at a world position while preserving its local offsets and size.
 *
 * @param box Collision box whose world-space x/y coordinates are updated.
 * @param position World-space origin to which offset_x/offset_y are applied.
 */
void unsigned_physics_collision_box_set_position(UCollisionBox *box, const Vec2 *position);

/**
 * @brief Safely tests two axis-aligned collision boxes for strict area overlap.
 *
 * @param a First collision box.
 * @param b Second collision box.
 * @return true when both pointers are valid and the boxes overlap with positive area; false for NULL, empty or non-overlapping boxes.
 */
bool unsigned_physics_collision_box_intersects(const UCollisionBox *a, const UCollisionBox *b);

#endif
