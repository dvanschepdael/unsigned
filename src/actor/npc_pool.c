/**
 * @file npc_pool.c
 * @brief Implements fixed-capacity NPC pool binding and ticking.
 */

#include "actor/npc_pool.h"

UNpcPoolInstance *unsigned_npc_pool_reserve(UPoolInstanceContainer *pool, UNpc *npc) {
    UNpcPoolInstance *instance = unsigned_pool_reserve_args(pool, npc);

    unsigned_npc_init(npc);
    return instance;
}

void unsigned_npc_pool_release(UPoolInstanceContainer *pool, UNpcPoolInstance *instance) {
    UNpc *npc = instance->args;
    unsigned_npc_destroy(npc);
    unsigned_pool_release(pool, instance);
}

void unsigned_npc_pool_tick(UPoolInstanceContainer *pool, const UTLSS *tlss) {
    for (u8 i = 0u; i < unsigned_pool_iteration_end(pool); ++i) {
        UNpcPoolInstance *instance = &pool->instances[i];
        if (!instance->active) {
            continue;
        }

        unsigned_npc_tick_scheduled(instance->args, tlss, i);
    }
}
