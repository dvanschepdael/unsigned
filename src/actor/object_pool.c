/**
 * @file object_pool.c
 * @brief Implements fixed-capacity world-object pool binding and ticking.
 */

#include "actor/object_pool.h"

UObjectPoolInstance *unsigned_object_pool_reserve(UPoolInstanceContainer *pool, UObject *object) {
    if (pool == NULL || object == NULL) {
        return NULL;
    }

    UObjectPoolInstance *instance = unsigned_pool_reserve(pool);

    if (instance == NULL) {
        return NULL;
    }

    object->actor.active = true;
    instance->args = object;
    instance->elapsed = 0;

    return instance;
}

void unsigned_object_pool_release(UPoolInstanceContainer *pool, UObjectPoolInstance *instance) {
    if (!unsigned_pool_owns(pool, instance) || !instance->active) {
        return;
    }

    UObject *object = instance->args;
    if (object != NULL) {
        unsigned_actor_destroy(&object->actor);
    }

    unsigned_pool_release(pool, instance);
}

void unsigned_object_pool_tick(UPoolInstanceContainer *pool) {
    if (pool == NULL || pool->count == 0u) {
        return;
    }

    for (u8 i = 0u; i < pool->capacity; ++i) {
        UObjectPoolInstance *instance = &pool->instances[i];

        if (!instance->active || instance->args == NULL) {
            continue;
        }

        UObject *object = instance->args;
        unsigned_actor_tick(&object->actor);
    }
}
