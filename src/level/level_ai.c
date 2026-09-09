/**
 * @file level_ai.c
 * @brief Implements level NPC activity classification and AI scheduling.
 */

#include "actor/npc.h"
#include "actor/npc_ai.h"
#include "actor/npc_config.h"
#include "display/viewport/viewport.h"
#include "level/level_internal.h"

/** Tests the NPC sprite bounds against the viewport expanded by the configured AI activity margins. */
static bool level_npc_is_in_active_region(const UNpc *npc, const UViewport *viewport) {
    if (npc == NULL || npc->character == NULL) {
        return false;
    }
    if (!unsigned_viewport_is_valid(viewport)) {
        return true;
    }

    const UActor *actor = &npc->character->actor;
    const USpriteDefinition *sprite_definition = actor->sprite.definition;
    s32 world_left = (s32)actor->position.x + actor->sprite.offset.x;
    s32 world_top = (s32)actor->position.y + actor->sprite.offset.y;
    s32 world_right = world_left + 1;
    s32 world_bottom = world_top + 1;

    if (sprite_definition != NULL) {
        world_right = world_left + (s32)sprite_definition->width_tiles * 16;
        world_bottom = world_top + (s32)sprite_definition->height_tiles * 16;
    }

    s32 active_left = (s32)viewport->camera->x - UNSIGNED_NPC_ACTIVE_MARGIN_X;
    s32 active_top = (s32)viewport->camera->y - UNSIGNED_NPC_ACTIVE_MARGIN_Y;
    s32 active_right = (s32)viewport->camera->x + viewport->width + UNSIGNED_NPC_ACTIVE_MARGIN_X;
    s32 active_bottom = (s32)viewport->camera->y + viewport->height + UNSIGNED_NPC_ACTIVE_MARGIN_Y;

    return world_right > active_left && world_left < active_right && world_bottom > active_top && world_top < active_bottom;
}

/** Classifies one NPC as active, off-screen or dormant from viewport distance/visibility. */
static void level_npc_update_activity(UNpc *npc, const UViewport *viewport) {
    if (npc == NULL) {
        return;
    }

    if (!unsigned_viewport_is_valid(viewport)) {
        if (npc->activity != U_NPC_ACTIVITY_DORMANT) {
            unsigned_npc_set_activity(npc, U_NPC_ACTIVITY_ACTIVE);
        }
        return;
    }

    bool in_active_region = level_npc_is_in_active_region(npc, viewport);

    if (npc->activity == U_NPC_ACTIVITY_DORMANT) {
        if (in_active_region) {
            unsigned_npc_set_activity(npc, U_NPC_ACTIVITY_ACTIVE);
        }
        return;
    }

    unsigned_npc_set_activity(npc, in_active_region ? U_NPC_ACTIVITY_ACTIVE : U_NPC_ACTIVITY_OFFSCREEN);
}

void level_ai_tick(ULevel *level, const UViewport *viewport) {
    if (level == NULL) {
        return;
    }

    UPoolInstanceContainer *pool = unsigned_level_npc_pool(level);
    if (pool == NULL || pool->count == 0u) {
        return;
    }

    for (u8 i = 0u; i < pool->capacity; ++i) {
        UPoolInstance *instance = &pool->instances[i];

        if (!instance->active || instance->args == NULL) {
            continue;
        }

        UNpc *npc = instance->args;
        level_npc_update_activity(npc, viewport);
        unsigned_npc_ai_tick(npc, &level->tlss, i);
    }
}
