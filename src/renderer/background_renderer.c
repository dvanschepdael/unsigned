/**
 * @file background_renderer.c
 * @brief Orchestrates scrolling background layers and their Neo Geo sprite-column ring buffers.
 */

#include "renderer/background_renderer.h"

#include "display/sprite/limits.h"
#include "system/background_backend.h"

/** Floor-divide pixels by one 16-pixel tile while returning a positive pixel remainder. */
static s32 background_renderer_floor_div_16(s32 value, u8 *remainder) {
    s32 quotient;
    u8 rest;

    if (value >= 0) {
        const u32 positive = (u32)value;
        quotient = (s32)(positive >> 4u);
        rest = (u8)(positive & 15u);
    } else {
        const u32 magnitude = (u32)(-(value + 1)) + 1u;
        const u8 magnitude_rest = (u8)(magnitude & 15u);

        quotient = -(s32)(magnitude >> 4u);
        if (magnitude_rest != 0u) {
            --quotient;
            rest = (u8)(16u - magnitude_rest);
        } else {
            rest = 0u;
        }
    }

    *remainder = rest;
    return quotient;
}

/** Convert fixed/pixel scrolling into the first visible source tile and sub-tile offset. */
static s32 background_renderer_source_start(const UBackgroundLayer *layer, u8 *pixel_offset) {
    const s32 pixel_scroll = unsigned_background_fixed_to_pixels(layer->scroll_fixed);
    return background_renderer_floor_div_16(pixel_scroll, pixel_offset);
}

static u8 background_renderer_column_count(const UBackgroundLayer *layer, const UViewport *viewport) {
    u32 columns = (((u32)viewport->width + 15u) >> 4u) + 1u;
    if (columns > layer->definition->width_tiles) {
        columns = layer->definition->width_tiles;
    }
    return (u8)columns;
}

static void background_renderer_invalidate_columns(UBackgroundLayerRenderState *render) {
    for (u8 i = 0u; i < UNSIGNED_BACKGROUND_RENDER_MAX_COLUMNS; ++i) {
        render->loaded_source_columns[i] = UINT16_MAX;
    }
}

static void background_renderer_reset_state(UBackgroundLayerRenderState *render) {
    *render = (UBackgroundLayerRenderState){0};
    background_renderer_invalidate_columns(render);
}

static u8 background_renderer_leftmost_slot(UBackgroundLayerRenderState *render, u8 columns, s32 source_start) {
    if (render->rendered_definition == NULL || render->rendered_columns != columns) {
        return 0u;
    }

    s32 delta = source_start - render->rendered_source_start;
    if (delta >= (s32)columns || delta <= -(s32)columns) {
        background_renderer_invalidate_columns(render);
        return 0u;
    }

    u8 slot = render->rendered_leftmost_slot;
    while (delta > 0) {
        ++slot;
        if (slot == columns) {
            slot = 0u;
        }
        --delta;
    }
    while (delta < 0) {
        if (slot == 0u) {
            slot = columns;
        }
        --slot;
        ++delta;
    }
    return slot;
}

static void background_renderer_prepare_uploads(const UBackgroundLayer *layer, UBackgroundLayerRenderState *next, UBackgroundLayerRenderPlan *plan, URenderPlan *frame_plan) {
    const UBackgroundLayerDefinition *definition = layer->definition;
    u8 slot = plan->leftmost_slot;
    u8 source_column = (u8)(layer->scroll_pixels >> 4u);

    for (u8 screen_column = 0u; screen_column < plan->columns; ++screen_column) {
        if (next->loaded_source_columns[slot] != source_column) {
            const bool full = next->loaded_source_columns[slot] == UINT16_MAX;
            UBackgroundColumnUploadPlan *upload = &plan->uploads[plan->upload_count++];
            upload->physical_slot = slot;
            upload->source_column = source_column;
            upload->full = full;
            next->loaded_source_columns[slot] = source_column;

#if UNSIGNED_RENDERER_DIAGNOSTICS
            ++frame_plan->stats.background_column_uploads;
            if (full) {
                ++frame_plan->stats.background_full_column_uploads;
            }
            unsigned_render_plan_add_words(frame_plan, U_RENDER_PRIORITY_NORMAL, (u32)definition->height_tiles * (full ? 2u : 1u));
#else
            unsigned_render_plan_mark_work(frame_plan);
#endif
        }

        ++slot;
        if (slot == plan->columns) {
            slot = 0u;
        }
        ++source_column;
        if (source_column == definition->width_tiles) {
            source_column = 0u;
        }
    }
}

