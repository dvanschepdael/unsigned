/**
 * @file tlss.c
 * @brief Implements temporal level-of-simulation scheduling.
 */

#include "core/tlss/tlss.h"

/** Converts a power-of-two TLSS scale into the frame mask used by schedule tests. */
static u8 unsigned_tlss_get_mask(UTLSSScale scale) {
    return (u8)((1U << (u8)scale) - 1U);
}

/** Returns the number of frames represented by one scheduling period for the selected scale. */
static u16 unsigned_tlss_get_period(UTLSSScale scale) {
    return (u16)(1U << (u16)scale);
}

/** Distributes peer phases across a cadence so deferred work is not clustered on one frame. */
static u8 unsigned_tlss_distribute_phase(u16 index, u16 count, UTLSSScale scale) {
    if (count == 1U) {
        return 0U;
    }

    const u16 period = unsigned_tlss_get_period(scale);
    const u32 value = (((u32)index * (u32)period) + (u32)count - 1U) / (u32)count;
    return (u8)(value & (u32)(period - 1U));
}

void unsigned_tlss_init(UTLSS *tlss) {
    tlss->frame = UINT16_MAX;
    tlss->scales = (UTLSSScaleConfig){
        .ai = U_TLSS_SCALE_1,
        .collision = U_TLSS_SCALE_1,
    };
}

void unsigned_tlss_begin_frame(UTLSS *tlss) {
    ++tlss->frame;
}

void unsigned_tlss_node_init(const UTLSS *tlss, UTLSSNode *node, u16 index, u16 count, UTLSSScale scale) {
    node->mask = unsigned_tlss_get_mask(scale);
    node->phase = unsigned_tlss_distribute_phase(index, count, scale);
    node->last_tick = tlss->frame;
}

void unsigned_tlss_node_set_scale_slot(UTLSSNode *node, u16 slot, UTLSSScale scale) {
    node->mask = unsigned_tlss_get_mask(scale);
    node->phase = (u8)slot & node->mask;
}

bool unsigned_tlss_node_matches_scale(const UTLSSNode *node, UTLSSScale scale) {
    return node->mask == unsigned_tlss_get_mask(scale);
}

bool unsigned_tlss_should_tick(const UTLSS *tlss, const UTLSSNode *node) {
    return ((u8)tlss->frame & node->mask) == node->phase;
}

u16 unsigned_tlss_tick(const UTLSS *tlss, UTLSSNode *node) {
    const u16 delta = (u16)(tlss->frame - node->last_tick);
    node->last_tick = tlss->frame;
    return delta;
}
