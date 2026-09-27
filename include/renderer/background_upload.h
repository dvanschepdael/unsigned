/**
 * @file background_upload.h
 * @brief Target-specific background ring-buffer upload planner hot path.
 */

#ifndef UNSIGNED_RENDERER_BACKGROUND_UPLOAD_H
#define UNSIGNED_RENDERER_BACKGROUND_UPLOAD_H

#include "renderer/background_renderer.h"

/*
 * Low-level planner boundary. Production links background_upload.asm;
 * host renderer tests link test/mock/background_upload.c. Scalar
 * arguments are widened to 32 bits so the target ABI is independent of -mshort.
 *
 * @pre columns > 0, leftmost_slot < columns, source_width > 0.
 * @return number of upload entries written to `uploads`.
 */
u32 unsigned_background_upload_build(u16 *loaded_source_columns, UBackgroundColumnUploadPlan *uploads, u32 columns, u32 leftmost_slot, u32 source_column, u32 source_width);

#if defined(__m68k__)
/* background_upload.asm writes UBackgroundColumnUploadPlan directly. */
typedef char UBackgroundUploadPhysicalSlotOffsetMustBeZero[(__builtin_offsetof(UBackgroundColumnUploadPlan, physical_slot) == 0u) ? 1 : -1];
typedef char UBackgroundUploadSourceColumnOffsetMustBeOne[(__builtin_offsetof(UBackgroundColumnUploadPlan, source_column) == 1u) ? 1 : -1];
typedef char UBackgroundUploadFullOffsetMustBeTwo[(__builtin_offsetof(UBackgroundColumnUploadPlan, full) == 2u) ? 1 : -1];
typedef char UBackgroundUploadSizeMustBeThree[(sizeof(UBackgroundColumnUploadPlan) == 3u) ? 1 : -1];
#endif

#endif
