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

typedef struct UAudioCatalog {
    const UAudioEventDefinition *events;
    u8 event_count;
} UAudioCatalog;

typedef struct UAudioEventState {
    UAudioSelectionState selection;
    u32 cooldown_started_tick;
} UAudioEventState;

/** Catalog memory remains owned by the caller and must outlive its use by the manager. */
bool unsigned_audio_manager_set_catalog(UAudioManager *audio, const UAudioCatalog *catalog);
void unsigned_audio_set_parameter(UAudioManager *audio, UAudioParameterId parameter, s16 value);

/** Resolve an event and enqueue its selected gameplay command. */
bool unsigned_audio_trigger(UAudioManager *audio, UAudioEventId event);

#endif
