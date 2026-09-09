/**
 * @file background_backend.c
 * @brief Translates scrolling background operations into Neo Geo SCB/VRAM writes.
 */

#include "display/sprite/attributes.h"
#include "system/neogeo/video.h"
#include "system/neogeo/vram_writer_internal.h"
#include "system/renderer/renderer_backend.h"

#include <ngdevkit/registers.h>

#define BACKGROUND_SCB3_STICKY 0x0040u

/** Converts pixel coordinates to 16-pixel hardware columns using truncation toward zero. */
static s32 background_backend_trunc_div_16(s32 value) {
    if (value >= 0) {
        return (s32)((u32)value >> 4u);
    }
    return -(s32)(((u32)(-(value + 1)) + 1u) >> 4u);
}

/** Encodes background tile palette/flip attributes into the Neo Geo tile attribute word. */
static u16 background_backend_attributes(const UBackgroundLayerDefinition *definition) {
    return (u16)(((u16)definition->palette << 8u) | (definition->auto_animation & U_SPRITE_AUTO_ANIMATION_MASK));
}

/** Encodes a background column Y position and height into the Neo Geo SCB3 register format. */
static u16 background_backend_scb3(const UBackgroundLayerDefinition *definition, s16 y) {
    return (u16)(((s32)UNSIGNED_VIDEO_SCB3_Y_ORIGIN - y) * 128 + definition->height_tiles);
}

/** Uploads the same SCB2 shrink value to every hardware sprite column owned by the background layer. */
static void background_backend_write_shrink(const UBackgroundLayerDefinition *definition, u8 columns, u16 scb2) {
    unsigned_neogeo_vram_set_mod(1u);
    *REG_VRAMADDR = (u16)(ADDR_SCB2 + definition->first_sprite);
    for (u8 slot = 0u; slot < columns; ++slot) {
        *REG_VRAMRW = scb2;
    }
}

/** Rebuilds SCB3 chain drivers/sticky links for the ring-buffer layout, including the wrap split when present. */
static void background_backend_write_chain(const UBackgroundLayerDefinition *definition, u8 columns, u8 leftmost_slot, s16 y) {
    const u16 driver_scb3 = background_backend_scb3(definition, y);

    unsigned_neogeo_vram_set_mod(1u);
    *REG_VRAMADDR = (u16)(ADDR_SCB3 + definition->first_sprite);
    for (u8 slot = 0u; slot < columns; ++slot) {
        const bool driver = slot == leftmost_slot || (leftmost_slot != 0u && slot == 0u);
        *REG_VRAMRW = driver ? driver_scb3 : BACKGROUND_SCB3_STICKY;
    }
}

/** Uploads one chain-driver X coordinate to SCB4 in Neo Geo fixed hardware units. */
static void background_backend_write_driver_x(u16 sprite, s16 x) {
    unsigned_neogeo_vram_set_mod(1u);
    *REG_VRAMADDR = (u16)(ADDR_SCB4 + sprite);
    *REG_VRAMRW = (u16)((s32)x * 128);
}

/** Uploads one chain-driver Y/height word to SCB3. */
static void background_backend_write_driver_y(const UBackgroundLayerDefinition *definition, u16 sprite, s16 y) {
    unsigned_neogeo_vram_set_mod(1u);
    *REG_VRAMADDR = (u16)(ADDR_SCB3 + sprite);
    *REG_VRAMRW = background_backend_scb3(definition, y);
}

/** Updates the X positions of the one or two chain drivers produced by a wrapped column ring buffer. */
static void background_backend_write_driver_positions(const UBackgroundLayerDefinition *definition, u8 columns, u8 leftmost_slot, s16 base_x) {
    const u8 first_segment_columns = (u8)(columns - leftmost_slot);
    background_backend_write_driver_x((u16)(definition->first_sprite + leftmost_slot), base_x);
    if (leftmost_slot != 0u) {
        background_backend_write_driver_x(definition->first_sprite, (s16)(base_x + (s16)first_segment_columns * 16));
    }
}

