/**
 * @file frame.h
 * @brief Optional Neo Geo frame CPU-budget measurement helpers.
 *
 * The game owns the measurement interval: call unsigned_frame_begin() where the
 * frame budget starts, then unsigned_frame_end() where the measured work ends.
 * Unsigned never calls these functions from its runtime loop.
 */

#ifndef UNSIGNED_SYSTEM_FRAME_H
#define UNSIGNED_SYSTEM_FRAME_H

#include "core/types.h"

#define UNSIGNED_FRAME_SCANLINE_CYCLES 768u
#define UNSIGNED_FRAME_SCANLINES_60HZ 264u
#define UNSIGNED_FRAME_SCANLINES_50HZ 312u
#define UNSIGNED_FRAME_CYCLES_60HZ 202752u
#define UNSIGNED_FRAME_CYCLES_50HZ 239616u

/**
 * @brief Captures the raster position used as the start of one CPU-budget measurement.
 *
 * @pre The matching unsigned_frame_end() is called before one complete video frame elapses.
 */
void unsigned_frame_begin(void);

/**
 * @brief Returns a conservative estimate of 68000 cycles remaining in the measured frame.
 *
 * @details
 * The measurement uses the read-only raster line counter exposed through REG_LSPCMODE, so it works
 * both on Neo Geo hardware and in MAME without modifying the runtime loop. The counter only exposes
 * scanline position, not horizontal pixel position, so the exact sub-scanline CPU position is not
 * observable from cartridge code.
 *
 * One scanline is 768 68000 cycles. To avoid overstating available CPU time,
 * unsigned_frame_end() charges the current partial scanline as fully consumed. The result is thus
 * quantized to 768 cycles and may be up to 768 cycles lower than the exact MAME debugger
 * `totalcycles` measurement.
 *
 * 60 Hz hardware uses 264 scanlines (202752 cycles/frame); PAL 50 Hz hardware uses 312 scanlines
 * (239616 cycles/frame). The video mode sampled by unsigned_frame_begin() defines the budget.
 *
 * @pre unsigned_frame_begin() was called for the current measurement interval.
 * @pre Less than one complete video frame elapsed since unsigned_frame_begin().
 * @return Conservative estimated number of 68000 cycles remaining in the measured frame budget.
 */
u32 unsigned_frame_end(void);

#endif
