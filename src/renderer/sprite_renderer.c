/**
 * @file sprite_renderer.c
 * @brief Orchestrates sprite visibility, dirty state and sampled display effects.
 */

#include "renderer/sprite_renderer.h"
#include "renderer/sprite_renderer_internal.h"

#include "display/sprite/limits.h"
#include "display/sprite/scale.h"
#include "display/viewport/viewport_internal.h"
#include "system/sprite_backend.h"

static void sprite_renderer_hide(USprite *sprite);

/** Invalidate hardware ownership and any prepared command after a layout ownership change. */
static void sprite_renderer_invalidate_layout_state(USpriteRenderState *render) {
    render->committed.initialized = false;
    render->committed.chained = false;
    render->committed.visible = false;
    render->prepared.valid = false;
    render->prepared.columns = NULL;
    render->committed.graphics_valid = false;
}

#define SPRITE_RENDER_FLIP_X 0x01u
#define SPRITE_RENDER_FLIP_Y 0x02u

/** Effective SCB1 mirror flags after logical and sprite-effect mirroring are combined. */
static u8 sprite_renderer_graphics_flip_flags(const USprite *sprite) {
    u8 flags = 0u;
    if ((sprite->flip_x != 0u) != (sprite->render.effect_flip_x != 0u)) {
        flags |= SPRITE_RENDER_FLIP_X;
    }
    if ((sprite->flip_y != 0u) != (sprite->render.effect_flip_y != 0u)) {
        flags |= SPRITE_RENDER_FLIP_Y;
    }
    return flags;
}

/** Publish the SCB1 content now known to reside in the sprite's current hardware range. */
static void sprite_renderer_cache_graphics(USprite *sprite) {
    USpriteRenderState *render = &sprite->render;
    render->committed.frame = sprite->current_frame;
    render->committed.palette = sprite->palette;
    render->committed.flip_flags = sprite_renderer_graphics_flip_flags(sprite);
    render->committed.graphics_valid = true;
}

void unsigned_sprite_renderer_snapshot_layout(USprite *sprite) {
    USpriteRenderState *render = &sprite->render;
    render->previous.first_sprite = render->layout.first_sprite;
    render->previous.range_valid = render->layout.assigned;
    render->previous.frame = render->committed.frame;
    render->previous.x = render->committed.x;
    render->previous.y = render->committed.y;
    render->previous.scb2 = render->committed.scb2;
    render->previous.palette = render->committed.palette;
    render->previous.flip_flags = render->committed.flip_flags;
    render->previous.graphics_valid = render->committed.graphics_valid;
    render->previous.chain_valid = render->committed.initialized && render->committed.chained && render->committed.visible;
}

bool unsigned_sprite_renderer_can_reuse_previous_graphics(const USprite *sprite, const USprite *previous_owner) {
    if (previous_owner == NULL || sprite->effect.function != NULL) {
        return false;
    }

    const USpriteRenderState *previous = &previous_owner->render;
    if (!previous->previous.range_valid || !previous->previous.graphics_valid || previous->previous.first_sprite != sprite->render.layout.first_sprite || previous_owner->definition != sprite->definition ||
        previous_owner->render.layout.sprite_count != sprite->render.layout.sprite_count) {
        return false;
    }

    u8 desired_flip_flags = 0u;
    if (sprite->flip_x != 0u) {
        desired_flip_flags |= SPRITE_RENDER_FLIP_X;
    }
    if (sprite->flip_y != 0u) {
        desired_flip_flags |= SPRITE_RENDER_FLIP_Y;
    }

    return previous->previous.frame == sprite->current_frame && previous->previous.palette == sprite->palette && previous->previous.flip_flags == desired_flip_flags;
}

void unsigned_sprite_renderer_reuse_previous_graphics(USprite *sprite) {
    unsigned_sprite_render_clear_dirty(&sprite->render, U_SPRITE_RENDER_DIRTY_GRAPHICS);
    sprite->render.effect_flip_x = 0u;
    sprite->render.effect_flip_y = 0u;
    sprite_renderer_cache_graphics(sprite);
}

