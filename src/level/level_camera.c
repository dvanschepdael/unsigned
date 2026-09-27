/**
 * @file level_camera.c
 * @brief Implements deterministic Beat'em-up camera following over active players.
 */

#include "level/level_camera.h"

#include "actor/player.h"
#include "core/math/math.h"
#include "level/level_runtime.h"
#include "physics/movement.h"

typedef struct ULevelPlayerExtent {
    s16 min_x;
    s16 max_x;
    s16 min_y;
    s16 max_y;
} ULevelPlayerExtent;

void unsigned_level_camera_init(ULevel *level, const ULevelCameraDefinition *definition) {
    UCamera *camera = &level->camera;

    unsigned_camera_clear_limits(camera);
    if (definition == NULL) {
        unsigned_camera_set_position(camera, 0, 0);
        unsigned_camera_begin_frame(camera);
        return;
    }

    unsigned_camera_set_position(camera, definition->start.x, definition->start.y);
    unsigned_camera_begin_frame(camera);
    unsigned_camera_set_limits(camera, &definition->limits);
}

/** Apply camera constraints to all active players without coupling actor state to camera internals. */
static void level_camera_constrain_players(ULevel *level) {
    UPoolInstanceContainer *players = &level->actor_pools->players;
    const UCamera *camera = &level->camera;

    const UMovementBounds bounds = {
        .min_x = camera->x,
        .max_x = unsigned_math_saturate_s16((s32)camera->x + camera->width - 1),
        .min_y = camera->y,
        .max_y = unsigned_math_saturate_s16((s32)camera->y + camera->height - 1),
    };

    for (u8 i = 0u; i < unsigned_pool_iteration_end(players); ++i) {
        UPoolInstance *instance = &players->instances[i];
        if (!instance->active) {
            continue;
        }
        UPlayer *player = instance->args;
        unsigned_physics_movement_constrain(&player->character->actor.position, &bounds);
    }
}

/** Build the inclusive player-origin extent used by the multiplayer follow policy.
 * @pre The player pool contains at least one active player.
 */
static ULevelPlayerExtent level_camera_player_extent(ULevel *level) {
    UPoolInstanceContainer *players = &level->actor_pools->players;
    const u8 end = unsigned_pool_iteration_end(players);
    const UPlayer *last_player = players->instances[end - 1u].args;
    const Vec2 last_position = last_player->character->actor.position;
    ULevelPlayerExtent extent = {
        .min_x = last_position.x,
        .max_x = last_position.x,
        .min_y = last_position.y,
        .max_y = last_position.y,
    };

    for (u8 i = 0u; i + 1u < end; ++i) {
        const UPoolInstance *instance = &players->instances[i];
        if (!instance->active) {
            continue;
        }
        const UPlayer *player = instance->args;
        const Vec2 position = player->character->actor.position;
        extent.min_x = unsigned_math_min_s16(extent.min_x, position.x);
        extent.max_x = unsigned_math_max_s16(extent.max_x, position.x);
        extent.min_y = unsigned_math_min_s16(extent.min_y, position.y);
        extent.max_y = unsigned_math_max_s16(extent.max_y, position.y);
    }

    return extent;
}

/** Compute a camera origin that keeps the multiplayer extent inside the configured comfort zone. */
static Vec2 level_camera_dead_zone_position(const UCamera *camera, const ULevelCameraDefinition *definition, ULevelPlayerExtent extent) {
    Vec2 result = {.x = camera->x, .y = camera->y};

    if (definition->follow_x) {
        const s16 left = (s16)definition->dead_zone_left;
        const s16 right = (s16)definition->dead_zone_right;
        const s32 right_limit = (s32)camera->x + right;
        const s32 left_limit = (s32)camera->x + left;

        /* Forward pressure wins when players span more than the comfort zone. */
        if ((s32)extent.max_x > right_limit) {
            result.x = unsigned_math_saturate_s16((s32)extent.max_x - right);
        } else if (definition->allow_backtracking_x && (s32)extent.min_x < left_limit) {
            result.x = unsigned_math_saturate_s16((s32)extent.min_x - left);
        }
    }

    if (definition->follow_y) {
        const s16 top = (s16)definition->dead_zone_top;
        const s16 bottom = (s16)definition->dead_zone_bottom;
        const s32 bottom_limit = (s32)camera->y + bottom;
        const s32 top_limit = (s32)camera->y + top;

        if ((s32)extent.max_y > bottom_limit) {
            result.y = unsigned_math_saturate_s16((s32)extent.max_y - bottom);
        } else if ((s32)extent.min_y < top_limit) {
            result.y = unsigned_math_saturate_s16((s32)extent.min_y - top);
        }
    }

    return result;
}

/** Midpoint of an inclusive multiplayer extent without overflow or a target divide instruction. */
static s16 level_camera_extent_midpoint(s16 min, s16 max) {
    return unsigned_math_saturate_s16((s32)min + (((s32)max - min) >> 1));
}

/**
 * Keep a solo player centered, but gate horizontal multiplayer scrolling until the whole group
 * occupies the same screen half. The trailing player is placed on the center line, so a faster
 * player cannot drag the camera away from the rest of the group.
 */
static Vec2 level_camera_centered_position(const UCamera *camera, const ULevelCameraDefinition *definition, ULevelPlayerExtent extent) {
    Vec2 result = {.x = camera->x, .y = camera->y};

    if (definition->follow_x) {
        const s32 half_width = (s32)(camera->width >> 1);
        const s32 center_x = (s32)camera->x + half_width;

        if ((s32)extent.min_x > center_x) {
            result.x = unsigned_math_saturate_s16((s32)extent.min_x - half_width);
        } else if (definition->allow_backtracking_x && (s32)extent.max_x < center_x) {
            result.x = unsigned_math_saturate_s16((s32)extent.max_x - half_width);
        }
    }

    if (definition->follow_y) {
        const s16 focus_y = level_camera_extent_midpoint(extent.min_y, extent.max_y);
        const s32 half_height = (s32)(camera->height >> 1);
        result.y = unsigned_math_saturate_s16((s32)focus_y - half_height);
    }

    return result;
}

/** Compute the camera origin using the level-authored follow policy. */
static Vec2 level_camera_follow_position(const UCamera *camera, const ULevelCameraDefinition *definition, ULevelPlayerExtent extent) {
    if (definition->follow_mode == U_LEVEL_CAMERA_FOLLOW_CENTERED) {
        return level_camera_centered_position(camera, definition, extent);
    }

    return level_camera_dead_zone_position(camera, definition, extent);
}

void unsigned_level_camera_tick(ULevel *level) {
    UCamera *camera = &level->camera;
    unsigned_camera_begin_frame(camera);

    if (level->definition->camera == NULL) {
        return;
    }

    const ULevelCameraDefinition *definition = level->definition->camera;

    if (level->actor_pools->players.count == 0u) {
        return;
    }

    const ULevelPlayerExtent extent = level_camera_player_extent(level);
    const Vec2 position = level_camera_follow_position(camera, definition, extent);
    unsigned_camera_set_position(camera, position.x, position.y);

    /* Camera movement can consume space behind a lagging player; keep every player visible. */
    if (definition->constrain_players) {
        level_camera_constrain_players(level);
    }
}

void unsigned_level_camera_set_limits(ULevel *level, const UCameraLimits *limits) {
    unsigned_camera_set_limits(&level->camera, limits);
}

void unsigned_level_camera_reset_limits(ULevel *level) {
    unsigned_camera_set_limits(&level->camera, &level->definition->camera->limits);
}
