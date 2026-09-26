/**
 * @file sprite_backend.c
 * @brief Translates renderer sprite operations into Neo Geo SCB/VRAM writes.
 */

#include "core/math/math.h"
#include "display/sprite/config.h"
#include "display/sprite/limits.h"
#include "display/sprite/scale.h"
#include "renderer/prepared_column.h"
#include "system/sprite_backend.h"

#include "system/m68k_hotpath.h"
#include "system/video.h"
#include "system/vram_writer_internal.h"

#include <ngdevkit/registers.h>

#define SPRITE_SCB3_STICKY 0x0040u

typedef struct USpritePaddingState {
    u16 transparent_tile;
    u8 clean_from_row;
    bool valid;
} USpritePaddingState;

/*
 * Software mirror of the unused SCB1 rows for each hardware sprite column.
 * clean_from_row means every row [clean_from_row, 31] is known to contain
 * transparent_tile with zero attributes. This lets Y-order relocations reuse
 * already-safe padding instead of rewriting up to 31 rows per column.
 */
static USpritePaddingState sprite_padding_state[UNSIGNED_SPRITE_LAST + 1u];

/** Record visible-row writes without discarding any still-valid padding below them. */
static void sprite_backend_note_graphics_write(const USprite *sprite) {
    const u8 height = sprite->definition->height_tiles;

    for (u8 column = 0u; column < sprite->render.layout.sprite_count; ++column) {
        USpritePaddingState *state = &sprite_padding_state[sprite->render.layout.first_sprite + column];
        if (state->valid && state->clean_from_row < height) {
            state->clean_from_row = height;
        }
    }
}

bool unsigned_sprite_backend_padding_is_valid(const USprite *sprite, u16 first_sprite) {
    const USpriteDefinition *definition = sprite->definition;
    if (!definition->clear_unused_rows || definition->height_tiles >= UNSIGNED_SPRITE_MAX_HEIGHT_TILES) {
        return true;
    }

    for (u8 column = 0u; column < sprite->render.layout.sprite_count; ++column) {
        const USpritePaddingState *state = &sprite_padding_state[first_sprite + column];
        if (!state->valid || state->transparent_tile != definition->transparent_tile || state->clean_from_row > definition->height_tiles) {
            return false;
        }
    }
    return true;
}

void unsigned_sprite_backend_invalidate_padding_range(u16 first_sprite, u16 sprite_count) {
    for (u16 column = 0u; column < sprite_count; ++column) {
        sprite_padding_state[first_sprite + column].valid = false;
    }
}

static bool sprite_backend_flip_x(const USprite *sprite) {
    return (sprite->flip_x != 0u) != (sprite->render.effect_flip_x != 0u);
}

static bool sprite_backend_flip_y(const USprite *sprite) {
    return (sprite->flip_y != 0u) != (sprite->render.effect_flip_y != 0u);
}

/** Encodes logical palette plus base/effect mirror state into the Neo Geo tile attribute word. */
static u16 sprite_backend_attributes(const USprite *sprite) {
    const USpriteDefinition *definition = sprite->definition;
    const u16 auto_animation = definition->auto_animation;
    const u16 flip_x = sprite_backend_flip_x(sprite) ? U_SPRITE_FLIP_X_MASK : 0u;
    const u16 flip_y = sprite_backend_flip_y(sprite) ? U_SPRITE_FLIP_Y_MASK : 0u;
    return (u16)(((u16)sprite->palette << 8u) | auto_animation | flip_x | flip_y);
}

/** Encodes a logical sprite Y position and height into the Neo Geo SCB3 register format. */
static u16 sprite_backend_scb3(const USprite *sprite, s16 screen_y) {
    return (u16)(((s32)UNSIGNED_VIDEO_SCB3_Y_ORIGIN - screen_y) * 128 + sprite->definition->height_tiles);
}

