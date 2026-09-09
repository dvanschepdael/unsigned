/**
 * @file ability.c
 * @brief Implements ability activation requirements and definition data.
 */

#include "gameplay/ability.h"

bool unsigned_ability_can_activate(const UGameplayTagContainer *tags, const UGameplayAbility *ability) {
    if (tags == NULL || ability == NULL) {
        return false;
    }

    return !unsigned_gameplay_tag_has_any(tags, &ability->blocked_by_tags);
}
