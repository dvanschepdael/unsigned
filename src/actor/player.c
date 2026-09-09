/**
 * @file player.c
 * @brief Implements player-controlled character input and ability bindings.
 */

#include "actor/player.h"

/** Returns whether the current input snapshot satisfies the binding trigger and button mask. */
static bool unsigned_input_binding_matches(const UInputState *input, const UGameplayAbilityBinding *binding) {
    if (input == NULL || binding == NULL || binding->input == 0) {
        return false;
    }

    switch (binding->trigger) {
        case U_INPUT_TRIGGER_HOLD:
            return (input->hold & binding->input) == binding->input;

        case U_INPUT_TRIGGER_DOWN:
            return (input->down & binding->input) == binding->input;

        case U_INPUT_TRIGGER_PRESSED:
            return (input->pressed & binding->input) == binding->input;

        case U_INPUT_TRIGGER_RELEASED:
            return (input->released & binding->input) == binding->input;
    }

    return false;
}

/** Validates that a cached ability slot is still active and belongs to the same binding generation. */
static bool player_binding_instance_is_current(const UGameplayAbilityBinding *binding) {
    if (binding == NULL || binding->active_instance == NULL || binding->ability == NULL) {
        return false;
    }

    const UPoolInstance *instance = binding->active_instance;
    return instance->active && instance->generation == binding->active_generation && instance->object == &binding->ability->base && instance->args == binding->args;
}

/** Drops the cached ability-slot handle without touching the ability pool. */
static void player_binding_clear_instance(UGameplayAbilityBinding *binding) {
    if (binding == NULL) {
        return;
    }

    binding->active_instance = NULL;
    binding->active_generation = 0u;
}

/** Releases the cached active ability when still valid, then clears the binding handle. */
static void player_binding_release_instance(UGameplayAbilityBinding *binding, UAbilityPool *abilities) {
    if (binding == NULL) {
        return;
    }

    if (abilities != NULL && player_binding_instance_is_current(binding)) {
        unsigned_gameplay_ability_pool_release(abilities, binding->active_instance);
    }

    player_binding_clear_instance(binding);
}

void unsigned_player_tick(UPlayer *player) {
    if (player == NULL || player->character == NULL) {
        return;
    }

    unsigned_character_tick(player->character);
}

void unsigned_player_tick_input(UPlayer *player, UInputController *controller, UAbilityPool *abilities) {
    if (player == NULL || player->character == NULL || controller == NULL || player->bindings == NULL || player->bindings->instances == NULL || player->bindings->count == 0u || player->bindings->count > player->bindings->capacity || abilities == NULL) {
        return;
    }

    UInputState state = controller->state;

    for (u8 i = 0u; i < player->bindings->count; ++i) {
        UGameplayAbilityBinding *binding = &player->bindings->instances[i];
        bool matches = unsigned_input_binding_matches(&state, binding);
        bool continuous = binding->trigger == U_INPUT_TRIGGER_DOWN;

        if (binding->active_instance != NULL && !player_binding_instance_is_current(binding)) {
            player_binding_clear_instance(binding);
        }

        if (continuous && !matches) {
            player_binding_release_instance(binding, abilities);
            continue;
        }

        if (!matches) {
            continue;
        }

        const UGameplayAbility *ability = binding->ability;

        if (ability == NULL) {
            continue;
        }

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

        if (instance == NULL) {
            continue;
        }

        binding->active_instance = instance;
        binding->active_generation = instance->generation;
    }
}

void unsigned_player_destroy(UPlayer *player, UAbilityPool *abilities) {
    if (player == NULL || player->character == NULL) {
        return;
    }

    if (player->bindings != NULL && player->bindings->instances != NULL && player->bindings->count <= player->bindings->capacity) {
        for (u8 i = 0u; i < player->bindings->count; ++i) {
            player_binding_release_instance(&player->bindings->instances[i], abilities);
        }
    }

    unsigned_character_destroy(player->character);
}
