/**
 * @file npc.h
 * @brief NPC runtime state, state graph and activity scheduling.
 */

#ifndef UNSIGNED_ACTOR_NPC_H
#define UNSIGNED_ACTOR_NPC_H

#include "actor/character.h"
#include "core/state/state_graph.h"
#include "core/tlss/tlss.h"

typedef enum UNpcActivity {
    /** Near enough to gameplay to use the normal AI cadence. */
    U_NPC_ACTIVITY_ACTIVE = 0,
    /** Outside the active viewport region; eligible for a reduced AI cadence. */
    U_NPC_ACTIVITY_OFFSCREEN,
    /** Explicitly sleeping until game logic promotes it; lowest-priority simulation class. */
    U_NPC_ACTIVITY_DORMANT,
} UNpcActivity;

typedef struct UNpc {
    UCharacter *character;
    UStateGraph state_graph;
    UTLSSNode tlss_ai;
    UNpcActivity activity;
} UNpc;

/**
 * @brief Initializes NPC AI scheduling and validates/starts the NPC state graph using the NPC as graph context.
 *
 * @param npc NPC whose character pointer and state_graph initial node must already be configured.
 * @return true when NPC prerequisites and the state graph are valid; false otherwise.
 */
bool unsigned_npc_init(UNpc *npc);

/**
 * @brief Advances the NPC by one scheduled engine frame.
 *
 * @param npc NPC runtime whose character/state-graph scheduling state is updated.
 */
void unsigned_npc_tick(UNpc *npc);

/**
 * @brief Tears down the NPC runtime state and releases its logical resources.
 *
 * @param npc NPC runtime whose character/state-graph scheduling state is updated.
 */
void unsigned_npc_destroy(UNpc *npc);

/**
 * @brief Changes the NPC simulation class; the level AI scheduler maps it to an appropriate TLSS cadence.
 *
 * @param npc NPC runtime whose character/state-graph scheduling state is updated.
 * @param activity NPC activity class controlling update cadence.
 */
void unsigned_npc_set_activity(UNpc *npc, UNpcActivity activity);

#endif
