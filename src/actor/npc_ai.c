/**
 * @file npc_ai.c
 * @brief Implements NPC AI update scheduling with TLSS throttling.
 */

#include "actor/npc_ai.h"

#include "actor/npc.h"
#include "actor/npc_config.h"

void unsigned_npc_ai_tick(UNpc *npc, const UTLSS *tlss, u16 slot) {
    if (npc->activity == U_NPC_ACTIVITY_DORMANT || npc->state_graph.initial == NULL) {
        return;
    }

    /* Off-screen NPCs may run less often than the global AI cadence, never more often. */
    const UTLSSScale offscreen_scale = UNSIGNED_NPC_OFFSCREEN_AI_SCALE;
    const UTLSSScale scale = npc->activity == U_NPC_ACTIVITY_OFFSCREEN && (unsigned int)offscreen_scale > (unsigned int)tlss->scales.ai ? offscreen_scale : tlss->scales.ai;
    if (!unsigned_tlss_node_matches_scale(&npc->tlss_ai, scale)) {
        unsigned_tlss_node_set_scale_slot(&npc->tlss_ai, slot, scale);
    }
    if (!unsigned_tlss_should_tick(tlss, &npc->tlss_ai)) {
        return;
    }

    (void)unsigned_tlss_tick(tlss, &npc->tlss_ai);
    unsigned_state_graph_tick(&npc->state_graph);
}
