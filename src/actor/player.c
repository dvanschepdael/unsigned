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
    }

    const UInputMask matched = (UInputMask)(state & binding->input);
    return binding->match == U_INPUT_MATCH_ANY ? matched != 0u : matched == binding->input;
}

/** Return the logical ability tag expected by a binding, including direct-definition fallback. */
static UGameplayTag player_binding_ability_tag(const UGameplayAbilityBinding *binding) {
    if (binding == NULL) {
        return UNSIGNED_GAMEPLAY_TAG_NONE;
    }
    if (binding->ability_tag != UNSIGNED_GAMEPLAY_TAG_NONE) {
        return binding->ability_tag;
    }
    return binding->ability != NULL ? binding->ability->base.tag : UNSIGNED_GAMEPLAY_TAG_NONE;
}

/** Resolve a binding through the character's tagged catalog, falling back to a direct definition. */
static const UGameplayAbility *player_binding_resolve_ability(const UPlayer *player, const UGameplayAbilityBinding *binding) {
    if (player == NULL || player->character == NULL || binding == NULL) {
        return NULL;
    }

    if (binding->ability_tag != UNSIGNED_GAMEPLAY_TAG_NONE) {
        return unsigned_ability_find(&player->character->abilities, binding->ability_tag);
    }
    return binding->ability;
}

/** Validates that a cached ability slot is still active and belongs to the same tagged binding generation. */
static bool player_binding_instance_is_current(const UGameplayAbilityBinding *binding) {
    if (binding == NULL || binding->active_instance == NULL) {
        return false;
    }

    const UGameplayTag ability_tag = player_binding_ability_tag(binding);
    const UPoolInstance *instance = binding->active_instance;
    return ability_tag != UNSIGNED_GAMEPLAY_TAG_NONE && instance->active && instance->generation == binding->active_generation && instance->key == ability_tag && instance->args == binding->args;
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
    if (player == NULL || controller == NULL) {
        return;
    }

    /* Keep the sampled state accessible to active abilities that need held/released input semantics. */
    player->input_state = &controller->state;

    if (player->character == NULL || player->bindings == NULL || player->bindings->instances == NULL || player->bindings->count == 0u || player->bindings->count > player->bindings->capacity || abilities == NULL) {
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

        const UGameplayAbility *ability = player_binding_resolve_ability(player, binding);

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

    player->input_state = NULL;
    unsigned_character_destroy(player->character);
}
