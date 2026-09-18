/**
 * @file level_camera.c
 * @brief Implements deterministic Beat'em-up camera following over active players.
 */

#include "level/level_camera.h"

#include "actor/player.h"
#include "core/math/math.h"
#include "display/viewport/viewport.h"
#include "level/level_actor.h"
#include "level/level_runtime.h"
#include "physics/movement.h"

typedef struct ULevelPlayerExtent {
    s16 min_x;
    s16 max_x;
    s16 min_y;
    s16 max_y;
    bool valid;
} ULevelPlayerExtent;

bool unsigned_level_camera_definition_valid(const ULevelCameraDefinition *definition) {
    return definition != NULL && unsigned_camera_bounds_valid(&definition->bounds) &&
           definition->dead_zone_left <= definition->dead_zone_right &&
           definition->dead_zone_top <= definition->dead_zone_bottom;
}

bool unsigned_level_camera_init(ULevel *level, const ULevelCameraDefinition *definition) {
    if (level == NULL) {
        return false;
    }

    if (definition == NULL) {
        unsigned_camera_init(&level->camera, 0, 0);
        return true;
    }
    if (!unsigned_level_camera_definition_valid(definition)) {
        return false;
    }

    unsigned_camera_init(&level->camera, definition->start.x, definition->start.y);
    return unsigned_camera_set_bounds(&level->camera, &definition->bounds);
}

/** Clamp one active player ground origin to the current visible world rectangle. */
static void level_camera_constrain_player(UPlayer *player, const UViewport *viewport) {
    if (player == NULL || player->character == NULL || !unsigned_viewport_is_valid(viewport)) {
        return;
    }

    const s32 max_x = (s32)viewport->camera->x + viewport->width - 1;
    const s32 max_y = (s32)viewport->camera->y + viewport->height - 1;
    const UMovementBounds bounds = {
        .min_x = viewport->camera->x,
        .max_x = unsigned_math_saturate_s16(max_x),
        .min_y = viewport->camera->y,
        .max_y = unsigned_math_saturate_s16(max_y),
    };
    unsigned_physics_movement_constrain(&player->character->actor.position, &bounds);
}

/** Apply viewport constraints to all active players without coupling actor state to the camera. */
static void level_camera_constrain_players(ULevel *level, const UViewport *viewport) {
    UPoolInstanceContainer *players = unsigned_level_player_pool(level);
    if (players == NULL) {
        return;
    }

    for (u8 i = 0u; i < players->capacity; ++i) {
        UPoolInstance *instance = &players->instances[i];
        if (!instance->active || instance->args == NULL) {
            continue;
        }
        level_camera_constrain_player(instance->args, viewport);
    }
}

/** Build the inclusive player-origin extent used by the multiplayer follow policy. */
static ULevelPlayerExtent level_camera_player_extent(ULevel *level) {
    ULevelPlayerExtent extent = { 0 };
    UPoolInstanceContainer *players = unsigned_level_player_pool(level);

    if (players == NULL) {
        return extent;
    }

    for (u8 i = 0u; i < players->capacity; ++i) {
        const UPoolInstance *instance = &players->instances[i];
        if (!instance->active || instance->args == NULL) {
            continue;
        }

        const UPlayer *player = instance->args;
        if (player->character == NULL || !player->character->actor.active) {
            continue;
        }

        const Vec2 position = player->character->actor.position;
        if (!extent.valid) {
            extent = (ULevelPlayerExtent){
                .min_x = position.x,
                .max_x = position.x,
                .min_y = position.y,
                .max_y = position.y,
                .valid = true,
            };
            continue;
        }

        extent.min_x = unsigned_math_min_s16(extent.min_x, position.x);
        extent.max_x = unsigned_math_max_s16(extent.max_x, position.x);
        extent.min_y = unsigned_math_min_s16(extent.min_y, position.y);
        extent.max_y = unsigned_math_max_s16(extent.max_y, position.y);
    }

    return extent;
}

/** Clamp one dead-zone edge to the viewport dimension supplied by the active game instance. */
static s16 level_camera_zone_edge(u16 edge, u16 dimension) {
    return (s16)unsigned_math_min_u16(edge, dimension);
}

/** Compute a camera origin that keeps the multiplayer extent inside the configured comfort zone. */
static Vec2 level_camera_follow_position(const UCamera *camera, const UViewport *viewport, const ULevelCameraDefinition *definition, ULevelPlayerExtent extent) {
    Vec2 result = { .x = camera->x, .y = camera->y };

    if (definition->follow_x) {
        const s16 left = level_camera_zone_edge(definition->dead_zone_left, viewport->width);
        const s16 right = level_camera_zone_edge(definition->dead_zone_right, viewport->width);
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
        const s16 top = level_camera_zone_edge(definition->dead_zone_top, viewport->height);
        const s16 bottom = level_camera_zone_edge(definition->dead_zone_bottom, viewport->height);
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
    if (level == NULL) {
        return;
    }

    unsigned_camera_begin_frame(&level->camera);

    if (level->definition == NULL || level->definition->camera == NULL || !unsigned_viewport_is_valid(viewport)) {
        return;
    }

    const ULevelCameraDefinition *definition = level->definition->camera;
    if (!unsigned_level_camera_definition_valid(definition)) {
        return;
    }

    const ULevelPlayerExtent extent = level_camera_player_extent(level);
    if (!extent.valid) {
        return;
    }

    const Vec2 position = level_camera_follow_position(&level->camera, viewport, definition, extent);
    unsigned_camera_set_position(&level->camera, position.x, position.y);

    /* Camera movement can consume space behind a lagging player; keep every player visible. */
    if (definition->constrain_players) {
        level_camera_constrain_players(level, viewport);
    }
}

bool unsigned_level_camera_set_bounds(ULevel *level, const UCameraBounds *bounds) {
    return level != NULL && unsigned_camera_set_bounds(&level->camera, bounds);
}

bool unsigned_level_camera_reset_bounds(ULevel *level) {
    if (level == NULL || level->definition == NULL || level->definition->camera == NULL) {
        return false;
    }

    return unsigned_camera_set_bounds(&level->camera, &level->definition->camera->bounds);
}
