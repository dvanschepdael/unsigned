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
 * slot can detect this reuse before acting on stale state.
 */
typedef struct UPoolInstance {
    /** Subsystem-defined lookup key; gameplay pools normally use the gameplay object's tag. */
    u16 key;
    /** Non-zero reuse counter incremented whenever this physical slot is reserved again. */
    u16 generation;
    /** Stable physical index in the backing array; used to validate pool ownership cheaply. */
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
    UPoolInstance *instances;
} UPoolInstanceContainer;

/**
 * @brief Initializes the pool to a valid empty runtime state.
 *
 * @param pool Fixed-capacity pool that owns the runtime slot.
 */
void unsigned_pool_init(UPoolInstanceContainer *pool);

/**
 * @brief Reserves an inactive slot from the pool and binds it to the supplied runtime data.
 *
 * @param pool Fixed-capacity pool that owns the runtime slot.
 * @return Newly activated slot, or NULL when the pool/input is invalid or no free slot remains.
 */
UPoolInstance *unsigned_pool_reserve(UPoolInstanceContainer *pool);

/**
 * @brief Releases the selected pool runtime slot for reuse.
 *
 * @param pool Fixed-capacity pool that owns the runtime slot.
 * @param instance Runtime pool slot to inspect or release.
 */
void unsigned_pool_release(UPoolInstanceContainer *pool, UPoolInstance *instance);

/**
 * @brief Returns whether the supplied runtime slot belongs to this pool backing array.
 *
 * @param pool Fixed-capacity pool that owns the runtime slot.
 * @param instance Runtime pool slot to inspect or release.
 * @return true only when `instance` points to the slot identified by its own `index` inside this pool.
 */
bool unsigned_pool_owns(const UPoolInstanceContainer *pool, const UPoolInstance *instance);

/**
 * @brief Finds the active pool slot whose stable key matches the requested key.
 *
 * @param pool Fixed-capacity pool that owns the runtime slot.
 * @param key Stable pool key used to identify an active runtime slot.
 * @return Matching active slot, or NULL when the key is not active or the pool is invalid.
 */
UPoolInstance *unsigned_pool_find_by_key(UPoolInstanceContainer *pool, u16 key);

/**
 * @brief Finds the active pool slot whose stable key matches the requested key without exposing mutable access.
 *
 * @param pool Fixed-capacity pool that owns the runtime slot.
 * @param key Stable pool key used to identify an active runtime slot.
 * @return Matching active slot, or NULL when the key is not active or the pool is invalid.
 */
const UPoolInstance *unsigned_pool_find_by_key_const(const UPoolInstanceContainer *pool, u16 key);

/**
 * @brief Returns the pool slot at the requested physical index, including inactive slots.
 *
 * @param pool Fixed-capacity pool that owns the runtime slot.
 * @param index Zero-based index of the requested entry.
 * @return Physical slot at `index` even when inactive, or NULL when the pool/index is invalid.
 */
UPoolInstance *unsigned_pool_get(UPoolInstanceContainer *pool, u8 index);

/**
 * @brief Returns the pool slot at the requested physical index without exposing mutable access.
 *
 * @param pool Fixed-capacity pool that owns the runtime slot.
 * @param index Zero-based index of the requested entry.
 * @return Physical slot at `index` even when inactive, or NULL when the pool/index is invalid.
 */
const UPoolInstance *unsigned_pool_get_const(const UPoolInstanceContainer *pool, u8 index);

#endif
