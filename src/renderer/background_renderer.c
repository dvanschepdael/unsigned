/**
 * @file background_renderer.c
 * @brief Orchestrates scrolling background layers and their Neo Geo sprite-column ring buffers.
 */

#include "renderer/background_renderer.h"

#include "display/sprite/limits.h"
#include "system/renderer_backend.h"

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

    if (remainder != NULL) {
        *remainder = rest;
    }
    return quotient;
}

/** Convert fixed/pixel scrolling into the first visible source tile and sub-tile offset. */
static s32 background_renderer_source_start(const UBackgroundLayer *layer, u8 *pixel_offset) {
    const s32 pixel_scroll = unsigned_background_fixed_to_pixels(layer->scroll_fixed);
    return background_renderer_floor_div_16(pixel_scroll, pixel_offset);
}

u8 unsigned_background_renderer_column_count(const UBackgroundLayer *layer, const UViewport *viewport) {
    if (layer == NULL || layer->definition == NULL || !layer->active || !unsigned_viewport_is_valid(viewport)) {
        return 0u;
    }

    u32 columns = (((u32)viewport->width + 15u) >> 4u) + 1u;
    if (columns > layer->definition->width_tiles) {
        columns = layer->definition->width_tiles;
    }
    if (columns == 0u || columns > UNSIGNED_BACKGROUND_RENDER_MAX_COLUMNS || columns > UINT8_MAX || !unsigned_sprite_range_is_valid(layer->definition->first_sprite, (u8)columns)) {
        return 0u;
    }

    return (u8)columns;
}

static void background_renderer_invalidate_columns(UBackgroundLayerRenderState *render) {
    for (u8 i = 0u; i < UNSIGNED_BACKGROUND_RENDER_MAX_COLUMNS; ++i) {
        render->loaded_source_columns[i] = UINT16_MAX;
    }
}

static void background_renderer_reset_state(UBackgroundLayerRenderState *render) {
    if (render == NULL) {
        return;
    }
    *render = (UBackgroundLayerRenderState){ 0 };
    background_renderer_invalidate_columns(render);
}

