/**
 * @file level_camera.h
 * @brief Beat'em-up follow policy for the level-owned camera.
 */

#ifndef UNSIGNED_LEVEL_CAMERA_H
#define UNSIGNED_LEVEL_CAMERA_H

#include "core/types.h"
#include "display/camera/camera.h"

struct ULevel;

/** High-level horizontal/vertical follow policy authored by a level. */
typedef enum ULevelCameraFollowMode {
    /** Existing beat'em-up comfort zone: camera moves only when players leave authored edges. */
    U_LEVEL_CAMERA_FOLLOW_DEAD_ZONE = 0,
    /** Center solo play; in multiplayer, scroll only once every active player is in the same screen half. */
    U_LEVEL_CAMERA_FOLLOW_CENTERED,
} ULevelCameraFollowMode;

/** Camera follow policy and world-space camera-origin limits. */
typedef struct ULevelCameraDefinition {
    Vec2 start;
    UCameraLimits limits;
    ULevelCameraFollowMode follow_mode;
    /** Screen-local comfort-zone edges used only by `U_LEVEL_CAMERA_FOLLOW_DEAD_ZONE`. */
    u16 dead_zone_left;
    u16 dead_zone_right;
    u16 dead_zone_top;
    u16 dead_zone_bottom;
    bool follow_x;
    bool follow_y;
    /** Allow horizontal camera movement toward -X for either follow policy. */
    bool allow_backtracking_x;
    /** Clamp active players to the final visible camera rectangle after following. */
    bool constrain_players;
} ULevelCameraDefinition;

/**
 * @brief Reset world position/limits from optional authored follow configuration.
 * @pre `level` owns an initialized camera. Limits are ordered; dead-zone edges are ordered when that mode is selected.
 */
void unsigned_level_camera_init(struct ULevel *level, const ULevelCameraDefinition *definition);

/**
 * Apply player screen constraints and the configured follow policy after gameplay movement for this frame.
 * NULL camera definitions keep the camera fixed but still maintain per-frame delta state.
 * @pre In dead-zone mode, authored horizontal/vertical edges are ordered and fit the camera rectangle.
 */
void unsigned_level_camera_tick(struct ULevel *level);

/** Override camera-origin limits at runtime, for example while an arena is locked. */
void unsigned_level_camera_set_limits(struct ULevel *level, const UCameraLimits *limits);

/** Restore the limits declared by the current level definition.
 * @pre The loaded level declares a camera definition.
 */
void unsigned_level_camera_reset_limits(struct ULevel *level);

#endif
