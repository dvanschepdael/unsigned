/**
 * @file level_renderer.h
 * @brief Coordinates background rendering and dynamic hardware-sprite allocation for level actors.
 */

#ifndef UNSIGNED_RENDERER_LEVEL_RENDERER_H
#define UNSIGNED_RENDERER_LEVEL_RENDERER_H

#include "level/level_definition.h"
#include "renderer/background_renderer.h"

typedef struct ULevelRenderer {
    UBackgroundRenderState background;
    const ULevelDefinition *rendered_definition;
    u16 actor_first_sprite;
    u16 rendered_actor_first_sprite;
    u16 rendered_actor_sprite_count;
    bool actor_first_sprite_valid;
    bool actor_layout_complete;
} ULevelRenderer;

/** Initialize an empty renderer with no hardware ranges owned. */
void unsigned_level_renderer_init(ULevelRenderer *renderer);

/** Render one level frame: background first, then prepare/layout/draw visible actors in depth order. */
void unsigned_level_renderer_render(ULevelRenderer *renderer, ULevel *level, const UViewport *viewport);

/** Hide all currently owned background and actor hardware sprites. */
void unsigned_level_renderer_hide(ULevelRenderer *renderer);

#endif
