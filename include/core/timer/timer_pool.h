/**
 * @file timer_pool.h
 * @brief Frame-based fixed-capacity timer runtime.
 */

#ifndef UNSIGNED_CORE_TIMER_POOL_H
#define UNSIGNED_CORE_TIMER_POOL_H

#include "core/pool/pool.h"
#include "core/timer/config.h"
#include "core/timer/timer.h"

typedef UPoolInstance UTimerPoolInstance;

typedef struct UTimerPool {
    UPoolInstanceContainer pool;
    UTimerPoolInstance instances[UNSIGNED_GAME_MAX_TIMER];
    u8 frames_per_second;
} UTimerPool;

/**
 * @brief Initializes the fixed timer pool and the refresh rate used for frame/second conversion.
 *
 * @param timers Timer pool to initialize.
 * @param frames_per_second Non-zero video refresh rate, typically 50 or 60.
 * @return true when timers and frames_per_second are valid; false otherwise.
 */
bool unsigned_timer_pool_init(UTimerPool *timers, u8 frames_per_second);

/**
 * @brief Releases every active timer slot without changing the pool capacity or configured refresh rate.
 *
 * @param timers Timer pool to clear; NULL is ignored.
 */
void unsigned_timer_pool_clear(UTimerPool *timers);

/**
 * @brief Reserves a timer slot and binds a timer definition plus opaque callback argument.
 *
 * @param timers Timer pool that owns the new slot.
 * @param timer Timer definition; duration_frames must be non-zero.
 * @param args Opaque argument passed to timer->callback on expiry.
 * @return Reserved timer handle, or NULL for invalid input or exhausted pool capacity.
 */
UTimerPoolInstance *unsigned_timer_pool_reserve(UTimerPool *timers, const UTimer *timer, void *args);

/**
 * @brief Releases an active timer handle without firing its callback.
 *
 * @param timers Timer pool that owns instance.
 * @param instance Timer handle to cancel; invalid/inactive handles are ignored.
 */
void unsigned_timer_pool_release(UTimerPool *timers, UTimerPoolInstance *instance);

/**
 * @brief Advances active timers by one frame, releasing expired slots before invoking their callbacks.
 *
 * @param timers Timer pool to advance once per engine frame.
 */
void unsigned_timer_pool_tick(UTimerPool *timers);

/**
 * @brief Returns the number of frames elapsed for an active timer handle.
 *
 * @param timers Timer pool that owns instance.
 * @param instance Timer handle to query.
 * @return Elapsed frame count, or 0 when the handle is invalid or inactive.
 */
u16 unsigned_timer_pool_elapsed_frames(const UTimerPool *timers, const UTimerPoolInstance *instance);

/**
 * @brief Returns elapsed whole seconds using the timer pool configured refresh rate.
 *
 * @param timers Timer pool that owns instance.
 * @param instance Timer handle to query.
 * @return Elapsed whole seconds, or 0 when the handle/pool is invalid.
 */
u16 unsigned_timer_pool_elapsed_seconds(const UTimerPool *timers, const UTimerPoolInstance *instance);

/**
 * @brief Returns the remaining frame count before an active timer expires.
 *
 * @param timers Timer pool that owns instance.
 * @param instance Timer handle to query.
 * @return Remaining frames, or 0 when the handle is invalid, inactive or already complete.
 */
u16 unsigned_timer_pool_remaining_frames(const UTimerPool *timers, const UTimerPoolInstance *instance);

/**
 * @brief Returns remaining seconds rounded up so any partial second still reports time remaining.
 *
 * @param timers Timer pool that owns instance.
 * @param instance Timer handle to query.
 * @return Ceiling of remaining_frames / frames_per_second, or 0 for invalid/complete timers.
 */
u16 unsigned_timer_pool_remaining_seconds(const UTimerPool *timers, const UTimerPoolInstance *instance);

#endif
