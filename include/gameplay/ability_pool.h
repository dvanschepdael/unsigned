/**
 * @file ability_pool.h
 * @brief Runtime pool for active gameplay abilities.
 */

#ifndef UNSIGNED_GAMEPLAY_ABILITY_POOL_H
#define UNSIGNED_GAMEPLAY_ABILITY_POOL_H

#include "gameplay/ability.h"
#include "gameplay/config.h"
#include "gameplay/gameplay_pool.h"

typedef UPoolInstance UAbilityPoolInstance;

typedef struct UAbilityPool {
    UPoolInstanceContainer pool;
    UAbilityPoolInstance instances[UNSIGNED_GAMEPLAY_MAX_ABILITY];
    const UGameplayAbility *abilities[UNSIGNED_GAMEPLAY_MAX_ABILITY];
    UGameplayTagContainer *tag_owners[UNSIGNED_GAMEPLAY_MAX_ABILITY];
} UAbilityPool;

/**
 * @brief Initializes the gameplay ability pool to a valid empty runtime state.
 *
 * @param abilities Runtime ability pool used to activate or release abilities.
 */
void unsigned_gameplay_ability_pool_init(UAbilityPool *abilities);

/**
 * @brief Clears the gameplay ability pool state without freeing caller-owned storage.
 *
 * @param abilities Runtime ability pool used to activate or release abilities.
 */
void unsigned_gameplay_ability_pool_clear(UAbilityPool *abilities);

/**
 * @brief Reserves an inactive slot from the gameplay ability pool and binds it to the supplied runtime data.
 *
 * @param abilities Runtime ability pool used to activate or release abilities.
 * @param ability Ability definition whose requirements or callbacks are evaluated.
 * @param tags Gameplay tag container used to evaluate or update tag state.
 * @param args Opaque caller arguments associated with the runtime instance.
 * @return Active ability slot.
 * @pre `abilities`, `ability` and `tags` are valid; the ability pool and granted-tag storage have sufficient authored capacity.
 * @pre The ability activation callback does not release/reuse the reserved slot.
 */
UAbilityPoolInstance *unsigned_gameplay_ability_pool_reserve(UAbilityPool *abilities, const UGameplayAbility *ability, UGameplayTagContainer *tags, void *args);

/**
 * @brief Releases the selected gameplay ability pool runtime slot for reuse.
 *
 * @param abilities Runtime ability pool used to activate or release abilities.
 * @param instance Runtime pool slot to inspect or release.
 */
void unsigned_gameplay_ability_pool_release(UAbilityPool *abilities, UAbilityPoolInstance *instance);

/** Release every active ability whose granted tags are owned by `owner`. */
void unsigned_gameplay_ability_pool_release_owner(UAbilityPool *abilities, UGameplayTagContainer *owner);

/**
 * Replace every active ability owned by `owner` with one new ability.
 * Existing owner abilities are released first; reservation then follows the normal fixed-pool contract.
 * @pre `abilities`, `owner` and `ability` are valid and tag storage has sufficient authored capacity.
 */
UAbilityPoolInstance *unsigned_gameplay_ability_pool_replace_owner(UAbilityPool *abilities, UGameplayTagContainer *owner, const UGameplayAbility *ability, void *args);

/**
 * @brief Advances the gameplay ability pool by one scheduled engine frame.
 *
 * @param abilities Runtime ability pool used to activate or release abilities.
 */
void unsigned_gameplay_ability_pool_tick(UAbilityPool *abilities);

#endif