bool unsigned_sprite_renderer_can_reuse_previous_chain(const USprite *sprite, const USprite *previous_owner) {
    if (previous_owner == NULL) {
        return false;
    }

    const USpriteRenderState *previous = &previous_owner->render;
    return previous->previous.range_valid && previous->previous.chain_valid && previous->previous.first_sprite == sprite->render.layout.first_sprite &&
           previous_owner->render.layout.sprite_count == sprite->render.layout.sprite_count && previous_owner->definition->height_tiles == sprite->definition->height_tiles;
}

void unsigned_sprite_renderer_reuse_previous_chain(USprite *sprite, const USprite *previous_owner) {
    USpriteRenderState *render = &sprite->render;
    const USpritePreviousRenderState *previous = &previous_owner->render.previous;

    /* The destination already owns the same hardware-width chain shape. Adopt its exact driver
     * mirror, then let normal prepare dirty only X/Y/SCB2 values that differ for the new owner. */
    render->committed.x = previous->x;
    render->committed.y = previous->y;
    render->committed.scb2 = previous->scb2;
    render->committed.initialized = true;
    render->committed.chained = true;
    render->committed.visible = true;
    unsigned_sprite_render_clear_dirty(render, U_SPRITE_RENDER_DIRTY_TRANSFORM | U_SPRITE_RENDER_DIRTY_LAYOUT | U_SPRITE_RENDER_DIRTY_CHAIN_BOUNDARY);
}

/** Mark transform fields that differ from the last chained hardware state. */
static void sprite_renderer_detect_chained_changes(USprite *sprite, s16 screen_x, s16 screen_y, u16 scb2) {
    USpriteRenderState *render = &sprite->render;

    if (render->committed.x != screen_x) {
        unsigned_sprite_render_mark_dirty(render, U_SPRITE_RENDER_DIRTY_X);
    }
    if (render->committed.y != screen_y) {
        unsigned_sprite_render_mark_dirty(render, U_SPRITE_RENDER_DIRTY_Y);
    }
    if (render->committed.scb2 != scb2) {
        unsigned_sprite_render_mark_dirty(render, U_SPRITE_RENDER_DIRTY_SCALE);
    }
}

/** Commit one chained transform and make the software mirror authoritative. */
static void sprite_renderer_flush_chained_transform(USprite *sprite, s16 screen_x, s16 screen_y, u16 scb2) {
    USpriteRenderState *render = &sprite->render;

    unsigned_sprite_backend_flush_chained_transform(sprite, screen_x, screen_y, scb2, render->dirty);

    render->committed.x = screen_x;
    render->committed.y = screen_y;
    render->committed.scb2 = scb2;
    render->committed.initialized = true;
    render->committed.chained = true;
    unsigned_sprite_render_clear_dirty(render, U_SPRITE_RENDER_DIRTY_TRANSFORM | U_SPRITE_RENDER_DIRTY_LAYOUT);
}

/** Common gameplay path: neither owner has a presentation effect to sample. */
static bool sprite_renderer_uses_identity_effects(const USprite *sprite, const UViewport *viewport) {
    return sprite->effect.function == NULL && viewport->effect.function == NULL;
}

/**
 * Mirror flags affect SCB1 tile order/attributes, so update cached effect state before
 * graphics are flushed. Sprite effects are presentation-only: viewport samples never
 * alter sprite mirroring or visibility.
 */
static void sprite_renderer_update_effect_flags(USprite *sprite, const UEffectSample *sprite_effect) {
    const u8 flip_x = (sprite_effect->flags & U_EFFECT_SAMPLE_FLIP_X) != 0u;
    const u8 flip_y = (sprite_effect->flags & U_EFFECT_SAMPLE_FLIP_Y) != 0u;

    if (sprite->render.effect_flip_x != flip_x || sprite->render.effect_flip_y != flip_y) {
        sprite->render.effect_flip_x = flip_x;
        sprite->render.effect_flip_y = flip_y;
        unsigned_sprite_render_mark_dirty(&sprite->render, U_SPRITE_RENDER_DIRTY_GRAPHICS);
    }
}

/**
 * Keep expensive unused-row padding lazy. It is only required while vertical shrinking
 * can sample SCB1 rows below the logical sprite height. Full-size sprites never need it.
 */
