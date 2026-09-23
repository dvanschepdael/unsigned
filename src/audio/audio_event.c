#include "audio/audio.h"
#include "audio/audio_resolver_internal.h"

static bool unsigned_audio_event_conditions_pass(const UAudioManager *audio, const UAudioEventDefinition *definition) {
    if (definition->condition_count == 0u) {
        return true;
    }
    for (u8 i = 0u; i < definition->condition_count; ++i) {
        const UAudioCondition *condition = &definition->conditions[i];
        if (!unsigned_audio_evaluate_condition(condition->operation, audio->parameters[condition->parameter], condition->expected)) {
            return false;
        }
    }
    return true;
}

static const UAudioVariantSet *unsigned_audio_event_set(const UAudioManager *audio, const UAudioEventDefinition *definition) {
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

void unsigned_audio_manager_set_events(UAudioManager *audio, const UAudioEventDefinition *events) {
    audio->events = events;
    for (u8 i = 0u; i < UNSIGNED_AUDIO_MAX_EVENTS; ++i) {
        audio->event_states[i] = (UAudioEventState){0};
        audio->event_states[i].selection.previous_index = 0xffu;
    }
}

void unsigned_audio_set_parameter(UAudioManager *audio, UAudioParameterId parameter, s16 value) {
    audio->parameters[parameter] = value;
}

bool unsigned_audio_trigger(UAudioManager *audio, UAudioEventId event) {
    const UAudioEventDefinition *definition = &audio->events[event];
    if (!unsigned_audio_event_conditions_pass(audio, definition)) {
        return false;
    }

    if (definition->behavior->cooldown_ticks != 0u && audio->event_states[event].selection.previous_index != 0xffu && (audio->tick - audio->event_states[event].cooldown_started_tick) < definition->behavior->cooldown_ticks) {
        return false;
    }

    const UAudioVariantSet *set = unsigned_audio_event_set(audio, definition);

    UAudioEventState next_state = audio->event_states[event];
    if (next_state.selection.active_key != set->key) {
        next_state.selection = (UAudioSelectionState){0};
        next_state.selection.active_key = set->key;
        next_state.selection.previous_index = 0xffu;
    }

    u16 next_random_state = audio->random_state;
    u8 selected = unsigned_audio_resolve_selection(definition->behavior->selection, set->variants, set->variant_count, &next_state.selection, &next_random_state);
    if (!unsigned_audio_play_sound_effect(audio, set->variants[selected].command)) {
        return false;
    }

    next_state.selection.previous_index = selected;
    next_state.cooldown_started_tick = audio->tick;
    audio->event_states[event] = next_state;
    audio->random_state = next_random_state;
    return true;
}
