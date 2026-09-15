/**
 * @file sprite_backend.c
 * @brief Translates renderer sprite operations into Neo Geo SCB/VRAM writes.
 */

#include "display/sprite/config.h"
#include "display/sprite/limits.h"
#include "display/sprite/scale.h"
#include "system/renderer_backend.h"
#include "system/video.h"
#include "system/vram_writer_internal.h"

#include <ngdevkit/registers.h>

#define SPRITE_SCB3_STICKY 0x0040u

static bool sprite_backend_flip_x(const USprite *sprite) {
    return (sprite->flip_x != 0u) != (sprite->render.effect_flip_x != 0u);
}

static bool sprite_backend_flip_y(const USprite *sprite) {
    return (sprite->flip_y != 0u) != (sprite->render.effect_flip_y != 0u);
}

/** Encodes logical palette plus base/effect mirror state into the Neo Geo tile attribute word. */
static u16 sprite_backend_attributes(const USprite *sprite) {
    const USpriteDefinition *definition = sprite->definition;
    const u16 auto_animation = (u16)(definition->auto_animation & U_SPRITE_AUTO_ANIMATION_MASK);
    const u16 flip_x = sprite_backend_flip_x(sprite) ? U_SPRITE_FLIP_X_MASK : 0u;
    const u16 flip_y = sprite_backend_flip_y(sprite) ? U_SPRITE_FLIP_Y_MASK : 0u;
    return (u16)(((u16)sprite->palette << 8u) | auto_animation | flip_x | flip_y);
}

/** Returns the source tile for one destination column/row after logical mirroring. */
static u16 sprite_backend_tile_at(const USprite *sprite, const UFrame *frame, u8 column, u8 row) {
    const USpriteDefinition *definition = sprite->definition;
    const u8 source_column = sprite_backend_flip_x(sprite) ? (u8)(definition->width_tiles - 1u - column) : column;
    const u8 source_row = sprite_backend_flip_y(sprite) ? (u8)(definition->height_tiles - 1u - row) : row;
    return (u16)(definition->first_tile + frame->tile_offset + (u16)source_row * definition->width_tiles + source_column);
}

/** Encodes a logical sprite Y position and height into the Neo Geo SCB3 register format. */
static u16 sprite_backend_scb3(const USprite *sprite, s16 screen_y) {
    return (u16)(((s32)UNSIGNED_VIDEO_SCB3_Y_ORIGIN - screen_y) * 128 + sprite->definition->height_tiles);
}

/** Uploads tile indices and attributes together for every tile in the sprite hardware range. */
static void sprite_backend_write_graphics_full(const USprite *sprite, const UFrame *frame) {
    const USpriteDefinition *definition = sprite->definition;
    const u16 attributes = sprite_backend_attributes(sprite);

    unsigned_neogeo_vram_set_mod(1u);
    for (u8 column = 0u; column < sprite->render.sprite_count; ++column) {
        *REG_VRAMADDR = (u16)(ADDR_SCB1 + (sprite->render.first_sprite + column) * 64u);
        for (u8 row = 0u; row < definition->height_tiles; ++row) {
            *REG_VRAMRW = sprite_backend_tile_at(sprite, frame, column, row);
            *REG_VRAMRW = attributes;
        }
    }
}

/**
 * Fill SCB1 rows outside the logical sprite height with a known transparent tile.
 *
 * Vertical shrinking on Neo Geo does not reduce the SCB3 display window. The
 * L0 zoom lookup can therefore address rows below `height_tiles`; if those rows
 * contain stale data from a previous owner, isolated garbage pixels become
 * visible. Padding is written only when the hardware layout is rebuilt, not on
 * normal animation-frame changes.
 */
static void sprite_backend_write_unused_rows(const USprite *sprite) {
    const USpriteDefinition *definition = sprite->definition;

    if (!definition->clear_unused_rows || definition->height_tiles >= UNSIGNED_SPRITE_MAX_HEIGHT_TILES) {
        return;
    }

    unsigned_neogeo_vram_set_mod(1u);
    for (u8 column = 0u; column < sprite->render.sprite_count; ++column) {
        *REG_VRAMADDR = (u16)(ADDR_SCB1 + (sprite->render.first_sprite + column) * 64u + (u16)definition->height_tiles * 2u);
        for (u8 row = definition->height_tiles; row < UNSIGNED_SPRITE_MAX_HEIGHT_TILES; ++row) {
            *REG_VRAMRW = definition->transparent_tile;
            *REG_VRAMRW = 0u;
        }
    }
}

