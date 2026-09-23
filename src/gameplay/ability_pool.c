/**
 * @file ability_pool.c
 * @brief Implements runtime pool for active gameplay abilities.
 */

#include "gameplay/ability_pool.h"

static void ability_pool_release_callback(void *context, UPoolInstance *instance) {
    unsigned_gameplay_ability_pool_release((UAbilityPool *)context, instance);
}

void unsigned_gameplay_ability_pool_init(UAbilityPool *abilities) {
    *abilities = (UAbilityPool){0};
    abilities->pool.capacity = UNSIGNED_GAMEPLAY_MAX_ABILITY;
    abilities->pool.instances = abilities->instances;
    unsigned_pool_init(&abilities->pool);
}

void unsigned_gameplay_ability_pool_clear(UAbilityPool *abilities) {
    unsigned_gameplay_pool_clear(&abilities->pool, ability_pool_release_callback, abilities);
}

UAbilityPoolInstance *unsigned_gameplay_ability_pool_reserve(UAbilityPool *abilities, const UGameplayAbility *ability, UGameplayTagContainer *tags, void *args) {
    UAbilityPoolInstance *instance = unsigned_gameplay_pool_reserve(&abilities->pool, &ability->base, args);

    unsigned_gameplay_tag_add_all(tags, &ability->granted_tags);

    const u8 index = instance->index;
    abilities->abilities[index] = ability;
    abilities->tag_owners[index] = tags;

    unsigned_gameplay_pool_activate(instance);
    return instance;
}

void unsigned_gameplay_ability_pool_release(UAbilityPool *abilities, UAbilityPoolInstance *instance) {
    const u8 index = instance->index;
    const UGameplayAbility *ability = abilities->abilities[index];
    UGameplayTagContainer *tag_owner = abilities->tag_owners[index];

    unsigned_gameplay_tag_remove(tag_owner, &ability->granted_tags);
    unsigned_gameplay_pool_release(&abilities->pool, instance);
}

void unsigned_gameplay_ability_pool_release_owner(UAbilityPool *abilities, UGameplayTagContainer *owner) {
    for (u8 i = 0u; i < unsigned_pool_iteration_end(&abilities->pool); ++i) {
        if (abilities->instances[i].active && abilities->tag_owners[i] == owner) {
            unsigned_gameplay_ability_pool_release(abilities, &abilities->instances[i]);
        }
    }
}

UAbilityPoolInstance *unsigned_gameplay_ability_pool_replace_owner(UAbilityPool *abilities, UGameplayTagContainer *owner, const UGameplayAbility *ability, void *args) {
    unsigned_gameplay_ability_pool_release_owner(abilities, owner);
    return unsigned_gameplay_ability_pool_reserve(abilities, ability, owner, args);
}

void unsigned_gameplay_ability_pool_tick(UAbilityPool *abilities) {
    unsigned_gameplay_pool_tick(&abilities->pool, ability_pool_release_callback, abilities);
}
