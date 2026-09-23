/**
 * @file character.c
 * @brief Implements character gameplay state shared by players and NPCs.
 */

#include "actor/character.h"

#include "core/math/math.h"

void unsigned_character_set_shadow(UCharacter *character, const UCharacterShadowDefinition *definition, u16 first_sprite) {
    unsigned_character_shadow_init(&character->shadow, definition, first_sprite);
    character->actor.underlay = &character->shadow.sprite;
    unsigned_character_shadow_set_height(&character->shadow, character->height);
}

void unsigned_character_set_height(UCharacter *character, s16 height) {
    if (character->height == height) {
        return;
    }

    if (character->height == 0 && height > 0) {
        character->ground_sprite_offset_y = character->actor.sprite.offset.y;
    }

    character->actor.sprite.offset.y = height == 0 ? character->ground_sprite_offset_y : unsigned_math_saturate_s16((s32)character->ground_sprite_offset_y - height);
    character->height = height;
    unsigned_character_shadow_set_height(&character->shadow, height);
}

void unsigned_character_set_facing(UCharacter *character, s16 horizontal_direction) {
    if (horizontal_direction == 0) {
        return;
    }

    character->facing_right = horizontal_direction > 0;
    unsigned_sprite_set_flip_x(&character->actor.sprite, !character->facing_right);
}

void unsigned_character_move(UCharacter *character, Vec2 direction, s16 horizontal_move_speed, s16 vertical_move_speed) {
    unsigned_character_set_facing(character, direction.x);

    if (horizontal_move_speed > 0) {
        if (direction.x > 0) {
            character->actor.position.x = unsigned_math_saturate_s16((s32)character->actor.position.x + horizontal_move_speed);
        } else if (direction.x < 0) {
            character->actor.position.x = unsigned_math_saturate_s16((s32)character->actor.position.x - horizontal_move_speed);
        }
    }

    if (vertical_move_speed > 0) {
        if (direction.y > 0) {
            character->actor.position.y = unsigned_math_saturate_s16((s32)character->actor.position.y + vertical_move_speed);
        } else if (direction.y < 0) {
            character->actor.position.y = unsigned_math_saturate_s16((s32)character->actor.position.y - vertical_move_speed);
        }
    }
}
