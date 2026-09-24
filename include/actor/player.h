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
    UInputMatch match;
    void *args;
    /** Logical ability identity resolved from the character-owned catalog. */
    UGameplayTag ability_tag;
    /** Cached slot for this binding; valid only while it is active and `active_generation` still matches. */
    UPoolInstance *active_instance;
    /** Reservation generation captured with `active_instance`; ignored while active_instance is NULL. */
    u16 active_generation;
} UGameplayAbilityBinding;

typedef struct UGameplayAbilityBindingContainer {
    /** Number of authored bindings in `instances`. */
    u8 count;
    UGameplayAbilityBinding *instances;
} UGameplayAbilityBindingContainer;

typedef struct UPlayer {
    UCharacter *character;
    UGameplayAbilityBindingContainer *bindings;
    /** Latest controller snapshot routed to this player; valid while the controller remains alive. */
    const UInputState *input_state;
    u8 controller_index;
} UPlayer;

/**
 * @brief Matches controller state against ability bindings and manages their runtime slots.
 *
 * `U_INPUT_TRIGGER_DOWN` is continuous: its ability is released when the input stops matching.
 * Edge/hold triggers start an ability once and let the ability's own lifetime decide when it ends.
 *
 * @param player Player runtime whose character/input state is read or updated.
 * @param controller Controller state used to drive player or UI input.
 * @param abilities Runtime ability pool used to activate or release abilities.
 * @pre Player, character, binding container, controller and ability pool are valid.
 * @pre Every binding has a valid `UInputTrigger` and `UInputMatch`.
 * @pre Every binding has a non-zero `ability_tag` that resolves in the character catalog.
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
