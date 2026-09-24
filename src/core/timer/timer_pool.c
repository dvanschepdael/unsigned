/**
 * @file timer_pool.c
 * @brief Implements frame-based fixed-capacity timer runtime.
 */

#include "core/timer/timer_pool.h"

void unsigned_timer_pool_init(UTimerPool *timers, u8 frames_per_second) {
    *timers = (UTimerPool){
        .frames_per_second = frames_per_second,
    };
    timers->pool.capacity = UNSIGNED_GAME_MAX_TIMER;
    timers->pool.instances = timers->instances;
    unsigned_pool_init(&timers->pool);
}

void unsigned_timer_pool_clear(UTimerPool *timers) {
    for (u8 i = 0u; i < unsigned_pool_iteration_end(&timers->pool); ++i) {
        if (timers->pool.instances[i].active) {
            unsigned_pool_release(&timers->pool, &timers->pool.instances[i]);
        }
    }
}

UTimerPoolInstance *unsigned_timer_pool_reserve(UTimerPool *timers, const UTimer *timer, void *args) {
    UTimerPoolInstance *instance = unsigned_pool_reserve(&timers->pool);

    instance->object = timer;
    instance->args = args;
    instance->elapsed = 0u;
    instance->duration = timer->duration_frames;
    return instance;
}

void unsigned_timer_pool_release(UTimerPool *timers, UTimerPoolInstance *instance) {
    unsigned_pool_release(&timers->pool, instance);
}

void unsigned_timer_pool_tick(UTimerPool *timers) {
    for (u8 i = 0u; i < unsigned_pool_iteration_end(&timers->pool); ++i) {
        UTimerPoolInstance *instance = &timers->pool.instances[i];
        if (!instance->active) {
            continue;
        }

        const UTimer *timer = instance->object;
        ++instance->elapsed;

        if (instance->elapsed >= instance->duration) {
            UCallbackFunc callback = timer->callback;
            void *args = instance->args;

            unsigned_pool_release(&timers->pool, instance);
            if (callback != NULL) {
                callback(args);
            }
        }
    }
}

u16 unsigned_timer_pool_elapsed_frames(const UTimerPoolInstance *instance) {
    return instance->elapsed;
}

u16 unsigned_timer_pool_elapsed_seconds(const UTimerPool *timers, const UTimerPoolInstance *instance) {
    return (u16)(instance->elapsed / timers->frames_per_second);
}

u16 unsigned_timer_pool_remaining_frames(const UTimerPoolInstance *instance) {
    if (instance->elapsed >= instance->duration) {
        return 0u;
    }
    return (u16)(instance->duration - instance->elapsed);
}

u16 unsigned_timer_pool_remaining_seconds(const UTimerPool *timers, const UTimerPoolInstance *instance) {
    u16 remaining_frames = unsigned_timer_pool_remaining_frames(instance);
    if (remaining_frames == 0u) {
        return 0u;
    }

    return (u16)((remaining_frames + timers->frames_per_second - 1u) / timers->frames_per_second);
}
