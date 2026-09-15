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
 * where 256 is 100%; the Neo Geo backend clamps enlargement above 100% because the
 * hardware can shrink sprites but cannot magnify them past their source size.
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
    UEffectLayout layout;
} UEffect;

/**
 * Bind a custom sampled effect.
 *
 * `context` is non-owning and must remain valid while the effect is bound. `speed` is added to the
 * 8-bit phase once per tick, so phase naturally wraps and speed 0 creates a static effect. Custom
 * effects default to per-column sampling and unknown bounds; renderers therefore keep conservative
 * visibility when they cannot prove the effect displacement.
 *
 * Passing a NULL function clears the effect.
 */
void unsigned_effect_set(UEffect *effect, UEffectFunction function, const void *context, u8 speed);

/**
 * Bind a custom effect with explicit sampling layout and conservative displacement bounds.
 *
 * Use `U_EFFECT_LAYOUT_UNIFORM` when every hardware column receives the same sample; this lets the
 * Neo Geo renderer preserve a chained sprite. Use `U_EFFECT_LAYOUT_PER_COLUMN` only when columns
 * genuinely differ. Passing a NULL function clears the effect.
 */
void unsigned_effect_set_advanced(UEffect *effect, UEffectFunction function, const void *context, u8 speed, UEffectLayout layout, UEffectBoundsFunction bounds);

/** Reset the effect to an inactive identity state. */
void unsigned_effect_clear(UEffect *effect);

/** Advance the active effect phase by its configured speed; 8-bit wrap defines the loop period. */
void unsigned_effect_tick(UEffect *effect);

/** Return an identity sample (zero translation/zoom, 100% scale, no flags). */
UEffectSample unsigned_effect_identity_sample(void);

/** Sample one effect column. Inactive effects return the identity sample. */
UEffectSample unsigned_effect_sample(const UEffect *effect, u8 column, u8 column_count);

/**
 * Compose two samples in display order. Translation/zoom add, scales multiply,
 * flip flags XOR and hidden flags OR. If the second sample changes scale, its
 * pivot becomes authoritative for the composed scale.
 */
UEffectSample unsigned_effect_compose_samples(UEffectSample first, UEffectSample second);

/** Sample two effects and compose them without exposing composition details to render backends. */
UEffectSample unsigned_effect_sample_composed(const UEffect *first, const UEffect *second, u8 column, u8 column_count);

/** Return whether an active effect requires independent per-column hardware transforms. */
bool unsigned_effect_is_per_column(const UEffect *effect);

/** Return conservative maximum effect displacement for renderer culling/layout. */
bool unsigned_effect_get_bounds(const UEffect *effect, u8 column_count, UEffectBounds *bounds);

/** Saturating sum of two conservative bounds. */
UEffectBounds unsigned_effect_add_bounds(UEffectBounds first, UEffectBounds second);

#endif