/** Uploads only SCB1 tile indices when frame graphics changed but attributes stayed valid. */
static void sprite_backend_write_tiles(const USprite *sprite, const UFrame *frame) {
    const USpriteDefinition *definition = sprite->definition;

    unsigned_neogeo_vram_set_mod(2u);
    for (u8 column = 0u; column < sprite->render.sprite_count; ++column) {
        *REG_VRAMADDR = (u16)(ADDR_SCB1 + (sprite->render.first_sprite + column) * 64u);
        for (u8 row = 0u; row < definition->height_tiles; ++row) {
            *REG_VRAMRW = sprite_backend_tile_at(sprite, frame, column, row);
        }
    }
}

/** Uploads only SCB1 palette/flip attributes when tile indices can be reused. */
static void sprite_backend_write_attributes(const USprite *sprite) {
    const u16 attributes = sprite_backend_attributes(sprite);

    unsigned_neogeo_vram_set_mod(2u);
    for (u8 column = 0u; column < sprite->render.sprite_count; ++column) {
        *REG_VRAMADDR = (u16)(ADDR_SCB1 + (sprite->render.first_sprite + column) * 64u + 1u);
        for (u8 row = 0u; row < sprite->definition->height_tiles; ++row) {
            *REG_VRAMRW = attributes;
        }
    }
}

/** Uploads the same SCB2 shrink value to every hardware column in the logical sprite chain. */
static void sprite_backend_write_shrink(const USprite *sprite, u16 scb2) {
    unsigned_neogeo_vram_set_mod(1u);
    *REG_VRAMADDR = (u16)(ADDR_SCB2 + sprite->render.first_sprite);
    for (u8 column = 0u; column < sprite->render.sprite_count; ++column) {
        *REG_VRAMRW = scb2;
    }
}

/** Rebuilds SCB3 with one Y/height driver followed by sticky chained columns. */
static void sprite_backend_write_chain(const USprite *sprite, s16 screen_y) {
    unsigned_neogeo_vram_set_mod(1u);
    *REG_VRAMADDR = (u16)(ADDR_SCB3 + sprite->render.first_sprite);
    *REG_VRAMRW = sprite_backend_scb3(sprite, screen_y);
    for (u8 column = 1u; column < sprite->render.sprite_count; ++column) {
        *REG_VRAMRW = SPRITE_SCB3_STICKY;
    }
}

/** Uploads the chain driver X coordinate to SCB4 in Neo Geo fixed hardware units. */
static void sprite_backend_write_driver_x(const USprite *sprite, s16 screen_x) {
    unsigned_neogeo_vram_set_mod(1u);
    *REG_VRAMADDR = (u16)(ADDR_SCB4 + sprite->render.first_sprite);
    *REG_VRAMRW = (u16)((s32)screen_x * 128);
}

/** Uploads the chain driver Y/height word to SCB3. */
static void sprite_backend_write_driver_y(const USprite *sprite, s16 screen_y) {
    unsigned_neogeo_vram_set_mod(1u);
    *REG_VRAMADDR = (u16)(ADDR_SCB3 + sprite->render.first_sprite);
    *REG_VRAMRW = sprite_backend_scb3(sprite, screen_y);
}

/** Uploads contiguous SCB3/SCB4 driver coordinates in one VRAM stride when both axes changed. */
static void sprite_backend_write_driver_xy(const USprite *sprite, s16 screen_x, s16 screen_y) {
    unsigned_neogeo_vram_set_mod(0x200u);
    *REG_VRAMADDR = (u16)(ADDR_SCB3 + sprite->render.first_sprite);
    *REG_VRAMRW = sprite_backend_scb3(sprite, screen_y);
    *REG_VRAMRW = (u16)((s32)screen_x * 128);
}

