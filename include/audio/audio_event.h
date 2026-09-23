/**
 * @file audio_event.h
 * @brief Data-driven mapping from gameplay audio events to concrete sound commands.
 *
 * Event definitions are immutable caller-owned data. Conditions/route parameters choose a variant
 * set, then the selected policy (weighted, shuffle, round-robin, etc.) chooses one command.
 */

#ifndef UNSIGNED_AUDIO_EVENT_H
#define UNSIGNED_AUDIO_EVENT_H

#include "audio/audio_types.h"

typedef struct UAudioVariant {
    USoundCommand command;
    /** Relative weight used only by weighted selection; zero-weight entries are never selected there. */
    u8 weight;
} UAudioVariant;

typedef struct UAudioSelectionState {
    u16 shuffle_remaining;
    s16 active_key;
    u8 next_index;
    u8 previous_index;
} UAudioSelectionState;

typedef enum UAudioSelectionMode {
    U_AUDIO_SELECT_FIRST = 0,
    U_AUDIO_SELECT_RANDOM,
    U_AUDIO_SELECT_RANDOM_NO_REPEAT,
    U_AUDIO_SELECT_WEIGHTED,
    U_AUDIO_SELECT_SHUFFLE,
    U_AUDIO_SELECT_ROUND_ROBIN,
    U_AUDIO_SELECT_COUNT,
} UAudioSelectionMode;

typedef enum UAudioConditionOperator {
    U_AUDIO_CONDITION_EQUAL = 0,
    U_AUDIO_CONDITION_LESS_THAN,
    U_AUDIO_CONDITION_AT_LEAST,
    U_AUDIO_CONDITION_TRUE,
    U_AUDIO_CONDITION_COUNT,
} UAudioConditionOperator;

typedef struct UAudioCondition {
    s16 expected;
    UAudioParameterId parameter;
    UAudioConditionOperator operation;
} UAudioCondition;

typedef struct UAudioVariantSet {
    const UAudioVariant *variants;
    s16 key;
    u8 variant_count;
} UAudioVariantSet;

typedef struct UAudioEventBehavior {
    UAudioSelectionMode selection;
    u16 cooldown_ticks;
} UAudioEventBehavior;

typedef struct UAudioEventDefinition {
    const UAudioEventBehavior *behavior;
    const UAudioVariantSet *sets;
    const UAudioCondition *conditions;
    UAudioParameterId route_parameter;
    u8 set_count;
    u8 default_set;
    u8 condition_count;
} UAudioEventDefinition;

typedef struct UAudioEventState {
    UAudioSelectionState selection;
    u32 cooldown_started_tick;
} UAudioEventState;

/**
 * @brief Installs immutable event content and resets per-event selection/cooldown state.
 *
 * The event array remains caller-owned and must outlive the manager while installed. Passing NULL
 * disables event-driven audio without affecting direct sound-effect playback.
 * @pre `audio` is a valid initialized manager.
 * @pre Every addressed event has a behavior, at least one non-empty variant set, a valid default set, valid
 *      parameter/condition ids and a selection mode supported by UAudioSelectionMode.
 * @pre Every variant command is in the game command range, weighted sets have a positive total
 *      weight, and shuffle sets contain no more than UNSIGNED_AUDIO_MAX_VARIANTS_PER_SET entries.
 */
void unsigned_audio_manager_set_events(UAudioManager *audio, const UAudioEventDefinition *events);
/** Set one runtime routing/condition parameter used by authored audio events.
 * @pre `parameter < UNSIGNED_AUDIO_MAX_PARAMETERS`.
 */
void unsigned_audio_set_parameter(UAudioManager *audio, UAudioParameterId parameter, s16 value);

/**
 * @brief Resolve one gameplay event and enqueue the selected concrete sound command.
 *
 * Conditions are evaluated first, optional route keys choose a variant set, cooldown is applied,
 * then the configured selection policy advances its deterministic state.
 *
 * @pre `audio->events` is installed, `event < UNSIGNED_AUDIO_MAX_EVENTS`, and the addressed event satisfies the authoring contract.
 */
bool unsigned_audio_trigger(UAudioManager *audio, UAudioEventId event);

#endif
