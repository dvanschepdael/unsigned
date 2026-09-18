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

/** Validate definition-intrinsic ordering; viewport-size validation happens at runtime. */
bool unsigned_level_camera_definition_valid(const ULevelCameraDefinition *definition);

/** Initialize the level-owned camera from optional content configuration. */
bool unsigned_level_camera_init(struct ULevel *level, const ULevelCameraDefinition *definition);

/**
 * Apply player screen constraints and dead-zone following after gameplay movement for this frame.
 * NULL camera definitions keep the camera fixed but still maintain per-frame delta state.
 */
void unsigned_level_camera_tick(struct ULevel *level, const struct UViewport *viewport);

/** Override camera-origin bounds at runtime, for example while an arena is locked. */
bool unsigned_level_camera_set_bounds(struct ULevel *level, const UCameraBounds *bounds);

/** Restore the bounds declared by the current level definition. */
bool unsigned_level_camera_reset_bounds(struct ULevel *level);

#endif
