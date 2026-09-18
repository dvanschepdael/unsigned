/**
 * @file tlss.c
 * @brief Implements temporal level-of-simulation scheduling.
 */

#include "core/tlss/tlss.h"

/** Accepts only the temporal scales represented by the public TLSS scale enum. */
static bool unsigned_tlss_scale_is_valid(UTLSSScale scale) {
    return (unsigned int)scale <= (unsigned int)U_TLSS_SCALE_16;
}

bool unsigned_tlss_scale_config_is_valid(const UTLSSScaleConfig *config) {
    return config != NULL && unsigned_tlss_scale_is_valid(config->ai) && unsigned_tlss_scale_is_valid(config->collision);
}

/** Converts a power-of-two TLSS scale into the frame mask used by schedule tests. */
static u8 unsigned_tlss_get_mask(UTLSSScale scale) {
    return (u8)((1U << (u8)scale) - 1U);
}

/** Returns the number of frames represented by one scheduling period for the selected scale. */
static u16 unsigned_tlss_get_period(UTLSSScale scale) {
    return (u16)(1U << (u16)scale);
}

/** Distributes node phase offsets across the selected TLSS cadence to avoid clustered work. */
static u8 unsigned_tlss_distribute_phase(u16 index, u16 count, UTLSSScale scale) {
    if (count == 0U) {
        return 0U;
    }

    if (count == 1U) {
        return 0U;
    }

    u16 period = unsigned_tlss_get_period(scale);
    u32 value = ((u32)index * (u32)period) + (u32)count - 1U;
    value /= (u32)count;

    return (u8)(value & (u32)(period - 1U));
}

void unsigned_tlss_init(UTLSS *tlss) {
    if (tlss == NULL) {
        return;
    }

    tlss->frame = UINT16_MAX;
    tlss->scales = (UTLSSScaleConfig){
        .ai = U_TLSS_SCALE_1,
        .collision = U_TLSS_SCALE_1,
    };
}

void unsigned_tlss_begin_frame(UTLSS *tlss) {
    if (tlss == NULL) {
        return;
    }

    ++tlss->frame;
}

void unsigned_tlss_node_init(const UTLSS *tlss, UTLSSNode *node, u16 index, u16 count, UTLSSScale scale) {
    if (tlss == NULL || node == NULL || !unsigned_tlss_scale_is_valid(scale)) {
        return;
    }

    node->mask = unsigned_tlss_get_mask(scale);

    node->phase = unsigned_tlss_distribute_phase(index, count, scale);

    node->last_tick = tlss->frame;
}

void unsigned_tlss_node_set_scale(UTLSSNode *node, u16 index, u16 count, UTLSSScale scale) {
    if (node == NULL || !unsigned_tlss_scale_is_valid(scale)) {
        return;
    }

    node->mask = unsigned_tlss_get_mask(scale);
    node->phase = unsigned_tlss_distribute_phase(index, count, scale);
}

void unsigned_tlss_node_set_scale_slot(UTLSSNode *node, u16 slot, UTLSSScale scale) {
    if (node == NULL || !unsigned_tlss_scale_is_valid(scale)) {
        return;
    }

    node->mask = unsigned_tlss_get_mask(scale);
    node->phase = (u8)slot & node->mask;
}

bool unsigned_tlss_node_matches_scale(const UTLSSNode *node, UTLSSScale scale) {
    return node != NULL && unsigned_tlss_scale_is_valid(scale) && node->mask == unsigned_tlss_get_mask(scale);
}

bool unsigned_tlss_should_tick(const UTLSS *tlss, const UTLSSNode *node) {
    if (tlss == NULL || node == NULL) {
        return false;
    }

    return ((u8)tlss->frame & node->mask) == node->phase;
}

u16 unsigned_tlss_tick(const UTLSS *tlss, UTLSSNode *node) {
    if (tlss == NULL || node == NULL) {
        return 0;
    }

    u16 delta = (u16)(tlss->frame - node->last_tick);
    node->last_tick = tlss->frame;
    return delta;
}
