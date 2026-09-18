/**
 * @file render_plan.h
 * @brief CPU-side frame plan and VRAM budget instrumentation shared by renderers.
 */

#ifndef UNSIGNED_RENDERER_RENDER_PLAN_H
#define UNSIGNED_RENDERER_RENDER_PLAN_H

#include "core/types.h"

typedef enum URenderPriority {
    /** Chain boundaries and stale-range clears that must land first after VBlank. */
    U_RENDER_PRIORITY_CRITICAL = 0,
    /** Actor graphics/transforms and other presentation that must complete this frame. */
    U_RENDER_PRIORITY_HIGH,
    /** Background scrolling/column uploads that may follow actor commits. */
    U_RENDER_PRIORITY_NORMAL,
    /** Reserved for future work that can safely be postponed to a later frame. */
    U_RENDER_PRIORITY_DEFERRED,
    U_RENDER_PRIORITY_COUNT,
} URenderPriority;

/** Per-frame CPU-side estimate of the words that the commit phase will write to VRAM. */
typedef struct URenderFrameStats {
    u32 estimated_vram_words[U_RENDER_PRIORITY_COUNT];
    u32 estimated_vram_words_total;
    u16 actor_relocations;
    u16 actor_sprite_columns;
    u16 background_column_uploads;
    u16 background_full_column_uploads;
} URenderFrameStats;

/** Common header embedded by concrete render plans. */
typedef struct URenderPlan {
    URenderFrameStats stats;
    bool valid;
} URenderPlan;

/** Reset one frame's plan metadata before CPU-side preparation starts. */
static inline void unsigned_render_plan_reset(URenderPlan *plan) {
    if (plan != NULL) {
        *plan = (URenderPlan){ 0 };
    }
}

/** Add one estimated VRAM-word cost to a priority bucket and the frame total. */
static inline void unsigned_render_plan_add_words(URenderPlan *plan, URenderPriority priority, u32 words) {
    if (plan == NULL || priority >= U_RENDER_PRIORITY_COUNT || words == 0u) {
        return;
    }
    plan->stats.estimated_vram_words[priority] += words;
    plan->stats.estimated_vram_words_total += words;
}

#endif
