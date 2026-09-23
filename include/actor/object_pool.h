/**
 * @file object_pool.h
 * @brief Fixed-capacity world-object pool binding and ticking.
 */

#ifndef UNSIGNED_ACTOR_OBJECT_POOL_H
#define UNSIGNED_ACTOR_OBJECT_POOL_H

#include "actor/object.h"
#include "core/pool/pool.h"

typedef UPoolInstance UObjectPoolInstance;

/**
 * @brief Reserves an inactive slot from the object pool and binds it to the supplied runtime data.
 *
 * @param pool Fixed-capacity pool that owns the runtime slot.
 * @param object World object bound to the requested runtime slot.
 * @return Reserved active object pool slot.
 * @pre The pool has free capacity.
 * @pre `pool` and `object` are valid; every active slot keeps a valid object binding.
 */
UObjectPoolInstance *unsigned_object_pool_reserve(UPoolInstanceContainer *pool, UObject *object);

/**
 * @brief Releases the selected object pool runtime slot for reuse.
 *
 * @param pool Fixed-capacity pool that owns the runtime slot.
 * @param instance Active runtime pool slot to release.
 * @pre `pool`, `instance` and the instance object binding are valid.
 */
void unsigned_object_pool_release(UPoolInstanceContainer *pool, UObjectPoolInstance *instance);

/**
 * @brief Advances the object pool by one scheduled engine frame.
 *
 * @param pool Fixed-capacity pool that owns the runtime slot.
 */
void unsigned_object_pool_tick(UPoolInstanceContainer *pool);

#endif