u16 unsigned_sprite_backend_encode_scb2(const USpriteRenderState *render, const UEffectSample *effect) {
    const UEffectSample identity = unsigned_effect_identity_sample();
    const UEffectSample *sample = effect != NULL ? effect : &identity;
    s32 shrink_x = unsigned_sprite_scale_shrink_x(render->shrink_x, sample->scale_x);
    s32 shrink_y = unsigned_sprite_scale_shrink_y(render->shrink_y, sample->scale_y);

    shrink_x -= sample->zoom_offset / 16;
    shrink_y -= sample->zoom_offset;

    if (shrink_x < 0) {
        shrink_x = 0;
    } else if (shrink_x > (s32)U_SPRITE_SHRINK_X_FULL) {
        shrink_x = U_SPRITE_SHRINK_X_FULL;
    }
    if (shrink_y < 0) {
        shrink_y = 0;
    } else if (shrink_y > (s32)U_SPRITE_SHRINK_Y_FULL) {
        shrink_y = U_SPRITE_SHRINK_Y_FULL;
    }
    return (u16)(((u16)shrink_x << 8u) | (u16)shrink_y);
}

void unsigned_sprite_backend_flush_graphics(const USprite *sprite, const UFrame *frame, u8 dirty) {
    if ((dirty & U_SPRITE_RENDER_DIRTY_GRAPHICS) == U_SPRITE_RENDER_DIRTY_GRAPHICS) {
        sprite_backend_write_graphics_full(sprite, frame);
    } else {
        if ((dirty & U_SPRITE_RENDER_DIRTY_TILES) != 0u) {
            sprite_backend_write_tiles(sprite, frame);
        }
        if ((dirty & U_SPRITE_RENDER_DIRTY_ATTRIBUTES) != 0u) {
            sprite_backend_write_attributes(sprite);
        }
    }

    if ((dirty & U_SPRITE_RENDER_DIRTY_LAYOUT) != 0u) {
        sprite_backend_write_unused_rows(sprite);
    }
}

void unsigned_sprite_backend_flush_chained_transform(const USprite *sprite, s16 screen_x, s16 screen_y, u16 scb2, u8 dirty) {
    if ((dirty & U_SPRITE_RENDER_DIRTY_SCALE) != 0u) {
        sprite_backend_write_shrink(sprite, scb2);
    }

    if ((dirty & U_SPRITE_RENDER_DIRTY_LAYOUT) != 0u) {
        sprite_backend_write_chain(sprite, screen_y);
        sprite_backend_write_driver_x(sprite, screen_x);
    } else if ((dirty & U_SPRITE_RENDER_DIRTY_POSITION) == U_SPRITE_RENDER_DIRTY_POSITION) {
        sprite_backend_write_driver_xy(sprite, screen_x, screen_y);
    } else if ((dirty & U_SPRITE_RENDER_DIRTY_X) != 0u) {
        sprite_backend_write_driver_x(sprite, screen_x);
    } else if ((dirty & U_SPRITE_RENDER_DIRTY_Y) != 0u) {
        sprite_backend_write_driver_y(sprite, screen_y);
    }
}

/**
 * Write independent SCB2/SCB3/SCB4 values when either effect breaks hardware chaining.
 * A uniform sprite scale still compacts columns and applies its pivot as one logical sprite;
 * viewport per-column offsets are then layered on top without leaking composition policy into
 * actor/game code.
 */
