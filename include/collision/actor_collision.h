/**
 * @file actor_collision.h
 * @brief Collision-box operations for one actor/character at a time.
 */

#ifndef UNSIGNED_COLLISION_ACTOR_COLLISION_H
#define UNSIGNED_COLLISION_ACTOR_COLLISION_H

#include "core/tlss/tlss.h"
#include "physics/collision.h"

struct UActor;
struct UCharacter;
struct UNpc;
struct UPlayer;

/**
 * Register the current animation frame's hurtbox/hitbox as dynamic collision geometry.
 * Returns false when the actor/frame is invalid or when at least one manager registration fails.
 */
bool unsigned_actor_collision_register(struct UActor *actor, UCollisionManager *manager);

/**
 * Register the current animation frame's boxes in persistent collision storage.
 * Used for level objects whose collision remains registered across frame clears.
 */
bool unsigned_actor_collision_register_static(struct UActor *actor, UCollisionManager *manager);

/** Convenience wrapper that registers character->actor as dynamic collision. */
bool unsigned_character_collision_register(struct UCharacter *character, UCollisionManager *manager);

/** Convenience wrapper that follows player->character and registers its actor. */
bool unsigned_player_collision_register(struct UPlayer *player, UCollisionManager *manager);

/** Convenience wrapper that follows npc->character and registers its actor. */
bool unsigned_npc_collision_register(struct UNpc *npc, UCollisionManager *manager);

/**
 * Decide whether this actor's collision work should run on the current TLSS frame.
 * A newly activated hitbox forces one immediate resolution so an attack cannot miss its
 * first active frame because of temporal scaling.
 */
bool unsigned_actor_collision_should_resolve(struct UActor *actor, const UTLSS *tlss, u16 slot);

/**
 * Return the current frame hitbox placed at `position`, including horizontal sprite flip.
 * The returned pointer refers to actor-owned scratch state and remains valid until overwritten.
 */
const UCollisionBox *unsigned_actor_collision_current_hitbox(struct UActor *actor, const Vec2 *position);

#endif
