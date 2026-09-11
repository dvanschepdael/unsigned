/**
 * @file sprite_renderer.c
 * @brief Orchestrates sprite visibility, dirty state and viewport effects.
 */

#include "renderer/sprite_renderer.h"

#include "display/sprite/limits.h"
#include "renderer/renderer_backend.h"

/**
 * Mark transform fields that differ from the last chained hardware state.
 * Dirty bits let the backend avoid rewriting unchanged SCB2/SCB3/SCB4 values.
 */
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

/**
 * Commit one chained transform and then make the software render state authoritative.
 * Layout changes rebuild the chain; ordinary movement only touches the changed axes.
 */
static void sprite_renderer_flush_chained_transform(USprite *sprite, s16 screen_x, s16 screen_y, u16 scb2) {
    USpriteRenderState *render = &sprite->render;

    unsigned_sprite_backend_flush_chained_transform(sprite, screen_x, screen_y, scb2, render->dirty);

    render->rendered_x = screen_x;
    render->rendered_y = screen_y;
    render->rendered_scb2 = scb2;
    render->hardware_initialized = true;
    unsigned_sprite_render_clear_dirty(render, U_SPRITE_RENDER_DIRTY_TRANSFORM | U_SPRITE_RENDER_DIRTY_LAYOUT);
}

/** Draw a sprite as one Neo Geo chain, optionally applying a uniform zoom effect. */
static void sprite_renderer_draw_chained(USprite *sprite, s16 screen_x, s16 screen_y, s16 zoom_offset) {
    const u16 scb2 = unsigned_sprite_backend_encode_scb2(&sprite->render, zoom_offset);

    sprite_renderer_detect_chained_changes(sprite, screen_x, screen_y, scb2);
    sprite_renderer_flush_chained_transform(sprite, screen_x, screen_y, scb2);
}

/** Validate everything required before the renderer may touch a sprite's hardware range. */
static bool sprite_renderer_draw_state_is_valid(const USprite *sprite, const UViewport *viewport, const Vec2 *position) {
    return sprite != NULL && sprite->definition != NULL && viewport != NULL && viewport->camera != NULL && viewport->width != 0u && viewport->height != 0u && position != NULL && sprite->render.sprite_count == sprite->definition->width_tiles &&
           unsigned_sprite_height_is_valid(sprite->definition->height_tiles) && unsigned_sprite_range_is_valid(sprite->render.first_sprite, sprite->render.sprite_count);
}

/**
 * Render a sprite already known to be visible.
 *
 * Graphics dirty bits update SCB1. Uniform effects preserve the hardware chain and only
 * offset/zoom its driver. Per-column effects break the normal chain representation for
 * the frame, so transform/layout bits are dirtied to force a clean chained rebuild later.
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

    if ((sprite->render.dirty & U_SPRITE_RENDER_DIRTY_GRAPHICS) != 0u) {
        unsigned_sprite_backend_flush_graphics(sprite, frame, sprite->render.dirty);
        unsigned_sprite_render_clear_dirty(&sprite->render, U_SPRITE_RENDER_DIRTY_GRAPHICS);
    }

    if (viewport->effect.function == NULL) {
        sprite_renderer_draw_chained(sprite, screen_x, screen_y, 0);
    } else if (viewport->effect.layout == U_EFFECT_LAYOUT_UNIFORM) {
        const UEffectSample effect = unsigned_effect_sample(&viewport->effect, 0u, sprite->render.sprite_count);
        sprite_renderer_draw_chained(sprite, (s16)(screen_x + effect.offset_x), (s16)(screen_y + effect.offset_y), effect.zoom_offset);
    } else {
        unsigned_sprite_backend_write_effect_positions(sprite, viewport, screen_x, screen_y);
        sprite->render.hardware_initialized = true;
        unsigned_sprite_render_mark_dirty(&sprite->render, U_SPRITE_RENDER_DIRTY_TRANSFORM | U_SPRITE_RENDER_DIRTY_LAYOUT);
    }

    sprite->render.visible = true;
}

/**
 * Hide every hardware sprite in a validated range.
 * This is used by higher-level renderers when a previously allocated range is released.
 */
void unsigned_sprite_renderer_clear_range(u16 first_sprite, u16 sprite_count) {
    if (sprite_count == 0u || first_sprite < UNSIGNED_SPRITE_FIRST || first_sprite > UNSIGNED_SPRITE_LAST || (u32)first_sprite + sprite_count - 1u > UNSIGNED_SPRITE_LAST) {
        return;
    }

    unsigned_sprite_backend_clear_range(first_sprite, sprite_count);
}

/**
 * Test world-space visibility against the viewport, including the maximum displacement
 * produced by the active viewport effect. If effect bounds cannot be determined, the
 * conservative result is visible so a sprite is never incorrectly culled.
 */
bool unsigned_sprite_renderer_is_visible(const USprite *sprite, const UViewport *viewport, const Vec2 *position) {
    UEffectBounds effect_bounds = { 0 };

    if (sprite == NULL || sprite->definition == NULL || viewport == NULL || viewport->camera == NULL || viewport->width == 0u || viewport->height == 0u || position == NULL) {
        return false;
    }

    if (!unsigned_effect_get_bounds(&viewport->effect, sprite->render.sprite_count, &effect_bounds)) {
        return true;
    }

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

/**
 * Move a logical sprite to another contiguous Neo Geo sprite range.
 * Existing hardware ownership is cleared before changing the range, then every render
 * component is dirtied so the next draw fully reconstructs the sprite.
 */
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

/**
 * Cull and render one sprite. This is the normal entry point when visibility has not
 * already been computed by a higher-level renderer.
 */
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

/**
 * Render a sprite whose layout/visibility was already prepared by actor_renderer.
 * This avoids repeating culling when actors are sorted and allocated as a batch.
 */
void unsigned_sprite_renderer_draw_prepared(USprite *sprite, const UViewport *viewport, const Vec2 *position) {
    if (!sprite_renderer_draw_state_is_valid(sprite, viewport, position) || !sprite->render.layout_visible || sprite->current_frame == NULL) {
        return;
    }

    sprite_renderer_draw_visible(sprite, viewport, position);
}

/**
 * Temporarily hide a sprite while preserving its hardware allocation.
 * When the chain layout is still valid, hiding only the driver is cheaper than clearing
 * every SCB3 entry; otherwise the whole owned range is cleared for safety.
 */
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

/**
 * Release all renderer ownership associated with a sprite.
 * The sprite remains a valid gameplay/display object, but its next allocation must fully
 * rebuild graphics, transform and chain state.
 */
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
