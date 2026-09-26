/**
 * @file level_renderer.h
 * @brief Coordinates CPU-side render preparation and prioritized Neo Geo VRAM commits.
 */

#ifndef UNSIGNED_RENDERER_LEVEL_RENDERER_H
#define UNSIGNED_RENDERER_LEVEL_RENDERER_H

#include "level/level_definition.h"
#include "renderer/actor_renderer.h"
#include "renderer/background_renderer.h"
#include "renderer/render_plan.h"
#include "renderer/sprite_column_plan.h"

typedef struct ULevelRenderPlan {
    URenderPlan frame;
    USpriteColumnPlanBuffer sprite_columns;
    UBackgroundRenderPlan background;
    UActorRenderPlan actors;
    ULevel *level;
    u16 actor_first_sprite;
    u16 actor_sprite_count;
    u16 clear_actor_first_sprite;
    u16 clear_actor_sprite_count;
} ULevelRenderPlan;

typedef struct ULevelRenderer {
    UBackgroundRenderState background;
    ULevelRenderPlan plan;
    const ULevelDefinition *rendered_definition;
    u16 actor_first_sprite;
    u16 rendered_actor_first_sprite;
    u16 rendered_actor_sprite_count;
    u16 rendered_actor_layout_revision;
} ULevelRenderer;

/** Initialize an empty renderer with no hardware ranges owned. */
void unsigned_level_renderer_init(ULevelRenderer *renderer);

/**
 * Prepare one complete level frame on the CPU without touching VRAM.
 * Culling, actor allocation, relocation detection, effect sampling and background upload planning
 * all happen here during active display.
 */
void unsigned_level_renderer_prepare(ULevelRenderer *renderer, ULevel *level, const UViewport *viewport);

/**
 * Commit the previously prepared frame in critical/high/normal priority order.
 * @pre `renderer` is valid and `unsigned_level_renderer_prepare()` was called for this frame.
 */
void unsigned_level_renderer_commit(ULevelRenderer *renderer);

/** Hide all currently owned background and actor hardware sprites. */
void unsigned_level_renderer_hide(ULevelRenderer *renderer);

#endif
