/**
 * @file render_plan.h
 * @brief CPU-side frame plan and optional VRAM budget instrumentation shared by renderers.
 */

#ifndef UNSIGNED_RENDERER_RENDER_PLAN_H
#define UNSIGNED_RENDERER_RENDER_PLAN_H

#include "core/types.h"
#include "renderer/config.h"

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

/** Optional per-frame diagnostics. Runtime builds do not pay to maintain these counters. */
typedef struct URenderFrameStats {
    u32 estimated_vram_words[U_RENDER_PRIORITY_COUNT];
    u32 estimated_vram_words_total;
    u16 actor_relocations;
    u16 actor_sprite_columns;
    u16 background_column_uploads;
    u16 background_full_column_uploads;
    /** Highest number of active Neo Geo hardware sprite columns measured on one visible scanline. */
    u16 max_sprite_columns_per_scanline;
    /** Number of visible scanlines whose active hardware sprite count exceeds the Neo Geo limit. */
    u16 sprite_overflow_scanlines;
} URenderFrameStats;

/** Common header embedded by concrete render plans. */
typedef struct URenderPlan {
    URenderFrameStats stats;
    /** True when at least one hardware write is required by the prepared frame. */
    bool has_work;
} URenderPlan;

/** Reset one frame's small metadata header before CPU-side preparation starts. */
static inline void unsigned_render_plan_reset(URenderPlan *plan) {
    plan->stats = (URenderFrameStats){0};
    plan->has_work = false;
}

/** Mark a frame as requiring a hardware transaction without calculating diagnostic costs. */
static inline void unsigned_render_plan_mark_work(URenderPlan *plan) {
    plan->has_work = true;
}

/**
 * @brief Adds one estimated VRAM cost and marks the frame as containing hardware work.
 * @pre `priority < U_RENDER_PRIORITY_COUNT`.
 */
static inline void unsigned_render_plan_add_words(URenderPlan *plan, URenderPriority priority, u32 words) {
    if (words == 0u) {
        return;
    }
    plan->has_work = true;
#if UNSIGNED_RENDERER_DIAGNOSTICS
    plan->stats.estimated_vram_words[priority] += words;
    plan->stats.estimated_vram_words_total += words;
#else
    (void)priority;
#endif
}

#endif
