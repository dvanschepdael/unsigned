/**
 * @file character.c
 * @brief Implements character gameplay state shared by players and NPCs.
 */

#include "actor/character.h"

#include "core/math/math.h"

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

    unsigned_actor_tick(&character->actor);
}

void unsigned_character_destroy(UCharacter *character) {
    if (character == NULL) {
        return;
    }

    unsigned_actor_destroy(&character->actor);
}
