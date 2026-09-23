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
    UTLSSNode tlss_presentation;
    /** Sprite playback revision represented by presentation_animation_last_tick. */
    u16 presentation_playback_revision;
    /** Last engine frame applied to the current animation; independent from effect catch-up. */
    u16 presentation_animation_last_tick;
    UNpcActivity activity;
} UNpc;

/**
 * @brief Initializes NPC scheduling and starts its optional state graph using the NPC as graph context.
 *
 * A NULL `state_graph.initial` is a supported passive-NPC configuration: no AI graph is scheduled,
 * while character presentation and collision remain available.
 *
 * @param npc NPC whose character pointer is already configured.
 * @pre `npc` and `npc->character` are valid. The optional state graph definition is authored correctly.
 */
void unsigned_npc_init(UNpc *npc);

/**
 * @brief Advances the NPC by one scheduled engine frame.
 *
 * @param npc NPC runtime whose character/state-graph scheduling state is updated.
 * @pre `npc` and `npc->character` are valid.
 */
/** Advance NPC presentation using off-screen TLSS only when animation semantics are batch-safe.
 * @pre `npc`, `npc->character` and `tlss` are valid.
 */
void unsigned_npc_tick_scheduled(UNpc *npc, const UTLSS *tlss, u16 slot);

/**
 * @brief Tears down the NPC runtime state and releases its logical resources.
 *
 * @param npc NPC runtime whose character/state-graph scheduling state is updated.
 */
void unsigned_npc_destroy(UNpc *npc);

#endif
