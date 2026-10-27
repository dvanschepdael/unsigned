/**
 * @file camera.h
 * @brief World-space camera, screen projection rectangle, culling and coordinate conversion.
 */

#ifndef UNSIGNED_DISPLAY_CAMERA_H
#define UNSIGNED_DISPLAY_CAMERA_H

#include "core/types.h"
#include "display/effect/effect.h"

/** Inclusive limits applied to the camera world-space origin. */
typedef struct UCameraLimits {
    s16 min_x;
    s16 max_x;
    s16 min_y;
    s16 max_y;
} UCameraLimits;

/** Visible world rectangle cached for repeated culling during one subsystem pass. */
typedef struct UCameraWorldBounds {
    s32 left;
    s32 top;
    s32 right;
    s32 bottom;
} UCameraWorldBounds;

/** Initial camera placement and screen projection rectangle. */
typedef struct UCameraConfig {
    Vec2 position;
    s16 screen_x;
    s16 screen_y;
    u16 width;
    u16 height;
} UCameraConfig;

/**
 * One developer-facing view of the world.
 *
 * `x`/`y` are the world-space origin projected at (`screen_x`, `screen_y`). The
 * screen rectangle defines the visible area; the level/world itself is independent
 * from the camera. Presentation effects are camera-owned so rendering, culling and
 * coordinate conversion all consume the same state without a second view object.
 */
typedef struct UCamera {
    s16 x;
    s16 y;
    /** Position captured at the beginning of the current engine frame. */
    s16 previous_x;
    s16 previous_y;
    s16 screen_x;
    s16 screen_y;
    u16 width;
    u16 height;
    /** Optional inclusive limits for the camera world-space origin. */
    UCameraLimits limits;
    bool limits_enabled;
    UEffect effect;
} UCamera;

/** Initialize one complete camera view.
 * @pre `camera` and `config` are valid; configured dimensions are non-zero.
 */
void unsigned_camera_init(UCamera *camera, const UCameraConfig *config);

/** Replace the screen-space rectangle projected by the camera. */
void unsigned_camera_set_screen_rect(UCamera *camera, s16 x, s16 y, u16 width, u16 height);

/** Capture the current position before follow/gameplay policy changes it this frame. */
void unsigned_camera_begin_frame(UCamera *camera);

/** Advance camera-owned presentation effects by one engine frame. */
void unsigned_camera_tick(UCamera *camera);

/**
 * @brief Set and enable camera-origin limits, clamping current and previous positions.
 * @pre `camera` and `limits` are valid.
 * @pre `limits->min_x <= limits->max_x` and `limits->min_y <= limits->max_y`.
 */
void unsigned_camera_set_limits(UCamera *camera, const UCameraLimits *limits);

/** Disable movement limits without changing the current camera position. */
void unsigned_camera_clear_limits(UCamera *camera);

/** Move the camera to a world-space origin, respecting active limits. */
void unsigned_camera_set_position(UCamera *camera, s16 x, s16 y);

/**
 * @brief Build the visible world rectangle once for repeated culling queries.
 * @pre `camera` and `bounds` are valid; camera dimensions are non-zero.
 */
void unsigned_camera_world_bounds(const UCamera *camera, UCameraWorldBounds *bounds);

/** Test a world rectangle against precomputed camera bounds expanded by optional margins.
 * @pre `bounds` is valid, `width` and `height` are positive, and margins are non-negative.
 */
bool unsigned_camera_world_bounds_intersects(const UCameraWorldBounds *bounds, s32 world_x, s32 world_y, s32 width, s32 height, s32 margin_x, s32 margin_y);

/** Convert one world-space X coordinate to screen space. */
s16 unsigned_camera_world_to_screen_x(const UCamera *camera, s32 world_x);

/** Convert one world-space Y coordinate to screen space. */
s16 unsigned_camera_world_to_screen_y(const UCamera *camera, s32 world_y);

/** Convert one screen-space X coordinate to world space. */
s16 unsigned_camera_screen_to_world_x(const UCamera *camera, s16 screen_x);

/** Convert one screen-space Y coordinate to world space. */
s16 unsigned_camera_screen_to_world_y(const UCamera *camera, s16 screen_y);

/**
 * Test an axis-aligned world rectangle against the visible camera rectangle expanded by margins.
 * @pre Camera dimensions are non-zero; width/height are positive and margins are non-negative.
 */
bool unsigned_camera_intersects_world_rect(const UCamera *camera, s32 world_x, s32 world_y, s32 width, s32 height, s32 margin_x, s32 margin_y);

#endif