static void sprite_renderer_prepare_padding(USprite *sprite, bool vertical_shrink_possible) {
    USpriteRenderState *render = &sprite->render;
    const USpriteDefinition *definition = sprite->definition;

    if (!definition->clear_unused_rows || definition->height_tiles >= UNSIGNED_SPRITE_MAX_HEIGHT_TILES || !vertical_shrink_possible) {
        unsigned_sprite_render_clear_dirty(render, U_SPRITE_RENDER_DIRTY_PADDING);
        return;
    }

    if (unsigned_sprite_backend_padding_is_valid(sprite, render->layout.first_sprite)) {
        unsigned_sprite_render_clear_dirty(render, U_SPRITE_RENDER_DIRTY_PADDING);
    } else {
        unsigned_sprite_render_mark_dirty(render, U_SPRITE_RENDER_DIRTY_PADDING);
    }
}

/** Apply sprite-local Q8 scale around its requested pivot without changing logical offset. */
static Vec2 sprite_renderer_scale_pivot_offset(const USprite *sprite, const UEffectSample *sprite_effect) {
    const u8 base_x = sprite->render.shrink_x;
    const u8 scaled_x = unsigned_sprite_scale_shrink_x(base_x, sprite_effect->scale_x);
    const u8 scaled_y = unsigned_sprite_scale_shrink_y(sprite->render.shrink_y, sprite_effect->scale_y);
    const u16 base_width = unsigned_sprite_scaled_width_pixels(sprite->definition->width_tiles, base_x);
    const u16 scaled_width = unsigned_sprite_scaled_width_pixels(sprite->definition->width_tiles, scaled_x);
    const u16 base_height = unsigned_sprite_scaled_height_pixels(sprite->definition->height_tiles, sprite->render.shrink_y);
    const u16 scaled_height = unsigned_sprite_scaled_height_pixels(sprite->definition->height_tiles, scaled_y);

    return (Vec2){
        .x = unsigned_sprite_scale_pivot_offset(base_width, scaled_width, sprite_effect->pivot_x),
        .y = unsigned_sprite_scale_pivot_offset(base_height, scaled_height, sprite_effect->pivot_y),
    };
}

/**
 * Freeze one per-column sprite transform into caller-owned frame scratch.
 *
 * This mirrors the transform math used by the Neo Geo backend and asks the backend to encode the
 * final SCB2/SCB3/SCB4 words while active display is still running. VRAM writes remain backend-
 * owned. The first sprite sample was already evaluated by the caller for visibility/flip policy
 * and is reused for column zero so custom callbacks run exactly once per column during preparation.
 */
