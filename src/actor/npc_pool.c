/**
 * @file npc_pool.c
 * @brief Implements fixed-capacity NPC pool binding and ticking.
 */

#include "actor/npc_pool.h"

UNpcPoolInstance *unsigned_npc_pool_reserve(UPoolInstanceContainer *pool, UNpc *npc) {
    if (pool == NULL || npc == NULL || npc->character == NULL) {
        return NULL;
    }

    UNpcPoolInstance *instance = unsigned_pool_reserve(pool);

    if (instance == NULL) {
        return NULL;
    }

    npc->character->actor.active = true;

    if (!unsigned_npc_init(npc)) {
        npc->character->actor.active = false;
        unsigned_pool_release(pool, instance);
        return NULL;
    }

    instance->args = npc;
    instance->elapsed = 0;

    return instance;
}

void unsigned_npc_pool_release(UPoolInstanceContainer *pool, UNpcPoolInstance *instance) {
    if (!unsigned_pool_owns(pool, instance) || !instance->active) {
        return;
    }

    UNpc *npc = instance->args;
    if (npc != NULL) {
        unsigned_npc_destroy(npc);
    }

    unsigned_pool_release(pool, instance);
}

void unsigned_npc_pool_tick(UPoolInstanceContainer *pool) {
    if (pool == NULL || pool->count == 0u) {
        return;
    }

    for (u8 i = 0u; i < pool->capacity; ++i) {
        UNpcPoolInstance *instance = &pool->instances[i];

        if (!instance->active || instance->args == NULL) {
            continue;
        }

        unsigned_npc_tick(instance->args);
    }
}
