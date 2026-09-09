/**
 * @file character.h
 * @brief Character gameplay state shared by players and NPCs.
 */

#ifndef UNSIGNED_ACTOR_CHARACTER_H
#define UNSIGNED_ACTOR_CHARACTER_H

#include "actor/actor.h"
#include "gameplay/ability.h"
#include "gameplay/attribute.h"
#include "gameplay/tag.h"

typedef struct UCharacter {
    UActor actor;
    u8 facing_right;
    UGameplayAttributeContainer attributes;
    UGameplayAbilityContainer abilities;
    UGameplayTagContainer tags;
} UCharacter;

/**
 * @brief Advances the character by one scheduled engine frame.
 *
 * @param character Character runtime whose actor/gameplay state is advanced or cleared.
 */
void unsigned_character_tick(UCharacter *character);

/**
 * @brief Tears down the character runtime state and releases its logical resources.
 *
 * @param character Character runtime whose actor/gameplay state is advanced or cleared.
 */
void unsigned_character_destroy(UCharacter *character);

#endif
