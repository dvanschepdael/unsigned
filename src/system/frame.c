/**
 * @file frame.c
 * @brief Implements optional Neo Geo frame CPU-budget measurement.
 */

#include "system/frame.h"

#include <ngdevkit/registers.h>

#define LSPC_RASTER_COUNTER_MASK 0x01ffu
#define LSPC_RASTER_COUNTER_SHIFT 7u
#define LSPC_RASTER_COUNTER_SPLIT 0x0100u
#define LSPC_PAL_MODE_MASK 0x0008u

static u16 frame_begin_line;
static u16 frame_total_lines;
static u32 frame_total_cycles;

/** Convert the LSPC vertical counter encoding to a zero-based raster line. */
static u16 frame_decode_line(u16 lspc_mode, u16 total_lines) {
    const u16 raster_counter = (u16)((lspc_mode >> LSPC_RASTER_COUNTER_SHIFT) & LSPC_RASTER_COUNTER_MASK);

    if (raster_counter >= LSPC_RASTER_COUNTER_SPLIT) {
        return (u16)(raster_counter - LSPC_RASTER_COUNTER_SPLIT);
    }

    return (u16)(raster_counter + total_lines - LSPC_RASTER_COUNTER_SPLIT);
}

void unsigned_frame_begin(void) {
    const u16 lspc_mode = *REG_LSPCMODE;

    if ((lspc_mode & LSPC_PAL_MODE_MASK) != 0u) {
        frame_total_lines = UNSIGNED_FRAME_SCANLINES_50HZ;
        frame_total_cycles = UNSIGNED_FRAME_CYCLES_50HZ;
    } else {
        frame_total_lines = UNSIGNED_FRAME_SCANLINES_60HZ;
        frame_total_cycles = UNSIGNED_FRAME_CYCLES_60HZ;
    }

    frame_begin_line = frame_decode_line(lspc_mode, frame_total_lines);
}

u32 unsigned_frame_end(void) {
    const u16 end_line = frame_decode_line(*REG_LSPCMODE, frame_total_lines);
    const u16 elapsed_lines = end_line >= frame_begin_line
                                  ? (u16)(end_line - frame_begin_line)
                                  : (u16)(end_line + frame_total_lines - frame_begin_line);
    const u16 elapsed_lines_x3 = (u16)(elapsed_lines + elapsed_lines + elapsed_lines);
    const u32 elapsed_cycles = (u32)elapsed_lines_x3 << 8u;

    return frame_total_cycles - elapsed_cycles;
}
