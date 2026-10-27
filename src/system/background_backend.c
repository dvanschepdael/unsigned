/**
 * @file background_backend.c
 * @brief Translates scrolling background operations into Neo Geo SCB/VRAM writes.
 */

#include "core/math/math.h"
#include "display/sprite/sprite_config.h"
#include "renderer/prepared_column.h"
#include "system/background_backend.h"
#include "system/sprite_backend.h"

#include "system/video.h"
#include "system/vram_writer.h"

#include <ngdevkit/registers.h>

#define BACKGROUND_SCB3_STICKY 0x0040u

/** Encodes a background column Y position and height into the Neo Geo SCB3 register format. */
static u16 background_backend_scb3(const UBackgroundLayerDefinition *definition, s16 y) {
    return (u16)(((s32)UNSIGNED_VIDEO_SCB3_Y_ORIGIN - y) * 128 + definition->height_tiles);
}

/** Uploads the same SCB2 shrink value to every hardware sprite column owned by the background layer. */
static void background_backend_write_shrink(const UBackgroundLayerDefinition *definition, u8 columns, u16 scb2) {
    unsigned_system_vram_set_mod(1u);
    *REG_VRAMADDR = (u16)(ADDR_SCB2 + definition->first_sprite);
    unsigned_system_vram_fill(REG_VRAMRW, scb2, columns);
}

/** Rebuilds SCB3 chain drivers/sticky links for the ring-buffer layout, including the wrap split when present. */
static void background_backend_write_chain(const UBackgroundLayerDefinition *definition, u8 columns, u8 leftmost_slot, s16 y) {
    const u16 driver_scb3 = background_backend_scb3(definition, y);

    unsigned_system_vram_set_mod(1u);
    *REG_VRAMADDR = (u16)(ADDR_SCB3 + definition->first_sprite);
    if (leftmost_slot == 0u) {
        *REG_VRAMRW = driver_scb3;
        if (columns > 1u) {
            unsigned_system_vram_fill(REG_VRAMRW, BACKGROUND_SCB3_STICKY, (u16)(columns - 1u));
        }
        return;
    }

    *REG_VRAMRW = driver_scb3;
    if (leftmost_slot > 1u) {
        unsigned_system_vram_fill(REG_VRAMRW, BACKGROUND_SCB3_STICKY, (u16)(leftmost_slot - 1u));
    }
    *REG_VRAMRW = driver_scb3;
    if ((u8)(leftmost_slot + 1u) < columns) {
        unsigned_system_vram_fill(REG_VRAMRW, BACKGROUND_SCB3_STICKY, (u16)(columns - leftmost_slot - 1u));
    }
}

/** Uploads one chain-driver X coordinate to SCB4 in Neo Geo fixed hardware units. */
static void background_backend_write_driver_x(u16 sprite, s16 x) {
    unsigned_system_vram_set_mod(1u);
    *REG_VRAMADDR = (u16)(ADDR_SCB4 + sprite);
    *REG_VRAMRW = (u16)((s32)x * 128);
}

