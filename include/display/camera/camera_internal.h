/**
 * @file camera_internal.h
 * @brief Contract-based camera helpers for frame-critical rendering paths.
 */

#ifndef UNSIGNED_DISPLAY_CAMERA_INTERNAL_H
#define UNSIGNED_DISPLAY_CAMERA_INTERNAL_H

#include "display/camera/camera.h"

/**
 * @brief Tests visibility against camera bounds already established by the owning render pass.
 *
 * @pre `bounds` describes an ordered rectangle, `width` and `height` are positive, and margins
 *      are non-negative. Render passes establish these conditions once instead of rechecking them
 *      for every sprite.
 */
static inline bool unsigned_camera_world_bounds_intersects_unchecked(const UCameraWorldBounds *bounds, s32 world_x, s32 world_y, s32 width, s32 height, s32 margin_x, s32 margin_y) {
    const s32 left = bounds->left - margin_x;
    const s32 top = bounds->top - margin_y;
    const s32 right = bounds->right + margin_x;
    const s32 bottom = bounds->bottom + margin_y;

    return world_x + width > left && world_x < right && world_y + height > top && world_y < bottom;
}

/** Convert world X using camera state already validated by the owning render pass. */
static inline s16 unsigned_camera_world_to_screen_x_unchecked(const UCamera *camera, s32 world_x) {
    return (s16)((s32)camera->screen_x + world_x - camera->x);
}

/** Convert world Y using camera state already validated by the owning render pass. */
static inline s16 unsigned_camera_world_to_screen_y_unchecked(const UCamera *camera, s32 world_y) {
    return (s16)((s32)camera->screen_y + world_y - camera->y);
}

#endif
