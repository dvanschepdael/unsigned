/**
 * @file sprite_renderer.c
 * @brief Orchestrates sprite visibility, dirty state and sampled display effects.
 */

#include "renderer/sprite_renderer.h"

#include "display/sprite/limits.h"
#include "display/sprite/scale.h"
#include "system/renderer_backend.h"

/** Mark transform fields that differ from the last chained hardware state. */
static void sprite_renderer_detect_chained_changes(USprite *sprite, s16 screen_x, s16 screen_y, u16 scb2) {
    USpriteRenderState *render = &sprite->render;

    if (render->rendered_x != screen_x) {
        unsigned_sprite_render_mark_dirty(render, U_SPRITE_RENDER_DIRTY_X);
    }
    if (render->rendered_y != screen_y) {
        unsigned_sprite_render_mark_dirty(render, U_SPRITE_RENDER_DIRTY_Y);
    }
    if (render->rendered_scb2 != scb2) {
        unsigned_sprite_render_mark_dirty(render, U_SPRITE_RENDER_DIRTY_SCALE);
    }
}

/** Commit one chained transform and make the software mirror authoritative. */
static void sprite_renderer_flush_chained_transform(USprite *sprite, s16 screen_x, s16 screen_y, u16 scb2) {
    USpriteRenderState *render = &sprite->render;

    unsigned_sprite_backend_flush_chained_transform(sprite, screen_x, screen_y, scb2, render->dirty);

    render->rendered_x = screen_x;
    render->rendered_y = screen_y;
    render->rendered_scb2 = scb2;
    render->hardware_initialized = true;
    unsigned_sprite_render_clear_dirty(render, U_SPRITE_RENDER_DIRTY_TRANSFORM | U_SPRITE_RENDER_DIRTY_LAYOUT);
}

/** Validate everything required before the renderer may touch a sprite's hardware range. */
static bool sprite_renderer_draw_state_is_valid(const USprite *sprite, const UViewport *viewport, const Vec2 *position) {
    return sprite != NULL && sprite->definition != NULL && viewport != NULL && viewport->camera != NULL && viewport->width != 0u && viewport->height != 0u && position != NULL && sprite->render.sprite_count == sprite->definition->width_tiles &&
           unsigned_sprite_height_is_valid(sprite->definition->height_tiles) && unsigned_sprite_range_is_valid(sprite->render.first_sprite, sprite->render.sprite_count);
}

