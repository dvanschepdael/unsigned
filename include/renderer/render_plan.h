/**
 * @file render_plan.h
 * @brief CPU-side frame work marker shared by renderers.
 */

#ifndef UNSIGNED_RENDERER_RENDER_PLAN_H
#define UNSIGNED_RENDERER_RENDER_PLAN_H

#include "core/types.h"

/** Common header embedded by concrete render plans. */
typedef struct URenderPlan {
    /** True when at least one hardware write is required by the prepared frame. */
    bool has_work;
} URenderPlan;

/** Reset one frame's small metadata header before CPU-side preparation starts. */
static inline void unsigned_render_plan_reset(URenderPlan *plan) {
    plan->has_work = false;
}

/** Mark a frame as requiring a hardware transaction. */
static inline void unsigned_render_plan_mark_work(URenderPlan *plan) {
    plan->has_work = true;
}

#endif
