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

/** Draw a sprite as one Neo Geo chain using an already composed transform sample. */
static void sprite_renderer_draw_chained(USprite *sprite, s16 screen_x, s16 screen_y, const UEffectSample *effect) {
    const u16 scb2 = unsigned_sprite_backend_encode_scb2(&sprite->render, effect);

    sprite_renderer_detect_chained_changes(sprite, screen_x, screen_y, scb2);
    sprite_renderer_flush_chained_transform(sprite, screen_x, screen_y, scb2);
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
 * Render a sprite already known to be inside culling bounds.
 *
 * Uniform viewport + sprite effects preserve the hardware chain. If either effect is
 * per-column, the backend writes independent SCB2/SCB3/SCB4 values and the chained
 * transform cache is invalidated for a clean rebuild when that effect is removed.
 */
static void sprite_renderer_draw_visible(USprite *sprite, const UViewport *viewport, const Vec2 *position) {
    const UFrame *frame = sprite->current_frame;

    if (frame == NULL) {
        if (sprite->render.visible) {
            unsigned_sprite_renderer_hide(sprite);
        }
        return;
    }

    const s32 world_x = (s32)position->x + sprite->offset.x;
    const s32 world_y = (s32)position->y + sprite->offset.y;
    const s16 screen_x = (s16)((s32)viewport->x + world_x - viewport->camera->x);
    const s16 screen_y = (s16)((s32)viewport->y + world_y - viewport->camera->y);
    const UEffectSample sprite_effect = unsigned_effect_sample(&sprite->effect, 0u, sprite->render.sprite_count);

    sprite_renderer_update_effect_flags(sprite, &sprite_effect);

    if ((sprite_effect.flags & U_EFFECT_SAMPLE_HIDDEN) != 0u) {
        if (sprite->render.visible) {
            unsigned_sprite_renderer_hide(sprite);
        }
        return;
    }

    if ((sprite->render.dirty & U_SPRITE_RENDER_DIRTY_GRAPHICS) != 0u) {
        unsigned_sprite_backend_flush_graphics(sprite, frame, sprite->render.dirty);
        unsigned_sprite_render_clear_dirty(&sprite->render, U_SPRITE_RENDER_DIRTY_GRAPHICS);
    }

    if (sprite_renderer_uses_per_column_effect(sprite, viewport)) {
        unsigned_sprite_backend_write_effect_positions(sprite, &viewport->effect, &sprite->effect, screen_x, screen_y);
        sprite->render.hardware_initialized = true;
        unsigned_sprite_render_mark_dirty(&sprite->render, U_SPRITE_RENDER_DIRTY_TRANSFORM | U_SPRITE_RENDER_DIRTY_LAYOUT);
    } else {
        const UEffectSample viewport_effect = unsigned_effect_sample(&viewport->effect, 0u, sprite->render.sprite_count);
        const UEffectSample composed = unsigned_effect_compose_samples(viewport_effect, sprite_effect);
        const Vec2 pivot = sprite_renderer_scale_pivot_offset(sprite, &sprite_effect);
        const s16 draw_x = (s16)((s32)screen_x + composed.offset_x + pivot.x);
        const s16 draw_y = (s16)((s32)screen_y + composed.offset_y + pivot.y);
        sprite_renderer_draw_chained(sprite, draw_x, draw_y, &composed);
    }

    sprite->render.visible = true;
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
    const s32 sprite_right = world_x + (s32)sprite->definition->width_tiles * 16;
    const s32 sprite_bottom = world_y + (s32)sprite->definition->height_tiles * 16;

    const s32 camera_left = (s32)viewport->camera->x - effect_bounds.offset_x;
    const s32 camera_top = (s32)viewport->camera->y - effect_bounds.offset_y;
    const s32 camera_right = (s32)viewport->camera->x + viewport->width + effect_bounds.offset_x;
    const s32 camera_bottom = (s32)viewport->camera->y + viewport->height + effect_bounds.offset_y;

    return sprite_right > camera_left && world_x < camera_right && sprite_bottom > camera_top && world_y < camera_bottom;
}

bool unsigned_sprite_renderer_relocate(USprite *sprite, u16 first_sprite) {
    if (sprite == NULL || sprite->definition == NULL || sprite->render.sprite_count != sprite->definition->width_tiles || !unsigned_sprite_range_is_valid(first_sprite, sprite->render.sprite_count)) {
        return false;
    }

    if (sprite->render.first_sprite == first_sprite) {
        return true;
    }

    if (sprite->render.hardware_initialized) {
        unsigned_sprite_backend_clear_range(sprite->render.first_sprite, sprite->render.sprite_count);
    }

    sprite->render.first_sprite = first_sprite;
    sprite->render.hardware_initialized = false;
    sprite->render.visible = false;
    unsigned_sprite_render_mark_dirty(&sprite->render, U_SPRITE_RENDER_DIRTY_ALL);
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
        return;
    }

    sprite_renderer_draw_visible(sprite, viewport, position);
}

void unsigned_sprite_renderer_draw_prepared(USprite *sprite, const UViewport *viewport, const Vec2 *position) {
    if (!sprite_renderer_draw_state_is_valid(sprite, viewport, position) || !sprite->render.layout_visible || sprite->current_frame == NULL) {
        return;
    }

    sprite_renderer_draw_visible(sprite, viewport, position);
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
    unsigned_sprite_render_mark_dirty(&sprite->render, U_SPRITE_RENDER_DIRTY_Y);
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
    unsigned_sprite_render_mark_dirty(&sprite->render, U_SPRITE_RENDER_DIRTY_ALL);
}