/** Return whether either owner requires one transform sample per hardware column. */
static bool sprite_renderer_uses_per_column_effect(const USprite *sprite, const UViewport *viewport) {
    return unsigned_effect_is_per_column(&sprite->effect) || unsigned_effect_is_per_column(&viewport->effect);
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

/** Apply sprite-local Q8 scale around its requested pivot without changing logical offset. */
static Vec2 sprite_renderer_scale_pivot_offset(const USprite *sprite, const UEffectSample *sprite_effect) {
    const u8 base_x = (u8)(sprite->render.shrink_x & U_SPRITE_SHRINK_X_FULL);
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
 * Prepare one visible sprite entirely on the CPU side.
 *
 * Uniform effects are fully sampled here so the post-VBlank commit only has to upload dirty
 * graphics and already-computed SCB values. Per-column effects keep their base screen position
 * cached but still use the backend's column loop during commit because the effect callback owns
 * arbitrary user data and the sprite width is not globally bounded to a small plan array.
 */
static bool sprite_renderer_prepare_visible(USprite *sprite, const UViewport *viewport, const Vec2 *position) {
    USpriteRenderState *render = &sprite->render;
    const s32 world_x = (s32)position->x + sprite->offset.x;
    const s32 world_y = (s32)position->y + sprite->offset.y;
    const s16 screen_x = unsigned_viewport_world_to_screen_x(viewport, world_x);
    const s16 screen_y = unsigned_viewport_world_to_screen_y(viewport, world_y);
    const UEffectSample sprite_effect = unsigned_effect_sample(&sprite->effect, 0u, render->sprite_count);

    render->prepared_valid = false;
    render->prepared_screen_x = screen_x;
    render->prepared_screen_y = screen_y;
    render->prepared_hidden = (sprite_effect.flags & U_EFFECT_SAMPLE_HIDDEN) != 0u;
    render->prepared_per_column = sprite_renderer_uses_per_column_effect(sprite, viewport);

    sprite_renderer_update_effect_flags(sprite, &sprite_effect);

    if (render->prepared_hidden) {
        render->prepared_valid = true;
        return true;
    }

    if (render->prepared_per_column) {
        render->prepared_valid = true;
        return true;
    }

    const UEffectSample viewport_effect = unsigned_effect_sample(&viewport->effect, 0u, render->sprite_count);
    const UEffectSample composed = unsigned_effect_compose_samples(viewport_effect, sprite_effect);
    const Vec2 pivot = sprite_renderer_scale_pivot_offset(sprite, &sprite_effect);
    const s16 draw_x = (s16)((s32)screen_x + composed.offset_x + pivot.x);
    const s16 draw_y = (s16)((s32)screen_y + composed.offset_y + pivot.y);
    const u16 scb2 = unsigned_sprite_backend_encode_scb2(render, &composed);

    render->prepared_draw_x = draw_x;
    render->prepared_draw_y = draw_y;
    render->prepared_scb2 = scb2;
    sprite_renderer_detect_chained_changes(sprite, draw_x, draw_y, scb2);
    render->prepared_valid = true;
    return true;
}

/** Commit a sprite whose CPU-side transform was prepared before VBlank. */
static void sprite_renderer_commit_prepared(USprite *sprite, const UViewport *viewport) {
    USpriteRenderState *render = &sprite->render;
    const UFrame *frame = sprite->current_frame;

    if (!render->prepared_valid || frame == NULL) {
        render->prepared_valid = false;
        return;
    }

    if (render->prepared_hidden) {
        if (render->visible) {
            unsigned_sprite_renderer_hide(sprite);
        }
        render->prepared_valid = false;
        return;
    }

    if ((render->dirty & (U_SPRITE_RENDER_DIRTY_GRAPHICS | U_SPRITE_RENDER_DIRTY_PADDING)) != 0u) {
        unsigned_sprite_backend_flush_graphics(sprite, frame, render->dirty);
        unsigned_sprite_render_clear_dirty(render, U_SPRITE_RENDER_DIRTY_GRAPHICS | U_SPRITE_RENDER_DIRTY_PADDING);
    }

    if (render->prepared_per_column) {
        unsigned_sprite_backend_write_effect_positions(sprite, &viewport->effect, &sprite->effect,
                                                       render->prepared_screen_x, render->prepared_screen_y);
        render->hardware_initialized = true;
        /* A later return to normal chaining must rebuild one authoritative driver/sticky chain. */
        unsigned_sprite_render_mark_dirty(render, U_SPRITE_RENDER_DIRTY_TRANSFORM | U_SPRITE_RENDER_DIRTY_LAYOUT);
    } else {
        sprite_renderer_flush_chained_transform(sprite, render->prepared_draw_x, render->prepared_draw_y, render->prepared_scb2);
    }

    render->visible = true;
    render->prepared_valid = false;
}

bool unsigned_sprite_renderer_prepare_draw(USprite *sprite, const UViewport *viewport, const Vec2 *position) {
    if (!sprite_renderer_draw_state_is_valid(sprite, viewport, position) || !sprite->render.layout_visible || sprite->current_frame == NULL) {
        if (sprite != NULL) {
            sprite->render.prepared_valid = false;
        }
        return false;
    }

    return sprite_renderer_prepare_visible(sprite, viewport, position);
}

void unsigned_sprite_renderer_estimate_prepared_vram_words(const USprite *sprite, u32 *critical_words, u32 *high_words) {
    u32 critical = 0u;
    u32 high = 0u;

    if (sprite != NULL && sprite->definition != NULL && sprite->render.prepared_valid) {
        const USpriteRenderState *render = &sprite->render;
        const u8 width = render->sprite_count;
        const u8 height = sprite->definition->height_tiles;
        const u8 dirty = render->dirty;

        if ((dirty & U_SPRITE_RENDER_DIRTY_CHAIN_BOUNDARY) != 0u) {
            ++critical;
        }

        if (render->prepared_hidden) {
            if (render->visible) {
                high += (dirty & U_SPRITE_RENDER_DIRTY_LAYOUT) != 0u ? width : 1u;
            }
        } else {
            if ((dirty & U_SPRITE_RENDER_DIRTY_GRAPHICS) == U_SPRITE_RENDER_DIRTY_GRAPHICS) {
                high += (u32)width * height * 2u;
            } else {
                if ((dirty & U_SPRITE_RENDER_DIRTY_TILES) != 0u) {
                    high += (u32)width * height;
                }
                if ((dirty & U_SPRITE_RENDER_DIRTY_ATTRIBUTES) != 0u) {
                    high += (u32)width * height;
                }
            }

            if ((dirty & U_SPRITE_RENDER_DIRTY_PADDING) != 0u && sprite->definition->clear_unused_rows && height < UNSIGNED_SPRITE_MAX_HEIGHT_TILES) {
                high += (u32)width * (UNSIGNED_SPRITE_MAX_HEIGHT_TILES - height) * 2u;
            }

            if (render->prepared_per_column) {
                high += (u32)width * 3u;
            } else {
                if ((dirty & U_SPRITE_RENDER_DIRTY_SCALE) != 0u) {
                    high += width;
                }
                if ((dirty & U_SPRITE_RENDER_DIRTY_LAYOUT) != 0u) {
                    high += (u32)width + 1u;
                } else if ((dirty & U_SPRITE_RENDER_DIRTY_POSITION) == U_SPRITE_RENDER_DIRTY_POSITION) {
                    high += 2u;
                } else if ((dirty & U_SPRITE_RENDER_DIRTY_POSITION) != 0u) {
                    high += 1u;
                }
            }
        }
    }

    if (critical_words != NULL) {
        *critical_words += critical;
    }
    if (high_words != NULL) {
        *high_words += high;
    }
}

void unsigned_sprite_renderer_clear_range(u16 first_sprite, u16 sprite_count) {
    if (sprite_count == 0u || first_sprite < UNSIGNED_SPRITE_FIRST || first_sprite > UNSIGNED_SPRITE_LAST || (u32)first_sprite + sprite_count - 1u > UNSIGNED_SPRITE_LAST) {
        return;
    }

    unsigned_sprite_backend_clear_range(first_sprite, sprite_count);
}

/**
 * Test world-space visibility including conservative displacement from viewport and
 * sprite effects. Scale-only effects cannot enlarge beyond source size on Neo Geo,
 * so the unscaled sprite rectangle remains a conservative culling extent.
 */
bool unsigned_sprite_renderer_is_visible(const USprite *sprite, const UViewport *viewport, const Vec2 *position) {
    UEffectBounds viewport_bounds = { 0 };
    UEffectBounds sprite_bounds = { 0 };

    if (sprite == NULL || sprite->definition == NULL || viewport == NULL || viewport->camera == NULL || viewport->width == 0u || viewport->height == 0u || position == NULL) {
        return false;
    }

    if (!unsigned_effect_get_bounds(&viewport->effect, sprite->render.sprite_count, &viewport_bounds) ||
        !unsigned_effect_get_bounds(&sprite->effect, sprite->render.sprite_count, &sprite_bounds)) {
        return true;
    }

    const UEffectBounds effect_bounds = unsigned_effect_add_bounds(viewport_bounds, sprite_bounds);
    const s32 world_x = (s32)position->x + sprite->offset.x;
    const s32 world_y = (s32)position->y + sprite->offset.y;
    return unsigned_viewport_intersects_world_rect(viewport, world_x, world_y,
                                                   (s32)sprite->definition->width_tiles * 16,
                                                   (s32)sprite->definition->height_tiles * 16,
                                                   effect_bounds.offset_x, effect_bounds.offset_y);
}

void unsigned_sprite_renderer_precommit_chain_boundary(USprite *sprite) {
    if (sprite == NULL || sprite->definition == NULL ||
        (sprite->render.dirty & U_SPRITE_RENDER_DIRTY_CHAIN_BOUNDARY) == 0u ||
        !unsigned_sprite_range_is_valid(sprite->render.first_sprite, sprite->render.sprite_count)) {
        return;
    }

    unsigned_sprite_backend_break_chain(sprite->render.first_sprite);
    unsigned_sprite_render_clear_dirty(&sprite->render, U_SPRITE_RENDER_DIRTY_CHAIN_BOUNDARY);
}

bool unsigned_sprite_renderer_relocate(USprite *sprite, u16 first_sprite) {
    if (sprite == NULL || sprite->definition == NULL || sprite->render.sprite_count != sprite->definition->width_tiles || !unsigned_sprite_range_is_valid(first_sprite, sprite->render.sprite_count)) {
        return false;
    }

    if (sprite->render.first_sprite == first_sprite) {
        return true;
    }

    const bool padding_valid = unsigned_sprite_backend_padding_is_valid(sprite, first_sprite);

    /*
     * Do not clear the old SCB3 range here. Actor layout is compact: ranges that
     * remain inside the actor span are immediately overwritten by their new owner,
     * while level_renderer clears only the trailing slots that truly become unused.
     * Clearing here creates a visible hole during Y-order swaps and doubles VRAM work.
     */
    sprite->render.first_sprite = first_sprite;
    sprite->render.hardware_initialized = false;
    sprite->render.visible = false;
    sprite->render.prepared_valid = false;
    unsigned_sprite_render_mark_dirty(&sprite->render, U_SPRITE_RENDER_DIRTY_RELOCATION);
    if (!padding_valid) {
        unsigned_sprite_render_mark_dirty(&sprite->render, U_SPRITE_RENDER_DIRTY_PADDING);
    }
    return true;
}

void unsigned_sprite_renderer_draw(USprite *sprite, const UViewport *viewport, const Vec2 *position) {
    if (!sprite_renderer_draw_state_is_valid(sprite, viewport, position)) {
        return;
    }

    if (sprite->current_frame == NULL || !unsigned_sprite_renderer_is_visible(sprite, viewport, position)) {
        if (sprite->render.visible) {
            unsigned_sprite_renderer_hide(sprite);
        }
        sprite->render.prepared_valid = false;
        return;
    }

    /* Standalone callers retain the legacy immediate path, implemented as prepare + commit. */
    sprite->render.layout_visible = true;
    if (sprite_renderer_prepare_visible(sprite, viewport, position)) {
        unsigned_sprite_renderer_precommit_chain_boundary(sprite);
        sprite_renderer_commit_prepared(sprite, viewport);
    }
}

void unsigned_sprite_renderer_draw_prepared(USprite *sprite, const UViewport *viewport, const Vec2 *position) {
    (void)position;
    if (sprite == NULL || viewport == NULL || !sprite->render.layout_visible || sprite->current_frame == NULL) {
        return;
    }

    sprite_renderer_commit_prepared(sprite, viewport);
}

void unsigned_sprite_renderer_hide(USprite *sprite) {
    if (sprite == NULL || !sprite->render.visible) {
        return;
    }

    if ((sprite->render.dirty & U_SPRITE_RENDER_DIRTY_LAYOUT) == 0u) {
        unsigned_sprite_backend_hide_chain(sprite);
    } else {
        unsigned_sprite_backend_clear_range(sprite->render.first_sprite, sprite->render.sprite_count);
    }

    sprite->render.visible = false;
    sprite->render.prepared_valid = false;
    unsigned_sprite_render_mark_dirty(&sprite->render, U_SPRITE_RENDER_DIRTY_Y);
}

void unsigned_sprite_renderer_unassign(USprite *sprite) {
    if (sprite == NULL) {
        return;
    }

    /* CPU-only counterpart used during pre-VBlank layout compaction. */
    sprite->render.hardware_initialized = false;
    sprite->render.visible = false;
    sprite->render.layout_visible = false;
    sprite->render.prepared_valid = false;
    unsigned_sprite_render_mark_dirty(&sprite->render, U_SPRITE_RENDER_DIRTY_ALL);
}

void unsigned_sprite_renderer_release(USprite *sprite) {
    if (sprite == NULL) {
        return;
    }

    if (sprite->render.hardware_initialized) {
        unsigned_sprite_backend_clear_range(sprite->render.first_sprite, sprite->render.sprite_count);
    }

    sprite->render.hardware_initialized = false;
    sprite->render.visible = false;
    sprite->render.layout_visible = false;
    sprite->render.prepared_valid = false;
    unsigned_sprite_render_mark_dirty(&sprite->render, U_SPRITE_RENDER_DIRTY_ALL);
}
