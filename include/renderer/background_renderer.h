/**
 * @file background_renderer.h
 * @brief Maintains hardware sprite-column ring buffers for scrolling background layers.
 */

#ifndef UNSIGNED_RENDERER_BACKGROUND_RENDERER_H
#define UNSIGNED_RENDERER_BACKGROUND_RENDERER_H

#include "display/viewport/viewport.h"
#include "level/background/background.h"
#include "renderer/render_plan.h"

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

typedef enum UBackgroundTransformPlanMode {
    U_BACKGROUND_TRANSFORM_PLAN_NONE = 0,
    U_BACKGROUND_TRANSFORM_PLAN_CHAINED,
    U_BACKGROUND_TRANSFORM_PLAN_PER_COLUMN,
} UBackgroundTransformPlanMode;

typedef struct UBackgroundColumnUploadPlan {
    u8 physical_slot;
    u8 source_column;
    bool full;
} UBackgroundColumnUploadPlan;

/** One layer's immutable CPU-side work list, built before VBlank and committed afterward. */
typedef struct UBackgroundLayerRenderPlan {
    const UBackgroundLayer *layer;
    UBackgroundLayerRenderState next_state;
    UBackgroundColumnUploadPlan uploads[UNSIGNED_BACKGROUND_RENDER_MAX_COLUMNS];
    UEffectSample effect_samples[UNSIGNED_BACKGROUND_RENDER_MAX_COLUMNS];
    u16 hide_first_sprite;
    u16 scb2;
    s16 base_x;
    s16 y;
    u8 hide_columns;
    u8 columns;
    u8 leftmost_slot;
    u8 upload_count;
    UBackgroundTransformPlanMode transform_mode;
    bool hide_old_range;
    bool shrink_changed;
    bool layout_changed;
    bool x_changed;
    bool y_changed;
    bool valid;
} UBackgroundLayerRenderPlan;

typedef struct UBackgroundRenderPlan {
    UBackgroundLayerRenderPlan layers[UNSIGNED_BACKGROUND_MAX_LAYERS];
    bool valid;
} UBackgroundRenderPlan;

/** Initialize every layer cache as empty/invalid. */
void unsigned_background_renderer_init(UBackgroundRenderState *state);

/** Return the number of 16-pixel sprite columns needed to cover the viewport plus scroll margin. */
u8 unsigned_background_renderer_column_count(const UBackgroundLayer *layer, const UViewport *viewport);

/** Build all background upload/transform commands on the CPU without touching VRAM. */
bool unsigned_background_renderer_prepare(const UBackgroundRenderState *state, const UBackground *background, const UViewport *viewport,
                                         UBackgroundRenderPlan *plan, URenderPlan *frame_plan);

/** Commit a previously prepared background plan and publish its next hardware mirror state. */
void unsigned_background_renderer_commit(UBackgroundRenderState *state, const UBackgroundRenderPlan *plan);

/** Compatibility helper: prepare and commit immediately. */
void unsigned_background_renderer_draw(UBackgroundRenderState *state, const UBackground *background, const UViewport *viewport);

/** Hide all owned background sprite ranges and reset their cached ring-buffer state. */
void unsigned_background_renderer_hide(UBackgroundRenderState *state);

#endif
