/**
 * @file effect_pool.c
 * @brief Implements runtime pool for active gameplay effects.
 */

#include "gameplay/effect_pool.h"

static void effect_pool_release_callback(void *context, UPoolInstance *instance) {
    unsigned_gameplay_effect_pool_release((UEffectPool *)context, instance);
}

/** Grants authored tags when the effect participates in tag ownership. */
static void gameplay_effect_grant_tags(UGameplayTagContainer *tags, const UGameplayEffect *effect) {
    if (tags != NULL) {
        unsigned_gameplay_tag_add_all(tags, &effect->granted_tags);
    }
}

static void gameplay_effect_remove_tags(UGameplayTagContainer *tags, const UGameplayEffect *effect) {
    if (tags != NULL) {
        unsigned_gameplay_tag_remove(tags, &effect->granted_tags);
    }
}

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
    unsigned_pool_init(&effects->pool, effects->instances, ARRAY_COUNT_U8(effects->instances));
}

void unsigned_gameplay_effect_pool_clear(UEffectPool *effects) {
    unsigned_gameplay_pool_clear(&effects->pool, effect_pool_release_callback, effects);
}

UEffectPoolInstance *unsigned_gameplay_effect_apply(UEffectPool *effects, const UGameplayEffect *effect, UGameplayTagContainer *tags, void *args) {
    const u16 duration = effect->base.duration;
    switch (effect->duration_type) {
    case U_GAMEPLAY_EFFECT_INSTANT:
        gameplay_effect_grant_tags(tags, effect);
        gameplay_effect_execute_instant(effect, tags, args);
        return NULL;
    case U_GAMEPLAY_EFFECT_DURATION:
    case U_GAMEPLAY_EFFECT_INFINITE:
        break;
    default:
        U_UNREACHABLE();
    }

    UEffectPoolInstance *instance = unsigned_gameplay_pool_reserve(&effects->pool, &effect->base, args);
    gameplay_effect_grant_tags(tags, effect);

    instance->duration = effect->duration_type == U_GAMEPLAY_EFFECT_DURATION ? duration : 0u;
    effects->effects[instance->index] = effect;
    effects->tag_owners[instance->index] = tags;

    unsigned_gameplay_pool_activate(instance);
    return instance;
}

void unsigned_gameplay_effect_pool_release(UEffectPool *effects, UEffectPoolInstance *instance) {
    const u8 index = instance->index;
    const UGameplayEffect *effect = effects->effects[index];
    UGameplayTagContainer *tag_owner = effects->tag_owners[index];

    gameplay_effect_remove_tags(tag_owner, effect);
    unsigned_gameplay_pool_release(&effects->pool, instance);
}

void unsigned_gameplay_effect_pool_tick(UEffectPool *effects) {
    unsigned_gameplay_pool_tick(&effects->pool, effect_pool_release_callback, effects);
}
