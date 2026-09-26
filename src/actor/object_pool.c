/**
 * @file object_pool.c
 * @brief Implements fixed-capacity world-object pool binding and ticking.
 */

#include "actor/object_pool.h"

UObjectPoolInstance *unsigned_object_pool_reserve(UPoolInstanceContainer *pool, UObject *object) {
    return unsigned_pool_reserve_args(pool, object);
}

void unsigned_object_pool_release(UPoolInstanceContainer *pool, UObjectPoolInstance *instance) {
    UObject *object = instance->args;
    unsigned_actor_destroy(&object->actor);

    unsigned_pool_release(pool, instance);
}

void unsigned_object_pool_tick(UPoolInstanceContainer *pool) {
    for (u8 i = 0u; i < unsigned_pool_iteration_end(pool); ++i) {
        UObjectPoolInstance *instance = &pool->instances[i];

        if (!instance->active) {
            continue;
        }
        UObject *object = instance->args;
        unsigned_actor_tick(&object->actor);
    }
}