/** Uploads one chain-driver Y/height word to SCB3. */
static void background_backend_write_driver_y(const UBackgroundLayerDefinition *definition, u16 sprite, s16 y) {
    unsigned_system_vram_set_mod(1u);
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

u16 unsigned_system_background_encode_scb2(s16 zoom_offset) {
    s32 shrink_x = 0x0f - unsigned_math_div_pow2_s32(zoom_offset, 4u);
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

void unsigned_system_background_encode_column(UPreparedColumn *column, s16 zoom_offset, s16 x, s16 y, u8 height_tiles) {
    column->scb2 = unsigned_system_background_encode_scb2(zoom_offset);
    column->scb3 = (u16)(((s32)UNSIGNED_VIDEO_SCB3_Y_ORIGIN - y) * 128 + height_tiles);
    column->scb4 = (u16)((s32)x * 128);
}

void unsigned_system_background_write_column(const UBackgroundLayerDefinition *definition, u8 physical_slot, u8 source_column, bool full) {
    u16 tile = (u16)(definition->first_tile + source_column);
    const u16 hardware_sprite = (u16)(definition->first_sprite + physical_slot);

    /* This SCB1 column no longer has sprite-owned padding guarantees. */
    unsigned_system_sprite_invalidate_padding(hardware_sprite, 1u);

    unsigned_system_vram_set_mod(full ? 1u : 2u);
    *REG_VRAMADDR = (u16)(ADDR_SCB1 + (definition->first_sprite + physical_slot) * 64u);
    if (full) {
        const u16 attributes = (u16)(((u16)definition->palette << 8u) | definition->auto_animation);
        unsigned_system_vram_stream_tiles(REG_VRAMRW, tile, attributes, definition->height_tiles, (s16)definition->width_tiles);
    } else {
        unsigned_system_vram_stream_arithmetic(REG_VRAMRW, tile, definition->height_tiles, (s16)definition->width_tiles);
    }
}

void unsigned_system_background_flush_chain(const UBackgroundLayerDefinition *definition, u8 columns, u8 leftmost_slot, s16 base_x, s16 y, u16 scb2, u8 transform_dirty) {
    if ((transform_dirty & U_BACKGROUND_TRANSFORM_DIRTY_SHRINK) != 0u) {
        background_backend_write_shrink(definition, columns, scb2);
    }
    if ((transform_dirty & U_BACKGROUND_TRANSFORM_DIRTY_LAYOUT) != 0u) {
        background_backend_write_chain(definition, columns, leftmost_slot, y);
        background_backend_write_driver_positions(definition, columns, leftmost_slot, base_x);
        return;
    }
    if ((transform_dirty & U_BACKGROUND_TRANSFORM_DIRTY_Y) != 0u) {
        background_backend_write_driver_y_positions(definition, leftmost_slot, y);
    }
    if ((transform_dirty & U_BACKGROUND_TRANSFORM_DIRTY_X) != 0u) {
        background_backend_write_driver_positions(definition, columns, leftmost_slot, base_x);
    }
}

/** Stream one contiguous segment of a prepared SCB plane through the native DBRA hot path. */
static void background_backend_stream_segment(u16 vram_address, const u16 *source, u8 count) {
    if (count == 0u) {
        return;
    }
    const u16 source_stride_words = (u16)(sizeof(UPreparedColumn) / sizeof(u16));
    *REG_VRAMADDR = vram_address;
    unsigned_system_vram_stream_strided(REG_VRAMRW, source, count, source_stride_words);
}

void unsigned_system_background_write_effects(const UBackgroundLayerDefinition *definition, u8 columns, u8 leftmost_slot, const UPreparedColumn *prepared) {
    /* The ring is contiguous in one segment when leftmost_slot == 0, otherwise in two. For
     * sufficiently wide backgrounds, streaming each SCB plane cuts VRAMADDR traffic from one
     * write per screen column to one/two writes per plane. Keep the old interleaved walk for very
     * small ranges where opening all three planes would cost more address writes. */
    const u8 planar_address_writes = leftmost_slot == 0u ? 3u : 6u;
    if (columns <= planar_address_writes) {
        u8 slot = leftmost_slot;

        unsigned_system_vram_set_mod(0x200u);
        for (u8 screen_column = 0u; screen_column < columns; ++screen_column) {
            const UPreparedColumn *column = &prepared[screen_column];
            *REG_VRAMADDR = (u16)(ADDR_SCB2 + definition->first_sprite + slot);
            *REG_VRAMRW = column->scb2;
            *REG_VRAMRW = column->scb3;
            *REG_VRAMRW = column->scb4;

            ++slot;
            if (slot == columns) {
                slot = 0u;
            }
        }
        return;
    }

    unsigned_system_vram_set_mod(1u);

    /* Prepared columns are in logical screen order while the hardware ring may wrap once.
     * Stream each SCB plane as one or two contiguous destination segments. */
    const u8 first_segment = (u8)(columns - leftmost_slot);
    const u8 second_segment = leftmost_slot;

    background_backend_stream_segment((u16)(ADDR_SCB2 + definition->first_sprite + leftmost_slot), &prepared[0].scb2, first_segment);
    background_backend_stream_segment((u16)(ADDR_SCB2 + definition->first_sprite), &prepared[first_segment].scb2, second_segment);
    background_backend_stream_segment((u16)(ADDR_SCB3 + definition->first_sprite + leftmost_slot), &prepared[0].scb3, first_segment);
    background_backend_stream_segment((u16)(ADDR_SCB3 + definition->first_sprite), &prepared[first_segment].scb3, second_segment);
    background_backend_stream_segment((u16)(ADDR_SCB4 + definition->first_sprite + leftmost_slot), &prepared[0].scb4, first_segment);
    background_backend_stream_segment((u16)(ADDR_SCB4 + definition->first_sprite), &prepared[first_segment].scb4, second_segment);
}

void unsigned_system_background_hide_range(u16 first_sprite, u8 columns) {
    unsigned_system_vram_clear_scb3(first_sprite, columns);
}
