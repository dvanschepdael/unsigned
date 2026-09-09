/**
 * @file ability.h
 * @brief Ability activation requirements and definition data.
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

typedef struct UGameplayAbilityContainer {
    u8 count;
    u8 capacity;
    const UGameplayAbility *instances;
} UGameplayAbilityContainer;

/**
 * @brief Checks whether the character tags satisfy an ability's required and blocked tag rules.
 *
 * @param tags Gameplay tag container used to evaluate or update tag state.
 * @param ability Ability definition whose requirements or callbacks are evaluated.
 * @return true when no active owner tag appears in `blocked_by_tags`; false for invalid inputs or a blocker match.
 */
bool unsigned_ability_can_activate(const UGameplayTagContainer *tags, const UGameplayAbility *ability);

#endif
