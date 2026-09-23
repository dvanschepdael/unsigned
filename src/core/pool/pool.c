/**
 * @file pool.c
 * @brief Implements generic fixed-capacity reusable instance pools.
 */

#include "core/pool/pool.h"

void unsigned_pool_init(UPoolInstanceContainer *pool) {
    pool->count = 0u;
    pool->active_span = 0u;
    pool->revision = 0u;

    for (u8 i = 0u; i < pool->capacity; ++i) {
        pool->instances[i] = (UPoolInstance){
            .index = i,
        };
    }
}

UPoolInstance *unsigned_pool_reserve(UPoolInstanceContainer *pool) {
    u8 index = pool->active_span;

    /* Stable physical indices may leave holes. Search only the live span, then append the first
     * trailing slot. This avoids scanning the unused tail of a mostly dense pool. */
    for (u8 i = 0u; i < pool->active_span; ++i) {
        if (!pool->instances[i].active) {
            index = i;
            break;
        }
    }

    if (index == pool->active_span) {
        ++pool->active_span;
    }
    UPoolInstance *instance = &pool->instances[index];

    ++instance->generation;
    if (instance->generation == 0u) {
        instance->generation = 1u;
    }
    instance->active = true;
    ++pool->count;
    ++pool->revision;
    return instance;
}

void unsigned_pool_release(UPoolInstanceContainer *pool, UPoolInstance *instance) {
    instance->active = false;
    --pool->count;

    if ((u16)instance->index + 1u == pool->active_span) {
        while (pool->active_span > 0u && !pool->instances[pool->active_span - 1u].active) {
            --pool->active_span;
        }
    }
    ++pool->revision;
}

UPoolInstance *unsigned_pool_find_by_key(UPoolInstanceContainer *pool, u16 key) {
    if (pool->count == 0u) {
        return NULL;
    }

    for (u8 i = 0u; i < unsigned_pool_iteration_end(pool); ++i) {
        UPoolInstance *instance = &pool->instances[i];
        if (instance->active && instance->key == key) {
            return instance;
        }
    }

    return NULL;
}
