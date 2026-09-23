/**
 * @file level_camera.h
 * @brief Beat'em-up camera policy for one loaded level.
 */

#ifndef UNSIGNED_LEVEL_CAMERA_H
#define UNSIGNED_LEVEL_CAMERA_H

#include "core/types.h"
#include "display/camera/camera.h"

struct ULevel;
struct UViewport;

/** Screen-space dead-zone edges (viewport-local pixels) and world-space camera-origin limits. */
typedef struct ULevelCameraDefinition {
    Vec2 start;
    UCameraBounds bounds;
    u16 dead_zone_left;
    u16 dead_zone_right;
    u16 dead_zone_top;
    u16 dead_zone_bottom;
    bool follow_x;
    bool follow_y;
    bool allow_backtracking_x;
    bool constrain_players;
} ULevelCameraDefinition;

/**
 * @brief Initialize the level camera from optional authored follow configuration.
 * @pre `level` is valid. Camera bounds and dead-zone edges are ordered when `definition` is non-NULL.
 */
void unsigned_level_camera_init(struct ULevel *level, const ULevelCameraDefinition *definition);

/**
 * Apply player screen constraints and dead-zone following after gameplay movement for this frame.
 * NULL camera definitions keep the camera fixed but still maintain per-frame delta state.
 * @pre Authored horizontal dead-zone edges are ordered and <= `viewport->width`.
 * @pre Authored vertical dead-zone edges are ordered and <= `viewport->height`.
 */
void unsigned_level_camera_tick(struct ULevel *level, const struct UViewport *viewport);

/** Override camera-origin bounds at runtime, for example while an arena is locked.
 * @pre `level` and `bounds` are valid.
 */
void unsigned_level_camera_set_bounds(struct ULevel *level, const UCameraBounds *bounds);

/** Restore the bounds declared by the current level definition.
 * @pre The loaded level declares a camera definition.
 */
void unsigned_level_camera_reset_bounds(struct ULevel *level);

#endif
