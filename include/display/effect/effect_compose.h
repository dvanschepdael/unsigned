/**
 * @file effect_compose.h
 * @brief Target-specific hot path for composing sampled presentation effects.
 */

#ifndef UNSIGNED_DISPLAY_EFFECT_EFFECT_COMPOSE_H
#define UNSIGNED_DISPLAY_EFFECT_EFFECT_COMPOSE_H

#include "display/effect/effect.h"

/*
 * Low-level composition boundary. The Neo Geo build links the sibling
 * effect_compose.asm implementation; host tests link the reference C
 * implementation from test/mock. Pointer arguments keep the C/ASM boundary
 * independent of aggregate-return ABI details.
 */
void unsigned_effect_compose(UEffectSample *composed, const UEffectSample *first, const UEffectSample *second);

#if defined(__m68k__)
/* effect_compose.asm reads UEffectSample fields directly. */
typedef char UEffectAsmOffsetXMustBeZero[(__builtin_offsetof(UEffectSample, offset_x) == 0u) ? 1 : -1];
typedef char UEffectAsmOffsetYMustBeTwo[(__builtin_offsetof(UEffectSample, offset_y) == 2u) ? 1 : -1];
typedef char UEffectAsmZoomOffsetMustBeFour[(__builtin_offsetof(UEffectSample, zoom_offset) == 4u) ? 1 : -1];
typedef char UEffectAsmScaleXMustBeSix[(__builtin_offsetof(UEffectSample, scale_x) == 6u) ? 1 : -1];
typedef char UEffectAsmScaleYMustBeEight[(__builtin_offsetof(UEffectSample, scale_y) == 8u) ? 1 : -1];
typedef char UEffectAsmPivotXMustBeTen[(__builtin_offsetof(UEffectSample, pivot_x) == 10u) ? 1 : -1];
typedef char UEffectAsmPivotYMustBeTwelve[(__builtin_offsetof(UEffectSample, pivot_y) == 12u) ? 1 : -1];
typedef char UEffectAsmFlagsMustBeFourteen[(__builtin_offsetof(UEffectSample, flags) == 14u) ? 1 : -1];
#endif

#endif
