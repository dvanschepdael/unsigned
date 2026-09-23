/**
 * @file audio_resolver_internal.h
 * @brief Internal deterministic helpers for event conditions and variant selection.
 */

#ifndef UNSIGNED_AUDIO_RESOLVER_INTERNAL_H
#define UNSIGNED_AUDIO_RESOLVER_INTERNAL_H

#include "audio/audio_event.h"

/** Resolve one authored non-empty variant set.
 * @pre `mode` is valid, `variants`, `state` and `random_state` are valid, `*random_state != 0`, `variant_count > 0`, and shuffle mode has `variant_count <= UNSIGNED_AUDIO_MAX_VARIANTS_PER_SET`.
 * @pre Weighted sets contain at least one variant with a non-zero weight.
 */
u8 unsigned_audio_resolve_selection(UAudioSelectionMode mode, const UAudioVariant *variants, u8 variant_count, UAudioSelectionState *state, u16 *random_state);
/** @pre `operation` is a valid UAudioConditionOperator. */
bool unsigned_audio_evaluate_condition(UAudioConditionOperator operation, s16 actual, s16 expected);

#endif