static void sprite_renderer_prepare_effect_columns(USprite *sprite, const UViewport *viewport, s16 screen_x, s16 screen_y, UEffectSample first_sprite_sample, USpriteColumnPlanBuffer *column_buffer) {
    USpriteRenderState *render = &sprite->render;
    const u8 sprite_count = render->layout.sprite_count;
    /* The frame planner owns enough scratch for every per-column actor sprite prepared this frame. */
    UPreparedColumn *columns = &column_buffer->columns[column_buffer->used];
    const bool viewport_uniform = !unsigned_effect_is_per_column(&viewport->effect);
    const bool sprite_uniform = !unsigned_effect_is_per_column(&sprite->effect);
    const UEffectSample uniform_viewport = viewport_uniform ? unsigned_effect_sample(&viewport->effect, 0u, sprite_count) : unsigned_effect_identity_sample();
    const u8 base_shrink_x = render->shrink_x;
    const u16 base_column_width = (u16)base_shrink_x + 1u;
    const u16 base_width = unsigned_sprite_scaled_width_pixels(sprite_count, base_shrink_x);
    const u16 base_height = unsigned_sprite_scaled_height_pixels(sprite->definition->height_tiles, render->shrink_y);
    u16 uniform_column_width = base_column_width;
    s16 uniform_pivot_x = 0;
    s16 uniform_pivot_y = 0;

    if (sprite_uniform) {
        const u8 scaled_x = unsigned_sprite_scale_shrink_x(render->shrink_x, first_sprite_sample.scale_x);
        const u8 scaled_y = unsigned_sprite_scale_shrink_y(render->shrink_y, first_sprite_sample.scale_y);
        const u16 scaled_width = unsigned_sprite_scaled_width_pixels(sprite_count, scaled_x);
        const u16 scaled_height = unsigned_sprite_scaled_height_pixels(sprite->definition->height_tiles, scaled_y);
        uniform_column_width = (u16)scaled_x + 1u;
        uniform_pivot_x = unsigned_sprite_scale_pivot_offset(base_width, scaled_width, first_sprite_sample.pivot_x);
        uniform_pivot_y = unsigned_sprite_scale_pivot_offset(base_height, scaled_height, first_sprite_sample.pivot_y);
    }

    const u16 column_step = sprite_uniform ? uniform_column_width : base_column_width;
    s32 column_x = screen_x;

    for (u8 column = 0u; column < sprite_count; ++column) {
        const UEffectSample viewport_sample = viewport_uniform ? uniform_viewport : unsigned_effect_sample(&viewport->effect, column, sprite_count);
        const UEffectSample local_sprite = sprite_uniform || column == 0u ? first_sprite_sample : unsigned_effect_sample(&sprite->effect, column, sprite_count);
        const UEffectSample composed = unsigned_effect_compose_samples(viewport_sample, local_sprite);
        s16 pivot_x = uniform_pivot_x;
        s16 pivot_y = uniform_pivot_y;

        if (!sprite_uniform) {
            const u8 scaled_x = unsigned_sprite_scale_shrink_x(render->shrink_x, local_sprite.scale_x);
            const u8 scaled_y = unsigned_sprite_scale_shrink_y(render->shrink_y, local_sprite.scale_y);
            const u16 scaled_column_width = (u16)scaled_x + 1u;
            const u16 scaled_height = unsigned_sprite_scaled_height_pixels(sprite->definition->height_tiles, scaled_y);
            pivot_x = unsigned_sprite_scale_pivot_offset(base_column_width, scaled_column_width, local_sprite.pivot_x);
            pivot_y = unsigned_sprite_scale_pivot_offset(base_height, scaled_height, local_sprite.pivot_y);
        }

        unsigned_sprite_backend_encode_prepared_column(&columns[column], render, &composed, (s16)(column_x + composed.offset_x + pivot_x), (s16)((s32)screen_y + composed.offset_y + pivot_y), sprite->definition->height_tiles,
                                                       (composed.flags & U_EFFECT_SAMPLE_HIDDEN) != 0u);
        column_x += column_step;
    }

    render->prepared.columns = columns;
    column_buffer->used = (u16)(column_buffer->used + sprite_count);
}

/**
 * Prepare one visible sprite entirely on the CPU side.
 *
 * Uniform effects are fully sampled here so the post-VBlank commit only has to upload dirty
 * graphics and already-computed SCB values. Per-column effects are frozen into caller-owned frame
 * scratch; capacity is a renderer composition contract, so VBlank never falls back to live sampling.
 */
