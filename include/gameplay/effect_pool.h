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

typedef enum UGameplayEffectApplyResult {
    U_GAMEPLAY_EFFECT_APPLY_FAILED = 0,
    U_GAMEPLAY_EFFECT_APPLY_INSTANT,
    U_GAMEPLAY_EFFECT_APPLY_ACTIVE,
} UGameplayEffectApplyResult;

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
 * @param effects Runtime pool used when the effect has duration or is infinite; must not be NULL.
 * @param effect Effect definition whose duration, callbacks and granted tags are applied; must not be NULL.
 * @param tags Optional tag container that receives/removes the effect's granted tags.
 * @param args Opaque caller context forwarded to effect callbacks.
 * @param out_instance Optional output receiving the active runtime instance; set to NULL for instant or failed application.
 * @return `U_GAMEPLAY_EFFECT_APPLY_FAILED` on validation/capacity/tag failure, `U_GAMEPLAY_EFFECT_APPLY_INSTANT` when executed immediately, or `U_GAMEPLAY_EFFECT_APPLY_ACTIVE`
 * when a runtime instance was activated.
 */
UGameplayEffectApplyResult unsigned_gameplay_effect_apply(UEffectPool *effects, const UGameplayEffect *effect, UGameplayTagContainer *tags, void *args, UEffectPoolInstance **out_instance);

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