static void background_renderer_prepare_chained(UBackgroundLayerRenderState *next, UBackgroundLayerRenderPlan *plan, s16 x, s16 y, s16 zoom_offset, URenderPlan *frame_plan) {
    const u16 scb2 = unsigned_background_backend_encode_scb2(zoom_offset);
    u8 transform_dirty = 0u;
    if (!next->chained_valid || next->rendered_scb2 != scb2) {
        transform_dirty |= U_BACKGROUND_TRANSFORM_DIRTY_SHRINK;
    }
    if (!next->chained_valid || next->rendered_columns != plan->columns || next->rendered_leftmost_slot != plan->leftmost_slot) {
        transform_dirty |= U_BACKGROUND_TRANSFORM_DIRTY_LAYOUT;
    }
    if (!next->chained_valid || next->rendered_x != x) {
        transform_dirty |= U_BACKGROUND_TRANSFORM_DIRTY_X;
    }
    if (!next->chained_valid || next->rendered_y != y) {
        transform_dirty |= U_BACKGROUND_TRANSFORM_DIRTY_Y;
    }
#if UNSIGNED_RENDERER_DIAGNOSTICS
    const u8 driver_count = plan->leftmost_slot != 0u ? 2u : 1u;
    u32 words = 0u;
#endif

    plan->transform_mode = U_BACKGROUND_TRANSFORM_PLAN_CHAINED;
    plan->base_x = x;
    plan->y = y;
    plan->scb2 = scb2;
    plan->transform_dirty = transform_dirty;

    if (transform_dirty != 0u) {
#if UNSIGNED_RENDERER_DIAGNOSTICS
        if ((transform_dirty & U_BACKGROUND_TRANSFORM_DIRTY_SHRINK) != 0u) {
            words += plan->columns;
        }
        if ((transform_dirty & U_BACKGROUND_TRANSFORM_DIRTY_LAYOUT) != 0u) {
            words += plan->columns + driver_count;
        } else {
            if ((transform_dirty & U_BACKGROUND_TRANSFORM_DIRTY_Y) != 0u) {
                words += driver_count;
            }
            if ((transform_dirty & U_BACKGROUND_TRANSFORM_DIRTY_X) != 0u) {
                words += driver_count;
            }
        }
        unsigned_render_plan_add_words(frame_plan, U_RENDER_PRIORITY_NORMAL, words);
#else
        unsigned_render_plan_mark_work(frame_plan);
#endif
    }

    next->rendered_scb2 = scb2;
    next->rendered_leftmost_slot = plan->leftmost_slot;
    next->rendered_x = x;
    next->rendered_y = y;
    next->rendered_columns = plan->columns;
    next->chained_valid = true;
}

static void background_renderer_prepare_per_column(const UViewport *viewport, UBackgroundLayerRenderState *next, UBackgroundLayerRenderPlan *plan, URenderPlan *frame_plan) {
    plan->transform_mode = U_BACKGROUND_TRANSFORM_PLAN_PER_COLUMN;
    s32 column_x = plan->base_x;
    for (u8 column = 0u; column < plan->columns; ++column) {
        const UEffectSample sampled = unsigned_effect_sample(&viewport->effect, column, plan->columns);
        unsigned_background_backend_encode_prepared_column(&plan->effect_columns[column], sampled.zoom_offset, (s16)(column_x + sampled.offset_x), (s16)((s32)plan->y + sampled.offset_y), plan->layer->definition->height_tiles);
        column_x += 16;
    }
#if UNSIGNED_RENDERER_DIAGNOSTICS
    unsigned_render_plan_add_words(frame_plan, U_RENDER_PRIORITY_NORMAL, (u32)plan->columns * 3u);
#else
    unsigned_render_plan_mark_work(frame_plan);
#endif

    next->chained_valid = false;
}

/** Reset only per-frame metadata; large upload/effect arrays are overwritten up to their counts. */
static void background_renderer_reset_plan(UBackgroundRenderPlan *plan) {
    for (u8 i = 0u; i < UNSIGNED_BACKGROUND_MAX_LAYERS; ++i) {
        UBackgroundLayerRenderPlan *layer_plan = &plan->layers[i];
        layer_plan->layer = NULL;
        layer_plan->hide_first_sprite = 0u;
        layer_plan->scb2 = 0u;
        layer_plan->base_x = 0;
        layer_plan->y = 0;
        layer_plan->hide_columns = 0u;
        layer_plan->columns = 0u;
        layer_plan->leftmost_slot = 0u;
        layer_plan->upload_count = 0u;
        layer_plan->transform_mode = U_BACKGROUND_TRANSFORM_PLAN_NONE;
        layer_plan->transform_dirty = 0u;
    }
}

void unsigned_background_renderer_init(UBackgroundRenderState *state) {
    *state = (UBackgroundRenderState){0};
    for (u8 i = 0u; i < UNSIGNED_BACKGROUND_MAX_LAYERS; ++i) {
        background_renderer_invalidate_columns(&state->layers[i]);
    }
}

