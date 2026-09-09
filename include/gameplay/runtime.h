/**
 * @file runtime.h
 * @brief Aggregate gameplay ability/effect/cue runtime.
 */

#ifndef UNSIGNED_GAMEPLAY_RUNTIME_H
#define UNSIGNED_GAMEPLAY_RUNTIME_H

#include "gameplay/ability_pool.h"
#include "gameplay/cue_pool.h"
#include "gameplay/effect_pool.h"

typedef struct UGameplayRuntime {
    UAbilityPool abilities;
    UEffectPool effects;
    UCuePool cues;
} UGameplayRuntime;

/**
 * @brief Initializes the gameplay runtime to a valid empty runtime state.
 *
 * @param gameplay Aggregate gameplay runtime whose ability/effect/cue pools are initialized or cleared.
 */
void unsigned_gameplay_runtime_init(UGameplayRuntime *gameplay);

/**
 * @brief Clears the gameplay runtime state without freeing caller-owned storage.
 *
 * @param gameplay Aggregate gameplay runtime whose ability/effect/cue pools are initialized or cleared.
 */
void unsigned_gameplay_runtime_clear(UGameplayRuntime *gameplay);

#endif
