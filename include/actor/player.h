/**
 * @file player.h
 * @brief Player-controlled character input and ability bindings.
 */

#ifndef UNSIGNED_ACTOR_PLAYER_H
#define UNSIGNED_ACTOR_PLAYER_H

#include "actor/character.h"
#include "gameplay/ability_pool.h"
#include "input/input.h"

typedef struct UGameplayAbilityBinding {
    UInputMask input;
    UInputTrigger trigger;
    void *args;
    const UGameplayAbility *ability;
    /** Cached slot for this binding; valid only while `active_generation` still matches. */
    UPoolInstance *active_instance;
    /** Protects against a released pool slot being reused at the same address by another ability. */
    u16 active_generation;
} UGameplayAbilityBinding;

typedef struct UGameplayAbilityBindingContainer {
    u8 count;
    u8 capacity;
    UGameplayAbilityBinding *instances;
} UGameplayAbilityBindingContainer;

typedef struct UPlayer {
    UCharacter *character;
    UGameplayAbilityBindingContainer *bindings;
    u8 controller_index;
} UPlayer;

/**
 * @brief Advances the player by one scheduled engine frame.
 *
 * @param player Player runtime whose character/input state is read or updated.
 */
void unsigned_player_tick(UPlayer *player);

/**
 * @brief Matches controller state against ability bindings and manages their runtime slots.
 *
 * `U_INPUT_TRIGGER_DOWN` is continuous: its ability is released when the input stops matching.
 * Edge/hold triggers start an ability once and let the ability's own lifetime decide when it ends.
 *
 * @param player Player runtime whose character/input state is read or updated.
 * @param controller Controller state used to drive player or UI input.
 * @param abilities Runtime ability pool used to activate or release abilities.
 */
void unsigned_player_tick_input(UPlayer *player, UInputController *controller, UAbilityPool *abilities);

/**
 * @brief Tears down the player runtime state and releases its logical resources.
 *
 * @param player Player runtime whose character/input state is read or updated.
 * @param abilities Runtime ability pool used to activate or release abilities.
 */
void unsigned_player_destroy(UPlayer *player, UAbilityPool *abilities);

#endif
