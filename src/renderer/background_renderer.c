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

/**
 * Compute the hardware columns needed to cover the viewport.
 * One extra 16-pixel column is reserved so sub-tile horizontal scrolling never exposes a gap.
 */
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

/** Mark every ring-buffer slot as unknown so the next sync uploads its source column. */
static void background_renderer_invalidate_columns(UBackgroundLayerRenderState *render) {
    for (u8 i = 0u; i < UNSIGNED_BACKGROUND_RENDER_MAX_COLUMNS; ++i) {
        render->loaded_source_columns[i] = UINT16_MAX;
    }
}

/** Reset one layer's renderer cache after its definition/range becomes invalid or changes. */
static void background_renderer_reset_state(UBackgroundLayerRenderState *render) {
    if (render == NULL) {
        return;
    }
    *render = (UBackgroundLayerRenderState){ 0 };
    background_renderer_invalidate_columns(render);
}

/**
 * Advance the ring-buffer driver slot by the number of source columns scrolled since the
 * previous frame. Large jumps invalidate the cache because no existing slot can be reused safely.
 */
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

/**
 * Synchronize source tile columns with the physical ring-buffer slots.
 * A newly allocated/invalid slot uploads tile+attribute pairs; a reused slot only uploads tiles.
 */
static void background_renderer_sync_graphics(const UBackgroundLayer *layer, UBackgroundLayerRenderState *render, u8 columns, u8 leftmost_slot) {
    const UBackgroundLayerDefinition *definition = layer->definition;
    u8 slot = leftmost_slot;
    u8 source_column = (u8)(layer->scroll_pixels >> 4u);

    for (u8 screen_column = 0u; screen_column < columns; ++screen_column) {
        if (render->graphics_dirty || render->loaded_source_columns[slot] != source_column) {
            const bool full = render->graphics_dirty || render->loaded_source_columns[slot] == UINT16_MAX;
            unsigned_background_backend_write_column(definition, slot, source_column, full);
            render->loaded_source_columns[slot] = source_column;
        }

        ++slot;
        if (slot == columns) {
            slot = 0u;
        }
        ++source_column;
        if (source_column == definition->width_tiles) {
            source_column = 0u;
        }
    }

    render->graphics_dirty = false;
}

/**
 * Render the layer as one or two chained segments depending on ring-buffer wrap.
 * The generic renderer computes change flags; the backend performs only the required SCB writes.
 */
static void background_renderer_draw_chained(const UBackgroundLayer *layer, UBackgroundLayerRenderState *render, u8 columns, u8 leftmost_slot, s16 base_x, s16 y, s16 zoom_offset) {
    const u16 scb2 = unsigned_background_backend_encode_scb2(zoom_offset);
    const bool layout_changed = !render->chain_valid || render->rendered_columns != columns || render->rendered_leftmost_slot != leftmost_slot;
    const bool x_changed = !render->position_valid || render->rendered_x != base_x;
    const bool y_changed = !render->position_valid || render->rendered_y != y;
    const bool shrink_changed = !render->shrink_valid || render->rendered_scb2 != scb2;

    unsigned_background_backend_flush_chained(layer->definition, columns, leftmost_slot, base_x, y, scb2, shrink_changed, layout_changed, x_changed, y_changed);

    render->rendered_scb2 = scb2;
    render->rendered_leftmost_slot = leftmost_slot;
    render->rendered_x = base_x;
    render->rendered_y = y;
    render->rendered_columns = columns;
    render->position_valid = true;
    render->shrink_valid = true;
    render->chain_valid = true;
}

/** Initialize all background layer caches in an empty/invalid state. */
void unsigned_background_renderer_init(UBackgroundRenderState *state) {
    if (state == NULL) {
        return;
    }

    *state = (UBackgroundRenderState){ 0 };
    for (u8 i = 0u; i < UNSIGNED_BACKGROUND_MAX_LAYERS; ++i) {
        background_renderer_invalidate_columns(&state->layers[i]);
    }
}

/**
 * Render every active background layer for the current viewport.
 *
 * Changing a layer definition, hardware range or column count first hides the old range and
 * invalidates the ring buffer. Uniform effects preserve chaining; per-column effects write each
 * column independently and invalidate cached transform state for the next normal chained frame.
 */
void unsigned_background_renderer_draw(UBackgroundRenderState *state, const UBackground *background, const UViewport *viewport) {
    if (state == NULL || background == NULL || !unsigned_viewport_is_valid(viewport)) {
        return;
    }

    for (u8 i = 0u; i < UNSIGNED_BACKGROUND_MAX_LAYERS; ++i) {
        const UBackgroundLayer *layer = &background->layers[i];
        UBackgroundLayerRenderState *render = &state->layers[i];

        if (!layer->active || layer->definition == NULL || !unsigned_sprite_height_is_valid(layer->definition->height_tiles)) {
            if (render->rendered) {
                unsigned_background_backend_hide_range(render->rendered_first_sprite, render->rendered_columns);
                background_renderer_reset_state(render);
            }
            continue;
        }

        const u8 columns = unsigned_background_renderer_column_count(layer, viewport);
        if (columns == 0u) {
            continue;
        }

        if (render->rendered && (render->rendered_definition != layer->definition || render->rendered_first_sprite != layer->definition->first_sprite || render->rendered_columns != columns)) {
            unsigned_background_backend_hide_range(render->rendered_first_sprite, render->rendered_columns);
            background_renderer_reset_state(render);
            render->graphics_dirty = true;
        } else if (!render->rendered) {
            render->graphics_dirty = true;
            background_renderer_invalidate_columns(render);
        }

        u8 pixel_offset;
        const s32 source_start = background_renderer_source_start(layer, &pixel_offset);
        const u8 leftmost_slot = background_renderer_leftmost_slot(render, columns, source_start);
        const s16 base_x = (s16)(viewport->x + layer->definition->screen_position.x - pixel_offset);
        const s16 y = (s16)(viewport->y + layer->definition->screen_position.y);

        background_renderer_sync_graphics(layer, render, columns, leftmost_slot);

        if (viewport->effect.function == NULL) {
            background_renderer_draw_chained(layer, render, columns, leftmost_slot, base_x, y, 0);
        } else if (viewport->effect.layout == U_EFFECT_LAYOUT_UNIFORM) {
            const UEffectSample effect = unsigned_effect_sample(&viewport->effect, 0u, columns);
            background_renderer_draw_chained(layer, render, columns, leftmost_slot, (s16)(base_x + effect.offset_x), (s16)(y + effect.offset_y), effect.zoom_offset);
        } else {
            unsigned_background_backend_write_effect_positions(layer, viewport, columns, leftmost_slot, base_x, y);
            render->position_valid = false;
            render->shrink_valid = false;
            render->chain_valid = false;
        }

        render->rendered_first_sprite = layer->definition->first_sprite;
        render->rendered_definition = layer->definition;
        render->rendered_source_start = source_start;
        render->rendered_leftmost_slot = leftmost_slot;
        render->rendered_columns = columns;
        render->rendered = true;
        render->source_valid = true;
    }
}

/** Hide all currently rendered background ranges and invalidate their cached state. */
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
