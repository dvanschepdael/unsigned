/**
 * @file audio_resolver.c
 * @brief Implements audio event condition/weighted selection resolver.
 */

#include "audio/audio_resolver_internal.h"
#include "audio/config.h"
#include "core/math/math.h"

/** Selects a uniformly random audio command variant. */
static u8 audio_select_random(u8 variant_count, u16 *random_state) {
    return (u8)(unsigned_math_random_u16(random_state) % variant_count);
}

/** Selects a random audio variant while avoiding the previous choice when possible. */
static u8 audio_select_random_no_repeat(u8 variant_count, UAudioSelectionState *state, u16 *random_state) {
    if (variant_count < 2u || state->previous_index >= variant_count) {
        return audio_select_random(variant_count, random_state);
    }

    u8 selected = (u8)(unsigned_math_random_u16(random_state) % (variant_count - 1u));
    if (selected >= state->previous_index) {
        ++selected;
    }
    return selected;
}

/** Selects an audio variant according to the configured per-variant weights. */
static u8 audio_select_weighted(const UAudioVariant *variants, u8 variant_count, u16 *random_state) {
    u16 total = 0u;

    for (u8 i = 0u; i < variant_count; ++i) {
        total = (u16)(total + variants[i].weight);
    }
    u16 selected_weight = (u16)(unsigned_math_random_u16(random_state) % total);
    for (u8 i = 0u; i < variant_count; ++i) {
        if (selected_weight < variants[i].weight) {
            return i;
        }
        selected_weight = (u16)(selected_weight - variants[i].weight);
    }
    return 0u; /* Positive total weight guarantees a selection in the loop above. */
}

/** Selects variants in a deterministic shuffled cycle without immediate repetition. */
static u8 audio_select_shuffle(u8 variant_count, UAudioSelectionState *state, u16 *random_state) {
    u8 remaining_count = 0u;
    bool refilled = false;

    u16 valid_mask = variant_count == 16u ? 0xffffu : (u16)((1u << variant_count) - 1u);
    state->shuffle_remaining &= valid_mask;
    if (state->shuffle_remaining == 0u) {
        state->shuffle_remaining = valid_mask;
        refilled = true;
    }

    u16 selection_mask = state->shuffle_remaining;
    if (refilled && variant_count > 1u && state->previous_index < variant_count) {
        selection_mask &= (u16) ~((u16)1u << state->previous_index);
    }

    u16 bit = 1u;
    for (u8 i = 0u; i < variant_count; ++i, bit = (u16)(bit << 1u)) {
        if ((selection_mask & bit) != 0u) {
            ++remaining_count;
        }
    }
    u8 selected_remaining = (u8)(unsigned_math_random_u16(random_state) % remaining_count);
    bit = 1u;
    for (u8 i = 0u; i < variant_count; ++i, bit = (u16)(bit << 1u)) {
        if ((selection_mask & bit) == 0u) {
            continue;
        }
        if (selected_remaining == 0u) {
            state->shuffle_remaining &= (u16)~bit;
            return i;
        }
        --selected_remaining;
    }
    return 0u; /* Authored non-empty masks always select inside the loop above. */
}

/** Selects the next audio variant in round-robin order. */
static u8 audio_select_round_robin(u8 variant_count, UAudioSelectionState *state) {
    u8 selected = state->next_index;
    if (selected >= variant_count) {
        selected = 0u;
    }
    state->next_index = (u8)(selected + 1u);
    return selected;
}

u8 unsigned_audio_resolve_selection(UAudioSelectionMode mode, const UAudioVariant *variants, u8 variant_count, UAudioSelectionState *state, u16 *random_state) {
    switch (mode) {
    case U_AUDIO_SELECT_FIRST:
        return 0u;
    case U_AUDIO_SELECT_RANDOM:
        return audio_select_random(variant_count, random_state);
    case U_AUDIO_SELECT_RANDOM_NO_REPEAT:
        return audio_select_random_no_repeat(variant_count, state, random_state);
    case U_AUDIO_SELECT_WEIGHTED:
        return audio_select_weighted(variants, variant_count, random_state);
    case U_AUDIO_SELECT_SHUFFLE:
        return audio_select_shuffle(variant_count, state, random_state);
    case U_AUDIO_SELECT_ROUND_ROBIN:
        return audio_select_round_robin(variant_count, state);
    case U_AUDIO_SELECT_COUNT:
    default:
        U_UNREACHABLE();
    }
}

bool unsigned_audio_evaluate_condition(UAudioConditionOperator operation, s16 actual, s16 expected) {
    switch (operation) {
    case U_AUDIO_CONDITION_EQUAL:
        return actual == expected;
    case U_AUDIO_CONDITION_LESS_THAN:
        return actual < expected;
    case U_AUDIO_CONDITION_AT_LEAST:
        return actual >= expected;
    case U_AUDIO_CONDITION_TRUE:
        return actual != 0;
    case U_AUDIO_CONDITION_COUNT:
    default:
        U_UNREACHABLE();
    }
}