static u8 background_renderer_leftmost_slot(UBackgroundLayerRenderState *render, u8 columns, s32 source_start) {
    if (render == NULL || columns == 0u || !render->source_valid || render->rendered_columns != columns) {
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

static void background_renderer_prepare_uploads(const UBackgroundLayer *layer, UBackgroundLayerRenderState *next,
                                                UBackgroundLayerRenderPlan *plan, URenderPlan *frame_plan) {
    const UBackgroundLayerDefinition *definition = layer->definition;
    u8 slot = plan->leftmost_slot;
    u8 source_column = (u8)(layer->scroll_pixels >> 4u);

    for (u8 screen_column = 0u; screen_column < plan->columns; ++screen_column) {
        if (next->graphics_dirty || next->loaded_source_columns[slot] != source_column) {
            const bool full = next->graphics_dirty || next->loaded_source_columns[slot] == UINT16_MAX;
            UBackgroundColumnUploadPlan *upload = &plan->uploads[plan->upload_count++];
            upload->physical_slot = slot;
            upload->source_column = source_column;
            upload->full = full;
            next->loaded_source_columns[slot] = source_column;

            if (frame_plan != NULL) {
                ++frame_plan->stats.background_column_uploads;
                if (full) {
                    ++frame_plan->stats.background_full_column_uploads;
                }
                unsigned_render_plan_add_words(frame_plan, U_RENDER_PRIORITY_NORMAL,
                                               (u32)definition->height_tiles * (full ? 2u : 1u));
            }
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

    next->graphics_dirty = false;
}

static void background_renderer_prepare_chained(UBackgroundLayerRenderState *next,
                                                UBackgroundLayerRenderPlan *plan, s16 x, s16 y, s16 zoom_offset,
                                                URenderPlan *frame_plan) {
    const u16 scb2 = unsigned_background_backend_encode_scb2(zoom_offset);
    const bool layout_changed = !next->chain_valid || next->rendered_columns != plan->columns || next->rendered_leftmost_slot != plan->leftmost_slot;
    const bool x_changed = !next->position_valid || next->rendered_x != x;
    const bool y_changed = !next->position_valid || next->rendered_y != y;
    const bool shrink_changed = !next->shrink_valid || next->rendered_scb2 != scb2;
    const u8 driver_count = plan->leftmost_slot != 0u ? 2u : 1u;
    u32 words = 0u;

    plan->transform_mode = U_BACKGROUND_TRANSFORM_PLAN_CHAINED;
    plan->base_x = x;
    plan->y = y;
    plan->scb2 = scb2;
    plan->layout_changed = layout_changed;
    plan->x_changed = x_changed;
    plan->y_changed = y_changed;
    plan->shrink_changed = shrink_changed;

    if (shrink_changed) {
        words += plan->columns;
    }
    if (layout_changed) {
        words += plan->columns + driver_count;
    } else {
        if (y_changed) {
            words += driver_count;
        }
        if (x_changed) {
            words += driver_count;
        }
    }
    unsigned_render_plan_add_words(frame_plan, U_RENDER_PRIORITY_NORMAL, words);

    next->rendered_scb2 = scb2;
    next->rendered_leftmost_slot = plan->leftmost_slot;
    next->rendered_x = x;
    next->rendered_y = y;
    next->rendered_columns = plan->columns;
    next->position_valid = true;
    next->shrink_valid = true;
    next->chain_valid = true;
}

static void background_renderer_prepare_per_column(const UViewport *viewport, UBackgroundLayerRenderState *next,
                                                   UBackgroundLayerRenderPlan *plan, URenderPlan *frame_plan) {
    plan->transform_mode = U_BACKGROUND_TRANSFORM_PLAN_PER_COLUMN;
    for (u8 column = 0u; column < plan->columns; ++column) {
        plan->effect_samples[column] = unsigned_effect_sample(&viewport->effect, column, plan->columns);
    }
    unsigned_render_plan_add_words(frame_plan, U_RENDER_PRIORITY_NORMAL, (u32)plan->columns * 3u);

    next->position_valid = false;
    next->shrink_valid = false;
    next->chain_valid = false;
}

void unsigned_background_renderer_init(UBackgroundRenderState *state) {
    if (state == NULL) {
        return;
    }

    *state = (UBackgroundRenderState){ 0 };
    for (u8 i = 0u; i < UNSIGNED_BACKGROUND_MAX_LAYERS; ++i) {
        background_renderer_invalidate_columns(&state->layers[i]);
    }
}

bool unsigned_background_renderer_prepare(const UBackgroundRenderState *state, const UBackground *background, const UViewport *viewport,
                                         UBackgroundRenderPlan *plan, URenderPlan *frame_plan) {
    if (state == NULL || background == NULL || !unsigned_viewport_is_valid(viewport) || plan == NULL) {
        return false;
    }

    *plan = (UBackgroundRenderPlan){ 0 };

    for (u8 i = 0u; i < UNSIGNED_BACKGROUND_MAX_LAYERS; ++i) {
        const UBackgroundLayer *layer = &background->layers[i];
        const UBackgroundLayerRenderState *current = &state->layers[i];
        UBackgroundLayerRenderPlan *layer_plan = &plan->layers[i];
        UBackgroundLayerRenderState *next = &layer_plan->next_state;

        *next = *current;
        layer_plan->layer = layer;
        layer_plan->valid = true;

        if (!layer->active || layer->definition == NULL || !unsigned_sprite_height_is_valid(layer->definition->height_tiles)) {
            if (current->rendered) {
                layer_plan->hide_old_range = true;
                layer_plan->hide_first_sprite = current->rendered_first_sprite;
                layer_plan->hide_columns = current->rendered_columns;
                unsigned_render_plan_add_words(frame_plan, U_RENDER_PRIORITY_NORMAL, current->rendered_columns);
            }
            background_renderer_reset_state(next);
            continue;
        }

        layer_plan->columns = unsigned_background_renderer_column_count(layer, viewport);
        if (layer_plan->columns == 0u) {
            continue;
        }

        if (current->rendered && (current->rendered_definition != layer->definition ||
                                  current->rendered_first_sprite != layer->definition->first_sprite ||
                                  current->rendered_columns != layer_plan->columns)) {
            layer_plan->hide_old_range = true;
            layer_plan->hide_first_sprite = current->rendered_first_sprite;
            layer_plan->hide_columns = current->rendered_columns;
            unsigned_render_plan_add_words(frame_plan, U_RENDER_PRIORITY_NORMAL, current->rendered_columns);
            background_renderer_reset_state(next);
            next->graphics_dirty = true;
        } else if (!current->rendered) {
            next->graphics_dirty = true;
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
            background_renderer_prepare_chained(next, layer_plan,
                                                (s16)(layer_plan->base_x + effect.offset_x),
                                                (s16)(layer_plan->y + effect.offset_y), effect.zoom_offset, frame_plan);
        } else {
            background_renderer_prepare_per_column(viewport, next, layer_plan, frame_plan);
        }

        next->rendered_first_sprite = layer->definition->first_sprite;
        next->rendered_definition = layer->definition;
        next->rendered_source_start = source_start;
        next->rendered_leftmost_slot = layer_plan->leftmost_slot;
        next->rendered_columns = layer_plan->columns;
        next->rendered = true;
        next->source_valid = true;
    }

    plan->valid = true;
    return true;
}

void unsigned_background_renderer_commit(UBackgroundRenderState *state, const UBackgroundRenderPlan *plan) {
    if (state == NULL || plan == NULL || !plan->valid) {
        return;
    }

    for (u8 i = 0u; i < UNSIGNED_BACKGROUND_MAX_LAYERS; ++i) {
        const UBackgroundLayerRenderPlan *layer_plan = &plan->layers[i];
        if (!layer_plan->valid) {
            continue;
        }

        if (layer_plan->hide_old_range && layer_plan->hide_columns != 0u) {
            unsigned_background_backend_hide_range(layer_plan->hide_first_sprite, layer_plan->hide_columns);
        }

        if (layer_plan->layer != NULL && layer_plan->layer->definition != NULL) {
            for (u8 upload_index = 0u; upload_index < layer_plan->upload_count; ++upload_index) {
                const UBackgroundColumnUploadPlan *upload = &layer_plan->uploads[upload_index];
                unsigned_background_backend_write_column(layer_plan->layer->definition, upload->physical_slot,
                                                         upload->source_column, upload->full);
            }

            if (layer_plan->transform_mode == U_BACKGROUND_TRANSFORM_PLAN_CHAINED) {
                unsigned_background_backend_flush_chained(layer_plan->layer->definition, layer_plan->columns,
                                                          layer_plan->leftmost_slot, layer_plan->base_x, layer_plan->y,
                                                          layer_plan->scb2, layer_plan->shrink_changed,
                                                          layer_plan->layout_changed, layer_plan->x_changed,
                                                          layer_plan->y_changed);
            } else if (layer_plan->transform_mode == U_BACKGROUND_TRANSFORM_PLAN_PER_COLUMN) {
                unsigned_background_backend_write_effect_samples(layer_plan->layer->definition, layer_plan->columns,
                                                                 layer_plan->leftmost_slot, layer_plan->base_x,
                                                                 layer_plan->y, layer_plan->effect_samples);
            }
        }

        state->layers[i] = layer_plan->next_state;
    }
}

void unsigned_background_renderer_draw(UBackgroundRenderState *state, const UBackground *background, const UViewport *viewport) {
    UBackgroundRenderPlan plan;
    if (unsigned_background_renderer_prepare(state, background, viewport, &plan, NULL)) {
        unsigned_background_renderer_commit(state, &plan);
    }
}

void unsigned_background_renderer_hide(UBackgroundRenderState *state) {
    if (state == NULL) {
        return;
    }

    for (u8 i = 0u; i < UNSIGNED_BACKGROUND_MAX_LAYERS; ++i) {
        UBackgroundLayerRenderState *render = &state->layers[i];
        if (render->rendered) {
            unsigned_background_backend_hide_range(render->rendered_first_sprite, render->rendered_columns);
        }
        background_renderer_reset_state(render);
    }
}
