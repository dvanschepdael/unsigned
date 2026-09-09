/**
 * @file runtime.c
 * @brief Implements aggregate gameplay ability/effect/cue runtime.
 */

#include "gameplay/runtime.h"

void unsigned_gameplay_runtime_init(UGameplayRuntime *gameplay) {
    if (gameplay == NULL) {
        return;
    }

    unsigned_gameplay_ability_pool_init(&gameplay->abilities);
    unsigned_gameplay_effect_pool_init(&gameplay->effects);
    unsigned_gameplay_cue_pool_init(&gameplay->cues);
}

void unsigned_gameplay_runtime_clear(UGameplayRuntime *gameplay) {
    if (gameplay == NULL) {
        return;
    }

    unsigned_gameplay_ability_pool_clear(&gameplay->abilities);
    unsigned_gameplay_effect_pool_clear(&gameplay->effects);
    unsigned_gameplay_cue_pool_clear(&gameplay->cues);
}
