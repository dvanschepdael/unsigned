/**
 * @file render_state.h
 * @brief Cached software mirror of one logical sprite's Neo Geo hardware state.
 */

#ifndef UNSIGNED_DISPLAY_SPRITE_RENDER_STATE_H
#define UNSIGNED_DISPLAY_SPRITE_RENDER_STATE_H

#include "core/types.h"

#define U_SPRITE_RENDER_DIRTY_TILES 0x01u
#define U_SPRITE_RENDER_DIRTY_ATTRIBUTES 0x02u
#define U_SPRITE_RENDER_DIRTY_X 0x04u
#define U_SPRITE_RENDER_DIRTY_Y 0x08u
#define U_SPRITE_RENDER_DIRTY_SCALE 0x10u
#define U_SPRITE_RENDER_DIRTY_LAYOUT 0x20u

#define U_SPRITE_RENDER_DIRTY_PALETTE U_SPRITE_RENDER_DIRTY_ATTRIBUTES
#define U_SPRITE_RENDER_DIRTY_GRAPHICS (U_SPRITE_RENDER_DIRTY_TILES | U_SPRITE_RENDER_DIRTY_ATTRIBUTES)
#define U_SPRITE_RENDER_DIRTY_POSITION (U_SPRITE_RENDER_DIRTY_X | U_SPRITE_RENDER_DIRTY_Y)
#define U_SPRITE_RENDER_DIRTY_TRANSFORM (U_SPRITE_RENDER_DIRTY_POSITION | U_SPRITE_RENDER_DIRTY_SCALE)
#define U_SPRITE_RENDER_DIRTY_ALL (U_SPRITE_RENDER_DIRTY_GRAPHICS | U_SPRITE_RENDER_DIRTY_TRANSFORM | U_SPRITE_RENDER_DIRTY_LAYOUT)

typedef struct USpriteRenderState {
    /** First Neo Geo hardware sprite owned by the logical sprite. */
    u16 first_sprite;
    /** Number of consecutive hardware sprite columns owned. */
    u8 sprite_count;
    u8 shrink_x;
    u8 shrink_y;
    /** Last committed driver X/Y and SCB2, used for change detection. */
    s16 rendered_x;
    s16 rendered_y;
    u16 rendered_scb2;
    u8 dirty;
    bool hardware_initialized;
    bool visible;
    /** Visibility cached by actor_renderer_prepare for the prepared rendering path. */
    bool layout_visible;
} USpriteRenderState;

/** Add dirty bits without disturbing already pending changes. */
static inline void unsigned_sprite_render_mark_dirty(USpriteRenderState *render, u8 dirty) {
    if (render != NULL) {
        render->dirty |= dirty;
    }
}

/** Clear dirty bits after the corresponding hardware state has been committed. */
static inline void unsigned_sprite_render_clear_dirty(USpriteRenderState *render, u8 dirty) {
    if (render != NULL) {
        render->dirty &= (u8)~dirty;
    }
}

#endif
