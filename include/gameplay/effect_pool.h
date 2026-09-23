/**
 * @file effect_pool.h
 * @brief Runtime pool for active gameplay effects.
 */

#ifndef UNSIGNED_GAMEPLAY_EFFECT_POOL_H
#define UNSIGNED_GAMEPLAY_EFFECT_POOL_H

#include "gameplay/config.h"
#include "gameplay/effect.h"
#include "gameplay/gameplay_pool.h"

typedef UPoolInstance UEffectPoolInstance;

typedef struct UEffectPool {
    UPoolInstanceContainer pool;
    UEffectPoolInstance instances[UNSIGNED_GAMEPLAY_MAX_EFFECT];
    const UGameplayEffect *effects[UNSIGNED_GAMEPLAY_MAX_EFFECT];
    UGameplayTagContainer *tag_owners[UNSIGNED_GAMEPLAY_MAX_EFFECT];
} UEffectPool;

/**
 * @brief Initializes the gameplay effect pool to a valid empty runtime state.
 *
 * @param effects Gameplay effect runtime pool to initialize, reserve, release or tick.
 */
void unsigned_gameplay_effect_pool_init(UEffectPool *effects);

/**
 * @brief Clears the gameplay effect pool state without freeing caller-owned storage.
 *
 * @param effects Gameplay effect runtime pool to initialize, reserve, release or tick.
 */
void unsigned_gameplay_effect_pool_clear(UEffectPool *effects);

/**
 * @brief Applies an effect as either an immediate change or a tracked runtime instance.
 *
 * @param effects Runtime pool used when the effect has duration or is infinite.
 * @param effect Effect definition whose duration, callbacks and granted tags are applied.
 * @param tags Optional tag container that receives/removes the effect's granted tags.
 * @param args Opaque caller context forwarded to effect callbacks.
 * @return Active runtime instance for duration/infinite effects, or NULL when an instant effect completed synchronously.
 * @pre `effects` and `effect` are valid, and the effect pool has sufficient authored capacity. `tags` may be NULL for effects that do not own gameplay tags.
 * @pre `effect->duration_type` is a valid `UGameplayEffectDurationType`; `U_GAMEPLAY_EFFECT_DURATION` effects have `base.duration > 0`.
 * @pre A tracked effect activation callback does not release/reuse the reserved slot.
 */
UEffectPoolInstance *unsigned_gameplay_effect_apply(UEffectPool *effects, const UGameplayEffect *effect, UGameplayTagContainer *tags, void *args);

/**
 * @brief Releases the selected gameplay effect pool runtime slot for reuse.
 *
 * @param effects Gameplay effect runtime pool to initialize, reserve, release or tick.
 * @param instance Runtime pool slot to inspect or release.
 */
void unsigned_gameplay_effect_pool_release(UEffectPool *effects, UEffectPoolInstance *instance);

/**
 * @brief Advances the gameplay effect pool by one scheduled engine frame.
 *
 * @param effects Gameplay effect runtime pool to initialize, reserve, release or tick.
 */
void unsigned_gameplay_effect_pool_tick(UEffectPool *effects);

#endif
