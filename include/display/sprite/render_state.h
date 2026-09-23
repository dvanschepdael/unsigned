/**
 * @file render_state.h
 * @brief Cached software mirror of one logical sprite's Neo Geo hardware state.
 */

#ifndef UNSIGNED_DISPLAY_SPRITE_RENDER_STATE_H
#define UNSIGNED_DISPLAY_SPRITE_RENDER_STATE_H

#include "core/types.h"
#include "renderer/prepared_column.h"

#define U_SPRITE_RENDER_DIRTY_TILES 0x01u
#define U_SPRITE_RENDER_DIRTY_ATTRIBUTES 0x02u
#define U_SPRITE_RENDER_DIRTY_X 0x04u
#define U_SPRITE_RENDER_DIRTY_Y 0x08u
#define U_SPRITE_RENDER_DIRTY_SCALE 0x10u
#define U_SPRITE_RENDER_DIRTY_LAYOUT 0x20u
#define U_SPRITE_RENDER_DIRTY_PADDING 0x40u
#define U_SPRITE_RENDER_DIRTY_CHAIN_BOUNDARY 0x80u

#define U_SPRITE_RENDER_DIRTY_PALETTE U_SPRITE_RENDER_DIRTY_ATTRIBUTES
#define U_SPRITE_RENDER_DIRTY_GRAPHICS (U_SPRITE_RENDER_DIRTY_TILES | U_SPRITE_RENDER_DIRTY_ATTRIBUTES)
#define U_SPRITE_RENDER_DIRTY_POSITION (U_SPRITE_RENDER_DIRTY_X | U_SPRITE_RENDER_DIRTY_Y)
#define U_SPRITE_RENDER_DIRTY_TRANSFORM (U_SPRITE_RENDER_DIRTY_POSITION | U_SPRITE_RENDER_DIRTY_SCALE)
#define U_SPRITE_RENDER_DIRTY_RELOCATION (U_SPRITE_RENDER_DIRTY_GRAPHICS | U_SPRITE_RENDER_DIRTY_TRANSFORM | U_SPRITE_RENDER_DIRTY_LAYOUT | U_SPRITE_RENDER_DIRTY_CHAIN_BOUNDARY)
#define U_SPRITE_RENDER_DIRTY_ALL (U_SPRITE_RENDER_DIRTY_RELOCATION | U_SPRITE_RENDER_DIRTY_PADDING)

/** Previous SCB1 ownership snapshot retained only across stable-layout relocation. */
typedef struct USpritePreviousRenderState {
    u16 first_sprite;
    const void *frame;
    u8 palette;
    u8 flip_flags;
    bool range_valid;
    bool graphics_valid;
} USpritePreviousRenderState;

/** Hardware-sprite range assigned by the actor layout phase. */
typedef struct USpriteLayoutRenderState {
    u16 first_sprite;
    u8 sprite_count;
    bool assigned;
    /** Visibility cached by actor_renderer_prepare for the prepared rendering path. */
    bool visible;
} USpriteLayoutRenderState;

/** Last hardware state committed for the currently assigned sprite range. */
typedef struct USpriteCommittedRenderState {
    const void *frame;
    s16 x;
    s16 y;
    u16 scb2;
    u8 palette;
    u8 flip_flags;
    bool graphics_valid;
    bool initialized;
    /** True when the current hardware range contains one valid SCB3 driver + sticky chain. */
    bool chained;
    bool visible;
} USpriteCommittedRenderState;

/** CPU-side transform frozen during active display and consumed during the commit phase. */
typedef struct USpritePreparedRenderState {
    s16 screen_y;
    s16 draw_x;
    s16 draw_y;
    u16 scb2;
    /** Optional per-column slice in the owning frame plan; NULL means no per-column transform data. */
    const UPreparedColumn *columns;
    bool valid;
    bool hidden;
    bool per_column;
} USpritePreparedRenderState;

typedef struct USpriteRenderState {
    USpriteCommittedRenderState committed;
    USpritePreviousRenderState previous;
    USpritePreparedRenderState prepared;
    USpriteLayoutRenderState layout;
    u8 shrink_x;
    u8 shrink_y;
    /** Presentation-effect mirror toggles last selected by the sprite renderer. */
    u8 effect_flip_x;
    u8 effect_flip_y;
    u8 dirty;
} USpriteRenderState;

/** Add dirty bits without disturbing already pending changes. @pre `render` is valid. */
static inline void unsigned_sprite_render_mark_dirty(USpriteRenderState *render, u8 dirty) {
    render->dirty |= dirty;
}

/** Clear dirty bits after the corresponding hardware state has been committed. @pre `render` is valid. */
static inline void unsigned_sprite_render_clear_dirty(USpriteRenderState *render, u8 dirty) {
    render->dirty &= (u8)~dirty;
}

#endif
