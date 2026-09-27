/**
 * @file level_ai.c
 * @brief Implements level NPC activity classification and AI scheduling.
 */

#include "actor/npc.h"
#include "actor/npc_ai.h"
#include "actor/npc_config.h"
#include "display/camera/camera_internal.h"
#include "level/level_internal.h"

/** Classifies one NPC as active, off-screen or dormant from camera distance/visibility. */
static void level_npc_update_activity(UNpc *npc, const UCameraWorldBounds *bounds) {
    const UActor *actor = &npc->character->actor;
    const USpriteDefinition *sprite_definition = actor->sprite.definition;
    const s32 world_left = (s32)actor->position.x + actor->sprite.offset.x;
    const s32 world_top = (s32)actor->position.y + actor->sprite.offset.y;
    const bool in_active_region = unsigned_camera_world_bounds_intersects_unchecked(
        bounds,
        world_left,
        world_top,
        (s32)sprite_definition->width_tiles * 16,
        (s32)sprite_definition->height_tiles * 16,
        UNSIGNED_NPC_ACTIVE_MARGIN_X,
        UNSIGNED_NPC_ACTIVE_MARGIN_Y);

    if (npc->activity == U_NPC_ACTIVITY_DORMANT) {
        if (in_active_region) {
            npc->activity = U_NPC_ACTIVITY_ACTIVE;
        }
        return;
    }

    npc->activity = in_active_region ? U_NPC_ACTIVITY_ACTIVE : U_NPC_ACTIVITY_OFFSCREEN;
}

void unsigned_level_ai_tick(ULevel *level) {
    UPoolInstanceContainer *pool = &level->actor_pools->npcs;
    if (pool->count == 0u) {
        return;
    }

    UCameraWorldBounds bounds;
    unsigned_camera_world_bounds(&level->camera, &bounds);

    for (u8 i = 0u; i < unsigned_pool_iteration_end(pool); ++i) {
        UPoolInstance *instance = &pool->instances[i];

        if (!instance->active) {
            continue;
        }

        UNpc *npc = instance->args;
        level_npc_update_activity(npc, &bounds);
        unsigned_actor_npc_ai_tick(npc, &level->tlss, i);
    }
}
