/**
 * @file camera.h
 * @brief World-space camera state and bounded movement helpers.
 */

#ifndef UNSIGNED_DISPLAY_CAMERA_H
#define UNSIGNED_DISPLAY_CAMERA_H

#include "core/types.h"

typedef struct UCameraBounds {
    s16 min_x;
    s16 max_x;
    s16 min_y;
    s16 max_y;
} UCameraBounds;

typedef struct UCamera {
    /** World-space origin currently projected at the viewport origin. */
    s16 x;
    s16 y;
    /** Position captured at the beginning of the current engine frame. */
    s16 previous_x;
    s16 previous_y;
    /** Optional inclusive limits for the camera origin. */
    UCameraBounds bounds;
    bool bounds_enabled;
} UCamera;

/** Initialize one camera at a world-space origin with no active bounds.
 * @pre `camera` points to writable runtime storage.
 */
void unsigned_camera_init(UCamera *camera, s16 x, s16 y);

/** Capture the current position before follow/gameplay policy changes it this frame. */
void unsigned_camera_begin_frame(UCamera *camera);

/**
 * @brief Set and enable camera-origin bounds, clamping current and previous positions.
 * @pre `camera` and `bounds` are valid.
 * @pre `bounds->min_x <= bounds->max_x` and `bounds->min_y <= bounds->max_y`.
 */
void unsigned_camera_set_bounds(UCamera *camera, const UCameraBounds *bounds);

/** Disable bounds without changing the current camera position. */
void unsigned_camera_clear_bounds(UCamera *camera);

/** Move the camera to a world-space origin, respecting active bounds. */
void unsigned_camera_set_position(UCamera *camera, s16 x, s16 y);

#endif
