/**
 * @file level_renderer.h
 * @brief Coordinates CPU-side render preparation and prioritized Neo Geo VRAM commits.
 */

#ifndef UNSIGNED_RENDERER_LEVEL_RENDERER_H
#define UNSIGNED_RENDERER_LEVEL_RENDERER_H

#include "level/level_definition.h"
#include "renderer/background_renderer.h"
#include "renderer/render_plan.h"

typedef struct ULevelRenderPlan {
    URenderPlan frame;
    UBackgroundRenderPlan background;
    ULevel *level;
    const UViewport *viewport;
    const ULevelDefinition *definition;
    u16 actor_first_sprite;
    u16 actor_sprite_count;
    u16 clear_actor_first_sprite;
    u16 clear_actor_sprite_count;
    bool clear_actor_range;
    bool definition_changed;
    bool actor_layout_complete;
    bool hide_all;
} ULevelRenderPlan;

typedef struct ULevelRenderer {
    UBackgroundRenderState background;
    ULevelRenderPlan plan;
    const ULevelDefinition *rendered_definition;
    u16 actor_first_sprite;
    u16 rendered_actor_first_sprite;
    u16 rendered_actor_sprite_count;
    bool actor_first_sprite_valid;
    bool actor_layout_complete;
} ULevelRenderer;

/** Initialize an empty renderer with no hardware ranges owned. */
void unsigned_level_renderer_init(ULevelRenderer *renderer);

/**
 * Prepare one complete level frame on the CPU without touching VRAM.
 * Culling, actor allocation, relocation detection, effect sampling and background upload planning
 * all happen here during active display.
 */
bool unsigned_level_renderer_prepare(ULevelRenderer *renderer, ULevel *level, const UViewport *viewport);

/** Commit the previously prepared frame in critical/high/normal priority order. */
void unsigned_level_renderer_commit(ULevelRenderer *renderer);

/** Compatibility helper: prepare and commit one level frame immediately. */
void unsigned_level_renderer_render(ULevelRenderer *renderer, ULevel *level, const UViewport *viewport);

/** Latest prepared frame's VRAM budget/relocation instrumentation, or NULL for a NULL renderer. */
const URenderFrameStats *unsigned_level_renderer_stats(const ULevelRenderer *renderer);

/** Return true while a CPU-side plan is waiting for its post-VBlank commit. */
bool unsigned_level_renderer_has_prepared_frame(const ULevelRenderer *renderer);

/** Hide all currently owned background and actor hardware sprites. */
void unsigned_level_renderer_hide(ULevelRenderer *renderer);

#endif
