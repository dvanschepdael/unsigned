#include "display/effect/transform.h"

#include "display/effect/effect_internal.h"

static UEffectBounds transform_bounds(u8 column_count, const void *context) {
    const UTransformEffect *transform = context;
    (void)column_count;

    return (UEffectBounds){
        .offset_x = unsigned_effect_abs_s16(transform->offset_x),
        .offset_y = unsigned_effect_abs_s16(transform->offset_y),
    };
}

static void transform_sample(UEffectSample *sample, u8 column, u8 column_count, u8 phase, const void *context) {
    const UTransformEffect *transform = context;
    (void)column;
    (void)column_count;
    (void)phase;

    sample->offset_x = transform->offset_x;
    sample->offset_y = transform->offset_y;
    sample->scale_x = transform->scale_x;
    sample->scale_y = transform->scale_y;
    sample->pivot_x = transform->pivot_x;
    sample->pivot_y = transform->pivot_y;
    if (transform->flip_x) {
        sample->flags |= U_EFFECT_SAMPLE_FLIP_X;
    }
    if (transform->flip_y) {
        sample->flags |= U_EFFECT_SAMPLE_FLIP_Y;
    }
    if (transform->hidden) {
        sample->flags |= U_EFFECT_SAMPLE_HIDDEN;
    }
}

void unsigned_effect_transform_init(UTransformEffect *transform) {
    *transform = (UTransformEffect){
        .scale_x = U_EFFECT_SCALE_ONE,
        .scale_y = U_EFFECT_SCALE_ONE,
    };
}

void unsigned_effect_transform_set_translation(UTransformEffect *transform, s16 offset_x, s16 offset_y) {
    transform->offset_x = offset_x;
    transform->offset_y = offset_y;
}

void unsigned_effect_transform_set_scale(UTransformEffect *transform, u16 scale_x, u16 scale_y, u16 pivot_x, u16 pivot_y) {
    transform->scale_x = scale_x;
    transform->scale_y = scale_y;
    transform->pivot_x = pivot_x;
    transform->pivot_y = pivot_y;
}

void unsigned_effect_transform_set_flip(UTransformEffect *transform, bool flip_x, bool flip_y) {
    transform->flip_x = flip_x;
    transform->flip_y = flip_y;
}

void unsigned_effect_transform_set_visible(UTransformEffect *transform, bool visible) {
    transform->hidden = !visible;
}

void unsigned_effect_set_transform(UEffect *effect, const UTransformEffect *transform) {
    unsigned_effect_bind(effect, transform_sample, transform, 0u, U_EFFECT_LAYOUT_UNIFORM, transform_bounds);
}
