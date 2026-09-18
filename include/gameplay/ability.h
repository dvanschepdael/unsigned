/**
 * @file ability.h
 * @brief Ability activation requirements, tagged catalogs and definition data.
 *
 * `blocked_by_tags` gates activation. `granted_tags` are reference-counted into the owner while
 * the reserved runtime ability is active and removed again on release.
 */

#ifndef UNSIGNED_GAMEPLAY_ABILITY_H
#define UNSIGNED_GAMEPLAY_ABILITY_H

#include "gameplay/gameplay.h"
#include "gameplay/tag.h"

typedef struct UGameplayAbility {
    UGameplayObject base;
    UGameplayTagContainer granted_tags;
    UGameplayTagContainer blocked_by_tags;
} UGameplayAbility;

/**
 * Lightweight view over caller-owned ability definitions.
 *
 * The container stores pointers instead of copying complete ability definitions. This keeps
 * character catalogs cheap in RAM and lets abilities stay defined beside their own behavior.
 * Ability tags are the public identity used to resolve one definition from the catalog.
 */
typedef struct UGameplayAbilityContainer {
    u8 count;
    u8 capacity;
    const UGameplayAbility *const *instances;
} UGameplayAbilityContainer;

/**
 * @brief Finds an ability in a character catalog by gameplay tag.
 *
 * Catalogs are intentionally small fixed collections, so a linear lookup keeps the API simple
 * and avoids lookup tables or dynamic allocation.
 *
 * @param abilities Ability catalog to search.
 * @param tag Non-zero ability tag to resolve.
 * @return Matching ability definition, or NULL when the tag/catalog is invalid or absent.
 */
const UGameplayAbility *unsigned_ability_find(const UGameplayAbilityContainer *abilities, UGameplayTag tag);

/**
 * @brief Checks whether the character tags satisfy an ability's blocked tag rules.
 *
 * @param tags Gameplay tag container used to evaluate or update tag state.
 * @param ability Ability definition whose requirements or callbacks are evaluated.
 * @return true when no active owner tag appears in `blocked_by_tags`; false for invalid inputs or a blocker match.
 */
bool unsigned_ability_can_activate(const UGameplayTagContainer *tags, const UGameplayAbility *ability);

#endif
