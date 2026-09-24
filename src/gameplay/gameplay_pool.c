/**
 * @file gameplay_pool.c
 * @brief Implements the timed runtime pool shared by abilities, effects and cues.
 */

#include "gameplay/gameplay_pool.h"

UPoolInstance *unsigned_gameplay_pool_reserve(UPoolInstanceContainer *pool, const UGameplayObject *object, void *args) {
    UPoolInstance *instance = unsigned_pool_reserve(pool);

    instance->key = object->tag;
    instance->object = object;
    instance->args = args;
    instance->elapsed = 0u;
    instance->duration = object->duration;
    return instance;
}

void unsigned_gameplay_pool_activate(UPoolInstance *instance) {
    const UGameplayObject *object = instance->object;
    if (object->activate != NULL) {
        object->activate(instance->args);
    }
}

void unsigned_gameplay_pool_release(UPoolInstanceContainer *pool, UPoolInstance *instance) {
    const UGameplayObject *object = instance->object;
    void *args = instance->args;

    unsigned_pool_release(pool, instance);
    if (object->end != NULL) {
        object->end(args);
    }
}

/** Route one release through the optional owner hook or the default gameplay lifetime. */
static void gameplay_pool_release_instance(UPoolInstanceContainer *pool, UPoolInstance *instance, UGameplayPoolReleaseFunction release_function, void *release_context) {
    if (release_function != NULL) {
        release_function(release_context, instance);
    } else {
        unsigned_gameplay_pool_release(pool, instance);
    }
}

void unsigned_gameplay_pool_clear(UPoolInstanceContainer *pool, UGameplayPoolReleaseFunction release_function, void *release_context) {
    for (u8 i = 0u; i < unsigned_pool_iteration_end(pool); ++i) {
        UPoolInstance *instance = &pool->instances[i];
        if (!instance->active) {
            continue;
        }

        gameplay_pool_release_instance(pool, instance, release_function, release_context);
    }
}

void unsigned_gameplay_pool_tick(UPoolInstanceContainer *pool, UGameplayPoolReleaseFunction release_function, void *release_context) {
    for (u8 i = 0u; i < unsigned_pool_iteration_end(pool); ++i) {
        UPoolInstance *instance = &pool->instances[i];
        if (!instance->active) {
            continue;
        }

        const UGameplayObject *object = instance->object;
        void *args = instance->args;

        if (object->tick != NULL) {
            object->tick(args);
        }

        const bool completed = object->complete != NULL && object->complete(args);

        if (completed) {
            gameplay_pool_release_instance(pool, instance, release_function, release_context);
            continue;
        }

        ++instance->elapsed;
        if (instance->duration == 0u || instance->elapsed < instance->duration) {
            continue;
        }

        gameplay_pool_release_instance(pool, instance, release_function, release_context);
    }
}
