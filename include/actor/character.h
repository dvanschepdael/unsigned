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
    UGameplayAttribute *attributes;
    UGameplayAbilityContainer abilities;
    UGameplayTagContainer tags;
} UCharacter;

/**
 * Initialize the character-owned shadow and bind its sprite as the actor underlay.
 * The shadow is rendered and allocated before the actor sprite so the character can overlap it.
 */
void unsigned_character_set_shadow(UCharacter *character, const UCharacterShadowDefinition *definition, u16 first_sprite);

/**
 * Set the character elevation in pixels above the ground plane.
 *
 * The actor world position is deliberately unchanged so depth ordering and ground movement keep
 * using the same origin. The body sprite moves vertically by the height delta while the optional
 * character shadow updates its scale independently.
 * @pre `character` is valid and `height >= 0`.
 */
void unsigned_character_set_height(UCharacter *character, s16 height);

/** Update logical facing and sprite flip from a horizontal direction. Zero leaves facing unchanged. */
void unsigned_character_set_facing(UCharacter *character, s16 horizontal_direction);

/**
 * Move the character on the ground plane using independent whole-pixel axis speeds.
 *
 * Direction components are interpreted only by sign: positive moves forward on the
 * corresponding world axis, negative moves backward, and zero leaves that axis unchanged.
 * Horizontal input also updates character facing. Position arithmetic saturates to s16.
 *
 * @pre `character` is valid.
 * @pre `horizontal_move_speed >= 0` and `vertical_move_speed >= 0`.
 */
void unsigned_character_move(UCharacter *character, Vec2 direction, s16 horizontal_move_speed, s16 vertical_move_speed);

#endif