/** Uploads tile indices and attributes together for every tile in the sprite hardware range. */
static void sprite_backend_write_graphics_full(const USprite *sprite, const UFrame *frame) {
    const USpriteDefinition *definition = sprite->definition;
    const u16 attributes = sprite_backend_attributes(sprite);
    const u16 frame_first_tile = (u16)(definition->first_tile + frame->tile_offset);
    const bool flip_x = sprite_backend_flip_x(sprite);
    const bool flip_y = sprite_backend_flip_y(sprite);
    const u8 width = definition->width_tiles;
    const u8 height = definition->height_tiles;
    const u16 last_row_offset = flip_y ? (u16)(height - 1u) * width : 0u;
    u16 scb1_address = (u16)(ADDR_SCB1 + sprite->render.layout.first_sprite * 64u);

    /* Keep mirror decisions outside the SCB1 row loop. On 68000 this is a hot path whenever an
     * animation frame changes; repeated flag tests and row*width multiplies cost more than the
     * small duplicated forward/reverse loops. */
    unsigned_neogeo_vram_set_mod(1u);
    for (u8 column = 0u; column < sprite->render.layout.sprite_count; ++column) {
        const u8 source_column = flip_x ? (u8)(width - 1u - column) : column;
        *REG_VRAMADDR = scb1_address;
        scb1_address = (u16)(scb1_address + 64u);

        if (!flip_y) {
            u16 tile = (u16)(frame_first_tile + source_column);
#if defined(__m68k__)
            unsigned_m68k_vram_stream_tile_attributes(REG_VRAMRW, tile, attributes, height, (s16)width);
#else
            for (u8 row = 0u; row < height; ++row) {
                *REG_VRAMRW = tile;
                *REG_VRAMRW = attributes;
                tile = (u16)(tile + width);
            }
#endif
        } else {
            u16 tile = (u16)(frame_first_tile + last_row_offset + source_column);
#if defined(__m68k__)
            unsigned_m68k_vram_stream_tile_attributes(REG_VRAMRW, tile, attributes, height, (s16)-(s16)width);
#else
            for (u8 row = 0u; row < height; ++row) {
                *REG_VRAMRW = tile;
                *REG_VRAMRW = attributes;
                tile = (u16)(tile - width);
            }
#endif
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

    u16 scb1_address = (u16)(ADDR_SCB1 + sprite->render.layout.first_sprite * 64u + (u16)definition->height_tiles * 2u);

    unsigned_neogeo_vram_set_mod(1u);
    for (u8 column = 0u; column < sprite->render.layout.sprite_count; ++column) {
        const u16 hardware_sprite = (u16)(sprite->render.layout.first_sprite + column);
        *REG_VRAMADDR = scb1_address;
        scb1_address = (u16)(scb1_address + 64u);
#if defined(__m68k__)
        unsigned_m68k_vram_fill_pair(REG_VRAMRW, definition->transparent_tile, 0u, (u16)(UNSIGNED_SPRITE_MAX_HEIGHT_TILES - definition->height_tiles));
#else
        for (u8 row = definition->height_tiles; row < UNSIGNED_SPRITE_MAX_HEIGHT_TILES; ++row) {
            *REG_VRAMRW = definition->transparent_tile;
            *REG_VRAMRW = 0u;
        }
#endif

        sprite_padding_state[hardware_sprite] = (USpritePaddingState){
            .transparent_tile = definition->transparent_tile,
            .clean_from_row = definition->height_tiles,
            .valid = true,
        };
    }
}

/** Uploads only SCB1 tile indices when frame graphics changed but attributes stayed valid. */
static void sprite_backend_write_tiles(const USprite *sprite, const UFrame *frame) {
    const USpriteDefinition *definition = sprite->definition;
    const u16 frame_first_tile = (u16)(definition->first_tile + frame->tile_offset);
    const bool flip_x = sprite_backend_flip_x(sprite);
    const bool flip_y = sprite_backend_flip_y(sprite);
    const u8 width = definition->width_tiles;
    const u8 height = definition->height_tiles;
    const u16 last_row_offset = flip_y ? (u16)(height - 1u) * width : 0u;
    u16 scb1_address = (u16)(ADDR_SCB1 + sprite->render.layout.first_sprite * 64u);

    unsigned_neogeo_vram_set_mod(2u);
    for (u8 column = 0u; column < sprite->render.layout.sprite_count; ++column) {
        const u8 source_column = flip_x ? (u8)(width - 1u - column) : column;
        *REG_VRAMADDR = scb1_address;
        scb1_address = (u16)(scb1_address + 64u);

        if (!flip_y) {
            u16 tile = (u16)(frame_first_tile + source_column);
#if defined(__m68k__)
            unsigned_m68k_vram_stream_arithmetic(REG_VRAMRW, tile, height, (s16)width);
#else
            for (u8 row = 0u; row < height; ++row) {
                *REG_VRAMRW = tile;
                tile = (u16)(tile + width);
            }
#endif
        } else {
            u16 tile = (u16)(frame_first_tile + last_row_offset + source_column);
#if defined(__m68k__)
            unsigned_m68k_vram_stream_arithmetic(REG_VRAMRW, tile, height, (s16)-(s16)width);
#else
            for (u8 row = 0u; row < height; ++row) {
                *REG_VRAMRW = tile;
                tile = (u16)(tile - width);
            }
#endif
        }
    }
}

/** Uploads only SCB1 palette/flip attributes when tile indices can be reused. */
static void sprite_backend_write_attributes(const USprite *sprite) {
    const u16 attributes = sprite_backend_attributes(sprite);
    u16 scb1_address = (u16)(ADDR_SCB1 + sprite->render.layout.first_sprite * 64u + 1u);

    unsigned_neogeo_vram_set_mod(2u);
    for (u8 column = 0u; column < sprite->render.layout.sprite_count; ++column) {
        *REG_VRAMADDR = scb1_address;
        scb1_address = (u16)(scb1_address + 64u);
#if defined(__m68k__)
        unsigned_m68k_vram_fill_nonzero(REG_VRAMRW, attributes, sprite->definition->height_tiles);
#else
        for (u8 row = 0u; row < sprite->definition->height_tiles; ++row) {
            *REG_VRAMRW = attributes;
        }
#endif
    }
}

/** Uploads the same SCB2 shrink value to every hardware column in the logical sprite chain. */
static void sprite_backend_write_shrink(const USprite *sprite, u16 scb2) {
    unsigned_neogeo_vram_set_mod(1u);
    *REG_VRAMADDR = (u16)(ADDR_SCB2 + sprite->render.layout.first_sprite);
#if defined(__m68k__)
    unsigned_m68k_vram_fill_nonzero(REG_VRAMRW, scb2, sprite->render.layout.sprite_count);
#else
    for (u8 column = 0u; column < sprite->render.layout.sprite_count; ++column) {
        *REG_VRAMRW = scb2;
    }
#endif
}

/** Rebuilds SCB3 with one Y/height driver followed by sticky chained columns. */
static void sprite_backend_write_chain(const USprite *sprite, s16 screen_y) {
    unsigned_neogeo_vram_set_mod(1u);
    *REG_VRAMADDR = (u16)(ADDR_SCB3 + sprite->render.layout.first_sprite);
    *REG_VRAMRW = sprite_backend_scb3(sprite, screen_y);
#if defined(__m68k__)
    if (sprite->render.layout.sprite_count > 1u) {
        unsigned_m68k_vram_fill_nonzero(REG_VRAMRW, SPRITE_SCB3_STICKY, (u16)(sprite->render.layout.sprite_count - 1u));
    }
#else
    for (u8 column = 1u; column < sprite->render.layout.sprite_count; ++column) {
        *REG_VRAMRW = SPRITE_SCB3_STICKY;
    }
#endif
}

/** Uploads the chain driver X coordinate to SCB4 in Neo Geo fixed hardware units. */
static void sprite_backend_write_driver_x(const USprite *sprite, s16 screen_x) {
    unsigned_neogeo_vram_set_mod(1u);
    *REG_VRAMADDR = (u16)(ADDR_SCB4 + sprite->render.layout.first_sprite);
    *REG_VRAMRW = (u16)((s32)screen_x * 128);
}

/** Uploads the chain driver Y/height word to SCB3. */
static void sprite_backend_write_driver_y(const USprite *sprite, s16 screen_y) {
    unsigned_neogeo_vram_set_mod(1u);
    *REG_VRAMADDR = (u16)(ADDR_SCB3 + sprite->render.layout.first_sprite);
    *REG_VRAMRW = sprite_backend_scb3(sprite, screen_y);
}

/** Uploads contiguous SCB3/SCB4 driver coordinates in one VRAM stride when both axes changed. */
static void sprite_backend_write_driver_xy(const USprite *sprite, s16 screen_x, s16 screen_y) {
    unsigned_neogeo_vram_set_mod(0x200u);
    *REG_VRAMADDR = (u16)(ADDR_SCB3 + sprite->render.layout.first_sprite);
    *REG_VRAMRW = sprite_backend_scb3(sprite, screen_y);
    *REG_VRAMRW = (u16)((s32)screen_x * 128);
}

u16 unsigned_sprite_backend_encode_scb2(const USpriteRenderState *render, const UEffectSample *effect) {
    s32 shrink_x = unsigned_sprite_scale_shrink_x(render->shrink_x, effect->scale_x);
    s32 shrink_y = unsigned_sprite_scale_shrink_y(render->shrink_y, effect->scale_y);

    shrink_x -= unsigned_math_div_pow2_s32(effect->zoom_offset, 4u);
    shrink_y -= effect->zoom_offset;

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

void unsigned_sprite_backend_encode_prepared_column(UPreparedColumn *column, const USpriteRenderState *render, const UEffectSample *effect, s16 x, s16 y, u8 height_tiles, bool hidden) {
    column->scb2 = unsigned_sprite_backend_encode_scb2(render, effect);
    column->scb3 = hidden ? 0u : (u16)(((s32)UNSIGNED_VIDEO_SCB3_Y_ORIGIN - y) * 128 + height_tiles);
    column->scb4 = (u16)((s32)x * 128);
}

void unsigned_sprite_backend_flush_graphics(const USprite *sprite, const UFrame *frame, u8 dirty) {
    if ((dirty & U_SPRITE_RENDER_DIRTY_GRAPHICS) == U_SPRITE_RENDER_DIRTY_GRAPHICS) {
        sprite_backend_write_graphics_full(sprite, frame);
        sprite_backend_note_graphics_write(sprite);
    } else {
        if ((dirty & U_SPRITE_RENDER_DIRTY_TILES) != 0u) {
            sprite_backend_write_tiles(sprite, frame);
            sprite_backend_note_graphics_write(sprite);
        }
        if ((dirty & U_SPRITE_RENDER_DIRTY_ATTRIBUTES) != 0u) {
            sprite_backend_write_attributes(sprite);
            sprite_backend_note_graphics_write(sprite);
        }
    }

    if ((dirty & U_SPRITE_RENDER_DIRTY_PADDING) != 0u) {
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

void unsigned_sprite_backend_write_driver_batch(USprite *const *sprites, u8 count, u8 dirty_position) {
    const u16 first_sprite = sprites[0]->render.layout.first_sprite;
    const u16 stride = sprites[0]->render.layout.sprite_count;

    if ((dirty_position & U_SPRITE_RENDER_DIRTY_Y) != 0u) {
        unsigned_neogeo_vram_set_mod(stride);
        *REG_VRAMADDR = (u16)(ADDR_SCB3 + first_sprite);
        for (u8 i = 0u; i < count; ++i) {
            const USprite *sprite = sprites[i];
            *REG_VRAMRW = sprite_backend_scb3(sprite, sprite->render.prepared.draw_y);
        }
    }

    if ((dirty_position & U_SPRITE_RENDER_DIRTY_X) != 0u) {
        unsigned_neogeo_vram_set_mod(stride);
        *REG_VRAMADDR = (u16)(ADDR_SCB4 + first_sprite);
        for (u8 i = 0u; i < count; ++i) {
            *REG_VRAMRW = (u16)((s32)sprites[i]->render.prepared.draw_x * 128);
        }
    }
}

void unsigned_sprite_backend_write_prepared_effect_positions(const USprite *sprite, const UPreparedColumn *columns) {
    const u16 first_sprite = sprite->render.layout.first_sprite;
    const u8 sprite_count = sprite->render.layout.sprite_count;
    /* For tiny sprites the interleaved SCB2 -> SCB3 -> SCB4 walk uses fewer VRAM-address
     * writes than opening three planes. Wider per-column sprites are cheaper as three contiguous
     * streams: one address setup per plane instead of one setup per hardware column. */
    if (sprite_count <= 3u) {
        unsigned_neogeo_vram_set_mod(0x200u);
        for (u8 column = 0u; column < sprite_count; ++column) {
            const UPreparedColumn *prepared = &columns[column];
            *REG_VRAMADDR = (u16)(ADDR_SCB2 + first_sprite + column);
            *REG_VRAMRW = prepared->scb2;
            *REG_VRAMRW = prepared->scb3;
            *REG_VRAMRW = prepared->scb4;
        }
        return;
    }

    unsigned_neogeo_vram_set_mod(1u);

#if defined(__m68k__)
    const u16 source_stride_words = (u16)(sizeof(*columns) / sizeof(u16));
#endif

    *REG_VRAMADDR = (u16)(ADDR_SCB2 + first_sprite);
#if defined(__m68k__)
    unsigned_m68k_vram_stream_strided(REG_VRAMRW, &columns[0].scb2, sprite_count, source_stride_words);
#else
    for (u8 column = 0u; column < sprite_count; ++column) {
        *REG_VRAMRW = columns[column].scb2;
    }
#endif

    *REG_VRAMADDR = (u16)(ADDR_SCB3 + first_sprite);
#if defined(__m68k__)
    unsigned_m68k_vram_stream_strided(REG_VRAMRW, &columns[0].scb3, sprite_count, source_stride_words);
#else
    for (u8 column = 0u; column < sprite_count; ++column) {
        *REG_VRAMRW = columns[column].scb3;
    }
#endif

    *REG_VRAMADDR = (u16)(ADDR_SCB4 + first_sprite);
#if defined(__m68k__)
    unsigned_m68k_vram_stream_strided(REG_VRAMRW, &columns[0].scb4, sprite_count, source_stride_words);
#else
    for (u8 column = 0u; column < sprite_count; ++column) {
        *REG_VRAMRW = columns[column].scb4;
    }
#endif
}

void unsigned_sprite_backend_break_chain(u16 hardware_sprite) {
    /*
     * A relocated logical sprite may start on a column that was sticky in the
     * previous frame. Clear that future driver before any chain is rebuilt so
     * an in-progress Y-order swap can never inherit the previous owner's chain.
     */
    unsigned_neogeo_vram_set_mod(1u);
    *REG_VRAMADDR = (u16)(ADDR_SCB3 + hardware_sprite);
    *REG_VRAMRW = 0u;
}

void unsigned_sprite_backend_clear_range(u16 first_sprite, u16 sprite_count) {
    unsigned_neogeo_vram_clear_scb3_range(first_sprite, sprite_count);
}

void unsigned_sprite_backend_hide_chain(const USprite *sprite) {
    unsigned_neogeo_vram_set_mod(1u);
    *REG_VRAMADDR = (u16)(ADDR_SCB3 + sprite->render.layout.first_sprite);
    *REG_VRAMRW = 0u;
}
