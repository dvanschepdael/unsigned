/**
 * @file effect_pool.c
 * @brief Implements runtime pool for active gameplay effects.
 */

#include "gameplay/effect_pool.h"

/** Clears the effect pool metadata state without freeing caller-owned storage. */
static void effect_pool_clear_metadata(UEffectPool *effects, u8 index) {
    effects->effects[index] = NULL;
    effects->tag_owners[index] = NULL;
}

/** Adapts generic gameplay-pool expiry to effect release so granted tags are removed before slot reuse. */
static void effect_pool_release_callback(void *context, UPoolInstance *instance) {
    unsigned_gameplay_effect_pool_release((UEffectPool *)context, instance);
}

/** Adds the effect-granted gameplay tags to the target tag container. */
static bool gameplay_effect_grant_tags(UGameplayTagContainer *tags, const UGameplayEffect *effect) {
    if (effect == NULL) {
        return false;
    }
    if (tags == NULL) {
        return effect->granted_tags.count == 0u;
    }

    return unsigned_gameplay_tag_add_all(tags, &effect->granted_tags);
}

/** Removes the effect-granted tag reference counts from the target tag container. */
static void gameplay_effect_remove_tags(UGameplayTagContainer *tags, const UGameplayEffect *effect) {
    if (tags != NULL && effect != NULL) {
        (void)unsigned_gameplay_tag_remove(tags, &effect->granted_tags);
    }
}

/** Executes the immediate portion of an effect without retaining a timed runtime slot. */
static void gameplay_effect_execute_instant(const UGameplayEffect *effect, UGameplayTagContainer *tags, void *args) {
    if (effect->base.activate != NULL) {
        effect->base.activate(args);
    }
    if (effect->base.end != NULL) {
        effect->base.end(args);
    }

    gameplay_effect_remove_tags(tags, effect);
}

void unsigned_gameplay_effect_pool_init(UEffectPool *effects) {
    if (effects == NULL) {
        return;
    }

    *effects = (UEffectPool){ 0 };
    effects->pool.capacity = UNSIGNED_GAMEPLAY_MAX_EFFECT;
    effects->pool.instances = effects->instances;
    unsigned_gameplay_pool_init(&effects->pool);
}

void unsigned_gameplay_effect_pool_clear(UEffectPool *effects) {
    if (effects == NULL) {
        return;
    }

    for (u8 i = 0u; i < effects->pool.capacity; ++i) {
        if (effects->pool.instances[i].active) {
            unsigned_gameplay_effect_pool_release(effects, &effects->pool.instances[i]);
        }
    }
}

UGameplayEffectApplyResult unsigned_gameplay_effect_apply(UEffectPool *effects, const UGameplayEffect *effect, UGameplayTagContainer *tags, void *args, UEffectPoolInstance **out_instance) {
    if (out_instance != NULL) {
        *out_instance = NULL;
    }
    if (effects == NULL || effect == NULL) {
        return U_GAMEPLAY_EFFECT_APPLY_FAILED;
    }

    u16 duration = effect->base.duration;
    if (effect->duration_type == U_GAMEPLAY_EFFECT_INSTANT || (effect->duration_type == U_GAMEPLAY_EFFECT_DURATION && duration == 0u)) {
        if (!gameplay_effect_grant_tags(tags, effect)) {
            return U_GAMEPLAY_EFFECT_APPLY_FAILED;
        }

        gameplay_effect_execute_instant(effect, tags, args);
        return U_GAMEPLAY_EFFECT_APPLY_INSTANT;
    }

    if (effect->duration_type != U_GAMEPLAY_EFFECT_DURATION && effect->duration_type != U_GAMEPLAY_EFFECT_INFINITE) {
        return U_GAMEPLAY_EFFECT_APPLY_FAILED;
    }

    UEffectPoolInstance *instance = unsigned_gameplay_pool_reserve(&effects->pool, &effect->base, args);
    if (instance == NULL) {
        return U_GAMEPLAY_EFFECT_APPLY_FAILED;
    }

    if (!gameplay_effect_grant_tags(tags, effect)) {
        unsigned_gameplay_pool_cancel(&effects->pool, instance);
        return U_GAMEPLAY_EFFECT_APPLY_FAILED;
    }

    instance->duration = effect->duration_type == U_GAMEPLAY_EFFECT_DURATION ? duration : 0u;
    u8 index = instance->index;
    effects->effects[index] = effect;
    effects->tag_owners[index] = tags;

    if (!unsigned_gameplay_pool_activate(&effects->pool, instance)) {
        return U_GAMEPLAY_EFFECT_APPLY_FAILED;
    }

    if (out_instance != NULL) {
        *out_instance = instance;
    }
    return U_GAMEPLAY_EFFECT_APPLY_ACTIVE;
}

void unsigned_gameplay_effect_pool_release(UEffectPool *effects, UEffectPoolInstance *instance) {
    if (effects == NULL || !unsigned_pool_owns(&effects->pool, instance) || !instance->active) {
        return;
    }

    u8 index = instance->index;
    const UGameplayEffect *effect = effects->effects[index];
    UGameplayTagContainer *tag_owner = effects->tag_owners[index];
    effect_pool_clear_metadata(effects, index);
    gameplay_effect_remove_tags(tag_owner, effect);
    unsigned_gameplay_pool_release(&effects->pool, instance);
}

void unsigned_gameplay_effect_pool_tick(UEffectPool *effects) {
    if (effects != NULL) {
        unsigned_gameplay_pool_tick(&effects->pool, effect_pool_release_callback, effects);
    }
}
