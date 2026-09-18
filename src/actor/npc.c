/**
 * @file npc.c
 * @brief Implements NPC runtime state, state graph and activity scheduling.
 */

#include "actor/npc.h"

#include <stdint.h>

bool unsigned_npc_init(UNpc *npc) {
    if (npc == NULL || npc->character == NULL || npc->state_graph.initial == NULL) {
        return false;
    }

    npc->tlss_ai = (UTLSSNode){
        .last_tick = UINT16_MAX,
    };

    return unsigned_state_graph_init(&npc->state_graph, npc->state_graph.global, npc->state_graph.initial, npc);
}

void unsigned_npc_tick(UNpc *npc) {
    if (npc == NULL || npc->character == NULL) {
        return;
    }

    unsigned_character_tick(npc->character);
}

void unsigned_npc_destroy(UNpc *npc) {
    if (npc == NULL) {
        return;
    }

    unsigned_state_graph_stop(&npc->state_graph);

    if (npc->character != NULL) {
        unsigned_character_destroy(npc->character);
    }
}

void unsigned_npc_set_activity(UNpc *npc, UNpcActivity activity) {
    if (npc == NULL) {
        return;
    }

    npc->activity = activity;
}