void unsigned_sprite_backend_write_effect_positions(const USprite *sprite, const UEffect *viewport_effect, const UEffect *sprite_effect, s16 screen_x, s16 screen_y) {
    const u16 first_sprite = sprite->render.first_sprite;
    const u8 sprite_count = sprite->render.sprite_count;
    const bool sprite_uniform = !unsigned_effect_is_per_column(sprite_effect);
    const UEffectSample uniform_sprite = unsigned_effect_sample(sprite_effect, 0u, sprite_count);
    const u8 base_shrink_x = (u8)(sprite->render.shrink_x & U_SPRITE_SHRINK_X_FULL);
    const u16 base_column_width = (u16)base_shrink_x + 1u;
    const u16 base_width = unsigned_sprite_scaled_width_pixels(sprite_count, base_shrink_x);
    const u16 base_height = unsigned_sprite_scaled_height_pixels(sprite->definition->height_tiles, sprite->render.shrink_y);
    u16 uniform_column_width = base_column_width;
    s16 uniform_pivot_x = 0;
    s16 uniform_pivot_y = 0;

    if (sprite_uniform) {
        const u8 scaled_x = unsigned_sprite_scale_shrink_x(sprite->render.shrink_x, uniform_sprite.scale_x);
        const u8 scaled_y = unsigned_sprite_scale_shrink_y(sprite->render.shrink_y, uniform_sprite.scale_y);
        const u16 scaled_width = unsigned_sprite_scaled_width_pixels(sprite_count, scaled_x);
        const u16 scaled_height = unsigned_sprite_scaled_height_pixels(sprite->definition->height_tiles, scaled_y);
        uniform_column_width = (u16)scaled_x + 1u;
        uniform_pivot_x = unsigned_sprite_scale_pivot_offset(base_width, scaled_width, uniform_sprite.pivot_x);
        uniform_pivot_y = unsigned_sprite_scale_pivot_offset(base_height, scaled_height, uniform_sprite.pivot_y);
    }

    unsigned_neogeo_vram_set_mod(0x200u);
    for (u8 column = 0u; column < sprite_count; ++column) {
        const UEffectSample viewport_sample = unsigned_effect_sample(viewport_effect, column, sprite_count);
        const UEffectSample local_sprite = sprite_uniform ? uniform_sprite : unsigned_effect_sample(sprite_effect, column, sprite_count);
        const UEffectSample composed = unsigned_effect_compose_samples(viewport_sample, local_sprite);
        const u16 scb2 = unsigned_sprite_backend_encode_scb2(&sprite->render, &composed);
        s16 pivot_x = uniform_pivot_x;
        s16 pivot_y = uniform_pivot_y;
        u16 column_step = uniform_column_width;

        if (!sprite_uniform) {
            const u8 scaled_x = unsigned_sprite_scale_shrink_x(sprite->render.shrink_x, local_sprite.scale_x);
            const u8 scaled_y = unsigned_sprite_scale_shrink_y(sprite->render.shrink_y, local_sprite.scale_y);
            const u16 scaled_column_width = (u16)scaled_x + 1u;
            const u16 scaled_height = unsigned_sprite_scaled_height_pixels(sprite->definition->height_tiles, scaled_y);
            pivot_x = unsigned_sprite_scale_pivot_offset(base_column_width, scaled_column_width, local_sprite.pivot_x);
            pivot_y = unsigned_sprite_scale_pivot_offset(base_height, scaled_height, local_sprite.pivot_y);
            column_step = base_column_width;
        }

        *REG_VRAMADDR = (u16)(ADDR_SCB2 + first_sprite + column);
        *REG_VRAMRW = scb2;
        if ((composed.flags & U_EFFECT_SAMPLE_HIDDEN) != 0u) {
            *REG_VRAMRW = 0u;
        } else {
            *REG_VRAMRW = (u16)(((s32)UNSIGNED_VIDEO_SCB3_Y_ORIGIN - screen_y - composed.offset_y - pivot_y) * 128 + sprite->definition->height_tiles);
        }
        *REG_VRAMRW = (u16)(((s32)screen_x + (s32)column * column_step + composed.offset_x + pivot_x) * 128);
    }
}

void unsigned_sprite_backend_clear_range(u16 first_sprite, u16 sprite_count) {
    unsigned_neogeo_vram_set_mod(1u);
    *REG_VRAMADDR = (u16)(ADDR_SCB3 + first_sprite);
    for (u16 column = 0u; column < sprite_count; ++column) {
        *REG_VRAMRW = 0u;
    }
}

void unsigned_sprite_backend_hide_chain(const USprite *sprite) {
    unsigned_neogeo_vram_set_mod(1u);
    *REG_VRAMADDR = (u16)(ADDR_SCB3 + sprite->render.first_sprite);
    *REG_VRAMRW = 0u;
}
