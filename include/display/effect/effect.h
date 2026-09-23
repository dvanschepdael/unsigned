/**
 * @file effect.h
 * @brief Generic sampled display effects shared by viewports and sprites.
 */

#ifndef UNSIGNED_DISPLAY_EFFECT_EFFECT_H
#define UNSIGNED_DISPLAY_EFFECT_EFFECT_H

#include "core/types.h"

#define U_EFFECT_SCALE_ONE 256u
#define U_EFFECT_PIVOT_START 0u
#define U_EFFECT_PIVOT_CENTER 128u
#define U_EFFECT_PIVOT_END 256u

typedef enum UEffectSampleFlag {
    U_EFFECT_SAMPLE_NONE = 0x00u,
    /** Toggle horizontal mirroring of a sprite. Ignored by viewport/background consumers. */
    U_EFFECT_SAMPLE_FLIP_X = 0x01u,
    /** Toggle vertical mirroring of a sprite. Ignored by viewport/background consumers. */
    U_EFFECT_SAMPLE_FLIP_Y = 0x02u,
    /** Temporarily hide a sprite without releasing its hardware allocation. */
    U_EFFECT_SAMPLE_HIDDEN = 0x04u,
} UEffectSampleFlag;

/**
 * One sampled presentation transform.
 *
 * Translation and `zoom_offset` are consumed by both viewport and sprite renderers.
 * `scale_*`, `pivot_*` and flags are sprite presentation fields. Scale values use Q8
 * where 256 is 100%; Neo Geo sprite presentation supports shrink only, so authored
 * scale values never exceed 100%.
 * @invariant `scale_x`/`scale_y` and `pivot_x`/`pivot_y` stay in the Q8 range 0..256.
 */
typedef struct UEffectSample {
    s16 offset_x;
    s16 offset_y;
    /** Legacy hardware-oriented uniform zoom delta used by viewport effects. Positive values shrink. */
    s16 zoom_offset;
    /** Q8 sprite scale: 256 = 100%, 128 = 50%. */
    u16 scale_x;
    u16 scale_y;
    /** Q8 pivot inside the unscaled sprite bounds: 0 = start edge, 128 = center, 256 = end edge. */
    u16 pivot_x;
    u16 pivot_y;
    u8 flags;
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
    /** Compact runtime storage; unsigned_effect_set_advanced() still accepts UEffectLayout. */
    u8 layout;
} UEffect;

/**
 * Bind a custom sampled effect.
 *
 * `context` is non-owning and must remain valid while the effect is bound. `speed` is added to the
 * 8-bit phase once per tick, so phase naturally wraps and speed 0 creates a static effect. Custom
 * effects default to per-column sampling and unknown bounds; renderers therefore keep conservative
 * visibility when they cannot prove the effect displacement.
 *
 * Clear an effect explicitly with unsigned_effect_clear().
 * @pre `effect` and `function` are valid. `context` may be NULL when the custom effect does not require state.
 */
void unsigned_effect_set(UEffect *effect, UEffectFunction function, const void *context, u8 speed);

/**
 * Bind a custom effect with explicit sampling layout and conservative displacement bounds.
 *
 * Use `U_EFFECT_LAYOUT_UNIFORM` when every hardware column receives the same sample; this lets the
 * Neo Geo renderer preserve a chained sprite. Use `U_EFFECT_LAYOUT_PER_COLUMN` only when columns
 * genuinely differ. Clear an effect explicitly with unsigned_effect_clear().
 * @pre `effect` and `function` are valid, and `layout` is U_EFFECT_LAYOUT_UNIFORM or U_EFFECT_LAYOUT_PER_COLUMN.
 * `context` may be NULL when the custom effect does not require state.
 */
void unsigned_effect_set_advanced(UEffect *effect, UEffectFunction function, const void *context, u8 speed, UEffectLayout layout, UEffectBoundsFunction bounds);

/** Reset the effect to an inactive identity state. @pre `effect` is valid. */
void unsigned_effect_clear(UEffect *effect);

/** Advance the active effect phase by its configured speed; 8-bit wrap defines the loop period. @pre `effect` is valid. */
void unsigned_effect_tick(UEffect *effect);

/** Advance an active effect by several engine ticks at once; 8-bit phase wrap is preserved. @pre `effect` is valid. */
void unsigned_effect_tick_frames(UEffect *effect, u16 ticks);

/** Return an identity sample (zero translation/zoom, 100% scale, no flags). */
UEffectSample unsigned_effect_identity_sample(void);

/** Sample one effect column. Inactive effects return the identity sample.
 * @pre `effect` is valid, `column_count > 0`, `column < column_count`, and the callback produces UEffectSample values satisfying the sample invariants.
 */
UEffectSample unsigned_effect_sample(const UEffect *effect, u8 column, u8 column_count);

/**
 * Compose two samples in display order. Translation/zoom add, scales multiply,
 * flip flags XOR and hidden flags OR. If the second sample changes scale, its
 * pivot becomes authoritative for the composed scale.
 */
UEffectSample unsigned_effect_compose_samples(UEffectSample first, UEffectSample second);

/** Return whether an active effect requires independent per-column hardware transforms. @pre `effect` is valid. */
bool unsigned_effect_is_per_column(const UEffect *effect);

/** Return conservative maximum effect displacement for renderer culling/layout. @pre `effect` and `bounds` are valid and `column_count > 0`. */
bool unsigned_effect_get_bounds(const UEffect *effect, u8 column_count, UEffectBounds *bounds);

/** Saturating sum of two conservative bounds. */
UEffectBounds unsigned_effect_add_bounds(UEffectBounds first, UEffectBounds second);

#endif
