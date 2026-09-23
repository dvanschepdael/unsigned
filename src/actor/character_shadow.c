/**
 * @file character_shadow.c
 * @brief Maps character elevation to a cached ground-shadow sprite transform.
 */

#include "actor/character_shadow.h"

#include "core/math/math.h"
#include "display/sprite/scale.h"

#define CHARACTER_SHADOW_Q16_ONE 0x10000u

/** Return normalized height in Q16. The reciprocal is cached so this hot path uses no division. */
static u32 character_shadow_progress_q16(const UCharacterShadow *shadow, s16 height) {
    if (height == 0) {
        return 0u;
    }

    const u16 max_height = shadow->definition->height_for_min_scale;
    if ((u16)height >= max_height) {
        return CHARACTER_SHADOW_Q16_ONE;
    }

    const u32 progress = (u32)(u16)height * shadow->height_reciprocal_q16;
    return progress < CHARACTER_SHADOW_Q16_ONE ? progress : CHARACTER_SHADOW_Q16_ONE - 1u;
}

static u16 character_shadow_scale_at(u16 minimum, u32 progress_q16) {
    if (progress_q16 >= CHARACTER_SHADOW_Q16_ONE) {
        return minimum;
    }

    const u16 range = (u16)(U_EFFECT_SCALE_ONE - minimum);
    const u16 step = (u16)(((u32)range * progress_q16 + 0x8000u) >> 16u);
    return (u16)(U_EFFECT_SCALE_ONE - step);
}

/** Bake the centered scale into the shadow sprite so normal sprite rendering remains cheap. */
static void character_shadow_apply_transform(UCharacterShadow *shadow) {
    USprite *sprite = &shadow->sprite;
    const USpriteDefinition *definition = sprite->definition;
    const u8 shrink_x = unsigned_sprite_scale_shrink_x(U_SPRITE_SHRINK_X_FULL, shadow->transform.scale_x);
    const u8 shrink_y = unsigned_sprite_scale_shrink_y(U_SPRITE_SHRINK_Y_FULL, shadow->transform.scale_y);
    const u16 base_width = (u16)definition->width_tiles * 16u;
    const u16 base_height = (u16)definition->height_tiles * 16u;
    const u16 scaled_width = unsigned_sprite_scaled_width_pixels(definition->width_tiles, shrink_x);
    const u16 scaled_height = unsigned_sprite_scaled_height_pixels(definition->height_tiles, shrink_y);
    const s16 pivot_x = unsigned_sprite_scale_pivot_offset(base_width, scaled_width, U_EFFECT_PIVOT_CENTER);
    const s16 pivot_y = unsigned_sprite_scale_pivot_offset(base_height, scaled_height, U_EFFECT_PIVOT_CENTER);

    sprite->offset.x = unsigned_math_saturate_s16((s32)shadow->definition->anchor.x - (s32)(base_width / 2u) + pivot_x);
    sprite->offset.y = unsigned_math_saturate_s16((s32)shadow->definition->anchor.y - (s32)(base_height / 2u) + pivot_y);
    unsigned_sprite_set_shrink(sprite, shrink_x, shrink_y);
}

void unsigned_character_shadow_init(UCharacterShadow *shadow, const UCharacterShadowDefinition *definition, u16 first_sprite) {
    *shadow = (UCharacterShadow){
        .definition = definition,
        .height_reciprocal_q16 = (CHARACTER_SHADOW_Q16_ONE + definition->height_for_min_scale / 2u) / definition->height_for_min_scale,
    };

    unsigned_sprite_init(&shadow->sprite, definition->sprite, first_sprite);
    unsigned_effect_transform_init(&shadow->transform);
    unsigned_effect_transform_set_scale(&shadow->transform, U_EFFECT_SCALE_ONE, U_EFFECT_SCALE_ONE, U_EFFECT_PIVOT_CENTER, U_EFFECT_PIVOT_CENTER);
    character_shadow_apply_transform(shadow);
}

void unsigned_character_shadow_set_height(UCharacterShadow *shadow, s16 height) {
    const u32 progress = character_shadow_progress_q16(shadow, height);
    const u16 scale_x = character_shadow_scale_at(shadow->definition->min_scale_x, progress);
    const u16 scale_y = character_shadow_scale_at(shadow->definition->min_scale_y, progress);

    if (shadow->transform.scale_x == scale_x && shadow->transform.scale_y == scale_y) {
        return;
    }

    unsigned_effect_transform_set_scale(&shadow->transform, scale_x, scale_y, U_EFFECT_PIVOT_CENTER, U_EFFECT_PIVOT_CENTER);
    character_shadow_apply_transform(shadow);
}
