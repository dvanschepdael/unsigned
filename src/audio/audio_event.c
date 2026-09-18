#include "audio/audio.h"
#include "audio/audio_resolver_internal.h"

static bool audio_event_definition_validate(const UAudioEventDefinition *definition) {
    if (definition == NULL || definition->behavior == NULL || definition->behavior->selection >= U_AUDIO_SELECT_COUNT || definition->sets == NULL || definition->set_count == 0u || definition->default_set >= definition->set_count ||
        (definition->route_parameter != U_AUDIO_NO_PARAMETER && definition->route_parameter >= UNSIGNED_AUDIO_MAX_PARAMETERS) || (definition->condition_count > 0u && definition->conditions == NULL)) {
        return false;
    }

    for (u8 i = 0u; i < definition->set_count; ++i) {
        const UAudioVariantSet *set = &definition->sets[i];
        if (set->variants == NULL || set->variant_count == 0u || set->variant_count > UNSIGNED_AUDIO_MAX_VARIANTS_PER_SET) {
            return false;
        }
        for (u8 variant = 0u; variant < set->variant_count; ++variant) {
            if (!unsigned_audio_command_is_game(set->variants[variant].command)) {
                return false;
            }
        }
    }

    for (u8 i = 0u; i < definition->condition_count; ++i) {
        const UAudioCondition *condition = &definition->conditions[i];
        if (condition->parameter >= UNSIGNED_AUDIO_MAX_PARAMETERS || condition->operation >= U_AUDIO_CONDITION_COUNT) {
            return false;
        }
    }

    return true;
}

static bool audio_catalog_validate(const UAudioCatalog *catalog) {
    if (catalog == NULL) {
        return true;
    }
    if (catalog->event_count > UNSIGNED_AUDIO_MAX_EVENTS || (catalog->event_count > 0u && catalog->events == NULL)) {
        return false;
    }
    for (u8 i = 0u; i < catalog->event_count; ++i) {
        if (!audio_event_definition_validate(&catalog->events[i])) {
            return false;
        }
    }
    return true;
}

static bool unsigned_audio_event_conditions_pass(const UAudioManager *audio, const UAudioEventDefinition *definition) {
    if (definition->condition_count == 0u) {
        return true;
    }
    if (definition->conditions == NULL) {
        return false;
    }

    for (u8 i = 0u; i < definition->condition_count; ++i) {
        const UAudioCondition *condition = &definition->conditions[i];
        if (condition->parameter >= UNSIGNED_AUDIO_MAX_PARAMETERS || !unsigned_audio_evaluate_condition(condition->operation, audio->parameters[condition->parameter], condition->expected)) {
            return false;
        }
    }
    return true;
}

static const UAudioVariantSet *unsigned_audio_event_set(const UAudioManager *audio, const UAudioEventDefinition *definition) {
    if (definition->sets == NULL || definition->set_count == 0u || definition->default_set >= definition->set_count) {
        return NULL;
    }

    if (definition->route_parameter < UNSIGNED_AUDIO_MAX_PARAMETERS) {
        const s16 key = audio->parameters[definition->route_parameter];
        for (u8 i = 0u; i < definition->set_count; ++i) {
            if (definition->sets[i].key == key) {
                return &definition->sets[i];
            }
        }
    }
    return &definition->sets[definition->default_set];
}

bool unsigned_audio_manager_set_catalog(UAudioManager *audio, const UAudioCatalog *catalog) {
    if (audio == NULL || !audio_catalog_validate(catalog)) {
        return false;
    }

    audio->catalog = catalog;
    for (u8 i = 0u; i < UNSIGNED_AUDIO_MAX_EVENTS; ++i) {
        audio->event_states[i] = (UAudioEventState){ 0 };
        audio->event_states[i].selection.previous_index = 0xffu;
    }
    return true;
}

void unsigned_audio_set_parameter(UAudioManager *audio, UAudioParameterId parameter, s16 value) {
    if (audio != NULL && parameter < UNSIGNED_AUDIO_MAX_PARAMETERS) {
        audio->parameters[parameter] = value;
    }
}

bool unsigned_audio_trigger(UAudioManager *audio, UAudioEventId event) {
    if (audio == NULL || audio->catalog == NULL || audio->catalog->events == NULL || event >= audio->catalog->event_count || event >= UNSIGNED_AUDIO_MAX_EVENTS) {
        return false;
    }

    const UAudioEventDefinition *definition = &audio->catalog->events[event];
    if (definition->behavior == NULL || definition->behavior->selection >= U_AUDIO_SELECT_COUNT || !unsigned_audio_event_conditions_pass(audio, definition)) {
        return false;
    }

    if (definition->behavior->cooldown_ticks != 0u && audio->event_states[event].selection.previous_index != 0xffu && (audio->tick - audio->event_states[event].cooldown_started_tick) < definition->behavior->cooldown_ticks) {
        return false;
    }

    const UAudioVariantSet *set = unsigned_audio_event_set(audio, definition);
    if (set == NULL || set->variants == NULL || set->variant_count == 0u || set->variant_count > UNSIGNED_AUDIO_MAX_VARIANTS_PER_SET) {
        return false;
    }

    UAudioEventState next_state = audio->event_states[event];
    if (next_state.selection.active_key != set->key) {
        next_state.selection = (UAudioSelectionState){ 0 };
        next_state.selection.active_key = set->key;
        next_state.selection.previous_index = 0xffu;
    }

    u16 next_random_state = audio->random_state;
    u8 selected = unsigned_audio_resolve_selection(definition->behavior->selection, set->variants, set->variant_count, &next_state.selection, &next_random_state);
    if (selected >= set->variant_count || !unsigned_audio_play_sound_effect(audio, set->variants[selected].command)) {
        return false;
    }

    next_state.selection.previous_index = selected;
    next_state.cooldown_started_tick = audio->tick;
    audio->event_states[event] = next_state;
    audio->random_state = next_random_state;
    return true;
}
