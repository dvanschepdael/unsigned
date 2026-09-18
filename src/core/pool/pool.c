/**
 * @file pool.c
 * @brief Implements generic fixed-capacity reusable instance pool.
 */

#include "core/pool/pool.h"

bool unsigned_pool_owns(const UPoolInstanceContainer *pool, const UPoolInstance *instance) {
    if (pool == NULL || pool->instances == NULL || instance == NULL || instance->index >= pool->capacity) {
        return false;
    }

    return &pool->instances[instance->index] == instance;
}

void unsigned_pool_init(UPoolInstanceContainer *pool) {
    if (pool == NULL) {
        return;
    }

    pool->count = 0u;
    if (pool->capacity == 0u || pool->instances == NULL) {
        return;
    }

    for (u8 i = 0u; i < pool->capacity; ++i) {
        pool->instances[i] = (UPoolInstance){
            .index = i,
        };
    }
}

UPoolInstance *unsigned_pool_reserve(UPoolInstanceContainer *pool) {
    if (pool == NULL || pool->instances == NULL || pool->count >= pool->capacity) {
        return NULL;
    }

    for (u8 i = 0u; i < pool->capacity; ++i) {
        UPoolInstance *instance = &pool->instances[i];

        if (!instance->active) {
            ++instance->generation;
            if (instance->generation == 0u) {
                instance->generation = 1u;
            }
            instance->active = true;
            ++pool->count;
            return instance;
        }
    }

    return NULL;
}

void unsigned_pool_release(UPoolInstanceContainer *pool, UPoolInstance *instance) {
    if (!unsigned_pool_owns(pool, instance) || !instance->active) {
        return;
    }

    instance->key = 0u;
    instance->args = NULL;
    instance->object = NULL;
    instance->active = false;
    instance->elapsed = 0u;
    instance->duration = 0u;

    if (pool->count > 0u) {
        --pool->count;
    }
}

UPoolInstance *unsigned_pool_find_by_key(UPoolInstanceContainer *pool, u16 key) {
    if (pool == NULL || pool->instances == NULL || pool->count == 0u) {
        return NULL;
    }

    for (u8 i = 0u; i < pool->capacity; ++i) {
        UPoolInstance *instance = &pool->instances[i];
        if (instance->active && instance->key == key) {
            return instance;
        }
    }

    return NULL;
}

const UPoolInstance *unsigned_pool_find_by_key_const(const UPoolInstanceContainer *pool, u16 key) {
    if (pool == NULL || pool->instances == NULL || pool->count == 0u) {
        return NULL;
    }

    for (u8 i = 0u; i < pool->capacity; ++i) {
        const UPoolInstance *instance = &pool->instances[i];
        if (instance->active && instance->key == key) {
            return instance;
        }
    }

    return NULL;
}

UPoolInstance *unsigned_pool_get(UPoolInstanceContainer *pool, u8 index) {
    if (pool == NULL || pool->instances == NULL || index >= pool->capacity) {
        return NULL;
    }

    return &pool->instances[index];
}

const UPoolInstance *unsigned_pool_get_const(const UPoolInstanceContainer *pool, u8 index) {
    if (pool == NULL || pool->instances == NULL || index >= pool->capacity) {
        return NULL;
    }

    return &pool->instances[index];
}