void unsigned_background_renderer_prepare(const UBackgroundRenderState *state, const UBackground *background, const UViewport *viewport, UBackgroundRenderPlan *plan, URenderPlan *frame_plan) {
    background_renderer_reset_plan(plan);

    for (u8 i = 0u; i < UNSIGNED_BACKGROUND_MAX_LAYERS; ++i) {
        const UBackgroundLayer *layer = &background->layers[i];
        const UBackgroundLayerRenderState *current = &state->layers[i];
        UBackgroundLayerRenderPlan *layer_plan = &plan->layers[i];
        UBackgroundLayerRenderState *next = &layer_plan->next_state;

        *next = *current;
        layer_plan->layer = layer;

        if (layer->definition == NULL) {
            if (current->rendered_definition != NULL) {
                layer_plan->hide_first_sprite = current->rendered_first_sprite;
                layer_plan->hide_columns = current->rendered_columns;
#if UNSIGNED_RENDERER_DIAGNOSTICS
                unsigned_render_plan_add_words(frame_plan, U_RENDER_PRIORITY_NORMAL, current->rendered_columns);
#else
                unsigned_render_plan_mark_work(frame_plan);
#endif
            }
            background_renderer_reset_state(next);
            continue;
        }

        layer_plan->columns = background_renderer_column_count(layer, viewport);
        if (layer_plan->columns == 0u) {
            continue;
        }

        if (current->rendered_definition != NULL && (current->rendered_definition != layer->definition || current->rendered_first_sprite != layer->definition->first_sprite || current->rendered_columns != layer_plan->columns)) {
            layer_plan->hide_first_sprite = current->rendered_first_sprite;
            layer_plan->hide_columns = current->rendered_columns;
#if UNSIGNED_RENDERER_DIAGNOSTICS
            unsigned_render_plan_add_words(frame_plan, U_RENDER_PRIORITY_NORMAL, current->rendered_columns);
#else
            unsigned_render_plan_mark_work(frame_plan);
#endif
            background_renderer_reset_state(next);
        } else if (current->rendered_definition == NULL) {
            background_renderer_invalidate_columns(next);
        }

        u8 pixel_offset;
        const s32 source_start = background_renderer_source_start(layer, &pixel_offset);
        layer_plan->leftmost_slot = background_renderer_leftmost_slot(next, layer_plan->columns, source_start);
        layer_plan->base_x = (s16)(viewport->x + layer->definition->screen_position.x - pixel_offset);
        layer_plan->y = (s16)(viewport->y + layer->definition->screen_position.y);

        background_renderer_prepare_uploads(layer, next, layer_plan, frame_plan);

        if (viewport->effect.function == NULL) {
            background_renderer_prepare_chained(next, layer_plan, layer_plan->base_x, layer_plan->y, 0, frame_plan);
        } else if (viewport->effect.layout == U_EFFECT_LAYOUT_UNIFORM) {
            const UEffectSample effect = unsigned_effect_sample(&viewport->effect, 0u, layer_plan->columns);
            background_renderer_prepare_chained(next, layer_plan, (s16)(layer_plan->base_x + effect.offset_x), (s16)(layer_plan->y + effect.offset_y), effect.zoom_offset, frame_plan);
        } else {
            background_renderer_prepare_per_column(viewport, next, layer_plan, frame_plan);
        }

        next->rendered_first_sprite = layer->definition->first_sprite;
        next->rendered_definition = layer->definition;
        next->rendered_source_start = source_start;
        next->rendered_leftmost_slot = layer_plan->leftmost_slot;
        next->rendered_columns = layer_plan->columns;
    }
}

void unsigned_background_renderer_commit(UBackgroundRenderState *state, const UBackgroundRenderPlan *plan) {
    for (u8 i = 0u; i < UNSIGNED_BACKGROUND_MAX_LAYERS; ++i) {
        const UBackgroundLayerRenderPlan *layer_plan = &plan->layers[i];
        if (layer_plan->hide_columns != 0u) {
            unsigned_background_backend_hide_range(layer_plan->hide_first_sprite, layer_plan->hide_columns);
        }

        if (layer_plan->layer->definition != NULL) {
            for (u8 upload_index = 0u; upload_index < layer_plan->upload_count; ++upload_index) {
                const UBackgroundColumnUploadPlan *upload = &layer_plan->uploads[upload_index];
                unsigned_background_backend_write_column(layer_plan->layer->definition, upload->physical_slot, upload->source_column, upload->full);
            }

            if (layer_plan->transform_mode == U_BACKGROUND_TRANSFORM_PLAN_CHAINED) {
                unsigned_background_backend_flush_chained(layer_plan->layer->definition, layer_plan->columns, layer_plan->leftmost_slot, layer_plan->base_x, layer_plan->y, layer_plan->scb2, layer_plan->transform_dirty);
            } else if (layer_plan->transform_mode == U_BACKGROUND_TRANSFORM_PLAN_PER_COLUMN) {
                unsigned_background_backend_write_prepared_effect_columns(layer_plan->layer->definition, layer_plan->columns, layer_plan->leftmost_slot, layer_plan->effect_columns);
            }
        }

        state->layers[i] = layer_plan->next_state;
    }
}

void unsigned_background_renderer_hide(UBackgroundRenderState *state) {
    for (u8 i = 0u; i < UNSIGNED_BACKGROUND_MAX_LAYERS; ++i) {
        UBackgroundLayerRenderState *render = &state->layers[i];
        if (render->rendered_definition != NULL) {
            unsigned_background_backend_hide_range(render->rendered_first_sprite, render->rendered_columns);
        }
        background_renderer_reset_state(render);
    }
}
