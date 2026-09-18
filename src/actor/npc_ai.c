/**
 * @file npc_ai.c
 * @brief Implements NPC AI update scheduling with TLSS throttling.
 */

#include "actor/npc_ai.h"

#include "actor/npc.h"
#include "actor/npc_config.h"

/** Chooses the NPC AI TLSS scale from global AI cadence and the NPC active/off-screen policy. */
static UTLSSScale npc_ai_scale(const UNpc *npc, const UTLSS *tlss) {
    UTLSSScale scale = tlss->scales.ai;
    UTLSSScale offscreen_scale = UNSIGNED_NPC_OFFSCREEN_AI_SCALE;

    if ((unsigned int)offscreen_scale > (unsigned int)U_TLSS_SCALE_16) {
        offscreen_scale = U_TLSS_SCALE_16;
    }

    if (npc->activity == U_NPC_ACTIVITY_OFFSCREEN && (unsigned int)offscreen_scale > (unsigned int)scale) {
        scale = offscreen_scale;
    }
    return scale;
}

void unsigned_npc_ai_tick(UNpc *npc, const UTLSS *tlss, u16 slot) {
    if (npc == NULL || npc->activity == U_NPC_ACTIVITY_DORMANT) {
        return;
    }

    if (tlss == NULL) {
        unsigned_state_graph_tick(&npc->state_graph);
        return;
    }

    UTLSSScale scale = npc_ai_scale(npc, tlss);
    if (!unsigned_tlss_node_matches_scale(&npc->tlss_ai, scale)) {
        unsigned_tlss_node_set_scale_slot(&npc->tlss_ai, slot, scale);
    }
    if (!unsigned_tlss_should_tick(tlss, &npc->tlss_ai)) {
        return;
    }

    (void)unsigned_tlss_tick(tlss, &npc->tlss_ai);
    unsigned_state_graph_tick(&npc->state_graph);
}

void unsigned_npc_pool_ai_tick(UPoolInstanceContainer *pool, const UTLSS *tlss) {
    if (pool == NULL || pool->count == 0u) {
        return;
    }

    for (u8 i = 0u; i < pool->capacity; ++i) {
        UPoolInstance *instance = &pool->instances[i];

        if (!instance->active || instance->args == NULL) {
            continue;
        }

        unsigned_npc_ai_tick(instance->args, tlss, i);
    }
}
