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
 * @brief Resets a gameplay runtime pool while preserving its caller-provided capacity/backing array.
 *
 * @param pool Fixed-capacity pool whose runtime slots are reset.
 */
void unsigned_gameplay_pool_init(UPoolInstanceContainer *pool);

/**
 * @brief Reserves an inactive slot and binds it to a gameplay object and caller context.
 *
 * @param pool Fixed-capacity pool that owns the slot.
 * @param object Gameplay object definition whose duration/callbacks are bound to the slot.
 * @param args Opaque context forwarded to the gameplay object callbacks.
 * @return Reserved slot, or NULL when arguments are invalid or no capacity remains.
 */
UPoolInstance *unsigned_gameplay_pool_reserve(UPoolInstanceContainer *pool, const UGameplayObject *object, void *args);

/**
 * @brief Runs the object activation callback and verifies that the reserved slot is still current afterward.
 *
 * @param pool Pool that owns instance.
 * @param instance Reserved active slot to activate.
 * @return true when activation leaves the same slot generation/object active; false if validation fails or the callback released/reused the slot.
 */
bool unsigned_gameplay_pool_activate(const UPoolInstanceContainer *pool, UPoolInstance *instance);

/**
 * @brief Cancels a reserved gameplay slot without running the gameplay object end callback.
 *
 * @param pool Pool that owns instance.
 * @param instance Slot to return directly to the pool.
 */
void unsigned_gameplay_pool_cancel(UPoolInstanceContainer *pool, UPoolInstance *instance);

/**
 * @brief Releases an active gameplay slot and then invokes its optional end callback.
 *
 * @param pool Pool that owns instance.
 * @param instance Active slot to release; invalid/inactive handles are ignored.
 */
void unsigned_gameplay_pool_release(UPoolInstanceContainer *pool, UPoolInstance *instance);

/**
 * @brief Ticks every active gameplay object and expires finite-duration slots after their elapsed frame count reaches duration.
 *
 * @param pool Pool whose active runtime objects are advanced.
 * @param release_function Optional subsystem-specific release callback used for expiry cleanup such as tag removal.
 * @param release_context Opaque context passed to release_function.
 */
void unsigned_gameplay_pool_tick(UPoolInstanceContainer *pool, UGameplayPoolReleaseFunction release_function, void *release_context);

#endif
