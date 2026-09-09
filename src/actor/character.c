/**
 * @file character.c
 * @brief Implements character gameplay state shared by players and NPCs.
 */

#include "actor/character.h"

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
