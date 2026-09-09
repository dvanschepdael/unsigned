/**
 * @file sprite_backend.c
 * @brief Translates renderer sprite operations into Neo Geo SCB/VRAM writes.
 */

#include "display/sprite/attributes.h"
#include "system/neogeo/video.h"
#include "system/neogeo/vram_writer_internal.h"
#include "system/renderer/renderer_backend.h"

#include <ngdevkit/registers.h>

#define SPRITE_SCB3_STICKY 0x0040u

/** Encodes logical sprite palette/flip attributes into the Neo Geo tile attribute word. */
static u16 sprite_backend_attributes(const USprite *sprite) {
    const USpriteDefinition *definition = sprite->definition;
    const u16 auto_animation = (u16)(definition->auto_animation & U_SPRITE_AUTO_ANIMATION_MASK);
    return (u16)(((u16)sprite->palette << 8u) | auto_animation | sprite->flip_x);
}

/** Returns the graphics tile index used by a logical sprite frame tile. */
static u16 sprite_backend_tile_at(const USprite *sprite, const UFrame *frame, u8 column) {
    const USpriteDefinition *definition = sprite->definition;
    const u8 source_column = sprite->flip_x ? (u8)(definition->width_tiles - 1u - column) : column;
    return (u16)(definition->first_tile + frame->tile_offset + source_column);
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
        u16 tile = sprite_backend_tile_at(sprite, frame, column);
        *REG_VRAMADDR = (u16)(ADDR_SCB1 + (sprite->render.first_sprite + column) * 64u);
        for (u8 row = 0u; row < definition->height_tiles; ++row) {
            *REG_VRAMRW = tile;
            *REG_VRAMRW = attributes;
            tile = (u16)(tile + definition->width_tiles);
        }
    }
}

/** Uploads only SCB1 tile indices when frame graphics changed but attributes stayed valid. */
static void sprite_backend_write_tiles(const USprite *sprite, const UFrame *frame) {
    const USpriteDefinition *definition = sprite->definition;

    unsigned_neogeo_vram_set_mod(2u);
    for (u8 column = 0u; column < sprite->render.sprite_count; ++column) {
        u16 tile = sprite_backend_tile_at(sprite, frame, column);
        *REG_VRAMADDR = (u16)(ADDR_SCB1 + (sprite->render.first_sprite + column) * 64u);
        for (u8 row = 0u; row < definition->height_tiles; ++row) {
            *REG_VRAMRW = tile;
            tile = (u16)(tile + definition->width_tiles);
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

u16 unsigned_sprite_backend_encode_scb2(const USpriteRenderState *render, s16 zoom_offset) {
    s32 shrink_x = (s32)(render->shrink_x & 0x0fu) - zoom_offset / 16;
    s32 shrink_y = (s32)render->shrink_y - zoom_offset;

    if (shrink_x < 0) {
        shrink_x = 0;
    } else if (shrink_x > 0x0f) {
        shrink_x = 0x0f;
    }
    if (shrink_y < 0) {
        shrink_y = 0;
    } else if (shrink_y > 0xff) {
        shrink_y = 0xff;
    }
    return (u16)(((u16)shrink_x << 8u) | (u16)shrink_y);
}

void unsigned_sprite_backend_flush_graphics(const USprite *sprite, const UFrame *frame, u8 dirty) {
    if ((dirty & U_SPRITE_RENDER_DIRTY_GRAPHICS) == U_SPRITE_RENDER_DIRTY_GRAPHICS) {
        sprite_backend_write_graphics_full(sprite, frame);
        return;
    }
    if ((dirty & U_SPRITE_RENDER_DIRTY_TILES) != 0u) {
        sprite_backend_write_tiles(sprite, frame);
    }
    if ((dirty & U_SPRITE_RENDER_DIRTY_ATTRIBUTES) != 0u) {
        sprite_backend_write_attributes(sprite);
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

void unsigned_sprite_backend_write_effect_positions(const USprite *sprite, const UViewport *viewport, s16 screen_x, s16 screen_y) {
    const u16 first_sprite = sprite->render.first_sprite;
    const u8 sprite_count = sprite->render.sprite_count;

    unsigned_neogeo_vram_set_mod(0x200u);
    for (u8 column = 0u; column < sprite_count; ++column) {
        const UEffectSample effect = unsigned_effect_sample(&viewport->effect, column, sprite_count);
        const u16 scb2 = unsigned_sprite_backend_encode_scb2(&sprite->render, effect.zoom_offset);

        *REG_VRAMADDR = (u16)(ADDR_SCB2 + first_sprite + column);
        *REG_VRAMRW = scb2;
        *REG_VRAMRW = (u16)(((s32)UNSIGNED_VIDEO_SCB3_Y_ORIGIN - screen_y - effect.offset_y) * 128 + sprite->definition->height_tiles);
        *REG_VRAMRW = (u16)(((s32)screen_x + (s32)column * 16 + effect.offset_x) * 128);
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
