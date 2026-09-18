/**
 * @file npc_pool.h
 * @brief Fixed-capacity NPC pool binding and ticking.
 */

#ifndef UNSIGNED_ACTOR_NPC_POOL_H
#define UNSIGNED_ACTOR_NPC_POOL_H

#include "actor/npc.h"
#include "core/pool/pool.h"

typedef UPoolInstance UNpcPoolInstance;

/**
 * @brief Reserves an inactive slot from the NPC pool and binds it to the supplied runtime data.
 *
 * @param pool Fixed-capacity pool that owns the runtime slot.
 * @param npc NPC runtime whose character/state-graph scheduling state is updated.
 * @return Reserved active NPC pool slot, or NULL when the request is invalid or the fixed pool is full.
 */
UNpcPoolInstance *unsigned_npc_pool_reserve(UPoolInstanceContainer *pool, UNpc *npc);

/**
 * @brief Releases the selected NPC pool runtime slot for reuse.
 *
 * @param pool Fixed-capacity pool that owns the runtime slot.
 * @param instance Runtime pool slot to inspect or release.
 */
void unsigned_npc_pool_release(UPoolInstanceContainer *pool, UNpcPoolInstance *instance);

/**
 * @brief Advances the NPC pool by one scheduled engine frame.
 *
 * @param pool Fixed-capacity pool that owns the runtime slot.
 */
void unsigned_npc_pool_tick(UPoolInstanceContainer *pool);

#endif
