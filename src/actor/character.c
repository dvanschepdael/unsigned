/**
 * @file character.c
 * @brief Implements character gameplay state shared by players and NPCs.
 */

#include "actor/character.h"

#include "core/math/math.h"

bool unsigned_character_set_shadow(UCharacter *character, const UCharacterShadowDefinition *definition, u16 first_sprite) {
    if (character == NULL || definition == NULL) {
        return false;
    }

    if (!unsigned_character_shadow_init(&character->shadow, definition, first_sprite)) {
        return false;
    }

    character->actor.underlay = &character->shadow.sprite;
    unsigned_character_shadow_set_height(&character->shadow, character->height);
    return true;
}

void unsigned_character_set_height(UCharacter *character, s16 height) {
    if (character == NULL) {
        return;
    }

    const s16 clamped_height = height > 0 ? height : 0;
    if (character->height == clamped_height) {
        return;
    }

    /* Height is presentation-only: preserve actor.position as the ground/depth origin. */
    if (character->height == 0 && clamped_height > 0) {
        character->ground_sprite_offset_y = character->actor.sprite.offset.y;
    }

    if (clamped_height == 0) {
        character->actor.sprite.offset.y = character->ground_sprite_offset_y;
    } else {
        character->actor.sprite.offset.y = unsigned_math_saturate_s16((s32)character->ground_sprite_offset_y - clamped_height);
    }
    character->height = clamped_height;
    unsigned_character_shadow_set_height(&character->shadow, clamped_height);
}

void unsigned_character_set_facing(UCharacter *character, s16 horizontal_direction) {
    if (character == NULL || horizontal_direction == 0) {
        return;
    }

    character->facing_right = horizontal_direction > 0;
    unsigned_sprite_set_flip_x(&character->actor.sprite, !character->facing_right);
}

void unsigned_character_move(UCharacter *character, Vec2 direction, s16 speed) {
    if (character == NULL) {
        return;
    }

    unsigned_character_set_facing(character, direction.x);
    character->actor.position.x = unsigned_math_saturate_s16((s32)character->actor.position.x + (s32)direction.x * speed);
    character->actor.position.y = unsigned_math_saturate_s16((s32)character->actor.position.y + (s32)direction.y * speed);
}

void unsigned_character_tick(UCharacter *character) {
    if (character == NULL) {
        return;
    }

    if (character->shadow.sprite.definition != NULL) {
        unsigned_sprite_tick(&character->shadow.sprite, character);
    }
    unsigned_actor_tick(&character->actor);
}

void unsigned_character_destroy(UCharacter *character) {
    if (character == NULL) {
        return;
    }

    unsigned_actor_destroy(&character->actor);
}
