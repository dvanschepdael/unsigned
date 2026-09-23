/**
 * @file actor_collision.h
 * @brief Materializes animation-frame collision for one actor.
 */

#ifndef UNSIGNED_COLLISION_ACTOR_COLLISION_H
#define UNSIGNED_COLLISION_ACTOR_COLLISION_H

#include "core/tlss/tlss.h"
#include "physics/collision.h"

struct UActor;

/**
 * Refresh and publish the actor current-frame boxes.
 * @param persistent Keep registrations across dynamic collision clears when true.
 * @pre `actor` belongs to an active actor-pool slot and is initialized with a current animation/frame.
 * @pre `manager` is valid and authored frame channels are valid.
 * @pre Collision channel capacities cover every actor that can publish during the frame.
 */
void unsigned_actor_collision_register(struct UActor *actor, UCollisionManager *manager, bool persistent);

/**
 * Inspect the current frame without transforming boxes or touching manager storage.
 * Returns whether the actor currently originates an attack hitbox and maintains the attack-edge cache.
 * @pre `actor` belongs to an active actor-pool slot and owns an initialized sprite with a current animation/frame.
 */
bool unsigned_actor_collision_probe(struct UActor *actor);

/** Clear frame-local active collision channels while preserving the cached box transform. */
void unsigned_actor_collision_deactivate_frame(struct UActor *actor);

/**
 * Decide whether collision resolution is due under TLSS. A newly activated hitbox bypasses the
 * normal temporal schedule once so its first active frame cannot be skipped.
 * @pre `actor` belongs to an active actor-pool slot and `tlss` is valid.
 */
bool unsigned_actor_collision_should_resolve(struct UActor *actor, const UTLSS *tlss, u16 slot);

/**
 * Return the current frame hitbox placed at `position`, including horizontal flip.
 * NULL means the current animation frame has no hitbox.
 * @pre `actor` is initialized with a current animation/frame and `position` is valid.
 */
const UCollisionBox *unsigned_actor_collision_current_hitbox(struct UActor *actor, const Vec2 *position);

#endif