static void sprite_renderer_prepare_visible(USprite *sprite, const UViewport *viewport, const Vec2 *position, USpriteColumnPlanBuffer *column_buffer) {
    USpriteRenderState *render = &sprite->render;
    const s32 world_x = (s32)position->x + sprite->offset.x;
    const s32 world_y = (s32)position->y + sprite->offset.y;
    const Vec2 screen = {
        .x = unsigned_viewport_world_to_screen_x_unchecked(viewport, world_x),
        .y = unsigned_viewport_world_to_screen_y_unchecked(viewport, world_y),
    };
    const s16 screen_x = screen.x;
    const s16 screen_y = screen.y;

    render->prepared.valid = false;
    render->prepared.columns = NULL;
    if (sprite_renderer_uses_identity_effects(sprite, viewport)) {
        const u16 scb2 = (u16)(((u16)render->shrink_x << 8u) | render->shrink_y);

        /* Dominant stable-state fast path. Camera scrolling changes only screen X for stationary
         * actors, so preserve the authoritative graphics/chain/padding state and dirty just SCB4.
         * This avoids the generic padding + transform-detection path for every crowd sprite. */
        if (render->effect_flip_x == 0u && render->effect_flip_y == 0u && render->dirty == 0u && render->committed.initialized && render->committed.chained && render->committed.visible && render->committed.graphics_valid &&
            render->committed.frame == sprite->current_frame && render->committed.palette == sprite->palette && render->committed.flip_flags == sprite_renderer_graphics_flip_flags(sprite) && render->committed.y == screen_y && render->committed.scb2 == scb2) {
            if (render->committed.x == screen_x) {
                return;
            }

            render->prepared.hidden = false;
            render->prepared.per_column = false;
            render->prepared.draw_x = screen_x;
            render->prepared.draw_y = screen_y;
            render->prepared.scb2 = scb2;
            unsigned_sprite_render_mark_dirty(render, U_SPRITE_RENDER_DIRTY_X);
            render->prepared.valid = true;
            return;
        }

        render->prepared.hidden = false;
        render->prepared.per_column = false;
        render->prepared.draw_x = screen_x;
        render->prepared.draw_y = screen_y;
        render->prepared.scb2 = scb2;

        if (render->effect_flip_x != 0u || render->effect_flip_y != 0u) {
            render->effect_flip_x = 0u;
            render->effect_flip_y = 0u;
            unsigned_sprite_render_mark_dirty(render, U_SPRITE_RENDER_DIRTY_GRAPHICS);
        }

        sprite_renderer_prepare_padding(sprite, render->shrink_y < U_SPRITE_SHRINK_Y_FULL);
        sprite_renderer_detect_chained_changes(sprite, screen_x, screen_y, scb2);
        render->prepared.valid = render->dirty != 0u || !render->committed.initialized || !render->committed.chained || !render->committed.visible;
        return;
    }

    render->prepared.screen_y = screen_y;
    const UEffectSample sprite_effect = unsigned_effect_sample(&sprite->effect, 0u, render->layout.sprite_count);
    render->prepared.hidden = (sprite_effect.flags & U_EFFECT_SAMPLE_HIDDEN) != 0u;
    render->prepared.per_column = unsigned_effect_is_per_column(&sprite->effect) || unsigned_effect_is_per_column(&viewport->effect);

    sprite_renderer_update_effect_flags(sprite, &sprite_effect);

    if (render->prepared.hidden) {
        render->prepared.valid = true;
        return;
    }

    if (render->prepared.per_column) {
        /* Arbitrary per-column effects may shrink one or more columns. Keep the
         * conservative guard, but reuse padding already known safe for this range. */
        sprite_renderer_prepare_padding(sprite, true);
        sprite_renderer_prepare_effect_columns(sprite, viewport, screen_x, screen_y, sprite_effect, column_buffer);
        render->prepared.valid = true;
        return;
    }

    const UEffectSample viewport_effect = unsigned_effect_sample(&viewport->effect, 0u, render->layout.sprite_count);
    const UEffectSample composed = unsigned_effect_compose_samples(viewport_effect, sprite_effect);
    const Vec2 pivot = sprite_renderer_scale_pivot_offset(sprite, &sprite_effect);
    const s16 draw_x = (s16)((s32)screen_x + composed.offset_x + pivot.x);
    const s16 draw_y = (s16)((s32)screen_y + composed.offset_y + pivot.y);
    const u16 scb2 = unsigned_sprite_backend_encode_scb2(render, &composed);

    sprite_renderer_prepare_padding(sprite, (u8)(scb2 & 0x00ffu) < U_SPRITE_SHRINK_Y_FULL);
    render->prepared.draw_x = draw_x;
    render->prepared.draw_y = draw_y;
    render->prepared.scb2 = scb2;
    sprite_renderer_detect_chained_changes(sprite, draw_x, draw_y, scb2);
    render->prepared.valid = render->dirty != 0u || !render->committed.initialized || !render->committed.chained || !render->committed.visible;
}

