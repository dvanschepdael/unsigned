/**
 * @file character.h
 * @brief Character gameplay state shared by players and NPCs.
 */

#ifndef UNSIGNED_ACTOR_CHARACTER_H
#define UNSIGNED_ACTOR_CHARACTER_H

#include "actor/actor.h"
#include "actor/character_shadow.h"
#include "gameplay/ability.h"
#include "gameplay/attribute.h"
#include "gameplay/tag.h"

typedef struct UCharacter {
    UActor actor;
    /** Optional ground shadow owned by the character and rendered as the actor underlay. */
    UCharacterShadow shadow;
    /** Presentation height in pixels above actor.position, which remains on the ground plane. */
    s16 height;
    /** Body sprite Y offset captured when leaving the ground, used to restore landing exactly. */
    s16 ground_sprite_offset_y;
    u8 facing_right;
    UGameplayAttributeContainer attributes;
    UGameplayAbilityContainer abilities;
    UGameplayTagContainer tags;
} UCharacter;

/**
 * Initialize the character-owned shadow and bind its sprite as the actor underlay.
 * The shadow is rendered and allocated before the actor sprite so the character can overlap it.
 */
bool unsigned_character_set_shadow(UCharacter *character, const UCharacterShadowDefinition *definition, u16 first_sprite);

/**
 * Set the character elevation in pixels above the ground plane.
 *
 * The actor world position is deliberately unchanged so depth ordering and ground movement keep
 * using the same origin. The body sprite moves vertically by the height delta while the optional
 * character shadow updates its scale independently. Negative values are clamped to zero.
 */
void unsigned_character_set_height(UCharacter *character, s16 height);

/** Update logical facing and sprite flip from a horizontal direction. Zero leaves facing unchanged. */
void unsigned_character_set_facing(UCharacter *character, s16 horizontal_direction);

/** Move a character by direction * speed and update facing. Spatial constraints are caller-owned. */
void unsigned_character_move(UCharacter *character, Vec2 direction, s16 speed);

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
