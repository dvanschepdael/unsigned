/**
 * @file gameplay_pool.h
 * @brief Generic timed runtime pool used by abilities, effects and cues.
 */

#ifndef UNSIGNED_GAMEPLAY_POOL_H
#define UNSIGNED_GAMEPLAY_POOL_H

#include "core/pool/pool.h"
#include "gameplay/gameplay.h"

typedef void (*UGameplayPoolReleaseFunction)(void *context, UPoolInstance *instance);

/**
 * @brief Reserves an inactive slot and binds it to a gameplay object and caller context.
 *
 * @param pool Fixed-capacity pool that owns the slot.
 * @param object Gameplay object definition whose duration/callbacks are bound to the slot.
 * @param args Opaque context forwarded to the gameplay object callbacks.
 * @return Reserved slot.
 * @pre The fixed pool has free capacity.
 * @pre `pool` is initialized and `object` is a valid immutable gameplay definition.
 */
UPoolInstance *unsigned_gameplay_pool_reserve(UPoolInstanceContainer *pool, const UGameplayObject *object, void *args);

/**
 * @brief Runs the optional activation callback for an already reserved gameplay slot.
 *
 * @param instance Reserved active slot to activate.
 * @pre `instance` is active and bound to a valid UGameplayObject.
 * @pre The activation callback does not release or reuse `instance`; lifecycle mutation belongs to tick/end processing.
 */
void unsigned_gameplay_pool_activate(UPoolInstance *instance);

/**
 * @brief Releases an active gameplay slot and then invokes its optional end callback.
 *
 * @param pool Pool that owns instance.
 * @param instance Active slot to release.
 * @pre `instance` belongs to `pool` and is still active.
 */
void unsigned_gameplay_pool_release(UPoolInstanceContainer *pool, UPoolInstance *instance);

/**
 * @brief Releases every active gameplay slot using the subsystem cleanup policy.
 *
 * @param pool Pool whose active slots are cleared.
 * @param release_function Optional subsystem-specific release callback; NULL uses the generic gameplay release path.
 * @param release_context Opaque context passed to release_function.
 * @pre `pool` is initialized and each active slot is bound to a valid gameplay object.
 */
void unsigned_gameplay_pool_clear(UPoolInstanceContainer *pool, UGameplayPoolReleaseFunction release_function, void *release_context);

/**
 * @brief Ticks every active gameplay object and expires finite-duration slots after their elapsed frame count reaches duration.
 *
 * @param pool Pool whose active runtime objects are advanced.
 * @param release_function Optional subsystem-specific release callback used for expiry cleanup such as tag removal.
 * @param release_context Opaque context passed to release_function.
 * @pre Object `tick` and `complete` callbacks do not release or reuse their currently executing pool slot.
 */
void unsigned_gameplay_pool_tick(UPoolInstanceContainer *pool, UGameplayPoolReleaseFunction release_function, void *release_context);

#endif