/** Commit a sprite whose CPU-side transform was prepared before VBlank. */
static void sprite_renderer_commit_prepared(USprite *sprite) {
    USpriteRenderState *render = &sprite->render;
    const UFrame *frame = sprite->current_frame;

    if (!render->prepared.valid) {
        render->prepared.columns = NULL;
        render->prepared.valid = false;
        return;
    }

    if (render->prepared.hidden) {
        if (render->committed.visible) {
            sprite_renderer_hide(sprite);
        } else if (!render->committed.initialized) {
            /* The assigned range can contain a previous owner's SCB3 chain after a
             * relocation. Clear it once, but keep graphics/layout dirty so the next
             * visible frame still performs an authoritative rebuild. */
            unsigned_sprite_backend_clear_range(render->layout.first_sprite, render->layout.sprite_count);
            render->committed.initialized = true;
            render->committed.chained = false;
        }
        render->prepared.columns = NULL;
        render->prepared.valid = false;
        return;
    }

    if ((render->dirty & (U_SPRITE_RENDER_DIRTY_GRAPHICS | U_SPRITE_RENDER_DIRTY_PADDING)) != 0u) {
        unsigned_sprite_backend_flush_graphics(sprite, frame, render->dirty);
        unsigned_sprite_render_clear_dirty(render, U_SPRITE_RENDER_DIRTY_GRAPHICS | U_SPRITE_RENDER_DIRTY_PADDING);
    }
    sprite_renderer_cache_graphics(sprite);

    if (render->prepared.per_column) {
        unsigned_sprite_backend_write_prepared_effect_positions(sprite, render->prepared.columns);
        render->committed.initialized = true;
        render->committed.chained = false;
        /* A later return to normal chaining must rebuild one authoritative driver/sticky chain. */
        unsigned_sprite_render_mark_dirty(render, U_SPRITE_RENDER_DIRTY_TRANSFORM | U_SPRITE_RENDER_DIRTY_LAYOUT);
    } else {
        sprite_renderer_flush_chained_transform(sprite, render->prepared.draw_x, render->prepared.draw_y, render->prepared.scb2);
    }

    render->committed.visible = true;
    render->prepared.columns = NULL;
    render->prepared.valid = false;
}

void unsigned_sprite_renderer_prepare_draw(USprite *sprite, const UViewport *viewport, const Vec2 *position, USpriteColumnPlanBuffer *column_buffer) {
    if (!sprite->render.layout.assigned) {
        sprite->render.prepared.columns = NULL;
        sprite->render.prepared.valid = false;
        return;
    }

    if (!sprite->render.layout.visible) {
        /* Stable actor layouts keep culled sprites assigned so visibility changes do
         * not reshuffle every later actor. Prepare one hide/clear only when needed. */
        if (!sprite->render.committed.visible && sprite->render.committed.initialized) {
            sprite->render.prepared.columns = NULL;
            sprite->render.prepared.valid = false;
            return;
        }

        sprite->render.prepared.columns = NULL;
        sprite->render.prepared.hidden = true;
        sprite->render.prepared.per_column = false;
        sprite->render.prepared.valid = true;
        return;
    }

    sprite_renderer_prepare_visible(sprite, viewport, position, column_buffer);
}

u8 unsigned_sprite_renderer_prepared_driver_dirty(const USprite *sprite) {
    const USpriteRenderState *render = &sprite->render;
    const u8 position_dirty = (u8)(render->dirty & U_SPRITE_RENDER_DIRTY_POSITION);
    if (!render->prepared.valid || render->prepared.hidden || render->prepared.per_column || !render->committed.initialized || !render->committed.chained || !render->committed.visible || !render->layout.assigned ||
        !render->layout.visible || position_dirty == 0u || (render->dirty & (u8)~U_SPRITE_RENDER_DIRTY_POSITION) != 0u) {
        return 0u;
    }
    return position_dirty;
}

void unsigned_sprite_renderer_commit_batched_driver_axis(USprite *sprite, u8 dirty_axis) {
    USpriteRenderState *render = &sprite->render;
    const u8 axis = (u8)(dirty_axis & U_SPRITE_RENDER_DIRTY_POSITION);
    if ((axis & U_SPRITE_RENDER_DIRTY_X) != 0u && (render->dirty & U_SPRITE_RENDER_DIRTY_X) != 0u) {
        render->committed.x = render->prepared.draw_x;
        unsigned_sprite_render_clear_dirty(render, U_SPRITE_RENDER_DIRTY_X);
    }
    if ((axis & U_SPRITE_RENDER_DIRTY_Y) != 0u && (render->dirty & U_SPRITE_RENDER_DIRTY_Y) != 0u) {
        render->committed.y = render->prepared.draw_y;
        unsigned_sprite_render_clear_dirty(render, U_SPRITE_RENDER_DIRTY_Y);
    }

    if (render->dirty == 0u) {
        render->committed.initialized = true;
        render->committed.chained = true;
        render->committed.visible = true;
        render->prepared.columns = NULL;
        render->prepared.valid = false;
    }
}

void unsigned_sprite_renderer_clear_range(u16 first_sprite, u16 sprite_count) {
    unsigned_sprite_backend_clear_range(first_sprite, sprite_count);
}

/**
 * Test world-space visibility including conservative displacement from viewport and
 * sprite effects. Scale-only effects cannot enlarge beyond source size on Neo Geo,
 * so the unscaled sprite rectangle remains a conservative culling extent.
 */
