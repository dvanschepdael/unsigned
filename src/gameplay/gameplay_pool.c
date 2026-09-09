/**
 * @file gameplay_pool.c
 * @brief Implements generic timed runtime pool used by abilities, effects and cues.
 */

#include "gameplay/gameplay_pool.h"

/** Detects callback-driven release/reuse by verifying slot ownership, generation and bound object identity. */
static bool gameplay_pool_instance_is_current(const UPoolInstanceContainer *pool, const UPoolInstance *instance, u16 generation, const UGameplayObject *object, const void *args) {
    return unsigned_pool_owns(pool, instance) && instance->active && instance->generation == generation && instance->object == object && instance->args == args;
}

void unsigned_gameplay_pool_init(UPoolInstanceContainer *pool) {
    unsigned_pool_init(pool);
}

UPoolInstance *unsigned_gameplay_pool_reserve(UPoolInstanceContainer *pool, const UGameplayObject *object, void *args) {
    if (pool == NULL || pool->instances == NULL || pool->count >= pool->capacity || object == NULL) {
        return NULL;
    }

    UPoolInstance *instance = unsigned_pool_reserve(pool);
    if (instance == NULL) {
        return NULL;
    }

    instance->key = object->tag;
    instance->object = object;
    instance->args = args;
    instance->elapsed = 0u;
    instance->duration = object->duration;
    return instance;
}

bool unsigned_gameplay_pool_activate(const UPoolInstanceContainer *pool, UPoolInstance *instance) {
    if (!unsigned_pool_owns(pool, instance) || !instance->active || instance->object == NULL) {
        return false;
    }

    u16 generation = instance->generation;
    const UGameplayObject *object = instance->object;
    void *args = instance->args;

    if (object->activate != NULL) {
        object->activate(args);
    }

    return gameplay_pool_instance_is_current(pool, instance, generation, object, args);
}

void unsigned_gameplay_pool_cancel(UPoolInstanceContainer *pool, UPoolInstance *instance) {
    unsigned_pool_release(pool, instance);
}

void unsigned_gameplay_pool_release(UPoolInstanceContainer *pool, UPoolInstance *instance) {
    if (!unsigned_pool_owns(pool, instance) || !instance->active) {
        return;
    }

    const UGameplayObject *object = instance->object;
    void *args = instance->args;

    unsigned_pool_release(pool, instance);

    if (object != NULL && object->end != NULL) {
        object->end(args);
    }
}

void unsigned_gameplay_pool_tick(UPoolInstanceContainer *pool, UGameplayPoolReleaseFunction release_function, void *release_context) {
    if (pool == NULL || pool->instances == NULL || pool->count == 0u) {
        return;
    }

    for (u8 i = 0u; i < pool->capacity; ++i) {
        UPoolInstance *instance = &pool->instances[i];

        if (!instance->active || instance->object == NULL) {
            continue;
        }

        u16 generation = instance->generation;
        const UGameplayObject *object = instance->object;
        void *args = instance->args;

        if (object->tick != NULL) {
            object->tick(args);
        }

        if (!gameplay_pool_instance_is_current(pool, instance, generation, object, args)) {
            continue;
        }

        if (instance->elapsed < UINT16_MAX) {
            ++instance->elapsed;
        }

        if (instance->duration == 0u || instance->elapsed < instance->duration) {
            continue;
        }

        if (release_function != NULL) {
            release_function(release_context, instance);
        } else {
            unsigned_gameplay_pool_release(pool, instance);
        }
    }
}
