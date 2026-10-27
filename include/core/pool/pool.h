/**
 * @file pool.h
 * @brief Generic fixed-capacity reusable instance pool.
 */

#ifndef UNSIGNED_CORE_POOL_H
#define UNSIGNED_CORE_POOL_H

#include "core/types.h"

/**
 * Reusable physical slot shared by several engine pools.
 *
 * A pointer to a slot is not by itself a permanent handle: the same address can be released and
 * reserved for another object. `generation` changes on every reservation so callers that cache a
 * slot can detect this reuse before acting on stale state. Payload fields are meaningful only
 * while `active` is true; release does not clear them because the next owner overwrites the fields it uses.
 */
typedef struct UPoolInstance {
    /** Subsystem-defined lookup key; gameplay pools normally use the gameplay object's tag. */
    u16 key;
    /** Non-zero reuse counter incremented whenever this physical slot is reserved again. */
    u16 generation;
    /** Stable physical index in the backing array; lets higher-level pools address parallel metadata. */
    u8 index;
    /** Caller/subsystem-owned immutable definition bound to the active slot. */
    const void *object;
    /** Opaque runtime context owned by the caller/subsystem. */
    void *args;
    /** Frame counter used by timed gameplay pools; generic pool code does not advance it. */
    u16 elapsed;
    /** Duration contract used by timed gameplay pools; zero semantics are subsystem-specific. */
    u16 duration;
    bool active;
} UPoolInstance;

typedef struct UPoolInstanceContainer {
    u8 count;
    u8 capacity;
    /** One past the highest active physical slot; zero when empty. Bounds hot iterations without changing stable indices. */
    u8 active_span;
    /** Increments whenever active membership changes through reserve/release. */
    u16 revision;
    UPoolInstance *instances;
} UPoolInstanceContainer;

/**
 * @brief Returns the half-open range containing every active slot.
 *
 * The pool keeps stable physical indices, so releases may leave holes. `active_span` bounds iteration
 * to the highest live slot instead of scanning the full configured capacity.
 *
 * @pre `pool` was initialized with unsigned_pool_init() and has not been structurally modified by callers.
 */
static inline u8 unsigned_pool_iteration_end(const UPoolInstanceContainer *pool) {
    return pool->active_span;
}

/**
 * @brief Initializes the pool to a valid empty runtime state.
 *
 * @param pool Fixed-capacity pool that owns the runtime slot.
 * @param instances Caller-owned backing storage for exactly `capacity` slots.
 * @param capacity Number of reusable slots in `instances`.
 * @pre `pool` is valid; `instances` is valid when `capacity > 0`.
 */
void unsigned_pool_init(UPoolInstanceContainer *pool, UPoolInstance *instances, u8 capacity);

/**
 * @brief Reserves an inactive slot from the pool and binds it to the supplied runtime data.
 *
 * @param pool Fixed-capacity pool that owns the runtime slot.
 * @return Newly activated slot.
 * @pre `pool` is initialized, owns `capacity` backing slots and `pool->count < pool->capacity`.
 */
UPoolInstance *unsigned_pool_reserve(UPoolInstanceContainer *pool);

/**
 * @brief Reserves a slot and binds its opaque runtime context.
 *
 * @param pool Pool that owns the new slot.
 * @param args Opaque subsystem context stored on the active slot.
 * @return Newly activated slot with `args` already bound.
 * @pre `pool` satisfies the same reservation contract as unsigned_pool_reserve().
 */
static inline UPoolInstance *unsigned_pool_reserve_args(UPoolInstanceContainer *pool, void *args) {
    UPoolInstance *instance = unsigned_pool_reserve(pool);
    instance->args = args;
    return instance;
}

/**
 * @brief Releases an active slot so it can be reused by a later reservation.
 *
 * @param pool Pool that owns `instance`.
 * @param instance Active slot previously returned by unsigned_pool_reserve().
 * @pre `instance` belongs to `pool` and is still active.
 * @post `instance->active` is false; its subsystem payload must not be read until the slot is reserved again.
 */
void unsigned_pool_release(UPoolInstanceContainer *pool, UPoolInstance *instance);

/**
 * @brief Finds the active pool slot whose stable key matches the requested key.
 *
 * @param pool Fixed-capacity pool that owns the runtime slot.
 * @param key Stable pool key used to identify an active runtime slot.
 * @return Matching active slot, or NULL when the key is not active.
 * @pre `pool` is initialized and owns its backing array.
 */
UPoolInstance *unsigned_pool_find_by_key(UPoolInstanceContainer *pool, u16 key);

#endif
