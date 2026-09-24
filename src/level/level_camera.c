/**
 * @file level_camera.c
 * @brief Implements deterministic Beat'em-up camera following over active players.
 */

#include "level/level_camera.h"

#include "actor/player.h"
#include "core/math/math.h"
#include "display/viewport/viewport.h"
#include "level/level_runtime.h"
#include "physics/movement.h"

typedef struct ULevelPlayerExtent {
    s16 min_x;
    s16 max_x;
    s16 min_y;
    s16 max_y;
} ULevelPlayerExtent;

void unsigned_level_camera_init(ULevel *level, const ULevelCameraDefinition *definition) {
    if (definition == NULL) {
        unsigned_camera_init(&level->camera, 0, 0);
        return;
    }
    unsigned_camera_init(&level->camera, definition->start.x, definition->start.y);
    unsigned_camera_set_bounds(&level->camera, &definition->bounds);
}

/** Apply viewport constraints to all active players without coupling actor state to the camera. */
static void level_camera_constrain_players(ULevel *level, const UViewport *viewport) {
    UPoolInstanceContainer *players = &level->actor_pools->players;

    const UMovementBounds bounds = {
        .min_x = viewport->camera->x,
        .max_x = unsigned_math_saturate_s16((s32)viewport->camera->x + viewport->width - 1),
        .min_y = viewport->camera->y,
        .max_y = unsigned_math_saturate_s16((s32)viewport->camera->y + viewport->height - 1),
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
static Vec2 level_camera_follow_position(const UCamera *camera, const ULevelCameraDefinition *definition, ULevelPlayerExtent extent) {
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

void unsigned_level_camera_tick(ULevel *level, const UViewport *viewport) {
    unsigned_camera_begin_frame(&level->camera);

    if (level->definition->camera == NULL) {
        return;
    }

    const ULevelCameraDefinition *definition = level->definition->camera;

    if (level->actor_pools->players.count == 0u) {
        return;
    }

    const ULevelPlayerExtent extent = level_camera_player_extent(level);
    const Vec2 position = level_camera_follow_position(&level->camera, definition, extent);
    unsigned_camera_set_position(&level->camera, position.x, position.y);

    /* Camera movement can consume space behind a lagging player; keep every player visible. */
    if (definition->constrain_players) {
        level_camera_constrain_players(level, viewport);
    }
}

void unsigned_level_camera_set_bounds(ULevel *level, const UCameraBounds *bounds) {
    unsigned_camera_set_bounds(&level->camera, bounds);
}

void unsigned_level_camera_reset_bounds(ULevel *level) {
    unsigned_camera_set_bounds(&level->camera, &level->definition->camera->bounds);
}
