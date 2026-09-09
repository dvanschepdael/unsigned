/**
 * @file audio_resolver_internal.h
 * @brief Internal deterministic helpers for event conditions and variant selection.
 */

#ifndef UNSIGNED_AUDIO_RESOLVER_INTERNAL_H
#define UNSIGNED_AUDIO_RESOLVER_INTERNAL_H

#include "audio/audio_event.h"

u8 unsigned_audio_resolve_selection(UAudioSelectionMode mode, const UAudioVariant *variants, u8 variant_count, UAudioSelectionState *state, u16 *random_state);
bool unsigned_audio_evaluate_condition(UAudioConditionOperator operation, s16 actual, s16 expected);

#endif
