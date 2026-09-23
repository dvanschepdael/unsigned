/**
 * @file object_pool.c
 * @brief Implements fixed-capacity world-object pool binding and ticking.
 */

#include "actor/object_pool.h"

UObjectPoolInstance *unsigned_object_pool_reserve(UPoolInstanceContainer *pool, UObject *object) {
    UObjectPoolInstance *instance = unsigned_pool_reserve(pool);

    instance->args = object;
    instance->elapsed = 0;

    return instance;
}

void unsigned_object_pool_release(UPoolInstanceContainer *pool, UObjectPoolInstance *instance) {
    UObject *object = instance->args;
    unsigned_actor_destroy(&object->actor);

    unsigned_pool_release(pool, instance);
}

void unsigned_object_pool_tick(UPoolInstanceContainer *pool) {
    if (pool->count == 0u) {
        return;
    }

    u8 remaining = pool->count;
    for (u8 i = 0u; i < unsigned_pool_iteration_end(pool) && remaining > 0u; ++i) {
        UObjectPoolInstance *instance = &pool->instances[i];

        if (!instance->active) {
            continue;
        }
        --remaining;
        UObject *object = instance->args;
        unsigned_actor_tick(&object->actor);
    }
}
