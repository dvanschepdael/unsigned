/**
 * @file timer_pool.c
 * @brief Implements frame-based fixed-capacity timer runtime.
 */

#include "core/timer/timer_pool.h"

/** Validates that a timer handle still belongs to this pool and references an active generation. */
static bool timer_pool_instance_is_active(const UTimerPool *timers, const UTimerPoolInstance *instance) {
    return timers != NULL && unsigned_pool_owns(&timers->pool, instance) && instance->active;
}

bool unsigned_timer_pool_init(UTimerPool *timers, u8 frames_per_second) {
    if (timers == NULL || frames_per_second == 0u) {
        return false;
    }

    *timers = (UTimerPool){
        .frames_per_second = frames_per_second,
    };
    timers->pool.capacity = UNSIGNED_GAME_MAX_TIMER;
    timers->pool.instances = timers->instances;
    unsigned_pool_init(&timers->pool);
    return true;
}

void unsigned_timer_pool_clear(UTimerPool *timers) {
    if (timers == NULL) {
        return;
    }

    for (u8 i = 0u; i < timers->pool.capacity; ++i) {
        if (timers->pool.instances[i].active) {
            unsigned_timer_pool_release(timers, &timers->pool.instances[i]);
        }
    }
}

UTimerPoolInstance *unsigned_timer_pool_reserve(UTimerPool *timers, const UTimer *timer, void *args) {
    if (timers == NULL || timer == NULL || timer->duration_frames == 0u) {
        return NULL;
    }

    UTimerPoolInstance *instance = unsigned_pool_reserve(&timers->pool);
    if (instance == NULL) {
        return NULL;
    }

    instance->object = timer;
    instance->args = args;
    instance->elapsed = 0u;
    instance->duration = timer->duration_frames;

    return instance;
}

void unsigned_timer_pool_release(UTimerPool *timers, UTimerPoolInstance *instance) {
    if (timers == NULL || !unsigned_pool_owns(&timers->pool, instance) || !instance->active) {
        return;
    }

    unsigned_pool_release(&timers->pool, instance);
}

void unsigned_timer_pool_tick(UTimerPool *timers) {
    if (timers == NULL || timers->pool.count == 0u || timers->frames_per_second == 0u) {
        return;
    }

    for (u8 i = 0u; i < timers->pool.capacity; ++i) {
        UTimerPoolInstance *instance = &timers->pool.instances[i];

        if (!instance->active) {
            continue;
        }

        const UTimer *timer = instance->object;
        if (timer == NULL) {
            unsigned_timer_pool_release(timers, instance);
            continue;
        }

        if (instance->elapsed < UINT16_MAX) {
            ++instance->elapsed;
        }

        if (instance->duration == 0u || instance->elapsed >= instance->duration) {
            UCallbackFunc callback = timer->callback;
            void *args = instance->args;

            unsigned_timer_pool_release(timers, instance);
            if (callback != NULL) {
                callback(args);
            }
        }
    }
}

u16 unsigned_timer_pool_elapsed_frames(const UTimerPool *timers, const UTimerPoolInstance *instance) {
    return timer_pool_instance_is_active(timers, instance) ? instance->elapsed : 0u;
}

u16 unsigned_timer_pool_elapsed_seconds(const UTimerPool *timers, const UTimerPoolInstance *instance) {
    if (!timer_pool_instance_is_active(timers, instance) || timers->frames_per_second == 0u) {
        return 0u;
    }

    return (u16)(instance->elapsed / timers->frames_per_second);
}

u16 unsigned_timer_pool_remaining_frames(const UTimerPool *timers, const UTimerPoolInstance *instance) {
    if (!timer_pool_instance_is_active(timers, instance) || instance->elapsed >= instance->duration) {
        return 0u;
    }

    return (u16)(instance->duration - instance->elapsed);
}

u16 unsigned_timer_pool_remaining_seconds(const UTimerPool *timers, const UTimerPoolInstance *instance) {
    if (timers == NULL || timers->frames_per_second == 0u) {
        return 0u;
    }

    u16 remaining_frames = unsigned_timer_pool_remaining_frames(timers, instance);
    if (remaining_frames == 0u) {
        return 0u;
    }

    return (u16)((remaining_frames + timers->frames_per_second - 1u) / timers->frames_per_second);
}
