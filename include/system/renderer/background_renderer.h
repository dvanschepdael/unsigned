/**
 * @file background_renderer.h
 * @brief Maintains hardware sprite-column ring buffers for scrolling background layers.
 */

#ifndef UNSIGNED_SYSTEM_RENDERER_BACKGROUND_RENDERER_H
#define UNSIGNED_SYSTEM_RENDERER_BACKGROUND_RENDERER_H

#include "display/viewport/viewport.h"
#include "level/background/background.h"

typedef struct UBackgroundLayerRenderState {
    const UBackgroundLayerDefinition *rendered_definition;
    /** Source column currently stored in each physical ring slot; UINT16_MAX means unknown. */
    u16 loaded_source_columns[UNSIGNED_BACKGROUND_RENDER_MAX_COLUMNS];
    u16 rendered_first_sprite;
    u16 rendered_scb2;
    s32 rendered_source_start;
    s16 rendered_x;
    s16 rendered_y;
    u8 rendered_columns;
    u8 rendered_leftmost_slot;
    bool rendered;
    bool graphics_dirty;
    bool position_valid;
    bool shrink_valid;
    bool chain_valid;
    bool source_valid;
} UBackgroundLayerRenderState;

typedef struct UBackgroundRenderState {
    UBackgroundLayerRenderState layers[UNSIGNED_BACKGROUND_MAX_LAYERS];
} UBackgroundRenderState;

/** Initialize every layer cache as empty/invalid. */
void unsigned_background_renderer_init(UBackgroundRenderState *state);

/** Return the number of 16-pixel sprite columns needed to cover the viewport plus scroll margin. */
u8 unsigned_background_renderer_column_count(const UBackgroundLayer *layer, const UViewport *viewport);

/** Synchronize and draw all active background layers for the current viewport. */
void unsigned_background_renderer_draw(UBackgroundRenderState *state, const UBackground *background, const UViewport *viewport);

/** Hide all owned background sprite ranges and reset their cached ring-buffer state. */
void unsigned_background_renderer_hide(UBackgroundRenderState *state);

#endif