/** Updates the Y position of every active chain driver after vertical movement. */
static void background_backend_write_driver_y_positions(const UBackgroundLayerDefinition *definition, u8 leftmost_slot, s16 y) {
    background_backend_write_driver_y(definition, (u16)(definition->first_sprite + leftmost_slot), y);
    if (leftmost_slot != 0u) {
        background_backend_write_driver_y(definition, definition->first_sprite, y);
    }
}

u16 unsigned_background_backend_encode_scb2(s16 zoom_offset) {
    s32 shrink_x = 0x0f - background_backend_trunc_div_16(zoom_offset);
    s32 shrink_y = 0xff - zoom_offset;

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

void unsigned_background_backend_write_column(const UBackgroundLayerDefinition *definition, u8 physical_slot, u8 source_column, bool full) {
    u16 tile = (u16)(definition->first_tile + source_column);

    unsigned_neogeo_vram_set_mod(full ? 1u : 2u);
    *REG_VRAMADDR = (u16)(ADDR_SCB1 + (definition->first_sprite + physical_slot) * 64u);
    if (full) {
        const u16 attributes = background_backend_attributes(definition);
        for (u8 row = 0u; row < definition->height_tiles; ++row) {
            *REG_VRAMRW = tile;
            *REG_VRAMRW = attributes;
            tile = (u16)(tile + definition->width_tiles);
        }
    } else {
        for (u8 row = 0u; row < definition->height_tiles; ++row) {
            *REG_VRAMRW = tile;
            tile = (u16)(tile + definition->width_tiles);
        }
    }
}

void unsigned_background_backend_flush_chained(const UBackgroundLayerDefinition *definition, u8 columns, u8 leftmost_slot, s16 base_x, s16 y, u16 scb2, bool shrink_changed, bool layout_changed, bool x_changed, bool y_changed) {
    if (shrink_changed) {
        background_backend_write_shrink(definition, columns, scb2);
    }
    if (layout_changed) {
        background_backend_write_chain(definition, columns, leftmost_slot, y);
        background_backend_write_driver_positions(definition, columns, leftmost_slot, base_x);
        return;
    }
    if (y_changed) {
        background_backend_write_driver_y_positions(definition, leftmost_slot, y);
    }
    if (x_changed) {
        background_backend_write_driver_positions(definition, columns, leftmost_slot, base_x);
    }
}

void unsigned_background_backend_write_effect_positions(const UBackgroundLayer *layer, const UViewport *viewport, u8 columns, u8 leftmost_slot, s16 base_x, s16 y) {
    const UBackgroundLayerDefinition *definition = layer->definition;
    u8 slot = leftmost_slot;

    unsigned_neogeo_vram_set_mod(0x200u);
    for (u8 screen_column = 0u; screen_column < columns; ++screen_column) {
        const UEffectSample sampled = unsigned_effect_sample(&viewport->effect, screen_column, columns);
        *REG_VRAMADDR = (u16)(ADDR_SCB2 + definition->first_sprite + slot);
        *REG_VRAMRW = unsigned_background_backend_encode_scb2(sampled.zoom_offset);
        *REG_VRAMRW = (u16)(((s32)UNSIGNED_VIDEO_SCB3_Y_ORIGIN - y - sampled.offset_y) * 128 + definition->height_tiles);
        *REG_VRAMRW = (u16)(((s32)base_x + (s32)screen_column * 16 + sampled.offset_x) * 128);

        ++slot;
        if (slot == columns) {
            slot = 0u;
        }
    }
}

void unsigned_background_backend_hide_range(u16 first_sprite, u8 columns) {
    unsigned_neogeo_vram_set_mod(1u);
    *REG_VRAMADDR = (u16)(ADDR_SCB3 + first_sprite);
    for (u8 column = 0u; column < columns; ++column) {
        *REG_VRAMRW = 0u;
    }
}
