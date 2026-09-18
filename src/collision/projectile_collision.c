/**
 * @file projectile_collision.c
 * @brief Implements projectile overlap resolution against actor collision data.
 */

#include "collision/projectile_collision.h"

#include "actor/projectile_pool.h"
#include "collision/actor_collision.h"

#include <stdint.h>

/** Returns projectile velocity reversed for bounce/reflection response. */
static s16 collision_projectile_reverse_velocity(s16 velocity) {
    return velocity == INT16_MIN ? INT16_MAX : (s16)-velocity;
}

/** Tests whether the projectile hitbox overlaps an indexed actor allowed by the target mask. */
static bool collision_projectile_hits_actor(const UCollisionActorIndex *actors, UPhysicsCollisionMask target_mask, const UCollisionBox *hitbox, const UActor *projectile_actor) {
    if (actors == NULL || hitbox == NULL || target_mask == 0u) {
        return false;
    }

    UCollisionActorRange range = unsigned_collision_actor_index_range(actors, hitbox);

    for (u8 i = range.first; i < range.end; ++i) {
        UActor *actor = actors->actors->instances[i];

        if (actor == NULL || actor == projectile_actor) {
            continue;
        }

        UPhysicsCollisionChannel hitbox_channel = actor->collision.hitbox_channel;
        UPhysicsCollisionChannel hurtbox_channel = actor->collision.hurtbox_channel;

        if (actor->collision.hitbox_active && hitbox_channel < U_COLLISION_CHANNEL_COUNT && (target_mask & (UPhysicsCollisionMask)(1u << hitbox_channel)) != 0u) {
            if (unsigned_physics_collision_box_intersects_fast(hitbox, &actor->collision.hitbox)) {
                return true;
            }
        }

        if (actor->collision.hurtbox_active && hurtbox_channel < U_COLLISION_CHANNEL_COUNT && (target_mask & (UPhysicsCollisionMask)(1u << hurtbox_channel)) != 0u) {
            if (unsigned_physics_collision_box_intersects_fast(hitbox, &actor->collision.hurtbox)) {
                return true;
            }
        }
    }

    return false;
}

void unsigned_collision_projectiles_resolve(UCollisionManager *collisions, const UCollisionActorIndex *actors, UPoolInstanceContainer *projectiles, const UTLSS *tlss) {
    if (collisions == NULL || actors == NULL || projectiles == NULL || projectiles->count == 0u) {
        return;
    }

    for (u8 i = 0u; i < projectiles->capacity; ++i) {
        UProjectilePoolInstance *instance = &projectiles->instances[i];
        const Vec2 *collision_position;

        if (!instance->active || instance->args == NULL) {
            continue;
        }

        UProjectile *projectile = instance->args;
        if (projectile->channel >= U_COLLISION_CHANNEL_COUNT || projectile->actor == NULL) {
            continue;
        }

        collision_position = projectile->collision_on_ground ? &projectile->ground_position : &projectile->visual_position;
        const UCollisionBox *hitbox = unsigned_actor_collision_current_hitbox(projectile->actor, collision_position);
        if (hitbox == NULL) {
            continue;
        }

        if (!unsigned_actor_collision_should_resolve(projectile->actor, tlss, i)) {
            continue;
        }

        UPhysicsCollisionMask collision_mask = collisions->layers[projectile->channel].collision_mask;
        UPhysicsCollisionMask actor_mask = (UPhysicsCollisionMask)(collision_mask & actors->channels);
        UPhysicsCollisionMask fallback_mask = (UPhysicsCollisionMask)(collision_mask & (UPhysicsCollisionMask)~actors->covered_channels);

        bool hit = collision_projectile_hits_actor(actors, actor_mask, hitbox, projectile->actor);
        if (!hit && fallback_mask != 0u) {
            hit = unsigned_physics_collision_any_mask(collisions, fallback_mask, hitbox);
        }
        if (!hit) {
            projectile->collision_latched = false;
            continue;
        }

        if (projectile->collision_latched) {
            continue;
        }
        projectile->collision_latched = true;

        u16 generation = instance->generation;

        if (projectile->on_hit != NULL) {
            projectile->on_hit(projectile->on_hit_args);
        }

        if (!instance->active || instance->generation != generation || instance->args != projectile) {
            continue;
        }

        if (projectile->bounce) {
            projectile->trajectory.velocity.x = collision_projectile_reverse_velocity(projectile->trajectory.velocity.x);
            projectile->trajectory.velocity.depth = collision_projectile_reverse_velocity(projectile->trajectory.velocity.depth);
        } else {
            unsigned_projectile_pool_release(projectiles, instance);
        }
    }
}
