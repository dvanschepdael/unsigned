/**
 * @file player.c
 * @brief Implements player-controlled character input and ability bindings.
 */

#include "actor/player.h"

/** Returns whether the current input snapshot satisfies the binding trigger and button mask. */
static bool unsigned_input_binding_matches(const UInputState *input, const UGameplayAbilityBinding *binding) {
    UInputMask state = 0u;
    switch (binding->trigger) {
    case U_INPUT_TRIGGER_HOLD:
        state = input->hold;
        break;
    case U_INPUT_TRIGGER_DOWN:
        state = input->down;
        break;
    case U_INPUT_TRIGGER_PRESSED:
        state = input->pressed;
        break;
    case U_INPUT_TRIGGER_RELEASED:
        state = input->released;
        break;
    default:
        U_UNREACHABLE();
    }

    const UInputMask matched = (UInputMask)(state & binding->input);
    switch (binding->match) {
    case U_INPUT_MATCH_ALL:
        return matched == binding->input;
    case U_INPUT_MATCH_ANY:
        return matched != 0u;
    default:
        U_UNREACHABLE();
    }
}

/** Returns whether the cached slot still belongs to the reservation generation stored by this binding. */
static bool player_binding_instance_is_current(const UGameplayAbilityBinding *binding) {
    if (binding->active_instance == NULL) {
        return false;
    }

    const UPoolInstance *instance = binding->active_instance;
    return instance->active && instance->generation == binding->active_generation;
}

/** Releases the cached active ability when still valid, then clears the binding handle. */
static void player_binding_release_instance(UGameplayAbilityBinding *binding, UAbilityPool *abilities) {
    if (player_binding_instance_is_current(binding)) {
        unsigned_gameplay_ability_pool_release(abilities, binding->active_instance);
    }

    binding->active_instance = NULL;
}

void unsigned_player_tick_input(UPlayer *player, UInputController *controller, UAbilityPool *abilities) {
    /* Keep the sampled state accessible to active abilities that need held/released input semantics. */
    player->input_state = &controller->state;

    if (player->bindings->count == 0u) {
        return;
    }

    UInputState state = controller->state;

    for (u8 i = 0u; i < player->bindings->count; ++i) {
        UGameplayAbilityBinding *binding = &player->bindings->instances[i];
        bool matches = unsigned_input_binding_matches(&state, binding);
        bool continuous = binding->trigger == U_INPUT_TRIGGER_DOWN;

        if (binding->active_instance != NULL && !player_binding_instance_is_current(binding)) {
            binding->active_instance = NULL;
        }

        if (continuous && !matches) {
            player_binding_release_instance(binding, abilities);
            continue;
        }

        if (!matches) {
            continue;
        }

        const UGameplayAbility *ability = unsigned_ability_find(&player->character->abilities, binding->ability_tag);

        if (!unsigned_ability_can_activate(&player->character->tags, ability)) {
            if (continuous) {
                player_binding_release_instance(binding, abilities);
            }
            continue;
        }

        if (binding->active_instance != NULL) {
            continue;
        }

        UPoolInstance *instance = unsigned_gameplay_ability_pool_reserve(abilities, ability, &player->character->tags, binding->args);
        binding->active_instance = instance;
        binding->active_generation = instance->generation;
    }
}

void unsigned_player_destroy(UPlayer *player, UAbilityPool *abilities) {
    for (u8 i = 0u; i < player->bindings->count; ++i) {
        player_binding_release_instance(&player->bindings->instances[i], abilities);
    }

    player->input_state = NULL;
    unsigned_actor_destroy(&player->character->actor);
}
