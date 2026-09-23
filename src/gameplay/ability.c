/**
 * @file ability.c
 * @brief Implements tagged ability lookup and activation requirements.
 */

#include "gameplay/ability.h"

const UGameplayAbility *unsigned_ability_find(const UGameplayAbilityContainer *abilities, UGameplayTag tag) {
    if (abilities->count == 0u) {
        return NULL;
    }

    for (u8 i = 0u; i < abilities->count; ++i) {
        const UGameplayAbility *ability = abilities->instances[i];
        if (ability->base.tag == tag) {
            return ability;
        }
    }

    return NULL;
}

bool unsigned_ability_can_activate(const UGameplayTagContainer *tags, const UGameplayAbility *ability) {
    return !unsigned_gameplay_tag_has_any(tags, &ability->blocked_by_tags);
}
