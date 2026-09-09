/**
 * @file effect.h
 * @brief Generic per-column viewport effect sampling.
 */

#ifndef UNSIGNED_DISPLAY_VIEWPORT_EFFECT_H
#define UNSIGNED_DISPLAY_VIEWPORT_EFFECT_H

#include "core/types.h"

typedef struct UEffectSample {
    s16 offset_x;
    s16 offset_y;
    s16 zoom_offset;
} UEffectSample;

typedef struct UEffectBounds {
    u16 offset_x;
    u16 offset_y;
} UEffectBounds;

typedef enum UEffectLayout {
    U_EFFECT_LAYOUT_PER_COLUMN = 0,
    U_EFFECT_LAYOUT_UNIFORM = 1,
} UEffectLayout;

typedef void (*UEffectFunction)(UEffectSample *sample, u8 column, u8 column_count, u8 phase, const void *context);
typedef UEffectBounds (*UEffectBoundsFunction)(u8 column_count, const void *context);

typedef struct UEffect {
    UEffectFunction function;
    UEffectBoundsFunction bounds;
    const void *context;
    u8 phase;
    u8 speed;
    UEffectLayout layout;
} UEffect;

/**
 * @brief Configures a per-column viewport effect callback with default layout and no explicit bounds callback.
 *
 * @param effect Effect runtime state to configure.
 * @param function Sampling callback; NULL disables sampling while preserving configuration state.
 * @param context Caller-owned callback context.
 * @param speed Phase increment added once per effect tick.
 */
void unsigned_effect_set(UEffect *effect, UEffectFunction function, const void *context, u8 speed);

/**
 * @brief Configures effect sampling, layout policy and optional conservative-bounds callback.
 *
 * @param effect Effect runtime state to configure.
 * @param function Sampling callback invoked per requested column.
 * @param context Caller-owned callback context passed to sampling/bounds callbacks.
 * @param speed Phase increment added once per effect tick.
 * @param layout Whether the renderer may treat the effect as uniform or must sample per column.
 * @param bounds Optional callback that returns maximum X/Y displacement required for culling.
 */
void unsigned_effect_set_advanced(UEffect *effect, UEffectFunction function, const void *context, u8 speed, UEffectLayout layout, UEffectBoundsFunction bounds);

/**
 * @brief Resets the effect to an inactive zero state.
 *
 * @param effect Effect runtime state to clear; NULL is ignored.
 */
void unsigned_effect_clear(UEffect *effect);

/**
 * @brief Advances the effect phase by its configured speed when a sampling function is active.
 *
 * @param effect Effect runtime state to advance; NULL/inactive effects are ignored.
 */
void unsigned_effect_tick(UEffect *effect);

/**
 * @brief Samples one effect column at the current phase.
 *
 * @param effect Effect to sample; NULL/inactive effects produce zero offsets.
 * @param column Zero-based column being sampled.
 * @param column_count Total columns in the current render span.
 * @return Sampled X/Y/zoom offsets; all zeros when the effect is inactive.
 */
UEffectSample unsigned_effect_sample(const UEffect *effect, u8 column, u8 column_count);

/**
 * @brief Returns conservative maximum effect displacement for renderer culling/layout.
 *
 * @param effect Effect whose bounds are queried.
 * @param column_count Column count passed to the configured bounds callback.
 * @param bounds Required output; initialized to zero before evaluation.
 * @return true for inactive effects or when a bounds callback is available; false when bounds is NULL or an active effect has no bounds callback.
 */
bool unsigned_effect_get_bounds(const UEffect *effect, u8 column_count, UEffectBounds *bounds);

#endif
