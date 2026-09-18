/**
 * @file character_shadow.c
 * @brief Maps character elevation to a generic sprite transform effect.
 */

#include "actor/character_shadow.h"

#include "core/math/math.h"

#define CHARACTER_SHADOW_Q16_ONE 0x10000u

static bool character_shadow_definition_is_valid(const UCharacterShadowDefinition *definition) {
    return definition != NULL && definition->sprite != NULL && definition->height_for_min_scale > 0u &&
           definition->min_scale_x <= U_EFFECT_SCALE_ONE && definition->min_scale_y <= U_EFFECT_SCALE_ONE;
}

/** Return normalized height in Q16. The reciprocal is cached so this hot path uses no division. */
static u32 character_shadow_progress_q16(const UCharacterShadow *shadow, s16 height) {
    if (height <= 0) {
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

/** Place the full-size canvas so its center coincides with the ground anchor. */
static void character_shadow_set_base_offset(UCharacterShadow *shadow) {
    const u16 width = (u16)shadow->sprite.definition->width_tiles * 16u;
    const u16 height = (u16)shadow->sprite.definition->height_tiles * 16u;
    const Vec2 anchor = shadow->definition->anchor;

    shadow->sprite.offset.x = unsigned_math_saturate_s16((s32)anchor.x - (s32)(width / 2u));
    shadow->sprite.offset.y = unsigned_math_saturate_s16((s32)anchor.y - (s32)(height / 2u));
}

bool unsigned_character_shadow_init(UCharacterShadow *shadow, const UCharacterShadowDefinition *definition, u16 first_sprite) {
    if (shadow == NULL || !character_shadow_definition_is_valid(definition)) {
        return false;
    }

    UCharacterShadow initialized = {
        .definition = definition,
        .height_reciprocal_q16 = (CHARACTER_SHADOW_Q16_ONE + definition->height_for_min_scale / 2u) / definition->height_for_min_scale,
    };

    if (!unsigned_sprite_init(&initialized.sprite, definition->sprite, first_sprite) ||
        !unsigned_sprite_play(&initialized.sprite, 0u, U_SPRITE_PLAY_LOOP)) {
        return false;
    }

    unsigned_effect_transform_init(&initialized.transform);
    unsigned_effect_transform_set_scale(&initialized.transform, U_EFFECT_SCALE_ONE, U_EFFECT_SCALE_ONE,
                                        U_EFFECT_PIVOT_CENTER, U_EFFECT_PIVOT_CENTER);

    *shadow = initialized;
    /* Bind only after the value copy so the effect context points at its final owner. */
    unsigned_effect_set_transform(&shadow->sprite.effect, &shadow->transform);
    character_shadow_set_base_offset(shadow);
    unsigned_character_shadow_set_height(shadow, 0);
    return true;
}

void unsigned_character_shadow_set_height(UCharacterShadow *shadow, s16 height) {
    if (shadow == NULL || shadow->definition == NULL || shadow->sprite.definition == NULL) {
        return;
    }

    const u32 progress = character_shadow_progress_q16(shadow, height);
    const u16 scale_x = character_shadow_scale_at(shadow->definition->min_scale_x, progress);
    const u16 scale_y = character_shadow_scale_at(shadow->definition->min_scale_y, progress);

    unsigned_effect_transform_set_scale(&shadow->transform, scale_x, scale_y,
                                        U_EFFECT_PIVOT_CENTER, U_EFFECT_PIVOT_CENTER);
}
