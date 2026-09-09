/**
 * @file npc_ai.h
 * @brief NPC AI update scheduling with TLSS throttling.
 */

#ifndef UNSIGNED_ACTOR_NPC_AI_H
#define UNSIGNED_ACTOR_NPC_AI_H

#include "core/pool/pool.h"
#include "core/tlss/tlss.h"

struct UNpc;

/**
 * @brief Advances one NPC state graph only on the TLSS frames assigned to its current activity level.
 *
 * @param npc NPC runtime whose character/state-graph scheduling state is updated.
 * @param tlss TLSS scheduler that determines temporal update cadence.
 * @param slot Stable NPC pool slot used to phase-distribute TLSS work across frames.
 */
void unsigned_npc_ai_tick(struct UNpc *npc, const UTLSS *tlss, u16 slot);

/**
 * @brief Runs TLSS-aware AI updates for every active NPC stored in the pool.
 *
 * @param pool Fixed-capacity pool that owns the runtime slot.
 * @param tlss TLSS scheduler that determines temporal update cadence.
 */
void unsigned_npc_pool_ai_tick(UPoolInstanceContainer *pool, const UTLSS *tlss);

#endif
