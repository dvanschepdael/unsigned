/**
 * @file projectile_pool.h
 * @brief Fixed-capacity projectile pool binding and ticking.
 */

#ifndef UNSIGNED_ACTOR_PROJECTILE_POOL_H
#define UNSIGNED_ACTOR_PROJECTILE_POOL_H

#include "actor/projectile.h"
#include "core/pool/pool.h"

typedef UPoolInstance UProjectilePoolInstance;

/**
 * @brief Reserves an inactive slot from the projectile pool and binds it to the supplied runtime data.
 *
 * @param pool Fixed-capacity pool that owns the runtime slot.
 * @param projectile Projectile runtime entity to query or update.
 * @return Reserved active projectile pool slot, or NULL when the request is invalid or the fixed pool is full.
 */
UProjectilePoolInstance *unsigned_projectile_pool_reserve(UPoolInstanceContainer *pool, UProjectile *projectile);

/**
 * @brief Releases the selected projectile pool runtime slot for reuse.
 *
 * @param pool Fixed-capacity pool that owns the runtime slot.
 * @param instance Runtime pool slot to inspect or release.
 */
void unsigned_projectile_pool_release(UPoolInstanceContainer *pool, UProjectilePoolInstance *instance);

/**
 * @brief Advances the projectile pool by one scheduled engine frame.
 *
 * @param pool Fixed-capacity pool that owns the runtime slot.
 */
void unsigned_projectile_pool_tick(UPoolInstanceContainer *pool);

#endif
