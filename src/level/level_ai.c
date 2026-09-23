/**
 * @file level_ai.c
 * @brief Implements level NPC activity classification and AI scheduling.
 */

#include "actor/npc.h"
#include "actor/npc_ai.h"
#include "actor/npc_config.h"
#include "display/viewport/viewport_internal.h"
#include "level/level_internal.h"

/** Tests the NPC sprite bounds against the viewport expanded by the configured AI activity margins. */
static bool level_npc_is_in_active_region(const UNpc *npc, const UViewportWorldBounds *bounds) {
    const UActor *actor = &npc->character->actor;
    const USpriteDefinition *sprite_definition = actor->sprite.definition;
    s32 world_left = (s32)actor->position.x + actor->sprite.offset.x;
    s32 world_top = (s32)actor->position.y + actor->sprite.offset.y;
    const s32 world_right = world_left + (s32)sprite_definition->width_tiles * 16;
    const s32 world_bottom = world_top + (s32)sprite_definition->height_tiles * 16;

    return unsigned_viewport_world_bounds_intersects_unchecked(bounds, world_left, world_top, world_right - world_left, world_bottom - world_top, UNSIGNED_NPC_ACTIVE_MARGIN_X, UNSIGNED_NPC_ACTIVE_MARGIN_Y);
}

/** Classifies one NPC as active, off-screen or dormant from viewport distance/visibility. */
static void level_npc_update_activity(UNpc *npc, const UViewportWorldBounds *bounds) {
    bool in_active_region = level_npc_is_in_active_region(npc, bounds);

    if (npc->activity == U_NPC_ACTIVITY_DORMANT) {
        if (in_active_region) {
            npc->activity = U_NPC_ACTIVITY_ACTIVE;
        }
        return;
    }

    npc->activity = in_active_region ? U_NPC_ACTIVITY_ACTIVE : U_NPC_ACTIVITY_OFFSCREEN;
}

void level_ai_tick(ULevel *level, const UViewport *viewport) {
    UViewportWorldBounds bounds;
    unsigned_viewport_world_bounds(viewport, &bounds);

    UPoolInstanceContainer *pool = &level->actor_pools->npcs;
    if (pool->count == 0u) {
        return;
    }

    u8 remaining = pool->count;
    for (u8 i = 0u; i < unsigned_pool_iteration_end(pool) && remaining > 0u; ++i) {
        UPoolInstance *instance = &pool->instances[i];

        if (!instance->active) {
            continue;
        }
        --remaining;

        UNpc *npc = instance->args;
        level_npc_update_activity(npc, &bounds);
        unsigned_npc_ai_tick(npc, &level->tlss, i);
    }
}