bool unsigned_sprite_renderer_is_visible_in_bounds(const USprite *sprite, const UViewport *viewport, const UViewportWorldBounds *bounds, const Vec2 *position) {
    UEffectBounds viewport_bounds = {0};
    UEffectBounds sprite_bounds = {0};

    const s32 world_x = (s32)position->x + sprite->offset.x;
    const s32 world_y = (s32)position->y + sprite->offset.y;

    if (sprite_renderer_uses_identity_effects(sprite, viewport)) {
        return unsigned_viewport_world_bounds_intersects_unchecked(bounds, world_x, world_y, (s32)sprite->definition->width_tiles * 16, (s32)sprite->definition->height_tiles * 16, 0, 0);
    }

    if (!unsigned_effect_get_bounds(&viewport->effect, sprite->render.layout.sprite_count, &viewport_bounds) || !unsigned_effect_get_bounds(&sprite->effect, sprite->render.layout.sprite_count, &sprite_bounds)) {
        return true;
    }

    const UEffectBounds effect_bounds = unsigned_effect_add_bounds(viewport_bounds, sprite_bounds);
    return unsigned_viewport_world_bounds_intersects_unchecked(bounds, world_x, world_y, (s32)sprite->definition->width_tiles * 16, (s32)sprite->definition->height_tiles * 16, effect_bounds.offset_x, effect_bounds.offset_y);
}

void unsigned_sprite_renderer_precommit_chain_boundary(USprite *sprite) {
    if ((sprite->render.dirty & U_SPRITE_RENDER_DIRTY_CHAIN_BOUNDARY) == 0u) {
        return;
    }

    unsigned_sprite_backend_break_chain(sprite->render.layout.first_sprite);
    unsigned_sprite_render_clear_dirty(&sprite->render, U_SPRITE_RENDER_DIRTY_CHAIN_BOUNDARY);
}

void unsigned_sprite_renderer_relocate(USprite *sprite, u16 first_sprite) {
    if (sprite->render.layout.first_sprite == first_sprite) {
        sprite->render.layout.assigned = true;
        return;
    }

    const bool padding_valid = unsigned_sprite_backend_padding_is_valid(sprite, first_sprite);

    /*
     * Do not clear the old SCB3 range here. Actor layout is compact: ranges that
     * remain inside the actor span are immediately overwritten by their new owner,
     * while level_renderer clears only the trailing slots that truly become unused.
     * Clearing here creates a visible hole during Y-order swaps and doubles VRAM work.
     */
    sprite->render.layout.first_sprite = first_sprite;
    sprite->render.layout.assigned = true;
    sprite_renderer_invalidate_layout_state(&sprite->render);
    unsigned_sprite_render_mark_dirty(&sprite->render, U_SPRITE_RENDER_DIRTY_RELOCATION);
    if (!padding_valid) {
        unsigned_sprite_render_mark_dirty(&sprite->render, U_SPRITE_RENDER_DIRTY_PADDING);
    }
}

void unsigned_sprite_renderer_draw_prepared(USprite *sprite) {
    sprite_renderer_commit_prepared(sprite);
}

static void sprite_renderer_hide(USprite *sprite) {
    if (!sprite->render.committed.visible) {
        return;
    }

    if ((sprite->render.dirty & U_SPRITE_RENDER_DIRTY_LAYOUT) == 0u) {
        unsigned_sprite_backend_hide_chain(sprite);
    } else {
        unsigned_sprite_backend_clear_range(sprite->render.layout.first_sprite, sprite->render.layout.sprite_count);
    }

    sprite->render.committed.visible = false;
    sprite->render.committed.chained = false;
    sprite->render.prepared.valid = false;
    sprite->render.prepared.columns = NULL;
    unsigned_sprite_render_mark_dirty(&sprite->render, U_SPRITE_RENDER_DIRTY_Y);
}

void unsigned_sprite_renderer_unassign(USprite *sprite) {
    /* CPU-only counterpart used during pre-VBlank layout compaction. */
    sprite_renderer_invalidate_layout_state(&sprite->render);
    sprite->render.layout.assigned = false;
    sprite->render.layout.visible = false;
    unsigned_sprite_render_mark_dirty(&sprite->render, U_SPRITE_RENDER_DIRTY_ALL);
}
