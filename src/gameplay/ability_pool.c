/**
 * @file ability_pool.c
 * @brief Implements runtime pool for active gameplay abilities.
 */

#include "gameplay/ability_pool.h"

/** Clears the ability pool metadata state without freeing caller-owned storage. */
static void ability_pool_clear_metadata(UAbilityPool *abilities, u8 index) {
    abilities->abilities[index] = NULL;
    abilities->tag_owners[index] = NULL;
}

/** Adapts generic gameplay-pool expiry to ability release so granted tags are removed before slot reuse. */
static void ability_pool_release_callback(void *context, UPoolInstance *instance) {
    unsigned_gameplay_ability_pool_release((UAbilityPool *)context, instance);
}

void unsigned_gameplay_ability_pool_init(UAbilityPool *abilities) {
    if (abilities == NULL) {
        return;
    }

    *abilities = (UAbilityPool){ 0 };
    abilities->pool.capacity = UNSIGNED_GAMEPLAY_MAX_ABILITY;
    abilities->pool.instances = abilities->instances;
    unsigned_gameplay_pool_init(&abilities->pool);
}

void unsigned_gameplay_ability_pool_clear(UAbilityPool *abilities) {
    if (abilities == NULL) {
        return;
    }

    for (u8 i = 0u; i < abilities->pool.capacity; ++i) {
        if (abilities->pool.instances[i].active) {
            unsigned_gameplay_ability_pool_release(abilities, &abilities->pool.instances[i]);
        }
    }
}

UAbilityPoolInstance *unsigned_gameplay_ability_pool_reserve(UAbilityPool *abilities, const UGameplayAbility *ability, UGameplayTagContainer *tags, void *args) {
    if (abilities == NULL || ability == NULL || tags == NULL) {
        return NULL;
    }

    UAbilityPoolInstance *instance = unsigned_gameplay_pool_reserve(&abilities->pool, &ability->base, args);
    if (instance == NULL) {
        return NULL;
    }

    u8 index = instance->index;
    if (!unsigned_gameplay_tag_add_all(tags, &ability->granted_tags)) {
        unsigned_gameplay_pool_cancel(&abilities->pool, instance);
        return NULL;
    }

    abilities->abilities[index] = ability;
    abilities->tag_owners[index] = tags;

    if (!unsigned_gameplay_pool_activate(&abilities->pool, instance)) {
        return NULL;
    }

    return instance;
}

void unsigned_gameplay_ability_pool_release(UAbilityPool *abilities, UAbilityPoolInstance *instance) {
    if (abilities == NULL || !unsigned_pool_owns(&abilities->pool, instance) || !instance->active) {
        return;
    }

    u8 index = instance->index;
    const UGameplayAbility *ability = abilities->abilities[index];
    UGameplayTagContainer *tag_owner = abilities->tag_owners[index];

    ability_pool_clear_metadata(abilities, index);

    if (ability != NULL && tag_owner != NULL) {
        (void)unsigned_gameplay_tag_remove(tag_owner, &ability->granted_tags);
    }

    unsigned_gameplay_pool_release(&abilities->pool, instance);
}

void unsigned_gameplay_ability_pool_tick(UAbilityPool *abilities) {
    if (abilities != NULL) {
        unsigned_gameplay_pool_tick(&abilities->pool, ability_pool_release_callback, abilities);
    }
}
