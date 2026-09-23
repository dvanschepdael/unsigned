/**
 * @file effect.h
 * @brief Gameplay effect definition and application contract.
 *
 * Instant effects execute activate/end in one application call. Timed and infinite effects keep
 * a pool slot, and their granted tags stay reference-counted on the target until release.
 */

#ifndef UNSIGNED_GAMEPLAY_EFFECT_H
#define UNSIGNED_GAMEPLAY_EFFECT_H

#include "gameplay/gameplay.h"
#include "gameplay/tag.h"

typedef enum UGameplayEffectDurationType {
    U_GAMEPLAY_EFFECT_INSTANT,
    U_GAMEPLAY_EFFECT_DURATION,
    U_GAMEPLAY_EFFECT_INFINITE,
} UGameplayEffectDurationType;

typedef struct UGameplayEffect {
    UGameplayObject base;
    UGameplayTagContainer granted_tags;
    UGameplayEffectDurationType duration_type;
} UGameplayEffect;

#endif
